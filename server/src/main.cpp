#include <unistd.h>
#include <signal.h>

#include <boost/program_options.hpp>

#include <fcgiapp.h>
#include <fcgio.h>

#include <roller/core/types.h>
#include <roller/core/log.h>
#include <roller/core/util.h>

#include "device_manager.h"

#include "raspi_gpio_switch.h"
#include "owfs_sensors.h"

#include "temperature_manager.h"
#include "current_limiter.h"
#include "pid.h"
#include "server_controller.h"
#include "dummy_controller.h"

#define AB_SERVER_FASTCGI_SOCKET "/var/run/ab.socket"
#define AB_SERVER_FASTCGI_BACKLOG 8

using namespace roller;
namespace po = boost::program_options;

// globals
bool g_appRunning = false;

// handleSignal
void handleSignal( i32 sig ) {
	Log::i( "sig %d caught, flagging app to stop running", sig );
	g_appRunning = false;
	FCGX_ShutdownPending();

	// hack to make fcgx actually die
	system("curl http://localhost/ab/ping &");
}

// handleRequest
void handleRequest( FCGX_Request& request, CurrentLimiter& currentLimiter );

// test Dummy controller
void handleStartDummy();
void handleStopDummy();
std::shared_ptr<DummyController> s_controller;

void configCurrentLimiter(CurrentLimiter& currentLimiter);

// main
i32 main( i32 argc, char** argv ) {

	printf( "ab server starting\n" );

	g_appRunning = true;

	setbuf( stdout, nullptr );

	signal( SIGINT, handleSignal );
	signal( SIGHUP, handleSignal );
	signal( SIGKILL, handleSignal );
	signal( SIGTERM, handleSignal );

	Log::setLogLevelMode( LOG_LEVEL_MODE_UNIX_TERMINAL );

	// initialize devman
	auto raspiGPIOManager = std::make_shared<RaspiGPIOSwitchManager>();
	StringId raspiSwitchManagerID = DeviceManager::registerSwitchManager( raspiGPIOManager );

	// initialize CurrentLimiter
	CurrentLimiter currentLimiter(700, 35000); // base is 0.7 amps, total allowed 35 amps
	configCurrentLimiter(currentLimiter);

	// initialize temperature manager
	auto owfsManager = std::make_shared<OWFSHardwareManager>( "--usb all" );
	StringId owfsManagerID = DeviceManager::registerTemperatureSensorManager( owfsManager );
	TemperatureManager temperatureManager;
	temperatureManager.run();

	// initialize fast cgi
	printf( "initializing fcgx\n" );
	int result = FCGX_Init();
	if ( result ) {
		fprintf( stderr, "FCGX_Init() failed: %d\n", result );
		return 1;
	}

	// TODO: make socket file configurable
	int listenSocket = FCGX_OpenSocket( AB_SERVER_FASTCGI_SOCKET, AB_SERVER_FASTCGI_BACKLOG );
	if ( listenSocket < 0 ) {
		fprintf( stderr, "FCGX_OpenSocket(%s) failed: %d\n", AB_SERVER_FASTCGI_SOCKET, listenSocket);
		return 1;
	}

	FCGX_Request request;
	result = FCGX_InitRequest( &request, listenSocket, FCGI_FAIL_ACCEPT_ON_INTR );
	if ( result ) {
		fprintf( stderr, "FCGX_InitRequest() failed: %d\n", result );
	}

	// TODO: hack to change server socket permissions
	system("chown root:www-data /var/run/ab.socket &");
	system("chmod g+w /var/run/ab.socket &");

	while ( g_appRunning ) {

		result = FCGX_Accept_r( &request );
		if ( result ) {
			fprintf( stderr, "FCGX_Request_r failed: %d\n", result );
			break;
		}

		try {

			handleRequest(request, currentLimiter);

		} catch( exception& e ) {
			Log::w( "Caught exception while trying to handle request (ignoring): %s", e.what());
			continue;
		}

	}

	return 0;
}

// handleRequest
void handleRequest( FCGX_Request& request, CurrentLimiter& currentLimiter ) {

	std::string scriptName = FCGX_GetParam("SCRIPT_NAME", request.envp);
	std::string pathInfo = FCGX_GetParam("PATH_INFO", request.envp);
	std::string requestMethod = FCGX_GetParam("REQUEST_METHOD", request.envp);
	std::string requestUri = FCGX_GetParam("REQUEST_URI", request.envp);

	Log::i( "handling request uri:    %s", requestUri.c_str());
	Log::i( "         script name:    %s", scriptName.c_str());
	Log::i( "         path info:      %s", pathInfo.c_str());
	Log::i( "         request method: %s", requestMethod.c_str());

	
	// get last part of URI
	// TODO: clean this up
	std::string handlerName = requestUri.substr(4, (requestUri.size() - 4));
	Log::i( "         request handler name: %s", handlerName.c_str());

	std::string jsonResponse = "{}";
	i32 responseCode = 200;
	if (handlerName == "start_dummy") {

		try {
			handleStartDummy();

			jsonResponse = "{ \"response\": \"OK\" }";
			responseCode = 200;

		} catch ( exception& e ) {
			Log::w( "caught exception in handleStartDummy: %s", e.what() );

			jsonResponse = roller::makeString( "{ \"response\": \"Failed to start Dummy controller\", \"reason\": \"%s\"}",
					e.what());
			responseCode = 500;
		}

	} else if (handlerName == "stop_dummy") {

		try {
			handleStopDummy();

			jsonResponse = "{ \"response\": \"OK\" }";
			responseCode = 200;

		} catch ( exception& e ) {
			Log::w( "caught exception in handleStopDummy: %s", e.what() );

			jsonResponse = roller::makeString( "{ \"response\": \"Failed to stop Dummy controller\", \"reason\": \"%s\"}",
					e.what());
			responseCode = 500;
		}

	} else if (handlerName == "status") {

		jsonResponse = "{ \"response\": \"All systems go\" }";
		responseCode = 200;

	// TODO: use wiring pi library here and track pin state?
	} else if (handlerName == "p1_on") {
		currentLimiter.enablePin(18);

	} else if (handlerName == "p1_off") {
		currentLimiter.disablePin(18);

	} else if (handlerName == "p2_on") {
		currentLimiter.enablePin(27);

	} else if (handlerName == "p2_off") {
		currentLimiter.disablePin(27);

	} else if (handlerName == "valve_on") {
		// system("gpio export 22 out; gpio -g write 22 1");
		currentLimiter.enablePin(22);
		/*
		currentLimiter.enablePin(24); // HLT safety
		currentLimiter.enablePin(10); // BK safety

		// BK
		currentLimiter.enablePin(17); // BK 
		CurrentLimiter::PinConfiguration config = currentLimiter.getPinConfiguration(17);
		config._pwmLoad = 1.0;
		currentLimiter.updatePinConfiguration(config);

		// HLT
		currentLimiter.enablePin(4); // BK 
		config = currentLimiter.getPinConfiguration(4);
		config._pwmLoad = 1.0;
		currentLimiter.updatePinConfiguration(config);
		*/


	} else if (handlerName == "valve_off") {
		// system("gpio export 22 out; gpio -g write 22 0");
		currentLimiter.disablePin(22);

	} else {

		jsonResponse = "{ \"response\": \"Unrecognized Handler\" }";
		responseCode = 400;
	}

	// assemble HTTP response
	std::string statusResponse;
	switch (responseCode)
	{
		case 200:
			statusResponse = "200 OK";
			break;

		case 400:
			statusResponse = "400 Bad Request";
			break;

		default:
			Log::w( "HTTP response code not set, assuming 500" );
			// intentionally fall through

		case 500:
			statusResponse = "500 Internal Server Error";
			break;
	}

	Log::i( "Sending response.." );
	FCGX_FPrintF( request.out, "Status: %s\r\n", statusResponse.c_str() );
	FCGX_FPrintF( request.out, "Content-Type: application/json; charset=utf-8\r\n" );
	FCGX_FPrintF( request.out, "Content-Length: %d\r\n", jsonResponse.size() );
	FCGX_FPrintF( request.out, "\r\n" );
	FCGX_PutStr( jsonResponse.c_str(), jsonResponse.size(), request.out );

	FCGX_Finish_r( &request );
}

void handleStartDummy() {

	if ( s_controller ) {
		throw RollerException( "Can't start dummy controller; already running" );
	}

	s_controller.reset( new DummyController());
	s_controller->start();
}

void handleStopDummy() {

	if ( ! s_controller ) {
		throw RollerException( "Can't stop dummy controller; none running" );
	}

	s_controller->stop();
	s_controller->join();
	s_controller.reset();
}

void configCurrentLimiter(CurrentLimiter& currentLimiter) {

	// TODO: pull this info from config file (etc.)

	// pump 1
	CurrentLimiter::PinConfiguration config;
	config._name = "Pump 1";
	config._pinNumber = 18;
	config._milliAmps = 1400;
	config._critical = true;
	config._pwm = false;
	config._pwmFrequency = 0;
	config._pwmLoad = 0.0f;
	currentLimiter.addPinConfiguration(
			config,
			DeviceManager::getSwitch(RaspiGPIOSwitchManager::s_id, StringId::format("%d", config._pinNumber)));

	// pump 2
	config._name = "Pump 2";
	config._pinNumber = 27;
	config._milliAmps = 1400;
	config._critical = true;
	config._pwm = false;
	config._pwmFrequency = 0;
	config._pwmLoad = 0.0f;
	currentLimiter.addPinConfiguration(
			config,
			DeviceManager::getSwitch(RaspiGPIOSwitchManager::s_id, StringId::format("%d", config._pinNumber)));

	// valve 1
	config._name = "Valve 1";
	config._pinNumber = 22;
	config._milliAmps = 200;
	config._critical = true;
	config._pwm = false;
	config._pwmFrequency = 0;
	config._pwmLoad = 0.0f;
	currentLimiter.addPinConfiguration(
			config,
			DeviceManager::getSwitch(RaspiGPIOSwitchManager::s_id, StringId::format("%d", config._pinNumber)));

	// BK element safety
	config._name = "BK Element Safety";
	config._pinNumber = 10;
	config._milliAmps = 34;
	config._critical = true;
	config._pwm = false;
	config._pwmFrequency = 0;
	config._pwmLoad = 0.0f;
	currentLimiter.addPinConfiguration(
			config,
			DeviceManager::getSwitch(RaspiGPIOSwitchManager::s_id, StringId::format("%d", config._pinNumber)));

	// HLT element safety
	config._name = "HLT Element Safety";
	config._pinNumber = 24;
	config._milliAmps = 34;
	config._critical = true;
	config._pwm = false;
	config._pwmFrequency = 0;
	config._pwmLoad = 0.0f;
	currentLimiter.addPinConfiguration(
			config,
			DeviceManager::getSwitch(RaspiGPIOSwitchManager::s_id, StringId::format("%d", config._pinNumber)));

	// BK element 
	config._name = "BK Element";
	config._pinNumber = 17;
	config._milliAmps = 23000;
	config._critical = false;
	config._pwm = true;
	config._pwmFrequency = 20;
	config._pwmLoad = 1.0f;
	currentLimiter.addPinConfiguration(
			config,
			DeviceManager::getSwitch(RaspiGPIOSwitchManager::s_id, StringId::format("%d", config._pinNumber)));

	// HLT element 
	config._name = "HLT Element";
	config._pinNumber = 4;
	config._milliAmps = 23000;
	config._critical = false;
	config._pwm = true;
	config._pwmFrequency = 20;
	config._pwmLoad = 1.0f;
	currentLimiter.addPinConfiguration(
			config,
			DeviceManager::getSwitch(RaspiGPIOSwitchManager::s_id, StringId::format("%d", config._pinNumber)));
}

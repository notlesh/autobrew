#include <unistd.h>
#include <signal.h>

#include <boost/program_options.hpp>

#include <fcgiapp.h>
#include <fcgio.h>

#include <roller/core/types.h>
#include <roller/core/log.h>
#include <roller/core/util.h>
#include <roller/core/thread.h>
#include <roller/core/serialization.h>

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
std::atomic_bool g_appRunning(false);

std::atomic_bool g_hltEnabled(false);
std::atomic_bool g_bkEnabled(false);
volatile float g_hltSetpoint = -100.0f;
volatile float g_bkSetpoint = -100.0f;
StringId g_hltTempProbeId = StringId::intern("28.3AA87D040000");
StringId g_bkTempProbeId = StringId::intern("28.EE9B8B040000");

CurrentLimiter g_currentLimiter(700, 35000); // base is 0.7 amps, total allowed 35 amps

// handleSignal
void handleSignal( i32 sig ) {
	Log::i( "sig %d caught, flagging app to stop running", sig );
	g_appRunning = false;
	FCGX_ShutdownPending();

	// hack to make fcgx actually die
	system("curl http://localhost/ab/ping &");
}

// handleRequest
void handleRequest( FCGX_Request& request );

// loop to update PID algorithms
void pidLoop();

// test Dummy controller
void handleStartDummy();
void handleStopDummy();
std::shared_ptr<DummyController> s_controller;

void configCurrentLimiter();

// main
i32 main( i32 argc, char** argv ) {

	try {
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
		configCurrentLimiter();

		// initialize temperature manager
		auto owfsManager = std::make_shared<OWFSHardwareManager>( "--usb all" );
		StringId owfsManagerID = DeviceManager::registerTemperatureSensorManager( owfsManager );

		// start PID thread
		Thread pidThread(pidLoop);
		pidThread.run();

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

				handleRequest(request);

			} catch( exception& e ) {
				Log::w( "Caught exception while trying to handle request (ignoring): %s", e.what());
				continue;
			}

		}

		Log::f("Joining PID thread...");
		pidThread.join();

		return 0;
	} catch (const exception& e) {
		Log::w("Caught exception in main, exiting program: %s", e.what());
	} catch (...) {
		Log::w("Caught unknown exception in main, exiting program");
	}

	return 1;

}

// handleRequest
void handleRequest( FCGX_Request& request ) {

	std::string scriptName = FCGX_GetParam("SCRIPT_NAME", request.envp);
	std::string pathInfo = FCGX_GetParam("PATH_INFO", request.envp);
	std::string requestMethod = FCGX_GetParam("REQUEST_METHOD", request.envp);
	std::string requestUri = FCGX_GetParam("REQUEST_URI", request.envp);

	Log::i( "handling request uri:    %s", requestUri.c_str());
	Log::i( "         script name:    %s", scriptName.c_str());
	Log::i( "         path info:      %s", pathInfo.c_str());
	Log::i( "         request method: %s", requestMethod.c_str());

	// parse URI
	std::map<std::string, std::string> params;
	std::string baseUri;

	std::string handlerName;

	size_t paramsStart = requestUri.find_first_of('?');
	if (paramsStart != std::string::npos) {

		baseUri = requestUri.substr(0, paramsStart);
		std::string paramsString = requestUri.substr(paramsStart + 1);

		std::vector<std::string> parts = split(paramsString, "&");
		for (const std::string& part : parts) {
			std::vector<std::string> paramParts = split(part, "=");
			if (paramParts.size() != 2) {
				throw RollerException("Illegal param (%s) in URI %s", part.c_str(), requestUri.c_str());
			}

			params[paramParts[0]] = paramParts[1];
			Log::f("request param: %s = %s", paramParts[0].c_str(), paramParts[1].c_str());
		}
	} else {
		baseUri = requestUri;
	}

	handlerName = params["cmd"];

	
	// get last part of URI
	// TODO: clean this up
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
		g_currentLimiter.enablePin(18);

	} else if (handlerName == "p1_off") {
		g_currentLimiter.disablePin(18);

	} else if (handlerName == "p2_on") {
		g_currentLimiter.enablePin(27);

	} else if (handlerName == "p2_off") {
		g_currentLimiter.disablePin(27);

	} else if (handlerName == "valve_on") {
		g_currentLimiter.enablePin(22);

	} else if (handlerName == "valve_off") {
		// system("gpio export 22 out; gpio -g write 22 0");
		g_currentLimiter.disablePin(22);

	} else if (handlerName == "test_hlt") {
		g_currentLimiter.enablePin(24); // HLT safety
		g_hltSetpoint = 45.0f;
		g_hltEnabled = true;

	} else if (handlerName == "test_bk") {
		g_currentLimiter.enablePin(10); // HLT safety
		g_bkSetpoint = 45.0f;
		g_bkEnabled = true;

	} else if (handlerName == "configure_bk") {

		bool enabled = Serialization::toBool(params["enabled"]);
		if (enabled) {

			// get optional "critical" field
			bool critical = Serialization::toBool(params["critical"]);
			// TODO: use critical field (requires mods to CurrentLimiter)

			if (params["type"] == "") {
				throw RollerException("configure_bk requires type when enabled=true");
			} else if (params["type"] == "pid") {
				if (params["setpoint"] == "") {
					throw RollerException("configure_bk requires setpoint when type=pid");
				} else {
					f32 setpoint = Serialization::toF32(params["setpoint"]);
					g_bkSetpoint = setpoint;
					g_bkEnabled = true;
					g_currentLimiter.enablePin(10);
				}
			} else if (params["type"] == "pwm") {
				if (params["load"] == "") {
					throw RollerException("configure_bk requires load when type=pwm");
				} else {
					f32 load = Serialization::toF32(params["load"]);
					CurrentLimiter::PinConfiguration pinConfiguration = g_currentLimiter.getPinConfiguration(17);
					pinConfiguration._pwmLoad = load;
					g_currentLimiter.updatePinConfiguration(pinConfiguration);
					g_currentLimiter.enablePin(10);
					g_bkEnabled = false;
				}
			} else {
				throw RollerException("illegal type parameter (%s) for configure_bk", params["type"].c_str());
			}
		} else {

			Log::i("Turning off BK");

			// set pwm load to 0
			CurrentLimiter::PinConfiguration pinConfiguration = g_currentLimiter.getPinConfiguration(17);
			pinConfiguration._pwmLoad = 0.0f;
			g_currentLimiter.updatePinConfiguration(pinConfiguration);

			// disable safety
			g_currentLimiter.disablePin(10);

			// flag bk pid to stop
			g_bkEnabled = false;
		}
	} else if (handlerName == "configure_hlt") {

		bool enabled = Serialization::toBool(params["enabled"]);
		if (enabled) {
			if (params["type"] == "") {
				throw RollerException("configure_hlt requires type when enabled=true");
			} else if (params["type"] == "pid") {
				if (params["setpoint"] == "") {
					throw RollerException("configure_hlt requires setpoint when type=pid");
				} else {
					f32 setpoint = Serialization::toF32(params["setpoint"]);
					g_hltSetpoint = setpoint;
					g_hltEnabled = true;
					g_currentLimiter.enablePin(24);
				}
			} else if (params["type"] == "pwm") {
				if (params["load"] == "") {
					throw RollerException("configure_hlt requires load when type=pwm");
				} else {
					f32 load = Serialization::toF32(params["load"]);
					CurrentLimiter::PinConfiguration pinConfiguration = g_currentLimiter.getPinConfiguration(4);
					pinConfiguration._pwmLoad = load;
					g_currentLimiter.updatePinConfiguration(pinConfiguration);
					g_currentLimiter.enablePin(24);
					g_hltEnabled = false;
				}
			} else {
				throw RollerException("illegal type parameter (%s) for configure_hlt", params["type"].c_str());
			}
		} else {
			Log::i("Turning off HLT");

			// set pwm load to 0
			CurrentLimiter::PinConfiguration pinConfiguration = g_currentLimiter.getPinConfiguration(4);
			pinConfiguration._pwmLoad = 0.0f;
			g_currentLimiter.updatePinConfiguration(pinConfiguration);

			// disable safety
			g_currentLimiter.disablePin(24);

			// flag hlt pid to stop
			g_hltEnabled = false;
		}

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

void configCurrentLimiter() {

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
	g_currentLimiter.addPinConfiguration(
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
	g_currentLimiter.addPinConfiguration(
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
	g_currentLimiter.addPinConfiguration(
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
	g_currentLimiter.addPinConfiguration(
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
	g_currentLimiter.addPinConfiguration(
			config,
			DeviceManager::getSwitch(RaspiGPIOSwitchManager::s_id, StringId::format("%d", config._pinNumber)));

	// BK element 
	config._name = "BK Element";
	config._pinNumber = 17;
	config._milliAmps = 23000;
	config._critical = false;
	config._pwm = true;
	config._pwmFrequency = 20;
	config._pwmLoad = 0.0f;
	g_currentLimiter.addPinConfiguration(
			config,
			DeviceManager::getSwitch(RaspiGPIOSwitchManager::s_id, StringId::format("%d", config._pinNumber)));

	// HLT element 
	config._name = "HLT Element";
	config._pinNumber = 4;
	config._milliAmps = 23000;
	config._critical = false;
	config._pwm = true;
	config._pwmFrequency = 20;
	config._pwmLoad = 0.0f;
	g_currentLimiter.addPinConfiguration(
			config,
			DeviceManager::getSwitch(RaspiGPIOSwitchManager::s_id, StringId::format("%d", config._pinNumber)));
}

void pidLoop() {

	// TODO: find a better home for this
	TemperatureManager temperatureManager;
	temperatureManager.run();

	bool hltSetup = false;
	bool bkSetup = false;

	shared_ptr<PID> hltPID;
	shared_ptr<PID> bkPID;

	int64_t lastHLTPIDUpdateTime = getTime();
	int64_t lastBKPIDUpdateTime = getTime();


	while (g_appRunning) {

		// update HLT PID if needed
		if (g_hltEnabled) {

			// initialize HLT PID if needed
			if (! hltSetup) {
				hltPID.reset(new PID(15.0f, 1.0f, 3.0f, g_hltSetpoint, -100.0f, 100.0f));
				hltPID->setErrorAccumulationCap(1.5f);
				hltSetup = true;

				// TODO: enable safety pin
			}

			hltPID->setSetpoint(g_hltSetpoint);

			ProbeStats stats = temperatureManager.getProbeStats(g_hltTempProbeId);
			int32_t temp = stats._lastTemp;

			int64_t now = getTime();
			hltPID->update( ((float)temp / 1000.f), ((float)(now - lastHLTPIDUpdateTime) / 1000.0f) );
			lastHLTPIDUpdateTime = now;

			// TODO: review / optimize -- this triggers a lot of work
			CurrentLimiter::PinConfiguration pinConfiguration = g_currentLimiter.getPinConfiguration(4);
			pinConfiguration._pwmLoad = std::max(0.0f, (hltPID->getOutput() / 100.0f ));
			g_currentLimiter.updatePinConfiguration(pinConfiguration);

		} else if (hltSetup) {
			Log::i("killing HLT pid...");
			hltPID.reset();
			hltSetup = false;
		}

		// update BK PID if needed
		if (g_bkEnabled) {

			// initialize BK PID if needed
			if (! bkSetup) {
				bkPID.reset(new PID(15.0f, 1.0f, 3.0f, g_bkSetpoint, -100.0f, 100.0f));
				bkPID->setErrorAccumulationCap(1.5f);
				bkSetup = true;

				// TODO: enable safety pin
			}

			bkPID->setSetpoint(g_bkSetpoint);

			ProbeStats stats = temperatureManager.getProbeStats(g_bkTempProbeId);
			int32_t temp = stats._lastTemp;

			int64_t now = getTime();
			bkPID->update( ((float)temp / 1000.f), ((float)(now - lastBKPIDUpdateTime) / 1000.0f) );
			lastBKPIDUpdateTime = now;

			// TODO: review / optimize -- this triggers a lot of work
			CurrentLimiter::PinConfiguration pinConfiguration = g_currentLimiter.getPinConfiguration(17);
			pinConfiguration._pwmLoad = std::max(0.0f, (bkPID->getOutput() / 100.0f ));
			g_currentLimiter.updatePinConfiguration(pinConfiguration);

		} else if (bkSetup) {
			Log::i("killing BK pid...");
			bkPID.reset();
			bkSetup = false;
		}
		
		usleep(1 * 1000 * 1000);
	}
}

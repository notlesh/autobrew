#include <unistd.h>
#include <signal.h>

#include <boost/program_options.hpp>

#include <roller/core/types.h>
#include <roller/core/log.h>
#include <roller/core/util.h>

#include "device_manager.h"

#include "owfs_sensors.h"

#include "raspi_gpio_switch.h"

#include "pid.h"
#include "pwm.h"

using namespace roller;
namespace po = boost::program_options;

// globals
bool g_appRunning = false;

// handleSignal
void handleSignal( i32 sig ) {
	Log::i( "sig %d caught, flagging app to stop running", sig );
	g_appRunning = false;
}

// mainNULL
i32 main( i32 argc, char** argv ) {

	string tempProbeId;
	i32 safetyPinId;
	i32 ssrPinId;
	f32 setpoint;
	f32 tolerance;
	f32 pGain;
	f32 iGain;
	f32 dGain;

	po::options_description mainOptions( "Main options" );
	mainOptions.add_options()
		("help,h",																"produce this help message")
		("temp-probe",		po::value<string>(&tempProbeId)->required(),		"id of the temperature probe to use (required)")
		("pin-id",			po::value<i32>(&ssrPinId)->required(),				"id of the pin to be controlled (required)")
		("safety-id",		po::value<i32>(&safetyPinId)->default_value(-1),	"id of the pin used for safety circuit (-1 for none)")
		("setpoint,s",		po::value<f32>(&setpoint)->required(),				"setpoint (required)")
		("tolerance,t",		po::value<f32>(&tolerance)->default_value(1.0f),	"tolerance")
		("p-gain,p",		po::value<f32>(&pGain)->default_value(15.0f),		"P gain")
		("i-gain,i",		po::value<f32>(&iGain)->default_value(1.0f),		"I gain")
		("d-gain,d",		po::value<f32>(&dGain)->default_value(3.0f),		"D gain")
		;

	po::variables_map mainOptionsMap;
	po::store( po::parse_command_line( argc, argv, mainOptions ), mainOptionsMap );

	if ( mainOptionsMap.count( "help" )) {
		std::cout << mainOptions << std::endl;
		return 0;
	}

	// will handle required, etc.
	po::notify( mainOptionsMap );

	Log::f( "temp probe id: %s", tempProbeId.c_str() );
	Log::f( "pin id: %d", ssrPinId );
	Log::f( "safety pin id: %d", safetyPinId );
	Log::f( "setpoint: %.2f", setpoint );
	Log::f( "tolerance: %.2f", tolerance );

	// XXX: testing
	if ( true ) { return 0; }

	g_appRunning = true;

	setbuf( stdout, nullptr );

	signal( SIGINT, handleSignal );
	signal( SIGHUP, handleSignal );
	signal( SIGKILL, handleSignal );
	signal( SIGTERM, handleSignal );

	Log::setLogLevelMode( LOG_LEVEL_MODE_UNIX_TERMINAL );

	// set up devman

	auto owfsManager = std::make_shared<OWFSHardwareManager>( "--usb all" );
	StringId owfsManagerID = DeviceManager::registerTemperatureSensorManager( owfsManager );

	auto raspiGPIOManager = std::make_shared<RaspiGPIOSwitchManager>();
	StringId raspiSwitchManagerID = DeviceManager::registerSwitchManager( raspiGPIOManager );

	PID pid( 
			pGain,
			iGain,
			dGain,
			setpoint,
			-100.0f,	// min output
			100.0f );	// max output
	pid.setErrorAccumulationCap( 1.5f );

	auto sensor = DeviceManager::getTemperatureSensor(
			owfsManagerID,
			StringId::intern( tempProbeId ));

	auto ssrPin = DeviceManager::getSwitch(
			raspiSwitchManagerID,
			StringId::format( "%d", ssrPinId ));

	// turn safety pin on if it was supplied
	if ( safetyPinId >= 0 ) {
		auto safetyPin = DeviceManager::getSwitch(
				raspiSwitchManagerID,
				StringId::format( "%d", safetyPinId ));

		safetyPin->setState( true );
	}

	PWMController pwm( ssrPin );
	pwm.setFrequency( 20 ); // 20 hz -- need to be less than our 60 hz 240v output
	Log::i( "Freq: %u", pwm.getFrequency() );
	pwm.unpause();

	i64 time = -1;
	i32 temp = -1;

	i64 lastPIDUpdateTime = getTime();
	i64 lastPrintTime = getTime() - 5000;

	while ( g_appRunning ) {

		temp = sensor->getTemperature( time );
		i64 now = getTime();

		// print every 5s
		if ( now - lastPrintTime > 5000 ) {
			Log::i( "%lld : %d", now, temp );
			Log::i( "PID: %f", pid.getOutput() );
			lastPrintTime += 5000;
		}

		// update pid
		pid.update( ((f32)temp / 1000.f), ((f32)(now - lastPIDUpdateTime) / 1000.0f) );
		lastPIDUpdateTime = now;

		pwm.setLoadCycle( (pid.getOutput() / 100.0f ));

		usleep( 50 * 1000 );
	}

	pwm.pause();
	pwm.stop();
	pwm.join();

	ssrPin->setState( false );

	// turn safety pin off if it was supplied
	if ( safetyPinId >= 0 ) {
		auto safetyPin = DeviceManager::getSwitch(
				raspiSwitchManagerID,
				StringId::format( "%d", safetyPinId ));

		safetyPin->setState( false );
	}

	DeviceManager::unregisterTemperatureSensorManager( owfsManagerID );
	DeviceManager::unregisterSwitchManager( raspiSwitchManagerID );

	return 0;
}

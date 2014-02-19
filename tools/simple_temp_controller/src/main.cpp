#include <unistd.h>
#include <signal.h>

#include <roller/core/types.h>
#include <roller/core/log.h>
#include <roller/core/util.h>

#include "device_manager.h"

#include "owfs_sensors.h"

#include "raspi_gpio_switch.h"

#include "pid.h"
#include "pwm.h"

using namespace roller;

// globals
bool g_appRunning = false;

// handleSignal
void handleSignal( i32 sig ) {
	Log::i( "sig %d caught, flagging app to stop running", sig );
	g_appRunning = false;
}

// mainNULL
i32 main( i32 argc, char** argv ) {

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
			15.0f,		// p gain
			1.0f,		// i gain
			3.0f,		// d gain
			57.0f,		// setpoint
			// 51.666f,	// setpoint
			-100.0f,	// min output
			100.0f );	// max output
	pid.setErrorAccumulationCap( 1.5f );

	auto sensor = DeviceManager::getTemperatureSensor(
			owfsManagerID,
			StringId::intern( "28.3AA87D040000" ));

	auto ssrPin = DeviceManager::getSwitch(
			raspiSwitchManagerID,
			StringId::intern( "4" ));
	auto safetyPin = DeviceManager::getSwitch(
			raspiSwitchManagerID,
			StringId::intern( "24" ));

	safetyPin->setState( true );

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
	safetyPin->setState( false );

	DeviceManager::unregisterTemperatureSensorManager( owfsManagerID );
	DeviceManager::unregisterSwitchManager( raspiSwitchManagerID );

	return 0;
}

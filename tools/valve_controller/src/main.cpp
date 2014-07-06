#include <unistd.h>
#include <signal.h>

#include <boost/program_options.hpp>

#include <roller/core/types.h>
#include <roller/core/log.h>
#include <roller/core/util.h>

#include "device_manager.h"

#include "owfs_sensors.h"

#include "raspi_gpio_switch.h"

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

	i32 floatPinId;
	i32 valvePinId;

	po::options_description mainOptions( "Main options" );
	mainOptions.add_options()
		("help,h",															"produce this help message")
		("valve,v",			po::value<i32>(&valvePinId)->required(),		"ID of the valve pin to use")
		("float,f",			po::value<i32>(&floatPinId)->required(),		"ID of the float switch pin to use")
		;

	po::variables_map mainOptionsMap;
	po::store( po::parse_command_line( argc, argv, mainOptions ), mainOptionsMap );

	if ( mainOptionsMap.count( "help" )) {
		std::cout << mainOptions << std::endl;
		return 0;
	}

	// will handle required, etc.
	po::notify( mainOptionsMap );

	Log::f( "float pin id: %d", floatPinId );
	Log::f( "valve pin id: %d", valvePinId );

	g_appRunning = true;

	setbuf( stdout, nullptr );

	signal( SIGINT, handleSignal );
	signal( SIGHUP, handleSignal );
	signal( SIGKILL, handleSignal );
	signal( SIGTERM, handleSignal );

	Log::setLogLevelMode( LOG_LEVEL_MODE_UNIX_TERMINAL );

	// set up devman

	auto raspiGPIOManager = std::make_shared<RaspiGPIOSwitchManager>();
	StringId raspiSwitchManagerID = DeviceManager::registerSwitchManager( raspiGPIOManager );

	auto floatPin = DeviceManager::getSwitch(
			raspiSwitchManagerID,
			StringId::format( "%d", floatPinId ));
	floatPin->setMode( Direction::IN );

	auto valvePin = DeviceManager::getSwitch(
			raspiSwitchManagerID,
			StringId::format( "%d", valvePinId ));
	valvePin->setMode( Direction::OUT );

	while ( g_appRunning ) {

		valvePin->setState( floatPin->getState() );

		usleep( 50 * 1000 );
	}

	valvePin->setState( false );

	DeviceManager::unregisterSwitchManager( raspiSwitchManagerID );

	return 0;
}

#include "valve_controller.h"

#include <unistd.h>

#include <roller/core/log.h>
#include <roller/core/exception.h>

#include "device_manager.h"
#include "raspi_gpio_switch.h"

// Constructor
ValveController::ValveController(
		CurrentLimiter& currentLimeter,
		uint32_t floatSwitchId,
		uint32_t valveSwitchId)
			: _thread(std::bind(&ValveController::run, this))
			, _started(false)
			, _running(false)
			, _mode(Mode::OFF)
			, _currentLimeter(currentLimeter)
			, _floatSwitchId(floatSwitchId)
			, _valveSwitchId(valveSwitchId)
{
}

// Destructor
ValveController::~ValveController() {
	Log::f( "Join()ing valve controller..." );
	_thread.join();
}

// start
void ValveController::setMode(Mode mode) {
	_mode = mode;
}

// start
void ValveController::start() {
	if (_started) {
		throw RollerException("Cannot start ValveController that has already been started");
	}

	_running = true;
	_started = true;
	_thread.run();
}

// stop
void ValveController::stop() {
	_running = false;
}

// join
void ValveController::join() {
	// TODO: throw exception if running?
	_thread.join();
}

// run
void ValveController::run() {

	Log::i("ValveController thread starting");

	Mode mode = _mode;

	auto valveSwitch = DeviceManager::getSwitch(
			RaspiGPIOSwitchManager::s_id,
			StringId::format("%d", _valveSwitchId));
	auto floatSwitch = DeviceManager::getSwitch(
			RaspiGPIOSwitchManager::s_id,
			StringId::format("%d", _floatSwitchId));

	while (_running) {

		bool changed = (mode != _mode);
		if (changed) {
			mode = _mode;
		}
		
		switch (_mode) {
		case Mode::OFF:
			if (changed) {
				_currentLimeter.disablePin(_valveSwitchId);
			}
			// TODO: turn off 
			break;

		case Mode::ON:
			if (changed) {
				_currentLimeter.enablePin(_valveSwitchId);
			}
			// TODO: turn on
			break;

		case Mode::FLOAT:
			bool floatState = floatSwitch->getState();
			if (floatState) {
				_currentLimeter.enablePin(_valveSwitchId);
			} else {
				_currentLimeter.disablePin(_valveSwitchId);
			}
			break;
		}

		usleep(100 * 1000);
	}

	Log::i("ValveController thread exiting");
	_running = false;
	_started = false;
}

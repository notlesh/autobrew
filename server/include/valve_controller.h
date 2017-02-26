#ifndef __AB2_VALVE_CONTROLLER_INCLUDED__
#define __AB2_VALVE_CONTROLLER_INCLUDED__

#include <atomic>

#include <roller/core/types.h>
#include <roller/core/thread.h>

#include <json.hpp>

#include "current_limiter.h"

using namespace roller;

/**
 * Valve controller. Reads from the float switch GPIO input and turns the valve on and
 * off accordingly.
 *
 * The ValveController can be set to several modes: On, off, and float. Float is described
 * above.
 */
class ValveController {

public:

	/**
	 * Enum representing the ValveController mode.
	 */
	enum class Mode {
		OFF,
		ON,
		FLOAT
	};

	/**
	 * Constructor. The thread will not be started until start() is called.
	 */
	ValveController(
			CurrentLimiter& currentLimeter, 
			uint32_t floatSwitchId,
			uint32_t valveSwitchId);

	/**
	 * Destructor. If the thread is currently running, this will join. If this is unacceptable,
	 * call stop() prior to destroying the controller.
	 */
	~ValveController();

	/**
	 * Set the current mode of the controller.
	 */
	void setMode(Mode mode);

	/**
	 * Get the mode
	 */
	Mode getMode();

	/**
	 * Start the thread. Call this to kick off the thread. Should only be called once.
	 */
	void start();

	/**
	 * Stop the thread. Can be called whether the thread has finished or not, but should
	 * only be called after the thread has been started.
	 */
	void stop();

	/**
	 * Joins the thread. Should only be called once the thread has been stopped.
	 */
	void join();

private:

	/**
	 * Our thread entry point.
	 */
	void run();

	Thread _thread;
	std::atomic_bool _started;
	std::atomic_bool _running;
	std::atomic<Mode> _mode;
	CurrentLimiter& _currentLimeter;
	uint32_t _floatSwitchId;
	uint32_t _valveSwitchId;
};

void to_json(nlohmann::json& j, const ValveController::Mode& mode);

#endif // __AB2_VALVE_CONTROLLER_INCLUDED__

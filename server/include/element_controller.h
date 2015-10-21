#ifndef __AB2_SERVER_PWM_CONTROLLER_INCLUDED__
#define __AB2_SERVER_PWM_CONTROLLER_INCLUDED__

#include <atomic>

#include <roller/core/types.h>
#include <roller/core/thread.h>

#include "server_controller.h"

using namespace roller;

/**
 * Controller for heating elements. A heating element has a safety switch and
 * a main control solid state relay that actually fires the element.
 *
 * The element will be controlled through the SSR via PWM. The PWM load can 
 * optionally be controlled through a PID. 
 *
 * Configuration brainstorm:
 *
 * - hardware related:
 *   - safety pin
 *   - ssr pin
 * - pwm output:
 *   - frequency
 *   - load
 * - pid related:
 *   - p
 *   - i
 *   - d
 *   - setpoint
 *   - min output
 *   - max output
 *   - error accumulation cap
 */
class ElementServerController : public ServerController {

public:

	/**
	 * Do run
	 */
	virtual void doRun() {
		while ( ! isCancelled()) {
			Log::i( "DUMMY RUNNING" );
			usleep( 1 * 1000 * 1000 );
		}
	}
};

#endif // __AB2_SERVER_PWM_CONTROLLER_INCLUDED__

#ifndef __AB2_SERVER_DUMMY_CONTROLLER_INCLUDED__
#define __AB2_SERVER_DUMMY_CONTROLLER_INCLUDED__

#include <atomic>

#include <roller/core/types.h>
#include <roller/core/thread.h>

#include "server_controller.h"

using namespace roller;

/**
 * ServerController test class.
 */
class DummyController : public ServerController {

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

#endif // __AB2_SERVER_DUMMY_CONTROLLER_INCLUDED__

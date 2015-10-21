#include "server_controller.h"

#include <roller/core/log.h>
#include <roller/core/exception.h>

// Constructor
ServerController::ServerController() 
		: _thread(std::bind(&ServerController::run, this))
		, _cancelled(false)
		, _started(false)
		, _finished(false)
{
}

// Destructor
ServerController::~ServerController() {
	Log::f( "Join()ing server controller..." );
	_thread.join();
}

// isCancelled
bool ServerController::isCancelled() {
	return _cancelled;
}

// start
void ServerController::start() {
	if ( _started ) {
		throw RollerException( "Cannot start ServerController that has already been started" );
	}

	_thread.run();

}

// stop
void ServerController::stop() {
	if ( ! _finished ) {
		_cancelled = true;
	}
}

// isFinished
bool ServerController::isFinished() {
	return _finished;
}

// join
void ServerController::join() {
	_thread.join();
}

// run
void ServerController::run() {
	doRun();
	_finished = true;
}

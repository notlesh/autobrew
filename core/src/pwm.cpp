#include "pwm.h"

#include <memory>

#include <roller/core/log.h>
#include <roller/core/util.h>

using namespace roller;
using namespace devman;
using std::shared_ptr;

// Constructor
PWMController::PWMController( shared_ptr<Switch> ioSwitch ) :
				_ioSwitch(ioSwitch),
				_thread( std::bind( &PWMController::doStart, this )),
				_load(0.0f),
				_periodUS(10000), // 100 hz
				_paused(true),
				_running(true),
				_history(10) {
	_thread.run();
}

// setLoadCycle
void PWMController::setLoadCycle( f32 load ) {
	MutexLocker locker( _lock );
	_load = load;
}

// getLoadCycle
f32 PWMController::getLoadCycle() const {
	MutexLocker locker( _lock );
	return _load;
}

// setFrequency
void PWMController::setFrequency( ui32 hz ) {
	MutexLocker locker( _lock );
	_periodUS = (1000000 / hz);
}

// getFrequency
ui32 PWMController::getFrequency() const {
	MutexLocker locker( _lock );
	return (1000000 / _periodUS);
}

// pause
void PWMController::pause() {
	MutexLocker locker( _lock );
	_paused = true;
}

// unpause
void PWMController::unpause() {
	MutexLocker locker( _lock );
	_paused = false;
}

// stop
void PWMController::stop() {
	MutexLocker locker( _lock );
	_running = false;
}

// join
void PWMController::join() {
	MutexLocker locker( _lock );
	if ( _running ) {
		throw RollerException( "Cannot join an unstopped PWM thread" );
	}

	locker.unlock();
	_thread.join();
}

// doStart
void PWMController::doStart() {

	MutexLocker locker( _lock );

	i64 lastRun = getTimeMicros();
	bool on = false;
	while ( _running ) {

		if ( _paused ) {
			_ioSwitch->setState( false );
			on = false;
		} else {

			// add last state to our history
			_history.add( (on ? 1 : 0) ); // add a 1 if we're on or a 0 if off
			ui32 sum = _history.sum();

			if ( ((f32)sum / ((f32)_history.size())) > _load ) {
				// if ( on ) {
					// Log::f( "PWM: off" );
				// }
				// we've been on too much, turn off
				_ioSwitch->setState( false );
				on = false;
			} else {
				// if ( ! on ) {
					// Log::f( "PWM: on" );
				// }
				// haven't been on enough, turn on
				_ioSwitch->setState( true );
				on = true;
			}
		}

		// TODO: find a way to be more precise with time
		// TODO: this is messy -- rewrite. should never get negative time amounts...

		// sleep until it's time to run again
		i64 now = getTimeMicros();

		i64 sleepTime = _periodUS - (now - lastRun);
		lastRun += _periodUS;

		if ( sleepTime > 0 ) {

			locker.unlock();
			usleep( sleepTime );
			locker.lock();
		} else {
			Log::w( "Negative sleep time!" );
		}
	}

	_ioSwitch->setState( false );

}

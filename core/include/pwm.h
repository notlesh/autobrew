#ifndef __AB2_PWM_H_INCLUDED__
#define __AB2_PWM_H_INCLUDED__

#include <memory>

#include <roller/core/types.h>
#include <roller/core/thread.h>
#include <roller/core/mutex.h>
#include <roller/core/ring_buffer.h>

#include "devices/switch.h"

using namespace roller;
using namespace devman;
using std::shared_ptr;

/**
 * PWM Controller. Turns a pin on and off very frequently.
 *
 * This class uses its own internal thread to manage the pin. All functions are
 * threadsafe.
 *
 * The PWMController starts in a paused state, so be sure to call unpause()
 * in order to actually start the controller.
 */
class PWMController {

public:

	/**
	 * Constructor.
	 */
	PWMController( shared_ptr<Switch> ioSwitch );

	/**
	 * Set the load cycle. Should range from 0-1.
	 */
	void setLoadCycle( f32 load );

	/**
	 * Returns the current load cycle.
	 */
	f32 getLoadCycle() const;

	/**
	 * Set the frequency. This dictates the shortest length of time that
	 * the pin will be turned on or off. Internally, this number is converted
	 * to period.
	 *
	 * Should be kept under 1 MHz. Defaults to 100 hz.
	 */
	void setFrequency( ui32 hz );

	/**
	 * Returns the frequency. Maybe slightly different than the argument
	 * given to setFrequency() due to rounding errors.
	 */
	ui32 getFrequency() const;

	/**
	 * Pause the controller. The pin will be turned off before pausing.
	 * The effect of pausing will happen at some time in the near future.
	 */
	void pause();

	/**
	 * Resume the controller.
	 */
	void unpause();

	/**
	 * Stop the controller. The pin will be turned off some time in
	 * the near future and the thread will stop running and join.
	 *
	 * The amount of time it takes to stop the thread is determined,
	 * in part, by the PWM frequency.
	 */
	void stop();

	/**
	 * Join the thread. This will block until the thread has joined.
	 * The PWMController must have been stop()ed previously.
	 */
	void join();

private:

	shared_ptr<Switch> _ioSwitch;
	Thread _thread;
	mutable Mutex _lock;
	f32 _load;
	i64 _periodUS;
	bool _paused;
	bool _running;
	RingBuffer<ui32> _history;

	/**
	 * Thread entry point
	 */
	void doStart();
};

#endif // __AB2_PWM_H_INCLUDED__

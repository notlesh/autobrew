#ifndef __AB2_PID_H_INCLUDED__
#define __AB2_PID_H_INCLUDED__

#include <roller/core/types.h>
#include <roller/core/ring_buffer.h>

using namespace roller;

/**
 * PID controller.
 *
 * Construct with parameters, feed information, and get PID output.
 */
class PID {

public:

	/**
	 * Constructor. Takes the PID tuning parameters kp, ki, and kd.
	 */
	PID( f32 kp, f32 ki, f32 kd, f32 setpoint, f32 minOutput, f32 maxOutput );

	/**
	 * Update. Should be called at regular[ish] intervals. Returns the updated PID output.
	 *
	 * @param dt is the change in time since this was last called. The unit should be in seconds.
	 */
	f32 update( f32 input, f32 dt );

	/**
	 * Get the current PID output
	 */
	f32 getOutput();

	/**
	 * Sets ErrorAccumulationCap
	 *
	 * @param errorAccumulationCap is the new value for ErrorAccumulationCap
	 */
	void setErrorAccumulationCap( f32 errorAccumulationCap );
	
	/**
	 * Returns ErrorAccumulationCap
	 *
	 * @return ErrorAccumulationCap
	 */
	f32 getErrorAccumulationCap() const;

private:

	// config
	f32 _kp;
	f32 _ki;
	f32 _kd;
	f32 _setpoint;
	f32 _minOutput;
	f32 _maxOutput;
	f32 _errorAccumulationCap;

	// state
	f32 _lastInput;
	f32 _output;
	f32 _errorSum;
	RingBuffer<f32> _inputAccumulation;
};

#endif // __AB2_PID_H_INCLUDED__

#include "pid.h"

#include <roller/core/log.h>
#include <roller/core/math/math_util.h>

// Constructor
PID::PID( f32 kp, f32 ki, f32 kd, f32 setpoint, f32 minOutput, f32 maxOutput ) :
				_kp(kp),
				_ki(ki),
				_kd(kd),
				_setpoint(setpoint),
				_minOutput(minOutput),
				_maxOutput(maxOutput),
				_errorAccumulationCap(1000.0f),
				_lastInput(0),
				_output(0),
				_errorSum(0),
				_inputAccumulation(64) {
}

// update
f32 PID::update( f32 input, f32 dt ) {

	_inputAccumulation.add( input );

	if ( dt > 1.0f ) {
		static bool warnedOnce = false;
		if ( warnedOnce ) {
			Log::w( "Warning: PID delta time suspiciously high (make sure unit of measure is seconds)" );
			warnedOnce = true;
		}
	}

	// Log::f( "PID update ---------------" );

	f32 e = (_setpoint - input);
	// Log::f( "  e: (%f - %f) = %f", _setpoint, input, e );

	f32 p = e;

	// Log::f( "  _errorSum (%f) += (%f * %f) = %f", _errorSum, e, dt, (_errorSum + (e * dt)) );
	_errorSum += (e * dt);
	if (_errorSum > _errorAccumulationCap ) {
		// Log::f( "  ** clamping I, too high" );
		_errorSum = _errorAccumulationCap;
	} else if (_errorAccumulationCap < (- _errorAccumulationCap)) {
		_errorSum = (- _errorAccumulationCap);
	}
	f32 i = _errorSum;

	f32 filteredInput = _inputAccumulation.avg();
	f32 d = (filteredInput - _lastInput) / dt;
	_lastInput = filteredInput;
	if ( ! close( d, 0.0f, 0.0001f )) {
		Log::f( "  d: (%f - %f) / %f = %f", input, _lastInput, dt, d );
	}

	_output = (_kp * p) + (_ki * i) + (_kd * d);
	// Log::f( "  output: (%.2f * %.2f) + (%.2f * %.2f) + (%.2f * %.2f) = %f", _kp, p, _ki, i, _kd, d, _output );
	if (_output > _maxOutput ) {
		// Log::f( "  ** clamping output, too high" );
		_output = _maxOutput;
	} else if ( _output < _minOutput ) {
		// Log::f( "  ** clamping output, too low" );
		_output = _minOutput;
	}

	return _output;
}

// getOutput
f32 PID::getOutput() {
	return _output;
}

// setErrorAccumulationCap
void PID::setErrorAccumulationCap( f32 errorAccumulationCap ) {
	_errorAccumulationCap = errorAccumulationCap;
}

// getErrorAccumulationCap
f32 PID::getErrorAccumulationCap() const {
	return _errorAccumulationCap;
}

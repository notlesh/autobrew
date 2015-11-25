#include "current_limiter.h"

CurrentLimiter::CurrentLimiter(uint32_t baseMilliAmps, uint32_t maxMilliAmps)
		: _baseMilliAmps(baseMilliAmps)
		, _maxMilliAmps(maxMilliAmps)
{
}

CurrentLimiter::~CurrentLimiter() {

	// turn off all pins and PWM controllers
	for (const auto& entry : _pinConfigurations) {
		const PinConfiguration& config = entry.second;
		PinState& state = _pinStates[config._pinNumber];

		// if pwm, turn controller off and free it
		if (config._pwm) {
			state._pwmController->stop();
			state._pwmController->join();
			state._pwmController.reset();
		}

		state._ioSwitch->setState(false);
	}
}

void CurrentLimiter::addPinConfiguration(const PinConfiguration& config, std::shared_ptr<Switch> gpio) {

	uint32_t pin = config._pinNumber;
	
	// if we already have a pin for this pin number, reject
	if (_pinConfigurations.find(pin) != _pinConfigurations.end()) {
		throw RollerException("Cannot add a pin configuration for pin %u, already have one", pin);
	}

	_pinConfigurations[pin] = config;

	// also add state entry and create PWM controller if needed
	PinState state;
	state._pinNumber = pin;
	state._desiredState = false;
	state._overriden = false;
	state._enabled = false;
	state._pwmLoad = config._pwmLoad;
	state._ioSwitch = gpio;

	if (config._pwm) {
		state._pwmController = std::make_shared<PWMController>(gpio);
		state._pwmController->setLoadCycle(0.0f); // we won't set load until 
		state._pwmController->setFrequency(config._pwmFrequency);
	}

	_pinStates[pin] = state;
}

CurrentLimiter::PinConfiguration CurrentLimiter::getPinConfiguration(uint32_t pin) {

	auto itr = _pinConfigurations.find(pin);
	if (itr == _pinConfigurations.end()) {
		throw RollerException("Cannot add a pin configuration for pin %u, already have one", pin);
	}

	return itr->second;
}

void CurrentLimiter::updatePinConfiguration(const CurrentLimiter::PinConfiguration& config) {

	uint32_t pin = config._pinNumber;

	auto itr = _pinConfigurations.find(pin);
	if (itr == _pinConfigurations.end()) {
		throw RollerException("Cannot update pin configuration for non-existent pin %u", pin);
	}

	CurrentLimiter::PinConfiguration& existingConfig = itr->second;
	Log::f("Updating pin configuration for %s", config._name.c_str());

	if (existingConfig._pwm != config._pwm) {
		throw RollerException("Cannot convert a pin to PWW or vise versa after initialization");
	}

	aver(_pinStates.find(pin) != _pinStates.end());

	existingConfig = config;

	// TODO: only need to re-evaluateConfiguration() if certain things changed
	evaluateConfiguration();
}

void CurrentLimiter::enablePin(uint32_t pin) {

	auto itr = _pinConfigurations.find(pin);
	if (itr == _pinConfigurations.end()) {
		throw RollerException("Cannot enable non-existent pin %u", pin);
	}

	PinState& state = _pinStates[pin];

	if (! state._enabled) {
		state._desiredState = true;
		evaluateConfiguration();
	}
}

void CurrentLimiter::disablePin(uint32_t pin) {

	auto itr = _pinConfigurations.find(pin);
	if (itr == _pinConfigurations.end()) {
		throw RollerException("Cannot disable non-existent pin %u", pin);
	}

	PinState& state = _pinStates[pin];

	if (state._enabled) {
		state._desiredState = false;
		evaluateConfiguration();
	}
}

void CurrentLimiter::evaluateConfiguration() {

	// TODO: the implementation here should really pre-calculate all pin states, and then
	//		make 2 passes: one to disable any pins that should be off and then finally
	//		a second to turn on any pins that should be on

	Log::f("CurrentLimiter::evaluateConfiguration()");

	Log::f("  Current configuration:");
	for (const auto& entry : _pinConfigurations) {
		const PinConfiguration& config = entry.second;
		Log::f("  - Name:       %s", config._name.c_str());
		Log::f("    Pin Number: %u", config._pinNumber);
		Log::f("    Milliamps:  %u", config._milliAmps);
		Log::f("    Critical:   %s", (config._critical ? "T" : "F"));
		Log::f("    PWM:        %s", (config._pwm ? "T" : "F"));
		if (config._pwm) {
			Log::f("    PWM load:   %.3f", config._pwmLoad);
		}
	}

	uint32_t available = _maxMilliAmps;
	Log::f("  Max available current: %u mA", _maxMilliAmps);

	available -= _baseMilliAmps;
	Log::f("  Base current: %u mA", _baseMilliAmps);
	Log::f("  Available: %u mA", available);

	// first, turn on all critical non-pwm pins
	Log::f("  Turning on critical non-PWM pins...");
	for (const auto& entry : _pinConfigurations) {
		const PinConfiguration& config = entry.second;
		PinState& state = _pinStates[config._pinNumber];
		
		if (config._critical && ! config._pwm) {

			if (state._desiredState) {

				int32_t remainder = (available - config._milliAmps);
				Log::f("    %s: %u - %u = %d",
						config._name.c_str(),
						available,
						config._milliAmps, 
						remainder);

				if (remainder > 0) {
					// update set state
					state._overriden = false;
					state._enabled = true;

					// update available
					available = (uint32_t)remainder;

					// we don't turn the pin on here; we do that after
					// we've pins off and have calculated what is to be
					// turned on

				} else {
					Log::w("    %s won't fit (critical / non-PWM)");

					// update set state
					state._overriden = true;
					state._enabled = false;

					// turn pin off
					state._ioSwitch->setState(false);
				}

			} else {

				// pin wasn't desired anyway, turn off
				state._ioSwitch->setState(false);
			}
		}
	}

	// TODO:
	Log::f("  TODO:   critical PWM pins");
	Log::f("  TODO:   non-critical non-PWM pins");


	Log::f("  Turning on non-critical PWM pins...");

	// make a first pass to tally the desired mA
	double totalDesiredMilliAmps = 0.0f;
	for (const auto& entry : _pinConfigurations) {
		const PinConfiguration& config = entry.second;
		PinState& state = _pinStates[config._pinNumber];

		if (! config._critical && config._pwm) {
			double loadMA = ((float)config._milliAmps * config._pwmLoad);
			Log::f("    %s wants %.3f mA (%.3f * %u)",
					config._name.c_str(),
					(float)loadMA,
					config._pwmLoad,
					config._milliAmps);
			totalDesiredMilliAmps += loadMA;
		}
	}

	double availableRatio = ((double)available / totalDesiredMilliAmps);

	Log::f("    Total desired mA: %.1f", (float)totalDesiredMilliAmps);
	Log::f("    Available mA:     %u", available);
	Log::f("    Available ratio:  %.3f", availableRatio);

	// if there is enough to go around, give everyone what they want
	if (totalDesiredMilliAmps > 0.001f) { // TODO: this precludes very small current demands
		if (totalDesiredMilliAmps < (double)available) {
			for (const auto& entry : _pinConfigurations) {
				const PinConfiguration& config = entry.second;

				if (! config._critical && config._pwm) {
					PinState& state = _pinStates[config._pinNumber];
					state._pwmLoad = config._pwmLoad;
				}
			}
		} else {
			// not enough current left, divide up amongst the requestors
			for (const auto& entry : _pinConfigurations) {
				const PinConfiguration& config = entry.second;

				if (! config._critical && config._pwm) {
					// TODO: configure as desired
					double loadMA = (double)config._milliAmps * (double)config._pwmLoad;
					double load = (loadMA * ((double)available / totalDesiredMilliAmps));
					Log::f("    %s's portion: %.3f * %.3f = %.3f",
							config._name.c_str(),
							(float)loadMA,
							(float)availableRatio,
							(float)load);
					
					PinState& state = _pinStates[config._pinNumber];
					state._pwmLoad = availableRatio;
				}
			}
			
		}
	}

	// finally, turn on pins that we calculated should be on
	for (const auto& entry : _pinConfigurations) {
		const PinConfiguration& config = entry.second;
		PinState& state = _pinStates[config._pinNumber];

		if (config._pwm) {
			Log::i("Setting pin %d to %.3f (PWM), %u (freq)",
					config._pinNumber,
					state._pwmLoad,
					config._pwmFrequency);
			state._pwmController->setLoadCycle(state._pwmLoad);
			state._pwmController->setFrequency(config._pwmFrequency);
			state._pwmController->unpause();
		} else if (state._enabled) {
			state._ioSwitch->setState(true);
			Log::i("Setting pin %d to on", config._pinNumber);
		}
	}


}


#ifndef __AB_CURRENT_LIMITER_H
#define __AB_CURRENT_LIMITER_H

#include <string>
#include <map>

#include "pwm.h"

/**
 * The CurrentLimiter provides an interface for indirectly turning things
 * on and off. Each pin must be registered prior to turning it on. This
 * registration includes the following information:
 *
 * - pin number
 * - current controlled
 * - available for disabling (if approaching max overall current limit)
 * - pwm (yes/no)
 * - pwm frequency
 * - pwm load
 *
 * <b>Limiting logic</b>
 *
 * Any time a pin is enabled or disabled, or a configuration of a PWM changes,
 * the pin outputs will be reevaluated. The logic follows this pattern:
 *
 * 1) All critical on-off pins will be enabled in an undefined order as long as
 *		maximum current has not been reached.
 * 2) All critical PWM pins will be enabled at their desired load in an undefined
 *		order until the max current has been reached. Any critical PWM pin that
 *		would exceed the max current will be given the remaining current (its
 *		load will be reduced to accomodate this.)
 * 3) All non-critical pins will be enabled in an undefined order as long as
 *		maximum current has not been reached.
 * 4) If all remaining non-critical PWM pins can be fired at their desired load
 *		without exceeding the max current, they will be enabled as such. Otherwise,
 *		the remaining current will be divided amongst them according to their
 *		desired loads.
 *
 * <b>A note on PWM limiting.</b>
 *
 * PWM limits are established by load. The underlying PWMController threads will
 * be left to turn the PWM pins on and off as they are designed to do. This means
 * that limits will likely be exceeded insttantaneously, but over a longer period
 * of time (e.g. seconds) they will smooth out such that the average doess not
 * exceed the desired limits. PWM frequencies should be configured with this in mind.
 *
 * This should be sufficient for most desired situations (not exceeding fuses,
 * circuit breakers, FCGIs, etc. and should also be adequate for avoiding thermal
 * issues related to wiring insulation, etc.
 *
 * If instantaneous limits are absolutely required, consider that the maximum
 * instantaneous load permited by the CurrentLimiter will be the sum of all
 * enabled non-PWM pins as well as all enabled PWM pins as though they were set
 * to a full (1.0) load.
 */
class CurrentLimiter {

public:
	
	/**
	 * This structure represents a pin configuration.
	 */
	struct PinConfiguration {
		std::string _name = "";
		uint32_t _pinNumber = 0;
		uint32_t _milliAmps = 0;
		bool _critical = false;
		bool _pwm = false;
		uint32_t _pwmFrequency = 0;
		float _pwmLoad = 0.0f;
	};

	/**
	 * Constructor
	 *
	 * @param baseMilliAmps is the base current used by the system with no other
	 *		pins enabled.
	 * @param maxMilliAmps is the maximum amount of current the CurrentLimiter will
	 *		allow to be used at any point in time.
	 */
	CurrentLimiter(uint32_t baseMilliAmps, uint32_t maxMilliAmps);

	/**
	 * Destructor
	 *
	 * This will turn off all pins and stop all PWM threads.
	 */
	~CurrentLimiter();

	/**
	 * Add a pin configuration
	 */
	void addPinConfiguration(const PinConfiguration& config, std::shared_ptr<Switch> gpio);

	/**
	 * Returns the pin configuration for a given pin
	 */
	PinConfiguration getPinConfiguration(uint32_t pin);

	/**
	 * Update a pin configuration
	 */
	void updatePinConfiguration(const PinConfiguration& config);

	/**
	 * Enable a pin. If this is a PWM configured pin, it will be set to its
	 * last configured PWM load/frequency.
	 */
	void enablePin(uint32_t pin);

	/**
	 * Disable a pin.
	 */
	void disablePin(uint32_t pin);

private:

	/**
	 * This structure keeps up with the state of a pin, including the controller
	 * (either a Switch or a PWMController as necessary) and also the overriden
	 * configuration for a pin (whether or not it's disabled if on/off or the
	 * scaled-back load if it's a PWM).
	 */
	struct PinState {
		uint32_t _pinNumber = 0;
		bool _desiredState = false;
		bool _overriden = false;
		bool _enabled = false; // override for non-pwm
		float _pwmLoad = 0.0f; // override for pwm
		std::shared_ptr<Switch> _ioSwitch; // valid whether pwm or not
		std::shared_ptr<PWMController> _pwmController; // only if pwm. includes dedicated thread
	};

	std::map<uint32_t, PinConfiguration> _pinConfigurations;
	std::map<uint32_t, PinState> _pinStates;
	uint32_t _baseMilliAmps = 0;
	uint32_t _maxMilliAmps = 0;

	/**
	 * Evaluate the pin configurations and override any as necessary.
	 *
	 * This is called when:
	 *
	 * 1) A pin's enabled state changes
	 * 2) A PWM pin's load configuration changes
	 * 3) A pin is removed.
	 */
	void evaluateConfiguration();
};

#endif // __AB_CURRENT_LIMITER_H

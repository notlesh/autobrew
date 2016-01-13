webix.protoUI({
	name: "elementPanel",
	defaults: {
		modal: true,
		borderless: true,
		body: {
			view: "form",
			id: "config_form",
			width: 300,
			elements: [
				{ view: "select", id: "config_type_select", label: "Type", value: 0, labelWidth: 60, options: [
					{id: "off", value: "Off"},
					{id: "pwm", value: "PWM"},
					{id: "pid", value: "PID"},
					{id: "mash_out_hold", value: "Mash Out Hold"},
					{id: "sparge_hold", value: "Sparge Hold"},
				],
				on: {
					onChange: function(newValue, oldValue) {
						$$("config_form").getParentView().$setType(newValue);
					}
				}},
				{
					view: "counter",
					id: "config_counter",
					label: " ",
					labelWidth: 60,
					value: 0,
					min: 0,
					max: 100,
					step: 1,
					on: {
						onChange: function(newValue, oldValue) {
							$$("config_slider").setValue(newValue);
						}
					}
				},
				{ 
					view: "slider",
					id: "config_slider",
					label: " ",
					labelWidth: 60,
					value: 0,
					min: 0,
					max: 100,
					step: 1,
					on: {
						onChange: function(newValue, oldValue) {
							$$("config_counter").setValue(newValue);
						},
						onSliderDrag: function() {
							$$("config_counter").setValue(this.getValue());
						},
					}
				},
				{ margin: 5, cols: [
					{ view: "button", id: "config_submit", value: "Submit", type: "form", on: {
						onItemClick: function() {
							$$("config_form").getParentView().$submit();
						}
					}},
					{ view: "button", id: "config_cancel", value: "Cancel", on: {
						'onItemClick': function() {
							$$("config_form").getParentView().close();
					}}}
				]},
			],
		},
	},

	$init:function(config) {
		if (typeof config.element != "string") {
			throw "Invalid element (should be HLT or BK)";
		}

		config.head = "" + config.element + " Element Config";

		// prime the layout
		webix.delay(function() {
			$$("config_counter").config.width = 400;
			$$("config_counter").resize();
			this.$setType("pid");
		}, this);
	},

	$submit:function() {
		var typeValue = $$("config_type_select").getValue();
		var settingValue = $$("config_counter").getValue();
		console.log("type select: " + typeValue);
		console.log("counter value: " + settingValue);

		var baseURL = "/ab?cmd=configure_"+ this.config.element.toLowerCase();

		if (typeValue == "off") {
			webix.ajax(baseURL + "&enabled=false");
		} else if (typeValue == 1) {
			webix.ajax(baseURL + "&enabled=true&type=pwm&load=" + settingValue);
		} else {
			// convert to celcius
			var celcius = (settingValue - 32) * 5 / 9;
			webix.ajax(baseURL + "&enabled=true&type=pid&setpoint=" + celcius);
		}

		$$("config_form").getParentView().close();
	},

	$setType:function(type) {

		var counter = $$("config_counter");
		var slider = $$("config_slider");

		var currentValue = counter.getValue();

		var min = null;
		var max = null;
		var explicitValue = currentValue;
		var enabled = true;
		var step = 1;

		switch (type) {
		case "pwm":
			min = 0;
			max = 100;
			break;
		case "pid":
			min = 32;
			max = 212;
			step = 0.1;
			break;
		case "mash_out_hold":
			min = 32;
			max = 212;
			explicitValue = 82;
			step = 0.1;
			break;
		case "sparge_hold":
			min = 32;
			max = 212;
			explicitValue = 79.25;
			step = 0.1;
			break;
		case "off":
			enabled = false;
			break;
		}

		if (! enabled) {
			counter.disable();
			slider.disable();
		} else {
			counter.enable();
			slider.enable();
		}

		counter.config.step = step;

		counter.config.min = min;
		counter.config.max = max;
		slider.config.min = min;
		slider.config.max = max;

		// if explicit value given, use that...
		if (explicitValue != currentValue) {
			counter.setValue(explicitValue);
			slider.setValue(explicitValue);
		} else {
			// otherwise ensure that value is within bounds
			if (currentValue > max) {
				counter.setValue(max);
				slider.setValue(max);
			}
			if (currentValue < min) {
				counter.setValue(min);
				slider.setValue(min);
			}
		}

		counter.render();
		slider.render();
	},

}, webix.ui.window);


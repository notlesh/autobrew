webix.protoUI({
	name: "elementPanel",
	defaults: {
		baseOptions: {
			off: { displayValue: "Off", type: "off", },
			pwm: { displayValue: "PWM", type: "pwm", defaultSetting: 0, },
			pid: { displayValue: "PID", type: "pid", defaultSetting: 0, },
		},
		modal: true,
		borderless: true,
		body: {
			view: "form",
			id: "config_form",
			width: 300,
			elements: [
				{ view: "richselect", id: "config_type_select", label: "Type / Preset", labelWidth: 110, value: "off", options: [],
				on: {
					onChange: function(newValue, oldValue) {
						$$("config_form").getParentView().$setType(newValue);
					}
				}},
				{ view: "fieldset", label: "Setting", body: { rows: [
					{
						view: "counter",
						id: "config_counter",
						label: " ",
						labelWidth: 0,
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
						labelWidth: 0,
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
				] }},
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

			// merge presets with base typeOptions
			var typeOptions = webix.copy(this.config.baseOptions);
			webix.extend(typeOptions, this.config.presets);
			this.config.typeOptions = typeOptions;

			// webix expects options as array of objects
			var options = [];
			for (var key in typeOptions) {
				if (typeOptions.hasOwnProperty(key)) {
					var entry = typeOptions[key];
					var newOption = {
						id: key,
						value: entry.displayValue,
					};
					options.push(newOption);
				}
			}

			// build select options from combined set
			$$("config_type_select").getPopup().getList().parse(options);
			$$("config_type_select").refresh();

		}, this);
	},

	$submit:function() {
		var typeValue = $$("config_type_select").getValue();
		var settingValue = $$("config_counter").getValue();

		var typeOption = this.config.typeOptions[typeValue];

		var type = typeOption.type;

		console.log("element: " + this.config.element.toLowerCase());
		console.log("type value: " + typeValue);
		console.log("type: " + type);
		console.log("counter value: " + settingValue);

		var baseURL = "/ab?cmd=configure_"+ this.config.element.toLowerCase();

		if (type == "off") {
			webix.ajax(baseURL + "&enabled=false");
		} else if (type == 1) {
			webix.ajax(baseURL + "&enabled=true&type=pwm&load=" + settingValue);
		} else {
			// convert to celcius
			var celcius = (settingValue - 32) * 5 / 9;
			webix.ajax(baseURL + "&enabled=true&type=pid&setpoint=" + celcius);
		}

		$$("config_form").getParentView().close();
	},

	$setType:function(type) {

		var typeOption = this.config.typeOptions[type];

		var counter = $$("config_counter");
		var slider = $$("config_slider");

		var currentValue = counter.getValue();

		var min = null;
		var max = null;
		var explicitValue = typeOption.defaultSetting;
		var enabled = true;
		var step = 1;

		switch (typeOption.type) {
		case "pwm":
			min = 0;
			max = 100;
			break;
		case "pid":
			min = 32;
			max = 212;
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
		if (explicitValue != null && explicitValue != currentValue) {
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


webix.protoUI({
	name: "elementPanel",
	defaults: {
		top: 100,
		left: 100,
		modal: true,
		borderless: true,
		body: {
			view: "form",
			id: "config_form",
			width: 300,
			elements: [
				{ view: "select", id: "config_type_select", label: "Control Type", value: 0, options: [
					{id: 0, value: "Off"},
					{id: 1, value: "PWM"},
					{id: 2, value: "PID"},
					{id: 3, value: "Mash Out Hold"},
					{id: 4, value: "Sparge Hold"},
				],
				on: {
					onChange: function(newValue, oldValue) {
						$$("config_form").getParentView().$changeType(newValue);
					}
				}},
				{ view: "counter", id: "config_counter", label: "Control Setting", value: 0, min: 0, max: 100, step: 1 },
				{ margin: 5, cols: [
					{ view: "button", id: "config_submit", value: "Submit", type: "form", on: {
						onItemClick: function() {
							$$("config_form").getParentView().$submit();
						}
					}},
					{ view: "button", id: "config_cancel", value: "Cancel", on: {
						'onItemClick': function() {
							$$("config_form").getParentView().hide();
					}}}
				]},
			],
		},
	},

	$submit:function() {
		var typeValue = $$("config_type_select").getValue();
		var settingValue = $$("config_counter").getValue();
		console.log("type select: " + typeValue);
		console.log("counter value: " + settingValue);

		var baseURL = "/ab?cmd=configure_"+ this.config.element.toLowerCase();

		if (typeValue == 0) {
			webix.ajax(baseURL + "&enabled=false");
		} else if (typeValue == 1) {
			webix.ajax(baseURL + "&enabled=true&type=pwm&load=" + settingValue);
		} else {
			// convert to celcius
			var celcius = (settingValue - 32) * 5 / 9;
			webix.ajax(baseURL + "&enabled=true&type=pid&setpoint=" + celcius);
		}

		$$("config_form").getParentView().hide();
	},

	$changeType:function(type) {

		var counter = $$("config_counter");

		if (type == 0) {
			counter.disable();
		} else {
			counter.enable();
		}

		switch (type) {
		case "1":
			console.log("0-100");
			counter.config.min = 0;
			counter.config.max = 100;
			break;
		case "2":
			console.log("32-212");
			counter.config.min = 32;
			counter.config.max = 212;
			break;
		case "3":
			console.log("32-212");
			counter.config.min = 32;
			counter.config.max = 212;
			counter.setValue(82);
			break;
		case "4":
			console.log("32-212");
			counter.config.min = 32;
			counter.config.max = 212;
			counter.setValue(79.25);
			break;
		case "0":
		}
	},

	$init:function(config) {
		if (typeof config.element != "string") {
			throw "Invalid element (should be HLT or BK)";
		}

		config.head = "" + config.element + " Element Config";
	},

}, webix.ui.window);


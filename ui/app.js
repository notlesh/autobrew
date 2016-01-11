function init() {

	g_probeSettings = {
		"28.EE9B8B040000": {
			"color": "#FF0000",
			"name": "BK"
		},
		"28.3AA87D040000": {
			"color": "#4CB6D9",
			"name": "HLT"
		},
		"28.A1F07C040000": {
			"color": "#5E5113",
			"name": "MT"
		},
		"28.42AB7D040000": {
			"color": "#00CC33",
			"name": "Ret"
		},

	};

	// main layout
	webix.ui({
		rows:[
			{ view:"smoothie", id:"main_graph" },
			{
				view:"toolbar", id: "bottomBar", padding: 5, cols:[
					{},
					{

						view: "icon", icon: "bars", click:function(){
							if( $$("menu").config.hidden){
								$$("menu").show();
							}
							else
								$$("menu").hide();
						}
					},
				]

			}
		]
	});

	webix.ui({
		view: "sidemenu",
		id: "menu",
		width: 200,
		position: "bottom",
		css: "bottomBar",
		state: function(state){
			state.bottom = $$("bottomBar").$height + 1;
		},
		body:{
			borderless:true,
			margin: 5,
			cols:[
				{},
				{rows:[
					{view:"label", id:"label_pump_1", label:"Pump 1", align:"center"},
					{view:"segmented", id:"selector_pump_1", align:"right", multiview:true, options: [
						{ id:0, value:"Off"},
						{ id:1, value:"On"}
					]}
				]},
				{},
				{rows:[
					{view:"label", id:"label_pump_2", label:"Pump 2", align:"center"},
					{view:"segmented", id:"selector_pump_2", align:"right", multiview:true, options: [
						{ id:0, value:"Off"},
						{ id:1, value:"On"}
					]}
				]},
				{},
				{rows:[
					{view:"label", id:"label_valve", label:"Valve", align:"center"},
					{view:"segmented", id:"selector_valve", align:"right", multiview:true, options: [
						{ id:0, value:"Off"},
						{ id:1, value:"On"},
						{ id:2, value:"Float"}
					]}
				]},
				{},
				{rows:[
					// {view:"label", id:"label_hlt", template:"HLT Element (set: #setpoint#, load: #load#)", align:"center", set: "-", load: "-"},
					/*
					{ cols: [ 
						{view:"label", id:"label_hlt", label:"HLT Element", align:"center", width: 120},
						{},
						{view:"button", id:"button_hlt", type:"icon", icon:"cog", align:"center"},
					]},
					*/

					{view:"button", id:"button_hlt", label:"HLT Element"},

					// --- bottom row

					/*
					{view:"segmented", id:"selector_hlt", align:"right", multiview:true, options: [
						{ id:0, value:"Off"},
						{ id:1, value:"PWM"},
						{ id:2, value:"PID"}
					]}
					*/

					{ cols: [ 
						{view:"label", label:"PID", align:"center"},
						{view:"label", label:"78.25 F", align:"center"},
						{view:"label", label:"25%", align:"center"},
					]},

				]},
				{},
				{rows:[
					{view:"label", id:"label_bk", label:"BK Element", align:"center"},
					{view:"segmented", id:"selector_bk", align:"right", multiview:true, options: [
						{ id:0, value:"Off"},
						{ id:1, value:"PWM"},
						{ id:2, value:"PID"}
					]}
				]},
				{},
			]
		}
	});

	$$("selector_pump_1").attachEvent("onChange", function(newValue, oldValue) {
		if (newValue == 0) {
			webix.ajax("/ab?cmd=p1_off");
		} else if (newValue == 1) {
			webix.ajax("/ab?cmd=p1_on");
		}
	});

	$$("selector_pump_2").attachEvent("onChange", function(newValue, oldValue) {
		if (newValue == 0) {
			webix.ajax("/ab?cmd=p2_off");
		} else if (newValue == 1) {
			webix.ajax("/ab?cmd=p2_on");
		}
	});

	$$("selector_valve").attachEvent("onChange", function(newValue, oldValue) {
		if (newValue == 0) {
			webix.ajax("/ab?cmd=valve_off");
		} else if (newValue == 1) {
			webix.ajax("/ab?cmd=valve_on");
		} else if (newValue == 2) {
			webix.ajax("/ab?cmd=valve_float");
		}
	});

	$$("button_hlt").attachEvent("onItemClick", function() {
		console.log("button click");
		webix.ui({
			view: "window",
			top: 100,
			left: 100,
			modal: true,
			borderless: true,
			id: "hlt_config_window",
			head: "HLT Config",
			body: {
				view: "form",
				id: "hlt_config_form",
				width: 300,
				elements: [
					{ view: "select", id: "hlt_config_type_select", label: "Control Type", value: 0, options: [
						{id: 0, value: "Off"},
						{id: 1, value: "PWM"},
						{id: 2, value: "PID"},
						{id: 3, value: "Mash Out Hold"},
						{id: 4, value: "Sparge Hold"},
					]},
					{ view: "counter", id: "hlt_config_counter", label: "Control Setting", value: 0, min: 0, max: 100, step: 1 },
					{ margin: 5, cols: [
						{ view: "button", id: "hlt_config_submit", value: "Submit", type: "form" },
						{ view: "button", id: "hlt_config_cancel", value: "Cancel" }
					]},
				],
			},

		});

		$$("hlt_config_cancel").attachEvent("onItemClick", function() {
			$$("hlt_config_window").hide();
		});

		$$("hlt_config_submit").attachEvent("onItemClick", function() {
			var typeValue = $$("hlt_config_type_select").getValue();
			var settingValue = $$("hlt_config_counter").getValue();
			console.log("type select: " + typeValue);
			console.log("counter value: " + settingValue);

			if (typeValue == 0) {
				webix.ajax("/ab?cmd=configure_hlt&enabled=false");
			} else if (typeValue == 1) {
				// webix.ajax("/ab?cmd=configure_hlt&enabled=true&type=pwm&load=" + settingValue);
			} else {
				// convert to celcius
				var celcius = (settingValue - 32) * 5 / 9;
				// webix.ajax("/ab?cmd=configure_hlt&enabled=true&type=pid&setpoint=" + settingValue);
			}

			$$("hlt_config_window").hide();
		});

		$$("hlt_config_type_select").attachEvent("onChange", function(newValue, oldValue) {

			var counter = $$("hlt_config_counter");

			if (newValue == 0) {
				counter.disable();
			} else {
				counter.enable();
			}

			switch (newValue) {
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
		});

		$$("hlt_config_window").show();

	});

	// timer to pull temperature data
	window.setInterval(function() {
		webix.ajax("/temp_data.json", function(text, data, XmlHttpRequest) {
			var tempData = JSON.parse(text);

			var mainGraph = $$("main_graph");

			for (var probeId in tempData) {

				var labelId = "" + probeId + "_temp_label";

				if (! mainGraph.$hasTimeSeries(probeId)) {
					mainGraph.$addTimeSeries(probeId, g_probeSettings[probeId].color);

					var bottomBar = $$("bottomBar");
					bottomBar.addView( {
						view:"label",
						id: labelId,
						temp: tempData[probeId].tempF.toFixed(2),
						name: g_probeSettings[probeId].name,
						template: "<span style='color: "+g_probeSettings[probeId].color+"; font-size: 2em;'>#name#: #temp#</span>",
					}, 0);
				} else {
					// update label
					var label = $$(labelId);
					label.data.temp = tempData[probeId].tempF.toFixed(2);
					label.render();
				}

				var probeData = tempData[probeId];

				mainGraph.$addTimeSeriesData(probeId, probeData.lastSeen, probeData.tempF);
			}

		});
	}, 1000);
}

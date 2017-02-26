function init() {

	g_probeSettings = {
		"28.EE9B8B040000": {
			"color": "#FF3A3C",
			"name": "B"
		},
		"28.3AA87D040000": {
			"color": "#4CB6D9",
			"name": "H"
		},
		"28.A1F07C040000": {
			"color": "#FF9A00",
			"name": "M"
		},
		"28.42AB7D040000": {
			"color": "#00FF40",
			"name": "R"
		},

	};

	g_probes = {};

	// main layout
	webix.ui({
		id:"root",
		width:"100%",
		rows:[
			{ view:"smoothie", id:"probes_data_view" },
			{
				view:"toolbar", id: "bottomBar", padding: 5, cols:[
					{ view: "icon", icon: "gear", click:function() {

							window.graphDisplayed = false;

							redrawTempLayout();
						}
					},
					{ id: "tempList", cols: [] },
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
						{ id:"off", value:"Off"},
						{ id:"on", value:"On"}
					],
					on: {
						onChange: function(newValue, oldValue) {
							if (newValue == "off") {
								webix.ajax("/ab?cmd=p1_off");
							} else if (newValue == "on") {
								webix.ajax("/ab?cmd=p1_on");
							}
						}
					}}
				]},

				{},
				{rows:[
					{view:"label", id:"label_pump_2", label:"Pump 2", align:"center"},
					{view:"segmented", id:"selector_pump_2", align:"right", multiview:true, options: [
						{ id:"off", value:"Off"},
						{ id:"on", value:"On"}
					],
					on: {
						onChange: function(newValue, oldValue) {
							if (newValue == "off") {
								webix.ajax("/ab?cmd=p2_off");
							} else if (newValue == "on") {
								webix.ajax("/ab?cmd=p2_on");
							}
						}
					}}
				]},
				{},
				{rows:[
					{view:"label", id:"label_valve", label:"Valve", align:"center"},
					{view:"segmented", id:"selector_valve", align:"right", multiview:true, options: [
						{ id:"off", value:"Off"},
						{ id:"on", value:"On"},
						{ id:"float", value:"Float"}
					],
					on: {
						onChange: function(newValue, oldValue) {
							if (newValue == "off") {
								webix.ajax("/ab?cmd=valve_off");
							} else if (newValue == "on") {
								webix.ajax("/ab?cmd=valve_on");
							} else if (newValue == "float") {
								webix.ajax("/ab?cmd=valve_float");
							}
						}
					}}
				]},
				{},
				{rows:[
					{view:"button", id:"button_hlt", label:"HLT Element", on: {
						onItemClick: function() {
							var pos = webix.html.offset($$("button_hlt").getNode());
							webix.ui({
								view: "elementPanel",
								id: "hlt_config_window",
								element: "HLT",
								top: pos.y - 250,
								left: pos.x - 150,
								presets: {
									mash_out_hold: { displayValue: "Mash Out Hold", type: "pid", defaultSetting: 179.6},
									sparge_hold: { displayValue: "Sparge Hold", type: "pid", defaultSetting: 174.65},
								},
							}).show();
						}
					}},
					{ cols: [ 
						{view:"label", label:"PWM", align:"center"},
						{view:"label", label:"60%", align:"center"},
					]},

				]},
				{},
				{rows:[
					{view:"button", id:"button_bk", label:"BK Element", on: {
						onItemClick: function() {
							var pos = webix.html.offset($$("button_bk").getNode());
							webix.ui({
								view: "elementPanel",
								id: "bk_config_window",
								element: "BK",
								top: pos.y - 250,
								left: pos.x - 150,
								presets: {
									pre_boil: { displayValue: "Pre-Boil", type: "pid", defaultSetting: 196},
									boil: { displayValue: "Boil", type: "pwm", defaultSetting: 60},
									berliner_weisse: { displayValue: "Berliner Weisse", type: "pid",
									defaultSetting: 118}, pbw: { displayValue: "PBW", type: "pid", defaultSetting: 135},
									preheat_mash: { displayValue: "Preheat for Mash", type: "pid", defaultSetting: 170},
								},
							}).show();
						}
					}},
					{ cols: [ 
						{view:"label", label:"PWM", align:"center"},
						{view:"label", label:"60%", align:"center"},
					]},

				]},
				{},
			]
		}
	});

	// timer to pull temperature data
	window.setInterval(function() {
		webix.ajax("/temp_data.json", function(text, data, XmlHttpRequest) {
			var tempData = JSON.parse(text);

			var mainGraph = $$("probes_data_view");

			for (var probeId in tempData) {

				var probeData = tempData[probeId];
				var isNewProbe = ! g_probes.hasOwnProperty(probeId);

				if (isNewProbe) {
					console.log("New probe: "+ probeId);
					// first time seeing this probe
					g_probes[probeId] = JSON.parse(JSON.stringify(probeData)); // TODO: better way to clone?
				}

				if (window.graphDisplayed) {
					if (! mainGraph.$hasTimeSeries(probeId)) {
						mainGraph.$addTimeSeries(probeId, g_probeSettings[probeId].color);
					}

					mainGraph.$addTimeSeriesData(probeId, probeData.lastSeen, probeData.tempF);
				} else {
					if (isNewProbe) {
						redrawTempLayout();
					}
				}


				var labelId = "" + probeId + "_temp_label";

				if ($$(labelId) == null) {

					var tempList = $$("tempList");
					tempList.addView( {
						view:"label",
						id: labelId,
						temp: tempData[probeId].tempF.toFixed(2),
						name: g_probeSettings[probeId].name,
						template: "<span style='color: "+g_probeSettings[probeId].color+"; font-size: 2em; white-space: nowrap;' >#name#: #temp#</span>",
					}, 0);
				} else {
					// update label
					var label = $$(labelId);
					label.data.temp = tempData[probeId].tempF.toFixed(2);
					label.render();
				}
			}

		});

		webix.ajax("/ab?cmd=status", function(text, data, XmlHttpRequest) {
			var statusData = JSON.parse(text);
			// console.log("valve status: "+ statusData.pins.valve1.state.enabled);

			// update the ui widgets. we temporarily block events so that
			// the new value will not trigger an API call back to the server

			// TODO: clean this up. a refactor with the webix jet stuff would be nice...
			var selectorUpdate = function(selector, value) {
				if (selector.getValue() != value) {
					selector.blockEvent();
					selector.setValue(value);
					selector.unblockEvent();
				}
			}

			selectorUpdate( $$("selector_valve"), statusData.controls.valve );
			selectorUpdate( $$("selector_pump_1"), statusData.controls.pump1 );
			selectorUpdate( $$("selector_pump_2"), statusData.controls.pump2 );

		});
	}, 1000);
	window.graphDisplayed = true;

	redrawTempLayout = function() {
		console.log("redrawTempLayout()");

		if (! window.graphDisplayed) {

			// overwrite temp probe area with new one
			webix.ui(
				{ 
					view: "dataview",
					id: "probes_data_view",
					xCount: 2,
					template: "<div style='position: relative; width: 240px; height: 120px;'><span style='color: #color#' class='probeDisplayTitle'>#name#</span><br/><span style='color: #color#' class='probeDisplayTemp'>#temp#</span></div>",
					data: [],
					sizeToContent: true

					
				},
				$$("root"),
				$$("probes_data_view")
			);

			for (var probeId in g_probes) {
				console.log("probe: " + probeId +": " + g_probes[probeId]);

				$$("probes_data_view").add( {
					name: g_probeSettings[probeId].name,
					color: g_probeSettings[probeId].color,
					temp: g_probes[probeId].tempF.toFixed(2),
				}, 0);
			}
		}
	};
}

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
	g_stateCounter = 0;
	g_lastDetailedStateUpdate = Date.now() - 5000;

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

							// window.graphDisplayed = false;
							// redrawTempLayout();

							// graph config window

							var configWindow = $$("graph_config_window");
							if (typeof configWindow === "undefined") {
								configWindow = webix.ui({
									view: "window",
									id: "graph_config_window",
									// head: "Graph Configuration",
									css: "graphConfig",
									width: 800,
									head: {
										cols: [
											{view: "label", label: "Graph Configuration"},
											{view:"button", label:"X", width:30, click:("$$('graph_config_window').hide();")},
										]
									},
									body: {
										view: "form",
										modal: "true",
										elements: [
											{label: "MPP", view:"segmented", id:"selector_graph_mpp", value: 1000, options: [
													{ id:100, value:"100"},
													{ id:500, value:"500"},
													{ id:1000, value:"1000"},
													{ id:5000, value:"5000"},
													{ id:10000, value:"10000"},
													{ id:50000, value:"50000"}
												],
												on: {
													onChange: function(newValue, oldValue) {
														$$("probes_data_view").smoothie.options.millisPerPixel = newValue;
													}
												}
											},
											{label: "Vert Lines", view:"segmented", id:"selector_graph_vert", value: 60000, options: [
													{ id:1000, value:"1s"},
													{ id:5000, value:"5s"},
													{ id:10000, value:"10s"},
													{ id:60000, value:"1m"},
													{ id:300000, value:"5m"},
													{ id:600000, value:"10m"},
													{ id:3000000, value:"30m"},
													{ id:6000000, value:"1h"},
												],
												on: {
													onChange: function(newValue, oldValue) {
														$$("probes_data_view").smoothie.options.grid.millisPerLine = newValue;
													}
												}
											},
											{label: "Min", view:"segmented", id:"selector_graph_min", value: 40, options: [
													{ id:40, value:"40"},
													{ id:60, value:"60"},
													{ id:80, value:"80"},
													{ id:100, value:"100"},
													{ id:120, value:"120"}
												],
												on: {
													onChange: function(newValue, oldValue) {
														$$("probes_data_view").smoothie.options.minValue = newValue;
													}
												}
											},
											{label: "Interpolation", view:"segmented", id:"selector_graph_interp", value: "linear", options: [
													{ id:"bezier", value:"Bezier"},
													{ id:"linear", value:"Linear"},
													{ id:"step", value:"Step"},
												],
												on: {
													onChange: function(newValue, oldValue) {
														$$("probes_data_view").smoothie.options.interpolation = newValue;
													}
												}
											},
										]
									}
								});
								configWindow.show();
								configWindow.setPosition(30, 30);
							} else {
								if (configWindow.isVisible()) {
									configWindow.hide();
								} else {
									configWindow.show();
								}
							}
						}
					},
					{ id: "tempList", cols: [] },
					{

						view: "icon", icon: "bars", id: "menuToggleButton", click:function(){
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
			paddingX: 10,
			cols:[
				{gravity: 2,
				rows:[
					{view:"label", id:"label_pump_1", label:"Pump 1", align:"center"},
					{view:"segmented", id:"selector_pump_1", align:"right", options: [
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

				{gravity: 2,
				rows:[
					{view:"label", id:"label_pump_2", label:"Pump 2", align:"center"},
					{view:"segmented", id:"selector_pump_2", align:"right", options: [
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
				{gravity: 2,
				rows:[
					{view:"label", id:"label_valve", label:"Valve", align:"center"},
					{view:"segmented", id:"selector_valve", align:"right", options: [
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
				{gravity:3,
				rows:[
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
					{view:"label", id:"hlt_status", label:"", align:"center", css:"elementStatusLabel"},

				]},
				{gravity: 3,
				rows:[
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
					{view:"label", id:"bk_status", label:"", align:"center", css:"elementStatusLabel"},

				]},
			]
		}
	});

	// prevent menu from automatically being dismissed
	webix.event( $$("root").$view, "click", function(e){
		var trg = e.target || e.srcElement;
		if ( ! $$("menuToggleButton").$view.contains(trg)) {
			e.showpopup = true;
		}
	});

	// show menu on initial load
	setTimeout( function() {
		$$("menu").show();
	}, 50);

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

			var now = Date.now();
			var forceUpdate = false;
			if (now - g_lastDetailedStateUpdate > 5000) {
				console.log("Forcing detailed update...");
				forceUpdate = true;
			}

			// if stateCounter is not up to date, request new state
			if (forceUpdate || statusData.stateCounter != g_stateCounter) {
				console.log("our state counter ("+g_stateCounter+") differs from server ("+
						statusData.stateCounter+") (or we are forcing update)");

				webix.ajax("/ab?cmd=getState", function(text, data, XmlHttpRequest) {
					var stateData = JSON.parse(text);

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

					selectorUpdate( $$("selector_valve"), stateData.controls.valve );
					selectorUpdate( $$("selector_pump_1"), (stateData.controls.pump1 ? "on":"off") );
					selectorUpdate( $$("selector_pump_2"), (stateData.controls.pump2 ? "on":"off") );

					console.log("updating our state counter from "+ g_stateCounter
							+ " to " + statusData.stateCounter);
					g_stateCounter = statusData.stateCounter;

					// TODO: as above -- clean this up
					var elementStatusLabelUpdate = function(label, type, elementJsonObj, pidJsonObj) {

						if (type == "off") {
							// label is: "Off"
							label.setValue("Off");
							console.log("Updating label to Off");

						} else {
							// we will show PWM or PID with PWM -- so build up PWM
							var pwmValueStr = (elementJsonObj.config.pwmLoad * 100).toFixed(1) + "%";
							var pwmActualStr = (elementJsonObj.state.pwmLoad * 100).toFixed(1) + "%";
							var pwmStr = "PWM: "+ pwmValueStr + "(" + pwmActualStr + ")";

							if (type == "pid") {
								// PID -- most complicated
								// Label is: PID 200F (PWM: 60% (50%))
								var tempF = pidJsonObj.setpoint * 1.8 + 32;
								var pidValueStr = tempF.toFixed(1) + "\xB0F";
								var pidStr = "PID: "+ pidValueStr +"("+ pwmStr +")";
								console.log("Updating label to "+ pidStr);
								label.setValue(pidStr);
							} else if (type == "pwm") {
								// PWM -- show set and actual
								// Label is: PWM: 60% (50%)
								label.setValue(pwmStr);
								console.log("Updating label to "+ pwmStr);
							} else {
								console.log("Unrecognized element type: "+ type);
								label.setValue("ERROR");
							}
						}
					}

					elementStatusLabelUpdate( $$("hlt_status"), stateData.controls.hlt, stateData.pins.hlt, stateData.pid.hlt );
					elementStatusLabelUpdate( $$("bk_status"), stateData.controls.bk, stateData.pins.bk, stateData.pid.bk );
				});

				g_lastDetailedStateUpdate = Date.now();
			}

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

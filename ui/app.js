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
			"name": "Wort Return"
		},

	};

	// main layout
	webix.ui({
		rows:[
			// { template: "Click button to show menu"},
			// { view:"canvas", id:"chart_canvas"},
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
					{view:"label", id:"label_valve", label:"valve", align:"center"},
					{view:"segmented", id:"selector_valve", align:"right", multiview:true, options: [
						{ id:0, value:"Off"},
						{ id:1, value:"On"},
						{ id:2, value:"Float"}
					]}
				]},
				{}
			]
		}
	});

	$$("selector_pump_1").attachEvent("onChange", function(newValue, oldValue) {
		webix.message("Pump 1 value changed from: "+oldValue+" to: "+newValue);
		if (newValue == 0) {
			webix.ajax("/ab?cmd=p1_off");
		} else if (newValue == 1) {
			webix.ajax("/ab?cmd=p1_on");
		}
	});

	$$("selector_pump_2").attachEvent("onChange", function(newValue, oldValue) {
		webix.message("Pump 2 value changed from: "+oldValue+" to: "+newValue);
		if (newValue == 0) {
			webix.ajax("/ab?cmd=p2_off");
		} else if (newValue == 1) {
			webix.ajax("/ab?cmd=p2_on");
		}
	});

	$$("selector_valve").attachEvent("onChange", function(newValue, oldValue) {
		webix.message("Valve changed from: "+oldValue+" to: "+newValue);
		if (newValue == 0) {
			webix.ajax("/ab?cmd=valve_off");
		} else if (newValue == 1) {
			webix.ajax("/ab?cmd=valve_on");
		} else if (newValue == 2) {
			webix.ajax("/ab?cmd=valve_float");
		}
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
						template: "<span style='color: red; font-size: 2em;'>#name#: #temp#</span>",
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

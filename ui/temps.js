var g_lastTempData = {};
var g_canvases = {};
var g_smoothieCharts = {};
var g_tempSeries = {};

var g_settings = {
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


/**
	* Retrieve the latest temperature data JSON
	*/
function getLatestTempData() {
	
	var request = new XMLHttpRequest;
	request.overrideMimeType("application/json");
	request.open("GET", window.location.origin + "/temp_data.json", true);
	request.onload = function() {
		window.g_lastTempData = JSON.parse(request.responseText);
		// console.log("new temps: " + JSON.stringify(window.g_lastTempData));
	};
	request.send(null);
}

/**
 * Add a probe
	*/
function addProbe(probeId) {

	console.log("adding probe " + probeId);

	var canvas = document.createElement("canvas");
	canvas.id = "canvas-" + probeId;
	canvas.width = window.innerWidth - 24;
	canvas.style = "margin: 8px";
	document.body.appendChild(canvas);
	window.g_canvases[probeId] = canvas;

	// millis per pixel will be (1000 * 60 * 10) / width -- entire width will give us 10 minutes
	var mpp = 100;
	if (canvas.width > 0) {
		mpp = (1000 * 60 * 30) / canvas.width;
	}

	var smoothie = new SmoothieChart(
		{
			millisPerPixel: mpp, 
			interpolation:"linear", 
			maxValue:220, 
			minValue:40,
			grid: {
				millisPerLine: 60000,
				verticalSections: 9,
				strokeStyle: 'rgba(119,119,119,0.5)'
			}
		});
	smoothie.streamTo(canvas, 1000);

	var color = "#00FF00";
	try {
		color = window.g_settings[probeId].color;
	} catch(e) {
		console.log("no color for " + probeId + " (exception: " + e + ")");
	}

	var tempSeries = new TimeSeries();
	smoothie.addTimeSeries(tempSeries, {lineWidth:2,strokeStyle:color});
	window.g_tempSeries[probeId] = tempSeries;

	window.g_smoothieCharts[probeId] = smoothie;
}

/**
 * Resize canvases so they equally fill the window
	*/
function resizeCanvases() {
	var newHeight;
	var numProbes = Object.keys(window.g_canvases).length;
	if (numProbes > 0) {
		newHeight = (window.innerHeight - (numProbes * 8 * 2)) / numProbes;
	}

	/*
	console.log("resizing canvases. window height: " + window.innerHeight);
	console.log("    num probes: " + numProbes);
	console.log("    height:     " + newHeight);
	*/

	for (var probeId in window.g_canvases) {
		var canvas = window.g_canvases[probeId];
		canvas.height = "" + newHeight;
	}
}

/**
	* Main entry point
	*/
function tempsStart() {

	// add canvas objects for well known probes // TODO: do this on the fly
	getLatestTempData();

	setInterval(function() {

		for (var probeId in window.g_lastTempData) {
			// console.log("probe id: " + probeId);
			var probeInfo = window.g_lastTempData[probeId];

			// add probe if it doesn't exist...
			if (window.g_canvases[probeId]) {
			} else {
				addProbe(probeId);
				resizeCanvases();
			}

			var tempData = window.g_lastTempData[probeId];
			var series = window.g_tempSeries[probeId];
			series.append(tempData.lastSeen, tempData.tempF);

		}

		getLatestTempData();
	}, 1000);

}


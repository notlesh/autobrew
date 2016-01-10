var g_lastTempData = {};
var g_smoothie = {};
var g_tempSeries = {};
var g_numericText = {};
var g_numericDisplays = {};


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

	var color = "#00FF00";
	try {
		color = window.g_settings[probeId].color;
	} catch(e) {
		console.log("no color for " + probeId + " (exception: " + e + ")");
	}

	var tempSeries = new TimeSeries();
	window.g_smoothie.addTimeSeries(tempSeries, {lineWidth:2,strokeStyle:color});
	window.g_tempSeries[probeId] = tempSeries;

	// add numeric display on right side
	var numericDisplay = document.createElement("p");
	numericDisplay.style.color = color;
	numericDisplay.className = "probeNumericDisplay";
	var numericText = document.createTextNode("");
	var rightColumn = document.getElementById("right_column");

	numericDisplay.appendChild(numericText);
	rightColumn.appendChild(numericDisplay);

	window.g_numericDisplays[probeId] = numericDisplay;
	window.g_numericText[probeId] = numericText;
}

/**
 * Resize chart
 */
function resizeChart(event) {

	// update canvas width and height relative to parent
	var div = document.getElementById("chart_main");
	var canvas = document.getElementById("chart_canvas");
	var parentStyle = window.getComputedStyle(div, null);
	canvas.width = parseFloat(parentStyle.width);
	canvas.height = parseFloat(parentStyle.height);
}

/**
 * Handle scale change
 */
function handleScaleChange(value) {
	console.log("handleScaleChange("+ value +")");
	if (value == "1m") {
		changeScale(60, 5);
	} else if (value == "5m") {
		changeScale(60 * 5, 15);
	} else if (value == "10m") {
		changeScale(60 * 10, 30);
	} else if (value == "30m") {
		changeScale(60 * 30, 60);
	} else if (value == "1h") {
		changeScale(3600, 60 * 5);
	} else if (value == "2h") {
		changeScale(3600 * 2, 60 * 10);
	} else if (value == "4h") {
		changeScale(3600 * 4, 60 * 15);
	} else if (value == "8h") {
		changeScale(3600 * 8, 60 * 30);
	} else if (value == "1d") {
		changeScale(3600 * 24, 60 * 60);
	} else if (value == "1w") {
		changeScale(3600 * 24 * 7, 60 * 60 * 4);
	}
}

/**
 * Change scale
 *
 * @secondsTotal is the number of seconds to graph
 * @secondsGridLines is the number of seconds between grid lines
 */
function changeScale(secondsTotal, secondsGridLines)
{

	var canvas = document.getElementById("chart_canvas");
	mpp = (1000 * secondsTotal) / canvas.width;

	window.g_smoothie.options.millisPerPixel = mpp;
	window.g_smoothie.options.grid.millisPerLine = 1000 * secondsGridLines;
}

/**
 * Main entry point
 */
function tempsStart() {

	// add canvas objects for well known probes // TODO: do this on the fly
	getLatestTempData();

	// create graph canvas and smoothie charts
	//

	resizeChart();

	// millis per pixel will be (1000 * 60 * 10) / width -- entire width will give us 10 minutes
	var canvas = document.getElementById("chart_canvas");
	var mpp = 100;
	if (canvas.width > 0) {
		mpp = (1000 * 60 * 30) / canvas.width;
	}

	window.g_smoothie = new SmoothieChart(
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
	window.g_smoothie.streamTo(canvas, 1000);

	// set up recurring function to update charts
	setInterval(function() {

		for (var probeId in window.g_lastTempData) {
			// console.log("probe id: " + probeId);
			var probeInfo = window.g_lastTempData[probeId];

			// add probe if it doesn't exist...
			if (window.g_tempSeries[probeId]) {
			} else {
				addProbe(probeId);
			}

			var tempData = window.g_lastTempData[probeId];
			var series = window.g_tempSeries[probeId];
			series.append(tempData.lastSeen, tempData.tempF);

			var textNode = window.g_numericText[probeId];
			textNode.nodeValue = "" + tempData.tempF.toFixed(2);

		}

		getLatestTempData();
	}, 1000);

	// add resize listener
	window.onresize = function(event) {
		resizeChart();
	};

}


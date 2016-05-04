console.log("adding smoothie def");
webix.protoUI({
	name:"smoothie",
	$init:function(config) {
		// not working at all:
		// webix.require("new/smoothie.js"); // TODO: why do we need new/ ?

		console.log("smoothie ctor");
		var elm = document.createElement("canvas");
		this._canvas = this.$view.appendChild(elm);

		this.timeSeries = {};

		this.smoothie = new SmoothieChart({
				millisPerPixel: 1800,
				// millisPerPixel: mpp, 
				interpolation:"linear", 
				maxValue:220, 
				minValue:40,
				grid: {
					millisPerLine: 60000,
					verticalSections: 9,
					strokeStyle: 'rgba(119,119,119,0.5)'
				}
		});

		this.smoothie.streamTo(elm);

	},
	$setSize:function(x,y) {
		if (webix.ui.view.prototype.$setSize.call(this,x,y)) {
			this._canvas.width = x;
			this._canvas.height = y;
		}
	},
	$addTimeSeries:function(name, color) {
		console.log("addTimeSeries()");
		if (this.timeSeries.hasOwnProperty(name)) {
			throw "time series " + name + " already exists";
		}
		var timeSeries = new TimeSeries();
		this.smoothie.addTimeSeries(timeSeries, {lineWidth:2, strokeStyle:color});
		this.timeSeries[name] = timeSeries;
	},
	$hasTimeSeries:function(name) {
		return this.timeSeries.hasOwnProperty(name);
	},
	$addTimeSeriesData:function(seriesName, xValue, yValue) {
		var series = this.timeSeries[seriesName];
		series.append(xValue, yValue);
	}
}, webix.ui.view);


/**
 * Make an RPC call.
 */
function rpcRequest(cmd) {
	
	var request = new XMLHttpRequest;
	request.overrideMimeType("application/json");
	request.open("GET", window.location.origin + "/ab?cmd=" + cmd);
	request.onload = function() {
		window.g_lastTempData = JSON.parse(request.responseText);
		console.log("rpc response for " + cmd + ": " + JSON.stringify(window.g_lastTempData));
	};
	request.send(null);
}


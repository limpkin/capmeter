if (typeof capmeter == 'undefined') capmeter = {};
capmeter.calibration = capmeter.calibration || {};

capmeter.calibration.calibrateCurrentX = function($button) {
  console.log("capmeter.calibration.calibrateCurrentX");
  $button.css("background", "orange");
}

capmeter.calibration.calibrateCurrentY = function($button) {
  console.log("capmeter.calibration.calibrateCurrentY");
  $button.css("background", "orange");
}

capmeter.calibration.calibrateCapacitance = function($button) {
  console.log("capmeter.calibration.calibrateCapacitance");
  $button.css("background", "orange");
}

capmeter.calibration.calibratePlatform = function($button) 
{
	console.log("capmeter.calibration.calibratePlatform");
	$button.css("background", "orange");  
	msg = new ArrayBuffer(packetSize);
	data = new Uint8Array(msg);
	data.set([0, CMD_OE_CALIB_START], 0);
	sendMsg(msg);
}
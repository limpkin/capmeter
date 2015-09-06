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
	console.log("Starting Open Ended Calibration...");
	//$button.css("background", "orange");  
	var d = new Date();
	sendRequest(CMD_OE_CALIB_START, [d.getDate(), d.getMonth() + 1, d.getFullYear()%100]);
}
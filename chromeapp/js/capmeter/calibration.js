if (typeof capmeter == 'undefined') capmeter = {};
capmeter.calibration = capmeter.calibration || {};

capmeter.calibration.calibrateCurrentX = function($button) {
  //console.log("capmeter.calibration.calibrateCurrentX");
  $button.css("background", "orange");

  $("#calibrateCurrentX").attr("disabled", 1);
  // $("#calibrateCurrentX").removeAttr("disabled");

  
  sendRequest(0xFF, null);
}

capmeter.calibration.calibrateCurrentY = function($button) {
  //console.log("capmeter.calibration.calibrateCurrentY");
  $button.css("background", "orange");
}

capmeter.calibration.calibrateCapacitance = function($button) {
  //console.log("capmeter.calibration.calibrateCapacitance");
  start_capacitance_calibration();
}

capmeter.calibration.calibratePlatform = function($button) 
{
	$button.css("background", "orange");
	start_open_ended_calibration();
}
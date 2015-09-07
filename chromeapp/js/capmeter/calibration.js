if (typeof capmeter == 'undefined') capmeter = {};
capmeter.calibration = capmeter.calibration || {};

capmeter.calibration.calibrateCurrentX = function($button) {
  console.log("capmeter.calibration.calibrateCurrentX");
  $button.css("background", "orange");

  $("#calibrateCurrentX").attr("disabled", 1);
  // $("#calibrateCurrentX").removeAttr("disabled");
}

capmeter.calibration.calibrateCurrentY = function($button) {
  console.log("capmeter.calibration.calibrateCurrentY");
  $button.css("background", "orange");
}

capmeter.calibration.calibrateCapacitance = function($button) {
  console.log("capmeter.calibration.calibrateCapacitance");
  $button.css("background", "orange");
}

capmeter.calibration.calibratePlatform = function($button) {
  console.log("capmeter.calibration.calibratePlatform");
  $button.css("background", "orange");
}
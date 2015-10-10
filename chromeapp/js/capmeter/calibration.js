if (typeof capmeter == 'undefined') capmeter = {};
capmeter.calibration = capmeter.calibration || {};

capmeter.calibration.calibrateCurrentX = function($button) {
  disable_gui_buttons();  
  sendRequest(CMD_BOOTLOADER_JUMP, null);
}

capmeter.calibration.calibrateCurrentY = function($button) {
  $button.css("background", "green");
  start_current_calibration();
}

capmeter.calibration.calibrateCapacitance = function($button) {
  $button.css("background", "green");
  start_capacitance_calibration();
}

capmeter.calibration.calibratePlatform = function($button) 
{
	$button.css("background", "green");
	start_open_ended_calibration();
}
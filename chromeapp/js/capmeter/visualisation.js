if (typeof capmeter == 'undefined') capmeter = {};
capmeter.visualisation = capmeter.visualisation || {};

capmeter.visualisation.current = function($button) {
  log("capmeter.visualisation.current");
  $button.css("background", "orange");
}

capmeter.visualisation.capacitance = function($button) {
  log("capmeter.visualisation.capacitance");
  $button.css("background", "orange");
}

capmeter.visualisation.maxVoltage = function($input) {  
  log("capmeter.visualisation.maxVoltage - " + $input.val());
  $input.css("background", "orange");
}
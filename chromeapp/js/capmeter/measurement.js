if (typeof capmeter == 'undefined') capmeter = {};
capmeter.measurement = capmeter.measurement || {};

capmeter.measurement._current = 500;
capmeter.measurement._capacitance = 900;

capmeter.measurement.measureCurrent = function($button) {
  log("capmeter.measurement.measureCurrent");
  $button.css("background", "orange");
}

capmeter.measurement.measureCapacitance = function($button) {
  log("capmeter.measurement.voltageCurrent");
  $button.css("background", "orange");
}

capmeter.measurement.voltageCurrent = function($input) {  
  log("capmeter.measurement.voltageCurrent - " + $input.val());
  $input.css("background", "orange");
}

capmeter.measurement.voltageCapacitance = function($input) {  
  log("capmeter.measurement.voltageCapacitance - " + $input.val());
  $input.css("background", "orange");
}

capmeter.measurement.get_current = function($display) {
  $display.html(capmeter.measurement._current);
}

capmeter.measurement.get_capacitance = function($display) {
  $display.html(capmeter.measurement._capacitance);
}
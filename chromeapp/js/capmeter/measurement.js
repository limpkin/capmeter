if (typeof capmeter == 'undefined') capmeter = {};
capmeter.measurement = capmeter.measurement || {};

capmeter.measurement._current = "nA";
capmeter.measurement._capacitance = "fF";
capmeter.measurement._capmes_setV = 0;
capmeter.measurement._currentmes_setV = 0;

capmeter.measurement.measureCurrent = function($button) 
{
	static_current_measurement_start(capmeter.measurement._currentmes_setV);
	//console.log("capmeter.measurement.measureCurrent");
	//$button.css("background", "orange");
}

capmeter.measurement.measureCapacitance = function($button) 
{
	static_capacitance_measurement_start(capmeter.measurement._capmes_setV);
	//console.log("capmeter.measurement.voltageCurrent");
  //$button.css("background", "orange");
}

capmeter.measurement.voltageCurrent = function($input) 
{  
	capmeter.measurement._currentmes_setV = $input.val();
	notify_vcurrent_change($input.val());
  //console.log("capmeter.measurement.voltageCurrent - " + $input.val());
  //$input.css("background", "orange");
}

capmeter.measurement.voltageCapacitance = function($input) 
{  
	capmeter.measurement._capmes_setV = $input.val();
	notify_vcap_change($input.val());
  //console.log("capmeter.measurement.voltageCapacitance - " + $input.val());
  //$input.css("background", "orange");
}

capmeter.measurement.get_current = function($display) {
  $display.html(capmeter.measurement._current);
}

capmeter.measurement.get_capacitance = function($display) {
  $display.html(capmeter.measurement._capacitance);
}
if (typeof capmeter == 'undefined') capmeter = {};
capmeter.visualisation = capmeter.visualisation || {};

capmeter.visualisation._maxVoltage = 0;
capmeter.visualisation._datapoints = 20;

capmeter.visualisation.current = function($button) {
	current_caracterization_start(capmeter.visualisation._maxVoltage);
}

capmeter.visualisation.capacitance = function($button) {
  capacitance_caracterization_start(capmeter.visualisation._maxVoltage);
}

capmeter.visualisation.maxVoltage = function($input) {  
  capmeter.visualisation._maxVoltage = $input.val();
}

capmeter.visualisation.dataPoints = function($input) {
	capmeter.visualisation._datapoints = $input.val();
}
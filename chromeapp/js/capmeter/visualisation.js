if (typeof capmeter == 'undefined') capmeter = {};
capmeter.visualisation = capmeter.visualisation || {};

capmeter.visualisation.current = function($button) {
  console.log("capmeter.visualisation.current");
  $button.css("background", "orange");
}

capmeter.visualisation.capacitance = function($button) {
  console.log("capmeter.visualisation.capacitance");
  $button.css("background", "orange");
}

capmeter.visualisation.maxVoltage = function($input) {  
  console.log("capmeter.visualisation.maxVoltage - " + $input.val());
  $input.css("background", "orange");

  // Change graph value
  capmeter.graph.value = parseFloat($input.val());
}
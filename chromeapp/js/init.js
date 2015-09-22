var _console_log = console.log;

console.log = function() {
  text = arguments[0];
  $("#log").val($("#log").val() + text + "\n");
  $("#log").animate({
        scrollTop:$("#log")[0].scrollHeight - $("#log").height()
  }, 100);
  return _console_log.apply(console, arguments);
}

$(function(){
  // Init buttons
  $("*[data-onclick]").on("click", function() {
    function_name = $(this).attr("data-onclick");

    if ($(this).attr('disabled')) return;
    capmeter.helper.executeFunctionByName(function_name, $(this));
  });

  // Init inputs
  $("*[data-input]").on("keyup change", function() {
    function_name = $(this).attr("data-input");

    if ($(this).attr('disabled')) return;
    capmeter.helper.executeFunctionByName(function_name, $(this));
  });

  // Init displays
  setInterval(function() {
    $("*[data-display]").each(function(){
      function_name = $(this).attr("data-display");
      capmeter.helper.executeFunctionByName(function_name, $(this));
    });
  }, 500);

  // Init graph
  capmeter.graph.init();

  // Some random chart data
  // setInterval(function(){
  //   random_data = [];
  //   for (i in capmeter.graph.yValues) {
  //     random_data.push(Math.random()*100);
  //   }
  //   capmeter.graph.changeYValues(random_data);    
  // }, 2000);  
});
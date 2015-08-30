// Frontend Javascript
// This file is loaded in all frontend files of this app.
var log = function(text) {
  $("#log").val($("#log").val() + text + "\n");
  $("#log").animate({
        scrollTop:$("#log")[0].scrollHeight - $("#log").height()
  }, 200);
}

$(function(){
  // Init buttons
  $("*[data-onclick]").on("click", function() {
    function_name = $(this).attr("data-onclick");
    capmeter.helper.executeFunctionByName(function_name, $(this));
  });

  // Init inputs
  $("*[data-input]").on("keyup change", function() {
    function_name = $(this).attr("data-input");
    capmeter.helper.executeFunctionByName(function_name, $(this));
  });

  // Init displays
  setInterval(function() {
    $("*[data-display]").each(function(){
      function_name = $(this).attr("data-display");
      capmeter.helper.executeFunctionByName(function_name, $(this));
    });
  }, 500);
});
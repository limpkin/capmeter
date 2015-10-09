if (typeof capmeter == 'undefined') capmeter = {};
capmeter.interface = capmeter.interface || {};

capmeter.interface.heightEqualizer = function() {
  $("*[data-height-equalizer]").each(function(){
    target = $(this).attr("data-height-equalizer");
    $target = $(target);
    $(this).height($target.height());
  });
}

capmeter.interface.resize = function(e) {
  capmeter.interface.heightEqualizer();
  var window_height = $(window).height();
  var content_height = $("#wrapper").height();
  var graph_height = $(".graph-container").height();
  var new_graph_height = graph_height + window_height - content_height;
  $(".graph-container").height(new_graph_height);
}

capmeter.interface.init = function() {
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

  // Resize window
  $(window).resize(capmeter.interface.resize);
  capmeter.interface.resize();
}
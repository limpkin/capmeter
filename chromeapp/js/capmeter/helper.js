if (typeof capmeter == 'undefined') capmeter = {};
capmeter.helper = capmeter.helper || {};

capmeter.helper.executeFunctionByName = function(functionName) {
  var context = window;
  var args = [].slice.call(arguments).splice(1);
  var namespaces = functionName.split(".");
  var func = namespaces.pop();
  for(var i = 0; i < namespaces.length; i++) {
    context = context[namespaces[i]];
  }
  return context[func].apply(this, args);
}
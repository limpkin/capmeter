chrome.app.runtime.onLaunched.addListener(function() {
  chrome.app.window.create('html/index.html', {
    'outerBounds': {
      'width': 800,
      'height': 500
    }
  });
});
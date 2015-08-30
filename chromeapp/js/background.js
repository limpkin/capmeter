chrome.app.runtime.onLaunched.addListener(function() {
  chrome.app.window.create('html/index.html', {
    'innerBounds': {
      'width': 800,
      'height': 623
    }
  });
});
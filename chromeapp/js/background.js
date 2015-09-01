function launch() 
{
  chrome.app.window.create('html/index.html', {'innerBounds': {'width': 800,'height': 623}});
}

chrome.runtime.onInstalled.addListener(launch);
chrome.runtime.onStartup.addListener(launch);
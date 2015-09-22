var app_launched = false;

function launch() 
{
	if(!app_launched)
	{
		chrome.app.window.create('html/index.html', {'innerBounds': {'width': 800,'height': 623}});
		app_launched = true;
	}  
}

chrome.runtime.onInstalled.addListener(launch);
chrome.runtime.onStartup.addListener(launch);
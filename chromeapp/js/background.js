var app_launched = false;

function launch() 
{
	if(!app_launched)
	{
		chrome.app.window.create('html/index.html', {'innerBounds': {'width': 860,'height': 620, "minWidth": 860, "minHeight": 620}});
		app_launched = true;
	}  
}

chrome.runtime.onInstalled.addListener(launch);
chrome.runtime.onStartup.addListener(launch);
var app_launched = false;

function launch() 
{
	if(!app_launched)
	{
		chrome.app.window.create('html/index.html', {'innerBounds': {'width': 860,'height': 675, "minWidth": 860, "minHeight": 675}});
		app_launched = true;
	}  
}

chrome.runtime.onInstalled.addListener(launch);
chrome.runtime.onStartup.addListener(launch);
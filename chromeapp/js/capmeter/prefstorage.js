var capmeter = capmeter || {};
capmeter.prefstorage = capmeter.prefstorage || {};

// Get stored preferences
capmeter.prefstorage.getStoredPreferences = function(callbackFunction)
{
	chrome.storage.sync.get(null, callbackFunction);	
}

// Stored preferences for a given key
capmeter.prefstorage.setStoredPreferences = function(items)
{
	// Save it using the Chrome extension storage API.
	chrome.storage.sync.set(items, function() 
	{
		if (chrome.runtime.error)
		{
			console.log("Store preferences error: " + chrome.runtime.lastError.message);
		}
		else
		{
			console.log("Preferences saved to local storage");			
		}
	});
}

// Clear preference storage
capmeter.prefstorage.clearStoredPreferences = function()
{
	chrome.storage.sync.clear();
}
var capmeter = capmeter || {};
capmeter.filehandler = capmeter.memmgmt || {};

// Temp vars
capmeter.filehandler.tempSyncFS = null;					// Temp SyncFS
capmeter.filehandler.tempFilename = null;				// Temp Filename
capmeter.filehandler.tempContents = "";					// Temp Contents
capmeter.filehandler.tempReadendCallback = null;		// Temp Callback
capmeter.filehandler.tempWriteendCallback = null;		// Temp Callback

capmeter.filehandler.errorHandler = function(e) 
{
	console.log("File handler error name: " + e.name + " / message: " + e.message);
}

// Ask the user to select a file to import its contents
capmeter.filehandler.selectAndReadContents = function(name, readEndCallBack)
{
	chrome.fileSystem.chooseEntry({type: 'openFile', suggestedName: name, accepts: new Array({'extensions': new Array("bin")}), acceptsAllTypes: false},	function(readOnlyEntry) 
																																							{
																																								if(chrome.runtime.lastError)
																																								{
																																									// Something went wrong during file selection
																																									console.log("File select error: "+ chrome.runtime.lastError.message);
																																									readEndCallBack(null);
																																								}
																																								else
																																								{
																																									// File chosen, create reader
																																									readOnlyEntry.file(	function(file) 
																																														{
																																															var reader = new FileReader();
																																															reader.onerror = capmeter.filehandler.errorHandler;
																																															reader.onloadend = readEndCallBack;
																																															reader.readAsText(file);
																																														});
																																								}
																																							});	
}

// Ask the user to select a file to import its contents
capmeter.filehandler.selectAndReadRawContents = function(name, readEndCallBack)
{
	chrome.fileSystem.chooseEntry({type: 'openFile', suggestedName: name, accepts: new Array({'extensions': new Array("img")}), acceptsAllTypes: false},	function(readOnlyEntry) 
																																							{
																																								if(chrome.runtime.lastError)
																																								{
																																									// Something went wrong during file selection
																																									console.log("File select error: "+ chrome.runtime.lastError.message);
																																									readEndCallBack(null);
																																								}
																																								else
																																								{
																																									// File chosen, create reader
																																									readOnlyEntry.file(	function(file) 
																																														{
																																															var reader = new FileReader();
																																															reader.onerror = capmeter.filehandler.errorHandler;
																																															reader.onloadend = readEndCallBack;
																																															reader.readAsArrayBuffer(file);
																																														});
																																								}
																																							});	
}

// Ask the user to select a file and save the provided contents in it
capmeter.filehandler.selectAndSaveFileContents = function(name, contents, writeEndCallback) 
{
	chrome.fileSystem.chooseEntry({type: 'saveFile', suggestedName: name, accepts: new Array({'extensions': new Array("bin")}), acceptsAllTypes: false},	function(writableFileEntry) 
																																							{
																																								if(chrome.runtime.lastError)
																																								{
																																									// Something went wrong during file selection
																																									console.log("File select error: "+ chrome.runtime.lastError.message);
																																								}
																																								else
																																								{
																																									// File chosen, create writer
																																									writableFileEntry.createWriter(	function(writer) 
																																																	{
																																																		var truncated = false;
																																																		// Setup error and writeend call backs, start write
																																																		writer.onerror = capmeter.filehandler.errorHandler;
																																																		writer.onwriteend =	function(e)
																																																							{
																																																								if(!truncated)
																																																								{
																																																									truncated = true;
																																																									this.truncate(this.position);
																																																									writeEndCallback();
																																																								}
																																																							};
																																																		writer.write(contents);
																																																	}, capmeter.filehandler.errorHandler);
																																								}
																																							});
}

// Try and initialize the syncable file storage
capmeter.filehandler.getSyncableFileSystemStatus = function(callbackFunction)
{
	chrome.syncFileSystem.getServiceStatus(callbackFunction);
}

// Set a callback function to listen to syncFC status changes
capmeter.filehandler.setSyncFSStateChangeCallback = function(callbackFunction)
{
	chrome.syncFileSystem.onServiceStatusChanged.addListener(callbackFunction);
}

// Request SyncFS
capmeter.filehandler.requestSyncFS = function(callbackFunction)
{
	chrome.syncFileSystem.requestFileSystem(callbackFunction);
}

// Callback for valid syncFS getFile, for read purposes
capmeter.filehandler.getFileCreateTrueFalseCallbackRead = function(fileEntry)
{
	if(chrome.runtime.lastError)
	{
		// Something went wrong during file selection
		console.log("SyncFS file select error: "+ chrome.runtime.lastError.message);
	}
	else
	{
		// File chosen, create reader
		fileEntry.file(		function(file) 
							{
								var reader = new FileReader();
								reader.onerror = capmeter.filehandler.errorHandler;
								reader.onloadend = capmeter.filehandler.tempReadendCallback;
								reader.readAsText(file);
							});
	}
}

// Callback for valid syncFS getFile, for write purposes
capmeter.filehandler.getFileCreateTrueFalseCallbackWrite = function(fileEntry)
{
	if(chrome.runtime.lastError)
	{
		// Something went wrong during file selection
		console.log("SyncFS file select error: "+ chrome.runtime.lastError.message);
	}
	else
	{
		// File chosen, create writer
		fileEntry.createWriter(	function(writer) 
								{
									var truncated = false;
									// Setup error and writeend call backs, start write
									writer.onerror = capmeter.filehandler.errorHandler;
									writer.onwriteend =	function(e)
														{
															if(!truncated)
															{
																truncated = true;
																this.truncate(this.position);
																capmeter.filehandler.tempWriteendCallback();
															}
														};
									writer.write(capmeter.filehandler.tempContents);
								}, capmeter.filehandler.errorHandler);
	}
}

// Callback for get file with create false
capmeter.filehandler.getFileCreateFalseErrorCallback = function(e)
{
	if(e.name == "NotFoundError")
	{
		capmeter.filehandler.tempSyncFS.root.getFile(capmeter.filehandler.tempFilename, {create:true}, capmeter.filehandler.getFileCreateTrueFalseCallbackRead, capmeter.filehandler.errorHandler);		
	}
	else
	{
		console.log("Unsupported error for getFile with create false: " + e.name + " / message: " + e.message);
	}
}

// Request file in SyncFS
capmeter.filehandler.requestOrCreateFileFromSyncFS = function(filesystem, filename, fileReadCallback)
{
	capmeter.filehandler.tempFilename = filename;
	capmeter.filehandler.tempSyncFS = filesystem;
	capmeter.filehandler.tempReadendCallback = fileReadCallback;
	capmeter.filehandler.tempSyncFS.root.getFile(capmeter.filehandler.tempFilename, {create:false}, capmeter.filehandler.getFileCreateTrueFalseCallbackRead, capmeter.filehandler.getFileCreateFalseErrorCallback);
}

// Write a file in SyncFS
capmeter.filehandler.writeFileToSyncFS = function(filesystem, filename, contents, fileWrittenCallback)
{
	capmeter.filehandler.tempContents = contents;
	capmeter.filehandler.tempFilename = filename;
	capmeter.filehandler.tempSyncFS = filesystem;
	capmeter.filehandler.tempWriteendCallback = fileWrittenCallback;
	capmeter.filehandler.tempSyncFS.root.getFile(capmeter.filehandler.tempFilename, {create:false}, capmeter.filehandler.getFileCreateTrueFalseCallbackWrite, capmeter.filehandler.errorHandler);
}















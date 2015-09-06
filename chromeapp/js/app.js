// Frontend Javascript
// This file is loaded in all frontend files of this app.
var CMD_DEBUG               = 0x00;
var CMD_PING                = 0x01;
var CMD_VERSION				= 0x02;
var CMD_OE_CALIB_STATE  	= 0x03;
var CMD_OE_CALIB_START  	= 0x04;
var CMD_GET_OE_CALIB  		= 0x05;

var device_info = { "vendorId": 0x1209, "productId": 0xdddd };      // capmeter
var version       = 'unknown'; 										// connected capmeter version
var connected     = false;  	 									// current connection state
var connection    = null;   										// connection to the capmeter
var connectMsg = null;  											// saved message to send after connecting
var packetSize = 64;    											// number of bytes in an HID packet
var waitingForAnswer = false;										// boolean indicating if we are waiting for a packet
var debug = false;
var _console_log = console.log;

/**
 * convert a string to a uint8 array
 * @param str the string to convert
 * @returns the uint8 array representing the string with a null terminator
 * @note does not support unicode yet
 */
function strToArray(str)
{
    var buf = new Uint8Array(str.length+1);
    for (var ind=0; ind<str.length; ind++)
    {
        buf[ind] = str.charCodeAt(ind);
    }
    buf[ind] = 0;
    return buf;
}

/**
 * convert a uint8 array to a string
 * @param buf the array to convert
 * @returns the string representation of the array
 * @note does not support unicode yet
 */
function arrayToStr(buf)
{
    res = '';
    for (var ind=0; ind<buf.length; ind++)
    {
        if (buf[ind] == 0)
        {
            return res;
        } else {
            res += String.fromCharCode(buf[ind]);
        }
    }
    return res;
}

console.log = function() {
  text = arguments[0];
  $("#log").val($("#log").val() + text + "\n");
  $("#log").animate({
        scrollTop:$("#log")[0].scrollHeight - $("#log").height()
  }, 200);
  return _console_log.apply(console, arguments);
}

$(function(){
  // Init buttons
  $("*[data-onclick]").on("click", function() {
    function_name = $(this).attr("data-onclick");
    capmeter.helper.executeFunctionByName(function_name, $(this));
  });

  // Init inputs
  $("*[data-input]").on("keyup change", function() {
    function_name = $(this).attr("data-input");
    capmeter.helper.executeFunctionByName(function_name, $(this));
  });

  // Init displays
  setInterval(function() {
    $("*[data-display]").each(function(){
      function_name = $(this).attr("data-display");
      capmeter.helper.executeFunctionByName(function_name, $(this));
    });
  }, 500);
});

/**
 * Capmeter connected, get its state to initialize GUI
 */
function init_gui_state()
{
	// Request open ended calibration state
	sendRequest(CMD_OE_CALIB_STATE, null);
}

/**
 * reset all state data
 */
function reset()
{
    connection = null;  // connection to the capmeter
    connected = false;
}

/**
 * Handler for receiving new data from the device.
 * Decodes the HID message and updates the HTML message divider with
 * to report the received message.
 * @param data the received data
 */
function onDataReceived(reportId, data)
{
    if (typeof reportId === "undefined" || typeof data === "undefined")
    {
        if (chrome.runtime.lastError)
        {
            var err = chrome.runtime.lastError;
            if (err.message != "Transfer failed.")
            {
                console.log("Error in onDataReceived: " + err.message);
            }
        }
        return;
    }

    var bytes = new Uint8Array(data);
    var msg = new Uint8Array(data,2);
	waitingForAnswer = false;
    var len = bytes[0]
    var cmd = bytes[1]

    if (debug && (cmd != CMD_PING) && (cmd != CMD_DEBUG))
    {
        console.log('Received CMD ' + cmd + ', len ' + len + ' ' + JSON.stringify(msg));
    }
	
	switch (cmd)
    {
        case CMD_DEBUG:
        {
            var msg = "";
            for (var i = 0; i < len; i++)
            {
                    msg += String.fromCharCode(bytes[i+2]);
            }
            console.log(msg);
            break;
        }
		
        case CMD_PING:
            //console.log('.');
            break;
			
        case CMD_VERSION:
        {
            version = arrayToStr(new Uint8Array(data, 2));
            if (!connected) 
			{
                console.log('Connected to Capmeter ' + version);
				init_gui_state();
                connected = true;
            }
            break;
        }
		
		case CMD_OE_CALIB_STATE:
		{
			if(len == 1)
			{
				console.log("Platform not calibrated!")
				break;
			}
			// If calibrated platform, print values...
		}
		case CMD_GET_OE_CALIB:
		case CMD_OE_CALIB_START: 
		{
			console.log("Calibration Data Dated From " + bytes[34] + "/" + bytes[33] + "/" + bytes[32])
			console.log("Max Voltage: " + (bytes[30] + bytes[31]*256) + "mV")
			console.log("Single Ended Offset: " + (bytes[26] + bytes[27]*256) + " (" + ((bytes[26] + bytes[27]*256)*1.24/4.095).toFixed(2) + "mV)")
			console.log("Single Ended Offset For Vbias: " + (bytes[24] + bytes[25]*256) + " (" + ((bytes[24] + bytes[25]*256)*1.24/4.095).toFixed(2) + "mV)")
			console.log("Oscillator Low Voltage: " + (bytes[28] + bytes[29]*256) + " (" + ((bytes[28] + bytes[29]*256)*1.24/4.095).toFixed(2) + "mV)")
			console.log("First Threshold Up: " + (bytes[22] + bytes[23]*256) + " (" + ((bytes[22] + bytes[23]*256)*1.24/4.095).toFixed(2) + "mV)")
			console.log("First Threshold Down: " + (bytes[18] + bytes[19]*256) + " (" + ((bytes[18] + bytes[19]*256)*1.24/4.095).toFixed(2) + "mV)")
			console.log("Second Threshold Up: " + (bytes[20] + bytes[21]*256) + " (" + ((bytes[20] + bytes[21]*256)*1.24/4.095).toFixed(2) + "mV)")
			console.log("Second Threshold Down: " + (bytes[16] + bytes[17]*256) + " (" + ((bytes[16] + bytes[17]*256)*1.24/4.095).toFixed(2) + "mV)")
			console.log("Current offset for 1x: " + (bytes[2] + bytes[3]*256))
			console.log("Current offset for 2x: " + (bytes[4] + bytes[5]*256))
			console.log("Current offset for 4x: " + (bytes[6] + bytes[7]*256))
			console.log("Current offset for 8x: " + (bytes[8] + bytes[9]*256))
			console.log("Current offset for 16x: " + (bytes[10] + bytes[11]*256))
			console.log("Current offset for 32x: " + (bytes[12] + bytes[13]*256))
			console.log("Current offset for 64x: " + (bytes[14] + bytes[15]*256))
			break;
		}

        default:
            console.log('unknown command '+ cmd);
            break;
	}
	
    if (connection) 
	{
        chrome.hid.receive(connection, onDataReceived);
    }
}

/**
 * Handler invoked when new USB capmeter devices are found.
 * Connects to the device and sends a version request.
 * @param devices array of device objects
 * @note only the last device is used, assumes that one capmeter is present.
 * Stale entries appear to be left in chrome if the capmeter is removed
 * and plugged in again, or the firmware is updated.
 */
function onDeviceFound(devices)
{
    if (devices.length <= 0)
    {
		console.log('No devices found');
        return;
    }

    var ind = devices.length - 1;
    var devId = devices[ind].deviceId;
    console.log('Found ' + devices.length + ' device(s):');
	for (i=0; i < devices.length; i++)
	{
		console.log('- device #' + devices[i].deviceId + ' vendorId ' + devices[i].vendorId + ' productId ' + devices[i].productId);
	}
    console.log('Connecting to device #' + devId);
    chrome.hid.connect(devId, function(connectInfo)
    {
        if (!chrome.runtime.lastError)
		{
            connection = connectInfo.connectionId;

            if (connectMsg)
            {
                sendMsg(connectMsg);
            }
            else
            {
				sendRequest(CMD_VERSION, null);
            }
        }
        else
        {
          console.log('Failed to connect to device: '+chrome.runtime.lastError.message);
          reset();
        }
    });
}

function sendMsg(msg)
{
	waitingForAnswer = true;
    if (debug) 
	{
        msgUint8 = new Uint8Array(msg);
        // don't output the CMD_PING command since this is the keep alive
        if (msgUint8[1] != CMD_PING) 
		{
            console.log('sending '+ JSON.stringify(new Uint8Array(msg)));
        }
    }
    chrome.hid.send(connection, 0, msg, function()
    {
        if (!chrome.runtime.lastError)
        {
            chrome.hid.receive(connection, onDataReceived);
        }
        else
        {
            if (connected)
            {
                console.log('Failed to send to device: ' + chrome.runtime.lastError.message);
                console.log('Disconnected from capmeter');
                reset();
            }
        }
    });
}

/**
 * Send a binary message to the capmeter
 * @param type the request type to send (e.g. CMD_VERSION)
 * @param content Uint8 array message content (optional)
 */
function sendRequest(type, content)
{
    msg = new ArrayBuffer(packetSize);
    header = new Uint8Array(msg, 0);
    body = new Uint8Array(msg, 2);

    if (content)
    {
        header.set([content.length, type], 0);
        body.set(content, 0);
    }
    else
    {
        header.set([0, type], 0);
    }

    if (!connection)
    {
        connect(msg);
        return;
    }
	
    sendMsg(msg);
}

function sendPing()
{
	sendRequest(CMD_PING, null);
}

function connect(msg)
{
    reset();
    if (msg)
    {
        connectMsg = msg;
    }
    chrome.hid.getDevices(device_info, onDeviceFound);
}

function checkConnection()
{
    if (!connected) 
	{
        connect();
    } 
	else 
	{
		if (!waitingForAnswer)
		{
			sendPing();
		}
    }
}

setInterval(checkConnection, 2000);
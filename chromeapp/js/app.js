// Frontend Javascript
// This file is loaded in all frontend files of this app.
var CMD_DEBUG               = 0x00;
var CMD_PING                = 0x01;
var CMD_VERSION				= 0x02;
var CMD_OE_CALIB_STATE  	= 0x03;
var CMD_OE_CALIB_START  	= 0x04;
var CMD_GET_OE_CALIB  		= 0x05;
var CMD_SET_VBIAS			= 0x06;
var CMD_DISABLE_VBIAS		= 0x07;
var CMD_CUR_MES_MODE		= 0x08;
var CMD_CUR_MES_MODE_EXIT	= 0x09;
var CMD_CAP_REPORT_FREQ		= 0x0A;
var CMD_CAP_MES_START  		= 0x0B;
var CMD_CAP_MES_REPORT 		= 0x0C;
var CMD_CAP_MES_EXIT   		= 0x0D;

// Current mode
var MODE_IDLE			= 0
var MODE_CUR_MES_REQ	= 1
var MODE_CUR_MES		= 2
var MODE_OE_CALIB		= 3
var MODE_CAP_MES_REQ	= 4
var MODE_CAP_MES		= 5
var MODE_CAP_CALIB_REQ	= 6
var MODE_CAP_CALIB		= 7

// Platforms mins
var VBIAS_MIN			= 1300

var device_info = { "vendorId": 0x1209, "productId": 0xdddd };      // capmeter
var version       = 'unknown'; 										// connected capmeter version
var current_mode = MODE_IDLE;										// current mode
var connected     = false;  	 									// current connection state
var connection    = null;   										// connection to the capmeter
var connectMsg = null;  											// saved message to send after connecting
var current_ampl = 0;												// current measurement amplification
var current_avg = 14;												// current measurement averaging
var ping_enabled = true;											// know if we send ping requests
var cap_calib_array_ind = 0;										// Index inside the calibration array to store the value
var cap_calib_array = [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0];	// Capacitance calibration array
var capacitance_report_freq = 3;									// Capacitance report frequency in bit shift
var capacitance_offset = 0;											// Capacitance offset
var cap_calibration_done = false;									// Capacitance calibration done bool
var packetSize = 64;    											// number of bytes in an HID packet
var waitingForAnswer = false;										// boolean indicating if we are waiting for a packet
var debug = false;
var _console_log = console.log;


function standardDeviation(values)
{
  var avg = average(values);
  
  var squareDiffs = values.map(function(value){
    var diff = value - avg;
    var sqrDiff = diff * diff;
    return sqrDiff;
  });
  
  var avgSquareDiff = average(squareDiffs);

  var stdDev = Math.sqrt(avgSquareDiff);
  return stdDev;
}

function average(data)
{
  var sum = data.reduce(function(sum, value){
    return sum + value;
  }, 0);

  var avg = sum / data.length;
  return avg;
}

function saveSettingsToStorage() 
{
	// Save it using the Chrome extension storage API.
	chrome.storage.sync.set({"calib_data": capacitance_offset}, function() 
	{
		if (chrome.runtime.error)
		{
			console.log("Runtime error!");
		}
		else
		{
			// Notify that we saved.
			console.log("Settings saved to local storage");			
		}
	});
}

function getSettingsFromStorage() 
{
	// Save it using the Chrome extension storage API.
	chrome.storage.sync.get("calib_data", function(items) 
	{
		if (!chrome.runtime.error) 
		{
			calib_data = items["calib_data"];
			if(calib_data != null)
			{
				console.log("Loaded capacitance offset: " + valueToElectronicString(calib_data, "F"));
				$('#calibrateCapacitance').css('background', 'orange');
				capacitance_offset = calib_data;
				cap_calibration_done = true;
			}
		}
	});
}

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

/**
 * convert a components value to a normal representable form
 * @param value	The value
 * @returns the string representation 
 */
function valueToElectronicString(value, unit)
{
	if(value < 1e-12)
	{
		if(value * 1e15 < 10)
		{
			return (value * 1e15).toFixed(2) + "f" + unit;
		}
		else
		{
			return (value * 1e15).toFixed(1) + "f" + unit;
		}
	}
	else if(value < 1e-9)
	{
		if(value * 1e12 < 10)
		{
			return (value * 1e12).toFixed(2) + "p" + unit;
		}
		else
		{
			return (value * 1e12).toFixed(1) + "p" + unit;
		}
	}
	else if(value < 1e-6)
	{
		if(value * 1e9 < 10)
		{
			return (value * 1e9).toFixed(2) + "n" + unit;
		}
		else
		{
			return (value * 1e9).toFixed(1) + "n" + unit;
		}
	}
	else if(value < 1e-3)
	{
		if(value * 1e6 < 10)
		{
			return (value * 1e6).toFixed(2) + "u" + unit;
		}
		else
		{
			return (value * 1e6).toFixed(1) + "u" + unit;
		}
	}
	else if(value < 1)
	{
		if(value * 1e3 < 10)
		{
			return (value * 1e3).toFixed(2) + "m" + unit;
		}
		else
		{
			return (value * 1e3).toFixed(1) + "m" + unit;
		}
	}
	else if(value > 1e3)
	{
		if(value * 1e-3 < 10)
		{
			return (value * 1e-3).toFixed(2) + "k" + unit;
		}
		else
		{
			return (value * 1e-3).toFixed(1) + "k" + unit;
		}
	}
	else
	{
		return value.toFixed(1);
	}
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

    if ($(this).attr('disabled')) return;
    capmeter.helper.executeFunctionByName(function_name, $(this));
  });

  // Init inputs
  $("*[data-input]").on("keyup change", function() {
    function_name = $(this).attr("data-input");

    if ($(this).attr('disabled')) return;
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

function disable_gui_buttons()
{
	$("#calibrateCurrentX").attr("disabled", 1);
	$("#calibrateCurrentY").attr("disabled", 1);
	$("#calibrateCapacitance").attr("disabled", 1);
	$("#calibratePlatform").attr("disabled", 1);
	$("#measureCurrent").attr("disabled", 1);
	$("#measureCapacitance").attr("disabled", 1);
	$("#capacitance").attr("disabled", 1);
	$("#current").attr("disabled", 1);
}

function enable_gui_buttons()
{
	$("#calibrateCurrentX").removeAttr("disabled");
	$("#calibrateCurrentY").removeAttr("disabled");
	$("#calibrateCapacitance").removeAttr("disabled");
	$("#calibratePlatform").removeAttr("disabled");
	$("#measureCurrent").removeAttr("disabled");
	$("#measureCapacitance").removeAttr("disabled");
	$("#capacitance").removeAttr("disabled");
	$("#current").removeAttr("disabled");
}

function start_open_ended_calibration()
{	
	if(current_mode == MODE_IDLE)
	{
		console.log("Starting Open Ended Calibration...");
		var d = new Date();
		sendRequest(CMD_OE_CALIB_START, [d.getDate(), d.getMonth() + 1, d.getFullYear()%100]);
		current_mode = MODE_OE_CALIB;
		disable_gui_buttons();
	}
}

function start_capacitance_calibration()
{	
	if(current_mode == MODE_IDLE)
	{
		disable_gui_buttons();
		capacitance_offset = 0;
		cap_calib_array_ind = 0;
		cap_calibration_done = false;
		current_mode = MODE_CAP_CALIB_REQ;
		sendRequest(CMD_SET_VBIAS, [VBIAS_MIN%256, Math.floor(VBIAS_MIN/256)]);
		console.log("Starting Capacitance Calibration...");
		console.log("Please hold 1pF capacitor between leads...");
	}
}

/**
 * Start current measurement
 */
function static_current_measurement_start(voltage)
{
	voltage = voltage * 1000;
	
	// Check current mode
	if(current_mode == MODE_CUR_MES)
	{
		// Already measuring, stop the mode
		disable_gui_buttons();
		sendRequest(CMD_CUR_MES_MODE_EXIT, null);
	}
	else if(current_mode == MODE_IDLE)
	{
		// Set desired voltage
		if(voltage < VBIAS_MIN)
		{
			voltage = VBIAS_MIN;
			$('#voltageCurrent').val(VBIAS_MIN/1000);
		}
		
		console.log("Requesting voltage set to " + voltage + "mV");
		sendRequest(CMD_SET_VBIAS, [voltage%256, Math.floor(voltage/256)]);		
		current_mode = MODE_CUR_MES_REQ;
		disable_gui_buttons();
	}	
}

/**
 * Start capacitance measurement
 */
function static_capacitance_measurement_start(voltage)
{
	voltage = voltage * 1000;
	
	// Check current mode
	if(current_mode == MODE_CAP_MES)
	{
		// Already measuring, stop the mode
		disable_gui_buttons();
		sendRequest(CMD_CAP_MES_EXIT, null);
	}
	else if(current_mode == MODE_IDLE)
	{
		// Set desired voltage
		if(voltage < VBIAS_MIN)
		{
			voltage = VBIAS_MIN;
			$('#voltageCapacitance').val(VBIAS_MIN/1000);
		}
		
		console.log("Requesting voltage set to " + voltage + "mV");
		sendRequest(CMD_SET_VBIAS, [voltage%256, Math.floor(voltage/256)]);		
		current_mode = MODE_CAP_MES_REQ;
		disable_gui_buttons();
	}
}

/**
 * reset all state data
 */
function reset()
{
    connection = null;  // connection to the capmeter
    connected = false;
	disable_gui_buttons();
	current_mode = MODE_IDLE;
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
		
		case CMD_OE_CALIB_START: 
		{
			if(len == 1)
			{
				console.log("Couldn't start open ended calibration!");
				break;
			}
			// Get back to idle mode
			current_mode = MODE_IDLE;
			enable_gui_buttons();
		}
		case CMD_OE_CALIB_STATE:
		{
			if(len == 1)
			{
				$("#calibratePlatform").removeAttr("disabled");
				console.log("Platform not calibrated!");
				break;
			}
			// If calibrated platform, print values and enable buttons
			enable_gui_buttons();
			$('#calibratePlatform').css('background', 'orange');
		}
		case CMD_GET_OE_CALIB:
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
		
		case CMD_CAP_REPORT_FREQ:
		{
			if(current_mode == MODE_CAP_MES_REQ)
			{
				if(msg[0] == 0)
				{
					console.log("Couldn't set report frequency!");
					current_mode = MODE_IDLE;
					enable_gui_buttons();
				}
				else
				{
					console.log("Report frequency set");					
					// We start capacitance measurement mode
					sendRequest(CMD_CAP_MES_START, null);
				}
			}
			break;
		}
		
		case CMD_SET_VBIAS:
		{
			console.log("Voltage set to " + (bytes[2] + bytes[3]*256) + "mV")
			// Here are the following actions depending on the mode we want
			if(current_mode == MODE_CUR_MES_REQ)
			{
				// We start current measurement mode
				sendRequest(CMD_CUR_MES_MODE, [current_ampl, current_avg]);
			}
			else if(current_mode == MODE_CAP_MES_REQ)
			{
				// Next step: set report frequency
				sendRequest(CMD_CAP_REPORT_FREQ, [capacitance_report_freq]);
			}
			else if(current_mode == MODE_CAP_CALIB_REQ)
			{
				// We start capacitance measurement mode
				sendRequest(CMD_CAP_MES_START, null);
			}
			break;
		}
		
		case CMD_DISABLE_VBIAS:
		{
			console.log("Vbias disabled")
			// Here are the following actions depending on the mode we want
			if(current_mode == MODE_CUR_MES_REQ)
			{
				// We start current measurement mode
				sendRequest(CMD_CUR_MES_MODE, [current_ampl, current_avg]);
			}
			else if(current_mode == MODE_CAP_MES_REQ)
			{
				// Next step: set report frequency
				sendRequest(CMD_CAP_REPORT_FREQ, [capacitance_report_freq]);
			}
			else if(current_mode == MODE_CAP_CALIB_REQ)
			{
				// We start capacitance measurement mode
				sendRequest(CMD_CAP_MES_START, null);
			}
			break;
		}
		
		case CMD_CAP_MES_START:
		{
			if(msg[0] == 0)
			{				
				if(current_mode == MODE_CAP_MES_REQ || current_mode == MODE_CAP_CALIB_REQ)
				{
					console.log("Couldn't start capacitance measurement mode");
					current_mode = MODE_IDLE;
					enable_gui_buttons();
				}
			}
			else
			{	
				if(current_mode == MODE_CAP_MES_REQ)
				{
					console.log("Capacitance measurement mode started");
					$('#measureCapacitance').css('background', 'orange');
					$("#measureCapacitance").removeAttr("disabled");
					current_mode = MODE_CAP_MES;
					ping_enabled = false;
				}
				else if(current_mode == MODE_CAP_CALIB_REQ)
				{
					console.log("Capacitance measurement mode for calibration started");
					current_mode = MODE_CAP_CALIB;
					ping_enabled = false;
				}
			}
			break;
		}
		
		case CMD_CAP_MES_EXIT:
		{
			if(msg[0] == 0)
			{				
				if(current_mode == MODE_CAP_MES || current_mode == MODE_CAP_CALIB)
				{
					console.log("Couldn't stop capacitance measurement mode");
				}
			}
			else
			{	
				if(current_mode == MODE_CAP_MES || current_mode == MODE_CAP_CALIB)
				{
					console.log("Capacitance measurement mode stopped");
					$('#measureCapacitance').css('background', '#3ED1D6');
					sendRequest(CMD_DISABLE_VBIAS, null);
					current_mode = MODE_IDLE;		
					enable_gui_buttons();			
					ping_enabled = true;
				}
			}
			break;
		}
		
		case CMD_CAP_MES_REPORT:
		{
			var counter_divider = (msg[1]<<8) + msg[0];
			var aggregate_fall = (msg[5]<<8) + (msg[4]<<16) + (msg[3]<<8) + msg[2];
			var counter_val = (msg[9]<<8) + (msg[8]<<16) + (msg[7]<<8) + msg[6];
			var report_freq = msg[10];
			var resistor_val = 2 * ((msg[12]<<8) + msg[11]);
			var second_threshold = (msg[14]<<8) + msg[13];
			var first_threshold = (msg[16]<<8) + msg[15];
			//console.log("Capacitance report - counter_divider: " + counter_divider + " aggregate_fall: " + aggregate_fall + " counter_val: " + counter_val + " report freq: " + report_freq + "Hz resistor: " + resistor_val + "Ohms second threshold: " + second_threshold + " first threshold: " + first_threshold);
			// C =  - counter divider * aggregate / 32M * counter * 2 * half_r * ln(Vt2/Vt1)
			var capacitance = 0;
			if(cap_calibration_done)
			{
				capacitance = -1*((counter_divider * aggregate_fall) / (32000000 * counter_val * resistor_val * Math.log(second_threshold/first_threshold))) - capacitance_offset;				
			}
			else
			{
				capacitance = -1*((counter_divider * aggregate_fall) / (32000000 * counter_val * resistor_val * Math.log(second_threshold/first_threshold)));				
			}
			if(capacitance < 0)
			{
				capacitance = 0;
			}
			//console.log("Measured capacitance: " + valueToElectronicString(capacitance, "F") + ", osc. freq.: " + counter_val*report_freq  + "Hz (" + resistor_val + "R)");
			capmeter.measurement._capacitance = valueToElectronicString(capacitance, "F") + "(" + valueToElectronicString(counter_val*report_freq, "Hz") + ")";
			
			if(current_mode == MODE_CAP_CALIB)
			{
				// Store value in array
				cap_calib_array[(cap_calib_array_ind++)%cap_calib_array.length] = capacitance;
				// Check if we have enough values to compute offset
				if(cap_calib_array_ind >= cap_calib_array.length)
				{
					var cap_standard_deviation = standardDeviation(cap_calib_array);
					var cap_average = average(cap_calib_array);
					console.log("Standard deviation: " + valueToElectronicString(cap_standard_deviation, "F") + " average: " + valueToElectronicString(cap_average, "F"));
					// Only accept if we are within 1%
					if(cap_average * 0.01 > cap_standard_deviation)
					{
						// Store offset and exit
						enable_gui_buttons();
						cap_calibration_done = true;
						capacitance_offset = cap_average - 1e-12;
						capacitance_offset = cap_average;
						sendRequest(CMD_CAP_MES_EXIT, null);
						console.log("Capacitance offset taken: " + valueToElectronicString(capacitance_offset, "F"));
						saveSettingsToStorage();
						$('#calibrateCapacitance').css('background', 'orange');
					}
				}
			}
			
			break;
		}
		
		case CMD_CUR_MES_MODE:
		{
			// Check return success
			if(len == 1)
			{
				// Fail when requesting measure!!!
				if(current_mode == MODE_CUR_MES_REQ)
				{
					// Go back to idle mode
					current_mode = MODE_IDLE;
					enable_gui_buttons();
				}
			}
			else
			{
				// Measurement received
				if(current_mode == MODE_CUR_MES_REQ)
				{
					current_mode = MODE_CUR_MES;		
					$("#measureCurrent").removeAttr("disabled");
					$('#measureCurrent').css('background', 'orange');
				}
				if(current_mode == MODE_CUR_MES)
				{
					// Start another measurement					
					var current = valueToElectronicString((((bytes[2] + bytes[3]*256)*1.24)/(0.2047*(1 << current_ampl)))*1e-9, "A");
					console.log("Received ADC current measurement: " + current);
					sendRequest(CMD_CUR_MES_MODE, [current_ampl, current_avg]);		
					capmeter.measurement._current = current;
				}
			}
			break;
		}
		
		case CMD_CUR_MES_MODE_EXIT:
		{
			if(bytes[2] != 0 && current_mode == MODE_CUR_MES)
			{
				enable_gui_buttons();
				current_mode = MODE_IDLE;
				sendRequest(CMD_DISABLE_VBIAS, null);
				$('#measureCurrent').css('background', '#3ED1D6');
				console.log("Current measurement mode excited!");
			}
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
		if (!waitingForAnswer && ping_enabled)
		{
			sendPing();
		}
    }
}

document.body.onload = function() 
{
	setInterval(checkConnection, 2000);
	$('#voltageCurrent').attr("value", VBIAS_MIN/1000);
	$('#voltageCapacitance').attr("value", VBIAS_MIN/1000);
	$('#maxVoltage').attr("value", 10);
	getSettingsFromStorage();
	disable_gui_buttons();
}
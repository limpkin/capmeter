var capmeter = capmeter || {};
capmeter.app = capmeter.app || {};

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
var CMD_SET_VBIAS_DAC		= 0x0E;
var CMD_RESET_STATE			= 0x0F;
var CMD_SET_EEPROM_VALS     = 0x10;
var CMD_READ_EEPROM_VALS    = 0x11;
var CMD_BOOTLOADER_JUMP		= 0xFF;

// Current mode
var MODE_IDLE			= 0
var MODE_CUR_MES_REQ	= 1
var MODE_CUR_MES		= 2
var MODE_OE_CALIB		= 3
var MODE_CAP_MES_REQ	= 4
var MODE_CAP_MES		= 5
var MODE_CAP_CALIB_REQ	= 6
var MODE_CAP_CALIB		= 7
var MODE_CAP_CARAC		= 8
var MODE_CAP_CARAC_REQ	= 9
var MODE_CUR_CARAC		= 10
var MODE_CUR_CARAC_REQ	= 11
var MODE_CUR_CALIB_REQ	= 12
var MODE_CUR_CALIB		= 13

// Platforms mins/maxs
var V_OSCILLATION			= 850			// Cap measurement oscillation center
var VBIAS_MIN				= 1300			// Minimum vbias voltage we allow
var VBIAS_CUR_CALIB_ST		= 2000			// Vbias at which we start current calibration
var NB_GRAPH_POINTS			= 20			// Default number of points for our graph
var NB_MS_WAIT_VBIAS_MES	= 100			// How many milliseconds we wait before measuring vbias
var EEPROM_STORED_DATA_SIZE = (1024-50)		// How many bytes we can store in our platform
var EEPROM_READ_NBBYTES 	= 60			// How many bytes we read
var EEPROM_WRITE_NBBYTES	= 59			// How many bytes we write

var device_info = { "vendorId": 0x1209, "productId": 0xdddd };      			// capmeter
var version       = 'unknown'; 													// connected capmeter version
var current_mode = MODE_IDLE;													// current mode
var connected     = false;  	 												// current connection state
var connection    = null;   													// connection to the capmeter
var connectMsg = null;  														// saved message to send after connecting
var carac_averaging = 8;														// Number of samples for carac averaging
var current_ampl = 0;															// current measurement amplification
var current_avg = 14;															// current measurement averaging
var current_calib_avg = 14;														// current measurement averaging during calibration
var ping_enabled = true;														// know if we send ping requests
var platform_max_vbias = 10000;													// max vbias the platform can generate
var capacitance_offset = null;													// Capacitance offset
var cap_calib_array_ind = 0;													// Index inside the calibration array to store the value
var cap_calib_array = [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0];				// Capacitance calibration array
var cap_last_values = [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0];				// Last capacitance values
var cap_last_value_ind = 0;														// Index to where to store the latest values
var capacitance_report_freq = 3;												// Capacitance report frequency in bit shift
var packetSize = 64;    														// number of bytes in an HID packet
var waitingForAnswer = false;													// boolean indicating if we are waiting for a packet
var vbias_mes_capacitance_changed = false;										// boolean indicating that vbias was changed while in mes capacitance mode
var vbias_mes_current_changed = false;											// boolean indicating that vbias was changed while in mes current mode
var cur_mes_new_vbias_req = 0;													// contains the new bias voltage requested
var cur_calib_dacv = 0;															// DAC value for the current calibration vbias start
var cur_calib_vbias = 0;														// Set voltage value for current calibration start
var max_cur_adc_val_1x = 2047;													// Max adc val for current measurement mode
var cur_calib_state = "null";													// Current calibration state
var eeprom_calib_data = [];														// EEPROM calib data
var eeprom_stored_data = [];													// EEPROM stored data
var eeprom_read_counter = 0;													// EEPROM read counter
var eeprom_data_to_store = [];													// EEPROM data to store
var eeprom_write_counter = 0;													// EEPROM write counter
var eeprom_stored_data_valid = false;											// Is the data we just received valid?
var debug = false;

capmeter.app.preferences = {"memmgmtPrefsStored": false, "syncFSAllowed": false, "capOffset": 0, "capCalibDone": false, "adcMapping1": new Array(1024), "adcMapping2": new Array(1024)};


// Load memory management preferences callback
capmeter.app.preferencesStorageCallback = function(items)
{	
	//console.log(items);
	if(chrome.runtime.lastError)
	{
		// Something went wrong during file selection
		console.log("preferencesCallback error: "+ chrome.runtime.lastError.message);
	}
	else
	{		
		if(items.memmgmtPrefsStored == null)// || true)
		{
			// Empty file, save new preferences
			console.log("Preferences storage: No preferences stored!");
			capmeter.prefstorage.setStoredPreferences(capmeter.app.preferences);
		}
		else
		{
			//capmeter.prefstorage.clearStoredPreferences();
			capmeter.app.preferences = items;
			if(capmeter.app.preferences.capCalibDone)
			{
				//console.log("Loaded capacitance offset: " + capmeter.util.valueToElectronicString(capmeter.app.preferences.capOffset, "F"));		
				$('#calibrateCapacitance').css('background', 'orange');				
			}
			// See if we have a calibrated current measurement
			capmeter.currentcalib.initCalib();			
			//console.log(capmeter.app.preferences);
		}
	}
}

function disable_gui_buttons()
{
	if (capmeter.settings.debug) return;
	$("button, .button").attr("disabled", 1);
}

function enable_gui_buttons()
{
	$("button, .button").removeAttr("disabled");
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
		cap_calib_array_ind = 0;
		current_mode = MODE_CAP_CALIB_REQ;
		capmeter.app.preferences.capOffset = 0;
		capmeter.app.preferences.capCalibDone = false;		
		sendRequest(CMD_SET_VBIAS, [VBIAS_MIN%256, Math.floor(VBIAS_MIN/256)]);
		console.log("Starting Capacitance Calibration...");
		console.log("Please hold 1pF capacitor between leads...");
	}
}

function start_current_calibration()
{	
	if(current_mode == MODE_IDLE)
	{
		disable_gui_buttons();
		cur_calib_state = "start";
		current_mode = MODE_CUR_CALIB_REQ;
		console.log("Starting Current Calibration...");
		capmeter.currentcalib.startCalib(capmeter.measurement._calibrationResistance * 1e6, 0);
		sendRequest(CMD_SET_VBIAS, [VBIAS_CUR_CALIB_ST%256, Math.floor(VBIAS_CUR_CALIB_ST/256)]);
	}	
	else if(current_mode == MODE_CUR_CALIB)
	{		
		disable_gui_buttons();
		cur_calib_state = "null";
		sendRequest(CMD_CUR_MES_MODE_EXIT, null);
	}
}

/**
 * Current caracterization
 */
function current_caracterization_start(max_voltage)
{
	max_voltage = max_voltage * 1000;
	if(max_voltage < VBIAS_MIN)
	{
		max_voltage = VBIAS_MIN;
		$('#maxVoltage').val(((VBIAS_MIN)/1000).toFixed(2));
	}
	else if(max_voltage > platform_max_vbias)
	{
		max_voltage = platform_max_vbias;
		$('#maxVoltage').val(((platform_max_vbias)/1000).toFixed(2));
	}
	
	if(current_mode == MODE_CUR_CARAC)
	{
		// Already measuring, stop the mode
		disable_gui_buttons();
		sendRequest(CMD_CUR_MES_MODE_EXIT, null);
	}
	else if(current_mode == MODE_IDLE)
	{
		capmeter.graphing.initCurMeasGraphing(max_voltage, capmeter.visualisation._datapoints, carac_averaging);	
		sendRequest(CMD_SET_VBIAS, [VBIAS_MIN%256, Math.floor(VBIAS_MIN/256)]);		
		current_mode = MODE_CUR_CARAC_REQ;
		disable_gui_buttons();
	}
}

/**
 * Capacitance caracterization
 */
function capacitance_caracterization_start(max_voltage)
{
	// Compute voltage in mV, add the oscillation voltage that is removed in the GUI
	max_voltage = max_voltage * 1000 + V_OSCILLATION;
	if(max_voltage < VBIAS_MIN)
	{
		max_voltage = VBIAS_MIN;
		$('#maxVoltage').val(((VBIAS_MIN - V_OSCILLATION)/1000).toFixed(2));
	}
	else if(max_voltage > platform_max_vbias - V_OSCILLATION)
	{
		max_voltage = platform_max_vbias;
		$('#maxVoltage').val(((platform_max_vbias - V_OSCILLATION)/1000).toFixed(2));
	}
	
	if(current_mode == MODE_CAP_CARAC)
	{
		// Already measuring, stop the mode
		disable_gui_buttons();
		sendRequest(CMD_CAP_MES_EXIT, null);
	}
	else if(current_mode == MODE_IDLE)
	{		
		capmeter.graphing.initCapMeasGraphing(max_voltage, capmeter.visualisation._datapoints, carac_averaging);	
		sendRequest(CMD_SET_VBIAS, [VBIAS_MIN%256, Math.floor(VBIAS_MIN/256)]);	
		current_mode = MODE_CAP_CARAC_REQ;
		disable_gui_buttons();
	}
}

/**
 * Start current measurement
 */
function static_current_measurement_start(voltage)
{
	voltage = voltage * 1000;
	if(voltage < VBIAS_MIN)
	{
		voltage = VBIAS_MIN;
		$('#voltageCurrent').val(((VBIAS_MIN)/1000).toFixed(2));
	}
	else if(voltage > platform_max_vbias)
	{
		voltage = platform_max_vbias;
		$('#voltageCurrent').val(((platform_max_vbias)/1000).toFixed(2));
	}
	
	if(vbias_mes_current_changed)
	{
		// If vbias was changed, clicking the button updates vbias
		$("#measureCurrent").attr("disabled", 1);
		vbias_mes_current_changed = false;	
		cur_mes_new_vbias_req = voltage;
	}
	else if(current_mode == MODE_CUR_MES)
	{
		// Already measuring, stop the mode
		disable_gui_buttons();
		sendRequest(CMD_CUR_MES_MODE_EXIT, null);
	}
	else if(current_mode == MODE_IDLE)
	{		
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
	// Compute voltage in mV, add the oscillation voltage that is removed in the GUI
	voltage = voltage * 1000 + V_OSCILLATION;
	if(voltage < VBIAS_MIN)
	{
		voltage = VBIAS_MIN;
		$('#voltageCapacitance').val(((VBIAS_MIN - V_OSCILLATION)/1000).toFixed(2));
	}
	else if(voltage > platform_max_vbias - V_OSCILLATION)
	{
		voltage = platform_max_vbias;
		$('#voltageCapacitance').val(((platform_max_vbias - V_OSCILLATION)/1000).toFixed(2));
	}
	
	if(vbias_mes_capacitance_changed)
	{
		// If vbias was changed, clicking the button updates vbias
		sendRequest(CMD_SET_VBIAS, [voltage%256, Math.floor(voltage/256)]);	
		console.log("Requesting voltage set to " + voltage + "mV");
		$("#measureCapacitance").attr("disabled", 1);
		vbias_mes_capacitance_changed = false;
	}
	else if(current_mode == MODE_CAP_MES)
	{
		// Already measuring, stop the mode
		disable_gui_buttons();
		sendRequest(CMD_CAP_MES_EXIT, null);
	}
	else if(current_mode == MODE_IDLE)
	{
		// Set desired voltage		
		console.log("Requesting voltage set to " + voltage + "mV");
		sendRequest(CMD_SET_VBIAS, [voltage%256, Math.floor(voltage/256)]);		
		current_mode = MODE_CAP_MES_REQ;
		disable_gui_buttons();
	}
}

/**
 * Notification that the voltage set has been changed
 */
function notify_vcurrent_change(voltage)
{	
	// Check if we are in the right mode
	if(current_mode == MODE_CUR_MES || current_mode == MODE_CUR_MES_REQ)
	{
		// change button color to allow the user to change the voltage
		$('#measureCurrent').css('background', 'green');
		vbias_mes_current_changed = true;
	}
}

/**
 * Notification that the voltage set has been changed
 */
function notify_vcap_change(voltage)
{
	// Check if we are in the right mode
	if(current_mode == MODE_CAP_MES || current_mode == MODE_CAP_MES_REQ)
	{
		// change button color to allow the user to change the voltage
		$('#measureCapacitance').css('background', 'green');
		vbias_mes_capacitance_changed = true;
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

// Export raw correction data to capmeter eeprom
function exportAdvancedCalibDataToEeprom()
{
	// Export our correction data
	eeprom_data_to_store = JSON.stringify({"cap": capacitance_offset, "cur": capmeter.currentcalib.getRawAdcToIdealAdcVector()});
	eeprom_write_counter = 0;	
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
		{
            //console.log('.');
            break;
		}
		
        case CMD_VERSION:
        {
            version = capmeter.util.arrayToStr(new Uint8Array(data, 2));
            if (!connected) 
			{
                console.log('Connected to Capmeter ' + version);
				if(version == "v0.1")
				{
					sendRequest(CMD_RESET_STATE, null);
				}
				else
				{
					sendRequest(CMD_OE_CALIB_STATE, null);
				}				
				//
				//sendRequest(CMD_SET_EEPROM_VALS, [0, 10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10]);
				//sendRequest(CMD_READ_EEPROM_VALS, [0, 10]);
                connected = true;
            }
            break;
        }
		
		case CMD_RESET_STATE:
		{
			// Capmeter reset, now we need to read the EEPROM contents for possible json compressed data
			//console.log("Capmeter State Reset");
			eeprom_calib_data = [];
			eeprom_read_counter = 0;
			eeprom_stored_data_valid = false;
			eeprom_stored_data = new Uint8Array(EEPROM_STORED_DATA_SIZE);
			sendRequest(CMD_READ_EEPROM_VALS, [0, 0, EEPROM_READ_NBBYTES]);
			break;
		}
		
		case CMD_READ_EEPROM_VALS:
		{
			//console.log("Received EEPROM data: " + bytes.subarray(2, 2 + bytes[0]));
			eeprom_stored_data.set(bytes.subarray(2, 2 + bytes[0], eeprom_read_counter), eeprom_read_counter);
			eeprom_read_counter += bytes[0];
			
			// Ask the next packet or stop
			if(eeprom_read_counter == EEPROM_STORED_DATA_SIZE)
			{
				// We received all we could read, try to parse the data
				eeprom_stored_data[EEPROM_STORED_DATA_SIZE-1] = 0;
				
				try
				{
					eeprom_calib_data = JSON.parse(capmeter.util.arrayToStr(eeprom_stored_data));
					console.log("EEPROM calibration data successfully parsed!");
					eeprom_stored_data_valid = true;
				}
				catch(e)
				{
					console.log("No advanced EEPROM calibration data in capmeter");
				}
				sendRequest(CMD_OE_CALIB_STATE, null);
			}
			else
			{
				// Still data to read...
				var nb_bytes_to_read = EEPROM_STORED_DATA_SIZE - eeprom_read_counter;
				if(nb_bytes_to_read > EEPROM_READ_NBBYTES)
				{
					nb_bytes_to_read = EEPROM_READ_NBBYTES;
				}
				sendRequest(CMD_READ_EEPROM_VALS, [(eeprom_read_counter%256), Math.floor(eeprom_read_counter/256), nb_bytes_to_read]);
			}
			break;
		}
		
		case CMD_SET_EEPROM_VALS:
		{
			if(bytes[2] == 0)
			{
				console.log("Couldn't set EEPROM vals!");
			}
			else
			{
				console.log("EEPROM vals set");				
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
			// Update max voltage
			platform_max_vbias = bytes[30] + bytes[31]*256;
			$('#maxVoltage').val(((platform_max_vbias)/1000).toFixed(2));
			capmeter.visualisation._maxVoltage = ((platform_max_vbias)/1000).toFixed(2);
			
			// Update max adc vals
			max_cur_adc_val_1x = 2047 - (bytes[2] + bytes[3]*256);
			
			// Print out debug info
			console.log("Calibration Data Received, Dated From " + bytes[34] + "/" + bytes[33] + "/" + bytes[32])
			console.log("Max Voltage: " + (bytes[30] + bytes[31]*256) + "mV")
			console.log("Single Ended Offset: " + (bytes[26] + bytes[27]*256) + " (" + ((bytes[26] + bytes[27]*256)*1.24/4.095).toFixed(2) + "mV)")
			console.log("Single Ended Offset For Vbias: " + (bytes[24] + bytes[25]*256) + " (" + ((bytes[24] + bytes[25]*256)*1.24/4.095).toFixed(2) + "mV)")
			console.log("Oscillator Low Voltage: " + (bytes[28] + bytes[29]*256) + " (" + ((bytes[28] + bytes[29]*256)*1.24/4.095).toFixed(2) + "mV)")
			console.log("First Threshold Up: " + (bytes[22] + bytes[23]*256) + " (" + ((bytes[22] + bytes[23]*256)*1.24/4.095).toFixed(2) + "mV)")
			console.log("First Threshold Down: " + (bytes[18] + bytes[19]*256) + " (" + ((bytes[18] + bytes[19]*256)*1.24/4.095).toFixed(2) + "mV)")
			console.log("Second Threshold Up: " + (bytes[20] + bytes[21]*256) + " (" + ((bytes[20] + bytes[21]*256)*1.24/4.095).toFixed(2) + "mV)")
			console.log("Second Threshold Down: " + (bytes[16] + bytes[17]*256) + " (" + ((bytes[16] + bytes[17]*256)*1.24/4.095).toFixed(2) + "mV)")
			console.log("Current offsets for 1/2/4/8/16/32/64x: " + (bytes[2] + bytes[3]*256) + "/" + (bytes[4] + bytes[5]*256) + "/" + (bytes[6] + bytes[7]*256) + "/" + (bytes[8] + bytes[9]*256) + "/" + (bytes[10] + bytes[11]*256) + "/" + (bytes[12] + bytes[13]*256) + "/" + (bytes[14] + bytes[15]*256));
			break;
		}
		
		case CMD_CAP_REPORT_FREQ:
		{
			if((current_mode == MODE_CAP_MES_REQ) || (current_mode == MODE_CAP_CARAC_REQ))
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
			console.log("Voltage set to " + (bytes[2] + bytes[3]*256) + "mV, DAC value is " + (bytes[4] + bytes[5]*256))
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
			else if(current_mode == MODE_CAP_CARAC_REQ)
			{
				// Next step: set report frequency
				sendRequest(CMD_CAP_REPORT_FREQ, [capacitance_report_freq]);
			}
			else if(current_mode == MODE_CUR_CARAC_REQ)
			{
				// Next step: start current measurement mode
				sendRequest(CMD_CUR_MES_MODE, [current_ampl, current_avg]);
			}
			else if(current_mode == MODE_CUR_CALIB_REQ)
			{
				// Next step: start current measurement mode
				cur_calib_vbias = (bytes[2] + bytes[3]*256);
				cur_calib_dacv = (bytes[4] + bytes[5]*256);
				sendRequest(CMD_CUR_MES_MODE, [current_ampl, current_calib_avg]);
			}
			else if(current_mode == MODE_CAP_CALIB_REQ)
			{
				// We start capacitance measurement mode
				sendRequest(CMD_CAP_MES_START, null);
			}
			else if(current_mode == MODE_CAP_MES)
			{
				// Vbias update
				$('#measureCapacitance').css('background', 'orange');
				$("#measureCapacitance").removeAttr("disabled");
			}
			else if(current_mode == MODE_CUR_MES)
			{
				// Vbias update
				sendRequest(CMD_CUR_MES_MODE, [current_ampl, current_avg]);
				$('#measureCurrent').css('background', 'orange');
				$("#measureCurrent").removeAttr("disabled");
			}
			else if(current_mode == MODE_CUR_CARAC)
			{
				// Vbias update, ask for next sample
				sendRequest(CMD_CUR_MES_MODE, [current_ampl, current_avg]);	
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
			else if(current_mode == MODE_CAP_CARAC_REQ)
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
				if(current_mode == MODE_CAP_MES_REQ || current_mode == MODE_CAP_CALIB_REQ || current_mode == MODE_CAP_CARAC_REQ)
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
				if(current_mode == MODE_CAP_CARAC_REQ)
				{
					console.log("Capacitance characterization mode started");
					$('#capacitance').css('background', 'orange');
					$("#capacitance").removeAttr("disabled");
					current_mode = MODE_CAP_CARAC;
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
				if(current_mode == MODE_CAP_MES || current_mode == MODE_CAP_CALIB || current_mode == MODE_CAP_CARAC)
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
					capmeter.measurement._capacitance = "fF";
					sendRequest(CMD_DISABLE_VBIAS, null);
					current_mode = MODE_IDLE;		
					enable_gui_buttons();			
					ping_enabled = true;
				}
				else if(current_mode == MODE_CAP_CARAC)
				{
					console.log("Capacitance characterization mode stopped");
					$('#capacitance').css('background', '#3ED1D6');
					capmeter.measurement._capacitance = "fF";
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
			// Parse answer
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
			
			// If capacitance calibration has been done, remove offset
			if(capmeter.app.preferences.capCalibDone)
			{
				capacitance = -1*((counter_divider * aggregate_fall) / (32000000 * counter_val * resistor_val * Math.log(second_threshold/first_threshold))) - capmeter.app.preferences.capOffset;				
			}
			else
			{
				capacitance = -1*((counter_divider * aggregate_fall) / (32000000 * counter_val * resistor_val * Math.log(second_threshold/first_threshold)));				
			}
			
			// Don't allow negative capacitances
			if(capacitance < 0)
			{
				capacitance = 0;
			}
			//console.log("Measured capacitance: " + capmeter.util.valueToElectronicString(capacitance, "F") + ", osc. freq.: " + counter_val*report_freq  + "Hz (" + resistor_val + "R)");
			
			// Store value in our buffer, compute capmeter.util.average and std deviation
			cap_last_values[(cap_last_value_ind++)%cap_last_values.length] = capacitance;
			var cap_standard_deviation = capmeter.util.standardDeviation(cap_last_values);
			var cap_average = capmeter.util.average(cap_last_values);
			
			// If we switched measured value (new values a lot different than the capmeter.util.average)
			if(capacitance > cap_average*1.1 || capacitance < cap_average*0.9)
			{
				// set all values to last measured value
				//console.log("New value measured, resetting last values");
				for (var i = 0; i < cap_last_values.length; i++) cap_last_values[i] = capacitance;
				cap_average = capacitance;
				cap_last_value_ind = 0;
			}
			capmeter.measurement._capacitance = capmeter.util.valueToElectronicString(cap_average, "F") + "(" + capmeter.util.valueToElectronicString(counter_val*report_freq, "Hz") + ")";
			
			if(current_mode == MODE_CAP_CARAC)
			{
				var action = capmeter.graphing.newCapValueArrival(capacitance);		
				
				if(action[0] == "finished")
				{
					sendRequest(CMD_CAP_MES_EXIT, null);
				}
				else if(action[0] == "change_vbias")
				{					
					sendRequest(CMD_SET_VBIAS, [action[1]%256, Math.floor(action[1]/256)]);
				}
				else
				{
					// We'll receive a new measurement soon
				}
			}
			else if(current_mode == MODE_CAP_CALIB)
			{
				// Store value in array
				cap_calib_array[(cap_calib_array_ind++)%cap_calib_array.length] = capacitance;
				// Check if we have enough values to compute offset
				if(cap_calib_array_ind >= cap_calib_array.length)
				{
					cap_standard_deviation = capmeter.util.standardDeviation(cap_calib_array);
					cap_average = capmeter.util.average(cap_calib_array);
					console.log("Standard deviation: " + capmeter.util.valueToElectronicString(cap_standard_deviation, "F") + " capmeter.util.average: " + capmeter.util.valueToElectronicString(cap_average, "F"));
					// Only accept if we are within 1%
					if(cap_average * 0.01 > cap_standard_deviation)
					{
						// Store offset and exit
						enable_gui_buttons();
						sendRequest(CMD_CAP_MES_EXIT, null);
						capmeter.app.preferences.capCalibDone = true;
						capmeter.app.preferences.capOffset = cap_average - 1e-12;
						capmeter.app.preferences.capOffset = cap_average;
						console.log("Capacitance offset taken: " + capmeter.util.valueToElectronicString(capmeter.app.preferences.capOffset, "F"));
						capmeter.prefstorage.setStoredPreferences(capmeter.app.preferences);
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
				if((current_mode == MODE_CUR_MES_REQ) || (current_mode == MODE_CUR_CARAC_REQ) || (current_mode == MODE_CUR_CALIB_REQ))
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
				else if(current_mode == MODE_CUR_CARAC_REQ)
				{
					current_mode = MODE_CUR_CARAC;		
					$("#current").removeAttr("disabled");
					$('#current').css('background', 'orange');					
				}
				else if(current_mode == MODE_CUR_CALIB_REQ)
				{
					current_mode = MODE_CUR_CALIB;		
					$("#calibrateCurrentY").removeAttr("disabled");					
				}
				
				// Current we received
				var adc_current = (bytes[2] + bytes[3]*256);
				var adc_current_corrected = capmeter.currentcalib.correctAdcValue(adc_current);
				
				if(current_mode == MODE_CUR_MES)
				{
					// Start another measurement					
					var current = capmeter.util.valueToElectronicString(((adc_current_corrected*1.24)/(0.2047*(1 << current_ampl)))*1e-9, "A");
					console.log("Received ADC current measurement: " + current);
					capmeter.measurement._current = current;					
					
					// Was vbias updated?
					if(cur_mes_new_vbias_req != 0)
					{						
						sendRequest(CMD_SET_VBIAS, [cur_mes_new_vbias_req%256, Math.floor(cur_mes_new_vbias_req/256)]);	
						console.log("Requesting voltage set to " + cur_mes_new_vbias_req + "mV");
						cur_mes_new_vbias_req = 0;
					}
					else
					{
						sendRequest(CMD_CUR_MES_MODE, [current_ampl, current_avg]);			
					}
				}
				else if(current_mode == MODE_CUR_CALIB)
				{					
					if(cur_calib_state == "start")
					{
						cur_calib_state = "running";
					}
					else
					{
						capmeter.currentcalib.addMeasurement(cur_calib_dacv, cur_calib_vbias, adc_current);
					}					
					
					// Check if it was the last measurement to perform or if the adc is saturated
					if(cur_calib_dacv == 0 || max_cur_adc_val_1x == adc_current)
					{
						// Leave current measurement mode
						cur_calib_state = "completed";
						sendRequest(CMD_CUR_MES_MODE_EXIT, null);		
						$('#calibrateCurrentY').css('background', 'orange');							
					}	
					else
					{
						// If not, decrease vbias dac value
						cur_calib_dacv--;
						sendRequest(CMD_SET_VBIAS_DAC, [cur_calib_dacv%256, Math.floor(cur_calib_dacv/256), NB_MS_WAIT_VBIAS_MES%256, Math.floor(NB_MS_WAIT_VBIAS_MES/256)]);						
					}
				}
				else if(current_mode == MODE_CUR_CARAC)
				{
					// Compute current
					var current = ((adc_current_corrected*1.24)/(0.2047*(1 << current_ampl)))*1e-9;
					var current_string = capmeter.util.valueToElectronicString(current, "A");
					// console.log("Received ADC current measurement: " + current_string);
					capmeter.measurement._current = current_string;						
					
					var action = capmeter.graphing.newCurValueArrival(current);		
				
					if(action[0] == "finished")
					{
						sendRequest(CMD_CUR_MES_MODE_EXIT, null);
					}
					else if(action[0] == "change_vbias")
					{				
						//console.log(action[1]);
						sendRequest(CMD_SET_VBIAS, [action[1]%256, Math.floor(action[1]/256)]);
					}
					else if(action[0] == "continue")
					{
						// Request other sample
						sendRequest(CMD_CUR_MES_MODE, [current_ampl, current_avg]);	
					}
				}
			}
			break;
		}
		
		case CMD_CUR_MES_MODE_EXIT:
		{
			if(bytes[2] != 0 && ((current_mode == MODE_CUR_MES) || (current_mode == MODE_CUR_CARAC) || (current_mode == MODE_CUR_CALIB)))
			{				
				// If we were calibrating current, export data
				if(current_mode == MODE_CUR_CALIB)
				{
					if(cur_calib_state == "completed")
					{
						capmeter.currentcalib.stopCalib();							
					}
					capmeter.currentcalib.initCalib();					
				}	
				
				enable_gui_buttons();
				current_mode = MODE_IDLE;
				capmeter.measurement._current = "nA";
				sendRequest(CMD_DISABLE_VBIAS, null);
				$('#current').css('background', '#3ED1D6');
				$('#measureCurrent').css('background', '#3ED1D6');				
				console.log("Current measurement mode excited!");			
			}
			break;
		}
		
		case CMD_SET_VBIAS_DAC:
		{
			if(current_mode == MODE_CUR_CALIB)
			{
				// Store set vbias and measure current
				cur_calib_vbias = (bytes[2] + bytes[3]*256);
				//console.log("Vbias set for DAC val of " + cur_calib_dacv + ": " + cur_calib_vbias + "mV");
				sendRequest(CMD_CUR_MES_MODE, [current_ampl, current_calib_avg]);
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
		//console.log('No devices found');
        return;
    }

    var ind = devices.length - 1;
    var devId = devices[ind].deviceId;
    //console.log('Found ' + devices.length + ' device(s):');
	for (i=0; i < devices.length; i++)
	{
		//console.log('- device #' + devices[i].deviceId + ' vendorId ' + devices[i].vendorId + ' productId ' + devices[i].productId);
	}
    //console.log('Connecting to device #' + devId);
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
		
	if(type != CMD_BOOTLOADER_JUMP)
	{
		waitingForAnswer = true;		
	}

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
			//console.log("pouet");
			sendPing();
		}
    }
}

document.body.onload = function() 
{
	setInterval(checkConnection, 2000);
	capmeter.visualisation._datapoints = NB_GRAPH_POINTS;
	$('#resistance').attr("value", 0);
	$('#data-points').attr("value", NB_GRAPH_POINTS);
	$('#voltageCurrent').attr("value", (VBIAS_MIN)/1000);
	$('#maxVoltage').attr("value", (VBIAS_MIN-V_OSCILLATION)/1000);
	$('#voltageCapacitance').attr("value", (VBIAS_MIN-V_OSCILLATION)/1000);
	capmeter.prefstorage.getStoredPreferences(capmeter.app.preferencesStorageCallback);
	console.log("Looking for devices...")
	disable_gui_buttons();
}
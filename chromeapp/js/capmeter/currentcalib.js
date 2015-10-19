if (typeof capmeter == 'undefined') capmeter = {};
capmeter.currentcalib = capmeter.currentcalib || {};

var SMOOTHING_FACTOR = 4;
// Current resistor value
capmeter.currentcalib.cur_res = 0;
// Current amplification
capmeter.currentcalib.cur_ampl = 0;
// Minimum/Maximum vbias
capmeter.currentcalib.minVbias = 100;
capmeter.currentcalib.maxVbias = 0;
// Mapping from actual ADC value to ideal ADC val
capmeter.currentcalib.adcToIdealAdcAgg = [];
capmeter.currentcalib.adcToIdealAdcAggNbSamples = [];
capmeter.currentcalib.temp_array = new Array(2048);
capmeter.currentcalib.finalAdcToIdealAdc = new Array(2048);


// File written callback
capmeter.currentcalib.file_written_callback = function()
{
	console.log("File written");
}

// Get current from adc value and ampl
capmeter.currentcalib.getCurrentFromAdcAndAmpl = function(adc_val, current_ampl)
{
	return ((adc_val*1.24)/(0.2047*(1 << current_ampl)))*1e-9;
}

// ADC to Ideal ADC conversion
capmeter.currentcalib.correctAdcValue = function(val)
{
	if(capmeter.currentcalib.finalAdcToIdealAdc[val] == null)
	{
		return val;
	}
	else
	{
		//console.log("ADC value " + val + " corrected to " + capmeter.currentcalib.finalAdcToIdealAdc[val]);
		return capmeter.currentcalib.finalAdcToIdealAdc[val];
	}
}

// Take raw adc to ideal adc correction data, smooth it, compress it and store it to the eeprom
capmeter.currentcalib.processCorrectionDataAndStoreToEeprom = function(array)
{	
	//console.log("Raw data:");
	//console.log(array);
	array = capmeter.util.smoothArray(array, 2);
	//console.log("Smoothed:");
	//console.log(array);
	
	// Find starting & ending indexes
	var values_offset = 0;
	var min_correction_data = 0;
	var max_correction_data = 0;
	var correction_data = new Array(2048);
	
	// Store data starting & ending index
	var starting_index = 0;
	while(array[starting_index] == null){starting_index++;}
	var ending_index = array.length - 1;
	while(array[ending_index] == null){ending_index--;}
	
	// Store starting correction
	var starting_correction = array[starting_index] - starting_index;
	
	// Replace null values with a constant
	for(var i = 0; i < starting_index; i++)
	{
		correction_data[i] = 10000;
	}
	for(var i = ending_index + 1; i < correction_data.length; i++)
	{
		correction_data[i] = 10000;
	}
	
	// Loop through the known values and store correction data
	var last_correction = starting_correction;
	for(var i = starting_index; i <= ending_index; i++)
	{
		var current_correction = array[i] - i;
		correction_data[i] = current_correction - last_correction;
		if(correction_data[i] < min_correction_data)
		{
			min_correction_data = correction_data[i];
		}
		else if(correction_data[i] > max_correction_data)
		{
			max_correction_data = correction_data[i];
		}
		last_correction = current_correction;
	}
	
	// Apply offset to all values
	values_offset = 1 - min_correction_data;
	for(var i = 0; i < correction_data.length; i++)
	{
		if(correction_data[i] == 10000)
		{
			correction_data[i] = 0;
		}
		else
		{
			correction_data[i] = correction_data[i] + values_offset;
		}
	}
	
	// We encode each difference on 3 bits, check that it fits
	if(max_correction_data - min_correction_data + 1 > 7)
	{
		console.log("Can't fit data into platform EEPROM!");
		console.log("Number of different values: " + (max_correction_data - min_correction_data + 2));
	}
	else
	{
		//console.log("Correction data fits into platform EEPROM");
		// Move from 3 bits values to 8 bits array
		var final_compressed_data = new Uint8Array(2048*3/8);
		for(var i = 0; i < final_compressed_data.length; i++)
		{
			var bit_data = 0x00;
			for(var j = 0; j < 8; j++)
			{
				var buffer_index = Math.floor((i*8+j)/3);
				var bit_index_mask = (1 << ((i*8+j)%3));
				if(correction_data[buffer_index] & bit_index_mask)
				{
					bit_data |= (1 << j);
				}
			}
			final_compressed_data[i] = bit_data;
		}
		//console.log("Pre-compressed:");
		//console.log(correction_data);
		//console.log("3 bits to 8 bits:");
		//console.log(final_compressed_data);		
		
		// Store data in global vars and trigger save, then parse it again to load correction array
		capmeter.app.current_value_offset = values_offset;
		capmeter.app.raw_calibration_data = final_compressed_data;
		capmeter.app.current_starting_correction = starting_correction;
		$('#calibrateCurrentY').css('background', 'orange');	
		capmeter.app.exportAdvancedCalibDataToEeprom();
		capmeter.currentcalib.parseCurrentCorrectionData();
	}
}

// Parse current calibration data
capmeter.currentcalib.parseCurrentCorrectionData = function()
{
	var buffer = capmeter.app.raw_calibration_data;
	var values_offset = capmeter.app.current_value_offset;
	var starting_correction = capmeter.app.current_starting_correction;
	
	var decompressed_data = new Uint8Array(2048);
	for(var i = 0; i < decompressed_data.length; i++)
	{
		var bit_data = 0x00;
		for(var j = 0; j < 3; j++)
		{
			var buffer_index = Math.floor((i*3+j)/8);
			var bit_index_mask = (1 << ((i*3+j)%8));
			if(buffer[buffer_index] & bit_index_mask)
			{
				bit_data |= (1 << j);
			}
		}
		decompressed_data[i] = bit_data;
	}
	//console.log("8 bits to 3 bits:");
	//console.log(decompressed_data);
	
	var last_correction = starting_correction;
	for(var i = 0; i < decompressed_data.length; i++)
	{
		if(decompressed_data[i] == 0)
		{
			capmeter.currentcalib.finalAdcToIdealAdc[i] = null;
		}
		else
		{
			var current_correction = last_correction + (decompressed_data[i] - values_offset);
			capmeter.currentcalib.finalAdcToIdealAdc[i] = i + current_correction;
			last_correction = current_correction;
		}
	}
	//console.log("Decompressed:");
	//console.log(capmeter.currentcalib.finalAdcToIdealAdc);
	
	// Populate missing values
	var last_val = 0;
	for(var i = 1024; i >= 0; i--)
	{
		if(capmeter.currentcalib.finalAdcToIdealAdc[i] != null)
		{
			last_val = capmeter.currentcalib.finalAdcToIdealAdc[i] - i;
		}
		else
		{
			capmeter.currentcalib.finalAdcToIdealAdc[i] = i + last_val;
		}
	}
	var last_val = 0;
	for(var i = 1024; i < capmeter.currentcalib.finalAdcToIdealAdc.length; i++)
	{
		if(capmeter.currentcalib.finalAdcToIdealAdc[i] != null)
		{
			last_val = capmeter.currentcalib.finalAdcToIdealAdc[i] - i;
		}
		else
		{
			capmeter.currentcalib.finalAdcToIdealAdc[i] = i + last_val;
		}
	}
	
	//console.log("Final Correction Vector");
	//console.log(capmeter.currentcalib.finalAdcToIdealAdc);
	$('#calibrateCurrentY').css('background', 'orange');		
}

// Print calibration data on the graph
capmeter.currentcalib.printCalibData = function()
{
	var graph_xlabels = new Array(capmeter.currentcalib.finalAdcToIdealAdc.length);
	var graph_yvalues = new Array(capmeter.currentcalib.finalAdcToIdealAdc.length);
	for (var i = 0; i < capmeter.currentcalib.finalAdcToIdealAdc.length; i++)
	{
			graph_xlabels[i] = i;
			graph_yvalues[i] = capmeter.currentcalib.finalAdcToIdealAdc[i] - i;
	}
	capmeter.graph.changeUnit("LSB");
	capmeter.graph.changeYLabel("LSB error");
	capmeter.graph.changeXLabels(graph_xlabels);
	capmeter.graph.changeYValues(graph_yvalues);		
}

// Export correction data
capmeter.currentcalib.exportAdcCorrec = function()
{	
	// Export data FYI?
	var export_csv = "ADC Val,Mapped ADC Val,Difference\r\n";
	for(var i = 0; i < capmeter.currentcalib.finalAdcToIdealAdc.length; i++)
	{
		export_csv += i + "," + capmeter.currentcalib.finalAdcToIdealAdc[i] + "," + (capmeter.currentcalib.finalAdcToIdealAdc[i]-i) + "\r\n";
	}
	capmeter.filehandler.selectAndSaveFileContents("export.txt", new Blob([export_csv], {type: 'text/plain'}), capmeter.currentcalib.file_written_callback);
}

// Start calibration procedure
capmeter.currentcalib.startCalib = function(resistor_val, current_ampl)
{
	capmeter.currentcalib.adcToIdealAdcAggNbSamples = new Array(2048);
	capmeter.currentcalib.adcToIdealAdcAgg = new Array(2048);
	capmeter.currentcalib.cur_ampl = current_ampl;
	capmeter.currentcalib.cur_res = resistor_val;
	capmeter.currentcalib.minVbias = 100000;
	capmeter.currentcalib.maxVbias = 0;
}

// End calibration procedure
capmeter.currentcalib.stopCalib = function()
{
	// Temp vars
	var ideal_current_na;
	var ideal_adc_value;
	var min_adc_value;
	var max_adc_value;
	
	// Min/Max ADC value
	var i = 0;
	do
	{
		if(capmeter.currentcalib.adcToIdealAdcAgg[i])
		{
			min_adc_value = i;
		}
	}
	while(!capmeter.currentcalib.adcToIdealAdcAgg[i++])
	i = capmeter.currentcalib.adcToIdealAdcAgg.length - 1;
	do
	{
		if(capmeter.currentcalib.adcToIdealAdcAgg[i])
		{
			max_adc_value = i;
		}
	}
	while(!capmeter.currentcalib.adcToIdealAdcAgg[i--])
	console.log("Min ADC val: " + min_adc_value + ", max ADC val: " + max_adc_value);

	// Compute mapping
	for(i = min_adc_value; i <= max_adc_value; i++)
	{
		// Check if we actually have a mapping
		if(capmeter.currentcalib.adcToIdealAdcAgg[i])
		{
			capmeter.currentcalib.temp_array[i] = Math.round(capmeter.currentcalib.adcToIdealAdcAgg[i] / capmeter.currentcalib.adcToIdealAdcAggNbSamples[i]);	
		}
		else
		{
			capmeter.currentcalib.temp_array[i] = capmeter.currentcalib.temp_array[i-1];
		}
		//console.log(i + " mapped to " + capmeter.app.preferences.adcMapping[i]);			
	}
	
	// Check if we're calibrated
	var current_calibrated = true;	
	for(var i = 30; i < capmeter.currentcalib.temp_array.length - 50; i++)
	{
		if(capmeter.currentcalib.temp_array[i] == null)
		{
			current_calibrated = false;
		}
	}
	if(current_calibrated)
	{
		console.log("Current Calibration Done, Storing Data in EEPROM");
		capmeter.currentcalib.processCorrectionDataAndStoreToEeprom(capmeter.currentcalib.temp_array);
	}
	else
	{
		$('#calibrateCurrentY').css('background', '#3ED1D6');
	}
}

// Add a new measurement
capmeter.currentcalib.addMeasurement = function(vbias_dacval, vbias, adc_cur)
{
	// Compute (ideal) current
	var measured_current = capmeter.currentcalib.getCurrentFromAdcAndAmpl(adc_cur, capmeter.currentcalib.cur_ampl);
	var ideal_current_a = (vbias / capmeter.currentcalib.cur_res) * 1e-3;
	var ideal_current_na = (vbias / capmeter.currentcalib.cur_res) * 1e6;
	var ideal_adc_value = Math.round(ideal_current_na * 0.2047 / 1.24);	
	
	console.log("DAC: " + vbias_dacval + ", Vbias: " + cur_calib_vbias + "mV, ADC: " + adc_cur + " (should be " + ideal_adc_value + "), current: " + capmeter.util.valueToElectronicString(measured_current, "A") + " (should be " + capmeter.util.valueToElectronicString(ideal_current_a, "A") + ")");
	
	// Store values
	if(!capmeter.currentcalib.adcToIdealAdcAgg[adc_cur])
	{
		capmeter.currentcalib.adcToIdealAdcAgg[adc_cur] = 0;
	}
	if(!capmeter.currentcalib.adcToIdealAdcAggNbSamples[adc_cur])
	{
		capmeter.currentcalib.adcToIdealAdcAggNbSamples[adc_cur] = 0;
	}
	capmeter.currentcalib.adcToIdealAdcAgg[adc_cur] += ideal_adc_value;
	capmeter.currentcalib.adcToIdealAdcAggNbSamples[adc_cur]++;
	
	// Store Min/Max Vbias
	if(capmeter.currentcalib.minVbias > vbias)
	{
		capmeter.currentcalib.minVbias = vbias;
	}
	if(capmeter.currentcalib.maxVbias < vbias)
	{
		capmeter.currentcalib.maxVbias = vbias;
	}
}











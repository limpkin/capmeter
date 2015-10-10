var capmeter = capmeter || {};
capmeter.graphing = capmeter.graphing || {};

capmeter.graphing.cap_graph_xlabels = [0];								// Graph X labels
capmeter.graphing.cap_graph_xvalues = [0];								// Graph X values
capmeter.graphing.cap_graph_yvalues = [0];								// Graph Y values
capmeter.graphing.cap_graph_store_index = 0;							// Where to store the next graph value
capmeter.graphing.cap_graph_current_interval = 0;						// Voltage interval for the graph
capmeter.graphing.cap_graph_cap_values = [0,0,0,0,0,0,0,0,0,0];			// Last capacitance values for a given vbias
capmeter.graphing.cap_graph_cap_buffer_store_ind = 0;					// Where to store the last value in our buffer


capmeter.graphing.newCapValueArrival = function(capacitance)
{
	var return_value;
	
	// Store capacitance in buffer to do avg
	capmeter.graphing.cap_graph_cap_values[(capmeter.graphing.cap_graph_cap_buffer_store_ind++)%capmeter.graphing.cap_graph_cap_values.length] = capacitance;
	var cap_standard_deviation = capmeter.util.standardDeviation(capmeter.graphing.cap_graph_cap_values);
	var cap_average = capmeter.util.average(capmeter.graphing.cap_graph_cap_values);
	
	// Check if buffer is filled and std is correct
	if(capmeter.graphing.cap_graph_cap_buffer_store_ind >= capmeter.graphing.cap_graph_cap_values.length)
	{
		// Only accept if we are within 1%
		if(cap_average * 0.01 > cap_standard_deviation)
		{
			capmeter.graphing.cap_graph_cap_buffer_store_ind = 0;
			
			// Store capmeter.util.average in array
			capmeter.graphing.cap_graph_yvalues[capmeter.graphing.cap_graph_store_index++] = cap_average;
			
			// Check if we finished running through the vbias points
			if(capmeter.graphing.cap_graph_store_index == capmeter.graphing.cap_graph_xvalues.length)
			{
				// Leave capacitance measurement mode
				return_value = ["finished", 0];
				
				// Update our graph, find lowest value
				var lowest_cap = Math.min.apply(null, capmeter.graphing.cap_graph_yvalues);
				
				// Update our min graph unit and our vector
				if(lowest_cap < 1e-12)
				{
					capmeter.graph.changeUnit("fF");
					for(var i = 0; i < capmeter.graphing.cap_graph_yvalues.length; i++)
					{
						capmeter.graphing.cap_graph_yvalues[i] = capmeter.graphing.cap_graph_yvalues[i] * 1e15;
					}
				}
				else if(lowest_cap < 1e-9)
				{
					capmeter.graph.changeUnit("pF");
					for(var i = 0; i < capmeter.graphing.cap_graph_yvalues.length; i++)
					{
						capmeter.graphing.cap_graph_yvalues[i] = capmeter.graphing.cap_graph_yvalues[i] * 1e12;
					}
				}
				else if(lowest_cap < 1e-6)
				{
					capmeter.graph.changeUnit("nF");
					for(var i = 0; i < capmeter.graphing.cap_graph_yvalues.length; i++)
					{
						capmeter.graphing.cap_graph_yvalues[i] = capmeter.graphing.cap_graph_yvalues[i] * 1e9;
					}
				}
				else if(lowest_cap < 1e-3)
				{
					capmeter.graph.changeUnit("uF");
					for(var i = 0; i < capmeter.graphing.cap_graph_yvalues.length; i++)
					{
						capmeter.graphing.cap_graph_yvalues[i] = capmeter.graphing.cap_graph_yvalues[i] * 1e6;
					}
				}
				else if(lowest_cap < 1)
				{
					capmeter.graph.changeUnit("mF");
					for(var i = 0; i < capmeter.graphing.cap_graph_yvalues.length; i++)
					{
						capmeter.graphing.cap_graph_yvalues[i] = capmeter.graphing.cap_graph_yvalues[i] * 1e3;
					}
				}					
				
				capmeter.graph.changeYValues(capmeter.graphing.cap_graph_yvalues);
			}
			else
			{
				// Move to the next voltage
				//console.log("Requesting voltage set to " + cap_graph_xvalues[cap_graph_store_index] + "mV");
				return_value = ["change_vbias", capmeter.graphing.cap_graph_xvalues[capmeter.graphing.cap_graph_store_index]];
			}
		}
		else
		{
			console.log("Buffer full but Standard deviation: " + capmeter.util.valueToElectronicString(cap_standard_deviation, "F") + " capmeter.util.average: " + capmeter.util.valueToElectronicString(cap_average, "F"));
			return_value = ["continue", 0];
		}
	}
	else
	{
		return_value = ["continue", 0];
	}
	
	return return_value;
}

capmeter.graphing.initCapMeasGraphing = function(max_voltage, nb_points, averaging)
{	
	// Compute intervals and init graphs
	capmeter.graphing.cap_graph_current_interval = (max_voltage - VBIAS_MIN) / (nb_points - 1);
	capmeter.graphing.cap_graph_cap_values = new Array(averaging);
	capmeter.graphing.cap_graph_xlabels = new Array(nb_points);
	capmeter.graphing.cap_graph_xvalues = new Array(nb_points);
	capmeter.graphing.cap_graph_yvalues = new Array(nb_points);
	capmeter.graphing.cap_graph_cap_buffer_store_ind = 0;
	capmeter.graphing.cap_graph_store_index = 0;
	for (var i = 0; i < nb_points; i++)
	{
			capmeter.graphing.cap_graph_xlabels[i] = ((VBIAS_MIN - V_OSCILLATION + capmeter.graphing.cap_graph_current_interval * i)/1000).toFixed(2) + "V";
			capmeter.graphing.cap_graph_xvalues[i] = (VBIAS_MIN + capmeter.graphing.cap_graph_current_interval * i);
			capmeter.graphing.cap_graph_yvalues[i] = 0;
	}
	capmeter.graph.changeUnit("F");
	capmeter.graph.changeYLabel("capacitance");
	capmeter.graph.changeXLabels(capmeter.graphing.cap_graph_xlabels);
	capmeter.graph.changeYValues(capmeter.graphing.cap_graph_yvalues);
}

capmeter.graphing.initCurMeasGraphing = function(max_voltage, nb_points, averaging)
{	
	// Compute intervals and init graphs
	capmeter.graphing.cap_graph_current_interval = (max_voltage - VBIAS_MIN) / (nb_points - 1);
	capmeter.graphing.cap_graph_cap_values = new Array(averaging);
	capmeter.graphing.cap_graph_xlabels = new Array(nb_points);
	capmeter.graphing.cap_graph_xvalues = new Array(nb_points);
	capmeter.graphing.cap_graph_yvalues = new Array(nb_points);
	capmeter.graphing.cap_graph_cap_buffer_store_ind = 0;
	capmeter.graphing.cap_graph_store_index = 0;
	for (var i = 0; i < nb_points; i++)
	{
			capmeter.graphing.cap_graph_xlabels[i] = ((VBIAS_MIN + capmeter.graphing.cap_graph_current_interval * i)/1000).toFixed(2) + "V";
			capmeter.graphing.cap_graph_xvalues[i] = (VBIAS_MIN + capmeter.graphing.cap_graph_current_interval * i);
			capmeter.graphing.cap_graph_yvalues[i] = 0;
	}
	capmeter.graph.changeUnit("A");
	capmeter.graph.changeYLabel("current");
	capmeter.graph.changeXLabels(capmeter.graphing.cap_graph_xlabels);
	capmeter.graph.changeYValues(capmeter.graphing.cap_graph_yvalues);
}
var capmeter = capmeter || {};
capmeter.util = capmeter.util || {};

capmeter.util.average = function(data)
{
	var sum = data.reduce(function(sum, value){
		return sum + value;
	}, 0);

	var avg = sum / data.length;
	return avg;
}

capmeter.util.standardDeviation = function(values)
{
	var avg = capmeter.util.average(values);

	var squareDiffs = values.map(function(value){
		var diff = value - avg;
		var sqrDiff = diff * diff;
		return sqrDiff;
	});

	var avgSquareDiff = capmeter.util.average(squareDiffs);

	var stdDev = Math.sqrt(avgSquareDiff);
	return stdDev;
}

capmeter.util.smoothArray = function(array, avg_factor)
{
	var return_val = array.slice(0);
	
	// Find starting & ending indexes
	var starting_index = 0;
	while(array[starting_index] == null){starting_index++;}
	var ending_index = array.length - 1;
	while(array[ending_index] == null){ending_index--;}
	
	// Smooth our vector
	for(var index = 0; index < array.length; index++)
	{
		if(array[index] != null)
		{
			var nb_elements_before = index - starting_index;
			var nb_elements_after = ending_index - index;
			if(nb_elements_before < avg_factor)
			{
				var agg = 0;
				for(var i = starting_index; i <= index + nb_elements_before; i++)
				{
					agg += array[i];
				}
				return_val[index] = Math.round(agg/(index + nb_elements_before + 1 - starting_index));
			}
			else if(nb_elements_after < avg_factor)
			{
				var agg = 0;
				for(var i = index - nb_elements_after; i <= ending_index; i++)
				{
					agg += array[i];
				}
				return_val[index] = Math.round(agg/(ending_index - index + nb_elements_after + 1));	
			}
			else
			{
				var agg = 0;
				for(var i = index - avg_factor; i <= index + avg_factor; i++)
				{
					agg += array[i];
				}
				return_val[index] = Math.round(agg/(avg_factor*2 + 1));	
			}			
		}
	}
	
	return return_val.slice(0);
}

/**
 * convert a string to a uint8 array
 * @param str the string to convert
 * @returns the uint8 array representing the string with a null terminator
 * @note does not support unicode yet
 */
capmeter.util.strToArray = function(str)
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
capmeter.util.arrayToStr = function(buf)
{
    res = '';
    for (var ind=0; ind<buf.length; ind++)
    {
        if (buf[ind] == 0)
        {
            return res;
        } 
		else 
		{
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
capmeter.util.valueToElectronicString = function(value, unit)
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
		return value.toFixed(1) + unit;
	}
}
from datetime import datetime
from array import array
from time import sleep
import platform
import os.path
import random
import struct
import string
import pickle
import serial
import copy
import time
import csv
import sys
import os

NB_DAC_VALS = 4096

def get_and_parse_fluke_answer(fluke_serial_port):	
	try:
		fluke_serial.write("VAL?\r\n")
		response = fluke_serial.readline()
		response2 = fluke_serial.readline()
		return float(response)		
	except Exception, e1:
		print "error communicating...: " + str(e1)
		
def get_averaged_value_from_fluke(fluke_serial_port, nb_vals):
	total = 0
	for i in range(0, nb_vals):
		total += get_and_parse_fluke_answer(fluke_serial_port)
		time.sleep(0.2)
	return total/nb_vals

if __name__ == '__main__':
	# Main function
	fluke_serial = serial.Serial()
	fluke_serial.port = "COM11"
	fluke_serial.baudrate = 9600
	fluke_serial.bytesize = serial.EIGHTBITS
	fluke_serial.parity = serial.PARITY_NONE
	fluke_serial.stopbits = serial.STOPBITS_ONE
	#fluke_serial.timeout = 0						#non-block read
	fluke_serial.timeout = 50						#timeout block read
	fluke_serial.xonxoff = False					#disable software flow control
	fluke_serial.rtscts = False						#disable hardware (RTS/CTS) flow control
	fluke_serial.dsrdtr = False						#disable hardware (DSR/DTR) flow control
	fluke_serial.writeTimeout = 2					#timeout for write
	
	platform_serial = serial.Serial()
	platform_serial.port = "COM3"
	platform_serial.baudrate = 115200
	platform_serial.bytesize = serial.EIGHTBITS
	platform_serial.parity = serial.PARITY_NONE
	platform_serial.stopbits = serial.STOPBITS_ONE
	#platform_serial.timeout = 0					#non-block read
	platform_serial.timeout = 50					#timeout block read
	platform_serial.xonxoff = False					#disable software flow control
	platform_serial.rtscts = False					#disable hardware (RTS/CTS) flow control
	platform_serial.dsrdtr = False					#disable hardware (DSR/DTR) flow control
	platform_serial.writeTimeout = 2				#timeout for write
	
	set_voltages = [0]*NB_DAC_VALS
	measured_curents = [0]*NB_DAC_VALS

	try: 
		fluke_serial.open()
	except Exception, e:
		print "error open serial port: " + str(e)
		exit()

	try: 
		platform_serial.open()
	except Exception, e:
		print "error open serial port: " + str(e)
		exit()

	if fluke_serial.isOpen() and platform_serial.isOpen():
		try:
			fluke_serial.flushInput() 		#flush input buffer, discarding all its contents
			fluke_serial.flushOutput()		#flush output buffer, aborting current output 
			platform_serial.flushInput() 	#flush input buffer, discarding all its contents
			platform_serial.flushOutput()	#flush output buffer, aborting current output 

			counter = 0
			time_start = datetime.now()
			platform_serial.write("!")
			while counter != 1:
				dac_value = int(platform_serial.readline())
				cur_measurement = int(platform_serial.readline())
				time.sleep(0.5)
				voltage_measured = get_averaged_value_from_fluke(fluke_serial, 10)
				#check that we didn't loop
				if dac_value <= 2 or cur_measurement == 2047:
					counter = counter + 1
				print "DAC: " + repr(dac_value) + ", current: " + repr(cur_measurement) + ", fluke: " + repr(int(voltage_measured*1000))
				set_voltages[dac_value] = int(voltage_measured*1000)
				measured_curents[dac_value] = cur_measurement
				platform_serial.write("a")
				
			#time
			time_end = datetime.now()
			time_diff = time_end - time_start
			print time_diff
			
			#export data
			csvexport = csv.writer(open("export.txt", "wb"), quoting=csv.QUOTE_NONNUMERIC)
			for i in range(0, NB_DAC_VALS):
				if measured_curents[i] != 0:
					csvexport.writerow([repr(i), repr(set_voltages[i]), repr(measured_curents[i])])	
			
			fluke_serial.close()
			platform_serial.close()
		except Exception, e1:
			print "error communicating...: " + str(e1)
	else:
		print "cannot open serial ports"
		
		
		
		
		
		
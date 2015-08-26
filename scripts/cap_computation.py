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
import math
import copy
import time
import csv
import sys
import os

VOLTAGE_MIN = 700
NB_VOLTAGE_VALS = 16000

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
		time.sleep(0.3)
	return total/nb_vals

if __name__ == '__main__':
	# Main function	
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

	try: 
		platform_serial.open()
	except Exception, e:
		print "error open serial port: " + str(e)
		exit()

	if platform_serial.isOpen():
		try:
			platform_serial.flushInput() 	#flush input buffer, discarding all its contents
			platform_serial.flushOutput()	#flush output buffer, aborting current output 

			state_machine = 0
			nb_zero_samples = 0
			offset_total = 0
			while True:
				text = platform_serial.readline()
				if state_machine == 0:
					if text == "SYNC\r\n":
						state_machine += 1
				else:
					if state_machine == 1:
						counter_divider = float(text)
						state_machine += 1
					elif state_machine == 2:
						agg_fall = float(text)
						state_machine += 1
					elif state_machine == 3:
						freq_counter = float(text)
						state_machine += 1
					elif state_machine == 4:
						half_res = float(text)
						state_machine += 1
					elif state_machine == 5:
						second_thres = float(text)
						state_machine += 1
					elif state_machine == 6:
						state_machine += 1
						first_thres = float(text)
						thres_div = second_thres / first_thres
					elif state_machine == 7:
						freq_meas = float(text)		
						capacitance = -1 * counter_divider * agg_fall / (32000000 * freq_counter * 2 * half_res * math.log(thres_div))
						if nb_zero_samples < 5:
							nb_zero_samples += 1
							offset_total += capacitance
						else:
							print "Measurement Freq: " + repr(freq_counter*freq_meas) + "Hz, Cap: " + repr(capacitance - offset_total/5)
						state_machine = 0			
			platform_serial.close()
		except Exception, e1:
			print "error communicating...: " + str(e1)
		finally:
			platform_serial.close()
	else:
		print "cannot open serial ports"
		
		
		
		
		
		
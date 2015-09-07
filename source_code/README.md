The Capmeter will enumerate as an HID communication device.

Data sent over the generic hid is made out of a 64 byte packet. The structure of the packet is as follows:

buffer[0] = length of data

buffer[1] = cmd identifier for this packet

buffer[2 - 2 + buffer[0]] = packet data

Current commands
================
Every sent packet will get one or more packets as an answer. Byte order is little endian.
Texts sent to and from the Capmeter have a payload length that includes the terminating 0.
The following commands are currently implemented:

0x00: send debug message
------------------------
From Capmeter: packet data containing the debug message.

0x01: ping
----------
From Plugin/app: ping packet

From Capmeter: same packet that the plugin/app sent

0x02: version request
---------------------
From Plugin/app: Capmeter version request

From Capmeter: String identifying the version

0x03: open ended calibration state
---------------------------------- 
From Plugin/app: Capmeter open ended calibration state request

From Capmeter: 1 byte 0 value if not calibrated, calibration data otherwise.

0x04: start open ended calibration
----------------------------------
From Plugin/app: Capmeter open ended calibration start request. First three bytes in the payload are DD MM YY

From Capmeter: The calibration data (see oe_calib_data_t) on success, 0 otherwise

0x05: Get open ended calibration data
-------------------------------------
From Plugin/app: Capmeter open ended calibration data request

From Capmeter: The calibration data (see oe_calib_data_t)

0x06: Enable bias voltage
-------------------------
From Plugin/app: Enable bias voltage, first 2 bytes are the voltage to be set

From Capmeter: The voltage actually set in mV

0x07: Disable bias voltage
--------------------------
From Plugin/app: Disable bias voltage

From Capmeter: Empty packet

0x08: Enable current measurement mode
-------------------------------------
From Plugin/app: Enable current measurement mode, first byte is amplification bit shift (0 is 1x, 1 is 2x, 2 is 4x....), second byte is the averaging in bit shift

From Capmeter: 0 on error, the averaged ADC value otherwise

0x09: Disable current measurement mode
-------------------------------------
From Plugin/app: Disable current measurement mode

From Capmeter: 0 on error, 1 on success

0x0A: Set capacitance measurement frequency
-------------------------------------------
From Plugin/app: Set the frequency at which counter values will be returned, in a bit shift format (0 is 1Hz, 1 is 2Hz, 2 is 4Hz...).

From Capmeter: 0 on error, 1 on success

0x0B: Start Capacitance Measurement Mode
----------------------------------------
From Plugin/app: Start capacitance measurement mode

From Capmeter: 0 on error, 1 on success

0x0C: Capacitance Measurement Report
------------------------------------
From Plugin/app: -

From Capmeter: see capacitance_report_t

0x0D: Stop Capacitance Measurement Mode
---------------------------------------
From Plugin/app: Stop capacitance measurement mode

From Capmeter: 0 on error, 1 on success



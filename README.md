The avr_tpi is used to program Microchip (formerly Atmel) attiny4/5/9/10 microcontrollers via the Tiny Programming Interface (TPI) interfaces since they do not support the standard isp interface.  

Important
---------
FTDI programs for Linux need to be run as root, so you'll have to use sudo (sudo avr_tpi) or use chown to allow user access to the ftdi device

Current Status
--------------
1. Enables TPI Access
2. Enables External Program Acces
3. Reads/Writes TPI with idle, start, parity and stop bit checking/generation
4. Reads/writes control and status register
5. Reads device ID
6. Disables External Program Access
7. Disables TPI Access

To Do
-----
1. Read/Write flash
2. Read commandline Args
3. Read Hex File
4. Program Flash from hex file
5. Dump flash to hex file
6. Compare flash to hex file
7. Clean up debug code. etc.


Details
-------

A few points of interest:
1. The Tiny Programming Interface (TPI) requires three signals:
   /RESET: Tiny Programming Interface enable input to device and output from programmer.
   TPICLK: Tiny Programming Interface clock input to device and output from programmer.
   TPIDATA: Tiny Programming Interface data input/output to device and output/input from the programmer.
2. Attiny devices need 5V power for programming even though the devices can run 1.8V to 5.5V.
   One caviot, if the RSTDISBL has been set, 12V is required on the /reset line. No plans to support this in this programmer solution.
3. The TPI interface is not similar to SPI nor I2C so a bitbang solution is required.
4. For debugging the programmer code, I've added 4th signal TPITST.  TPITST can be used to generate output to test read functions

The application environment will be linux (ubuntu and raspbian) with a ft232 usb to serial bridge used in bitbang mode.

/*      Signal Hookup to device
* FT232RL             Attiny4/5/9/10 (SOT-23 Pins)
*      TX-------------TPICLK (3)
*      RX-------------/RESET (6)
*     CTS-------------TPIDAT (1), Data can be read/written on this signal
* 
*/



/*      Signal Hookup for programmer code debuging
* FT232RL              Breadboard
*      TX--------------(TPICLK LED) |>|--resistor--GND
*      RX--------------(TPIRST LED) |>|--resistor--GND
*     CTS------------+-(TPIDAT LED) |>|--resistor--GND
*                    |  
*     RTS-+-resistor-+ 
*         |
*         +-resistor---(TPITST LED) |>|--resistor--GND 
*
*   resistor - I've been testing with 550 Ohms, but 1k should work too.
*/

I used the ft232 usb serial bridge is the now retired ProtoBoard - Diprotodon (USB+Mix) since it offers 5v logic levels and bitbang mode
https://www.sparkfun.com/products/retired/8723


Inspiration and References
--------------------------
The inspiration for this approach came from Phil Burgess' "Introduction to FTDI bitbang mode" post on hackaday: http://hackaday.com/2009/09/22/introduction-to-ftdi-bitbang-mode/ Many thanks for the tutorial and code examples.

The FTDI API documentation is available at https://www.intra2net.com/en/developer/libftdi/documentation/group__libftdi.html
Using libftdi (sudo apt-get install libftdi-dev) 
	 - FTDI programs for Linux need to be run as root, so you'll have to use sudo (sudo avr_tpi) or use chown to allow user access to the ftdi device

cnlohr/pi_tpi GitHub project was a great help https://github.com/cnlohr/pi_tpi.   That project also references An Article about someone else doing something like this 6 years ago: https://hackaday.com/2012/08/23/programming-the-attiny10-with-an-arduino/ along with notes on initializing TPI mode: https://pcm1723.hateblo.jp/entry/20111208/1323351725

I may try to emulate some of the applicable command line arguments in the avrosp AVR911: AVR Open-source Programmer for tinyAVR and megaAVR devices: https://www.microchip.com/wwwAppNotes/AppNotes.aspx?appnote=en591218

Implementation Status |Parameter	|Description
 ---|---------------|-----------
tbd|-d\<name\>	|Device name. Must be applied when programming the device.
-if\<infile\>	|Name of Flash input file. Required for programming or verification of the Flash memory. The file format is Intel Extended HEX. 
-ie\<infile\>	|Name of EEPROM input file. Required for programming or verification of the EEPROM memory. The file format is Intel Extended HEX. 
-of\<outfile\>	|Name of Flash output file. Required for readout of the Flash memory. The file format is Intel Extended HEX. 
-oe\<outfile\>	|Name of EEPROM output file. Required for readout of the EEPROM memory. The file format is Intel Extended HEX. 
-s	|Read signature bytes. 
-O\<addr\>	|Read oscillator calibration byte from device. addr is optional. 
-O#\<value\>	|User-defined oscillator calibration value. Use this to provide a custom calibration value instead of reading it from the device with –O<addr>. 
-Sf\<addr\>	|Write oscillator calibration byte to Flash memory. addr is byte address. 
-Se\<addr\>	|Write oscillator calibration byte to EEPROM memory. addr is byte address. 
-e	|Erase device. The device will be erased before any other programming takes place. 
-p\<t\>	|Program device. Set t to f for Flash, e for EEPROM or b for both. Corresponding input files are required. 
-r\<t\>	|Read out device. Set t to f for Flash, e for EEPROM or b for both. Corresponding output files are required. 
-v\<t\>	|Verify device. Set t to f for Flash, e for EEPROM or b for both. Can be used with –p<t> or alone. Corresponding input files are required. 
-l\<value\>	|Set lock byte. value is an 8-bit hex value. 
-L\<value\>	|Verify lock byte. value is an 8-bit hex value to verify against. 
-y	|Read back lock byte. 
-f\<value\>	|Set fuse bytes. value is a 16-bit hex value describing the settings for the upper and lower fuse bytes. 
-E\<value\>	|Set extended fuse byte. value is an 8-bit hex value describing the extend fuse settings. 
-F\<value\>	|Verify fuse bytes. value is a 16-bit hex value to verify against. 
-G\<value\>	|Verify extended fuse byte. value is an 8-bit hex value to verify against. 
-q	|Read back fuse bytes. 
-x\<value\>	|Fill unspecified locations with a value (00-FF). The default is to not program locations not specified in the input files. 
-af\<start\>,\<stop\>	|Flash address range. Specifies the address range of operations. The default is the entire Flash. Byte addresses in hex. 
-ae\<start\>,\<stop\>	|EEPROM address range. Specifies the address range of operations. The default is the entire EEPROM. Byte addresses in hex.  
-g	|Silent operation. No output to screen. 
-z	|No progress indicator. E.g. if piping to a file for log purposes, use this option to avoid the characters used for the indicator. 
-Y\<addr\>	|Used for internal RC oscillator calibration of ATtiny4/5/9/10/20/40 devices. Refer application note AVR057 
-h	|Help information (overrides all other settings). 
-?	|Same as –h

Example command line:
avr_tpi –d ATtiny9 –pf –vf –if program.hex –e 
The above example will first erase the entire memory contents and then program and verify the data contained in 
program.hex to an attached Atmel ATtiny9 device. 

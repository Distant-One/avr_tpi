/** 
* @file avr_tpi.c
* @brief TPI Programmer for Attiny4/5/9/10
* @details TPI prgrammer for Microchip (formerly Atmel)AVR ATTINY4/5/9/10 Microporcessorsi using linux/raspian and ft232r bitbang mode.
* These parts can only be programmed through the TPI interface which is not compatible with the typical ISP interface for other AVR devices. 
*
* @version 0001
* @date Thu 15 Nov 2018 10:40:37 PM EST
* @author Distant-One
* @copyright The GNU General Public License
* 
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
* 
* This software is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*	This source code calls functions from libftdi which licensed under GPL.
*	All libftdi source code and executable code are subject to the libftdi license
*	The libftdi license can be found on their website:  @see http://www.intra2net.com/en/developer/libftdi/
*
*	The inspiration for this approach came from Phil Burgess' "Introduction to FTDI bitbang mode"
*	post on hackaday: @see http://hackaday.com/2009/09/22/introduction-to-ftdi-bitbang-mode/
*	Many thanks for the tutorial and code examples.
*
*	FTDI Application Note Application Note AN_232R-01 Bit Bang Mode Availability for the FT232R and FT245R can be found on their website: i
*	(pdf warning) @see https://www.ftdichip.com/Support/Documents/AppNotes/AN_232R-01_Bit_Bang_Mode_Available_For_FT232R_and_Ft245R.pdf
*
*	@note Using libftdi (sudo apt-get install libftdi-dev) 
*	 - FTDI programs for Linux need to be run as root, so you'll have to use sudo (sudo avr_tpi.out)
*	@par
*	
*
*	@note compiler string:
*	 - cc avr_tpi.c -lftdi -o avr_tpi.out
*	@par	
*	
*	@note Signal Assignments BASED ON ftdi ttl-232 Cable @par
*     	PIN	WIRE	NAME	SPI	TPI
*	1.	black	gnd	gnd	gnd
*	2.	brown	cts	miso	n/a	
*	3.	red	5v	5v	5v
*	4.	orange	txd	sck	tpiclk
*	5.	yellow	rxd	n/a	/reset
*	6.	green	rts	mosi	tpidata
*	@par
*
*/

/* --- include files --- */

/* --- defines and constants --- */

/* --- global variables --- */

/* --- function prototypes --- */

/** @brief Sequence for enabling the Tiny Programming Interface */
int tpi_enable_programming_cmd();
/** @brief Sequence for writing a byte on the TPI interface */
int tpi_write_frame(unsigned char *writebyte);
/** @brief Sequence for reading a byte on the TPI interface */
int tpi_read_frame(unsigned char *readbyte);
int tpi_write_break();
int tpi_sld_pri_cmd(unsigned char *readbyte);
int tpi_sld_prip_cmd(unsigned char *readbyte);
int tpi_sst_pri_cmd(unsigned char *writebyte);
int tpi_sst_pprip_cmd(unsigned char *writebyte);
int tpi_sstpr_cmd(unsigned char *pointerbyte);
int tpi_sin_cmd(unsigbed char *readbyte);
int tpi_sout_cmd(unsigbed char *writebyte);
int tpi_sldcs_cmd(unsigbed char *readbyte);
int tpi_sstcs_cmd(unsigbed char *writebyte);
int tpi_skey_cmd(unsigbed char *nvm_program_enable_key, unsigned int key_size);


/* --- main --- */

/* --- functions --- */

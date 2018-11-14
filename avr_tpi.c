/** 
* @file avr_tpi.c
* @brief TPI Programmer for ATTINT4/5/9/10
* @details TPI prgrammer for Microchip (formerly Atmel)AVR ATTINY4/5/9/10 Microporcessorsi using linux/raspian and ft232r bitbang mode.
* These parts can only be programmed through the TPI interface which is not compatible with the typical ISP interface for other AVR devices. 
*
* @version 0001
* @date Tue 13 Nov 2018 11:16:07 PM EST
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
*  @attribution
*	This source code calls functions from libftdi which licensed under GPL.
*	All libftdi source code and executable code are subject to the libftdi license
*	The libftdi license can be found on their website:  @see http://www.intra2net.com/en/developer/libftdi/
*
*	The inspiration for this approach came from Phil Burgess' "Introduction to FTDI bitbang mode"
*	post on hackaday: @see http://hackaday.com/2009/09/22/introduction-to-ftdi-bitbang-mode/
*	Many thanks for the tutorial and code examples.
*
*	FTDI Application Note Application Note AN_232R-01 Bit Bang Mode Availability for the FT232R and FT245R can be found on their website: @see https://www.ftdichip.com/Support/Documents/AppNotes/AN_232R-01_Bit_Bang_Mode_Available_For_FT232R_and_Ft245R.pdf
*/

/* --- include files --- */

/* --- defines and constants --- */

/* --- global variables --- */

/* --- function prototypes --- */
/** @brief Sequence for enabling the Tiny Programming Interface */
int tpi_enable_programming_cmd();
/**< none no parameters at this tim */
/**< @param[none] none no parameters at this time */
int tpi_write_frame(unsigned char *writebyte);
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

/** 
* @file avr_tpi.c
* @brief TPI Programmer for ATTINT4/5/9/10
* @details Microchip (formerly Atmel)AVR ATTINY4/5/9/10 Microporcessors can only be programmed through the TPI interface which is not compatible with the typical ISP interface for other AVR devices. 
*
* The application is compatible with ubuntu/raspian and ft232r usb serial bridge using bitbang mode 
* @version 0001
* @date Tue 13 Nov 2018 10:29:42 PM EST
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
*/

/* --- include files --- */

/* --- defines and constants --- */

/* --- global variables --- */

/* --- function prototypes --- */
int tpi_enable_programming_cmd();
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

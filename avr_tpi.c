/** 
* @file avr_tpi.c
* @brief
* @details 
* @version
* @date Thu 13 Dec 2018 04:04:20 PM EST
* @author 
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
*
* @note compiler string: cc ftdi_tpi.c avr_tpi.c -lftdi -o avr_tpi
*
*/

/* --- include files --- */
#include <stdio.h>
#ifndef S_SPLINT_S
#include <ftdi.h>
#include <argp.h>
#endif
#include <stdlib.h>
#include "ftdi_tpi.h"



/* --- defines and constants --- */

/* --- global variables --- */


/* --- function prototypes --- */

/* --- main --- */
int main()
{
	int result=0;
	uint32_t device_id;

	result=ftdi_programmer_init();
        if (result < 0)
        {
		fprintf( stderr, "Error: Can't connect to programmer (code: %d)\n", result);
		exit(EXIT_FAILURE);
        }
	
	result=tpi_init(&device_id);
        if (result < 0)
        {
		fprintf( stderr, "Error: Can't connect to device (code: %d)\n", result);
		exit(EXIT_FAILURE);
        }
	
	result=tpi_stop();
        if (result < 0)
        {
		fprintf( stderr, "Error: Can't disable tpi access (code: %d)\n", result);
		exit(EXIT_FAILURE);
        }
	

	return result;
}
	

/* --- functions --- */

/** 
* @file intelhex.h
* @brief
* @details 
* @version
* @date Tue 18 Dec 2018 04:33:02 PM EST
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
*/

/* --- include files --- */
#include <stdio.h>
# ifndef S_SPLINT_S
#include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <stdint.h>



/* --- defines and constants --- */

/* --- global variables --- */

/* --- function prototypes --- */
FILE *openfile(uint8_t *file, uint8_t *mode);	//open file for reading

/* --- main --- */

/* --- functions --- */

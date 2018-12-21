/** 
* @file intelhex.c
* @brief
* @details 
* @version
* @date Thu 20 Dec 2018 10:52:17 PM EST
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
*
*
*	Intel hex file format bsed on https://en.wikipedia.org/wiki/Intel_HEX
*
*	Each line contains teh following fields
*	Start Code: ASCII Character 0: 1 character, Alwas an ASCII :
*	Byte Count: ASCII Characters 1,2: 2 characters, representing number 
*		of bytes (pairs of ascii characters in the data fields
*	Address: ASCII Characters 3,4,5,6: 4 characters, representing four hex digits, 
*		representing the 16-bit beginning memory address offset of the data. 
*		The physical address of the data is computed by adding this offset to 
*		a previously established base address, thus allowing memory addressing 
*		beyond the 64 kilobyte limit of 16-bit addresses. The base address, 
*		which defaults to zero, can be changed by various types of records. 
*		Base addresses and address offsets are always expressed as big endian values.
*	Record Type: ASCII Characters 7,8: representing two hex digits, 00 to 05, defining the meaning of the data field.
*		00: Data
*		01: End of File
*		02: Extended Segment Address
*		04: Extended Linear Address
*		05: Start Linear Address 
*	Data: ASCII characters 9-2n: a sequence of n bytes of data, represented by 2n (ASCII) hex digits.
*	Checksum: ASCII Chatacters 2n+1 through 2n+2, two ascii characters representing two hex digits, 
*		a computed value that can be used to verify the record has no errors.
*		Checksum calculation:
*		A record's checksum byte is the two's complement (negative) of the least significant byte (LSB) 
*		of the sum of all decoded byte values in the record preceding the checksum. 
*		It is computed by summing the decoded byte values and extracting the LSB of the sum 
*		(i.e., the data checksum), and then calculating the two's complement of the LSB i
*		(e.g., by inverting its bits and adding one). 
*	
*	Some inspiration from
*		https://www.instructables.com/id/HEX-File-to-Array-in-C/
*
*/

/* --- include files --- */
#include "intelhex.h"
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

/* --- main --- */

/* --- functions --- */
/** @brief Openfile for reading
*/
FILE *openfile(uint8_t *file, uint8_t *mode)
{
	FILE *fileptr=fopen(file, mode);
	
	if ( fileptr==NULL)
	{
		perror("Unable to open the file");
		exit (EXIT_FAILURE);	
	}	
	return fileptr;
}

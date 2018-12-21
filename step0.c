/** 
* @file step0.c
* @brief
* @details 
* @version
* @date Thu 20 Dec 2018 11:12:22 PM EST
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
#include "intelhex.h"
#include <stdio.h>
# ifndef S_SPLINT_S
#include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <argp.h> 


/* --- defines and constants --- */
#define HEXFILESIZEMAX 0x8000

/* --- global variables --- */

/* --- function prototypes --- */

/* --- main --- */

char *devicename;
char *infile;
char *outfile;
static int 
parse_opt (int key, char *arg, 
	struct argp_state *state) 
{ 
	switch (key) 
	{ 
		case 'd': 
		{
			devicename=arg;
			printf ("Target device is %s \n", devicename);
			break; 
		}
		case 'e': 
		{
			printf ("Erase Device Flash \n");
			break; 
		}
		case 'i': 
		{
			infile=arg;
			printf ("Input File is %s \n", infile);
			break; 
		}
		case 'o': 
		{
			outfile=arg;
			printf ("Out File is %s \n", outfile);
			break; 
		}
		case 'p': 
		{	
			infile=arg;
			printf ("Program Device Flash using %s \n", infile);
			//will parse file into an array
			uint8_t hexarray[HEXFILESIZEMAX];	
			//bytes read into the array
			int hexarraysize;	
			//file to be loaded
			FILE *hexfile;
			hexfile=openfile(infile,"rb");
			printf("file opened \n");

			fclose(hexfile);
			
			break; 
		}
		case 'r': 
		{
			printf ("Read Device Flash to stdout or <outfile>\n");
			break; 
		}
		case 'v': 
		{
			printf ("Verify device Flash against <infile>\n");
			break; 
		}
	} 
	return 0; 
} 

int
main (int argc, char **argv) 
{ 
	struct argp_option options[] = 
	{	 
		{ 0, 'd', "<attiny4|attiny5|attiny9|attiny10>", 0, "Device name. Must be applied when programming the device."}, 
		{ 0, 'e', 0, 0, "Erase device. The device will be erased before any other programming takes place."}, 
		{ 0, 'i', "<inputfile>", 0, "Name of Flash input file. Required for programming or verification of the Flash memory. The file format is Intel Extended HEX."}, 
		{ 0, 'o', "<outputfile>", 0, "Name of Flash output file. Required for readout of the Flash memoryi to a file. The file format is Intel Extended HEX."}, 
		{ 0, 'p', "<inputfile>", 0, "Program device Flash. Corresponding input files are required."}, 
		{ 0, 'r', 0, 0, "Read out device Flash. Will output to stdout if output file is not specified."}, 
		{ 0, 'v', 0, 0, "Verify device Flash. Can be used with –p or alone. Corresponding input files are required."}, 
		{ 0 } 
	}; 
	struct argp argp = { options, parse_opt, 0,0 }; 
  	return argp_parse (&argp, argc, argv, 0, 0, 0); 
} 

/* --- functions --- */

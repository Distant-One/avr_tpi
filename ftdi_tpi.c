/** 
* @file ftdi_tpi.c
* @brief
* @details 
* @version
* @date Wed 28 Nov 2018 11:27:44 PM EST
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
#include "ftdi_tpi.h"
#include <stdio.h>
#include <ftdi.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


/* --- defines and constants --- */
#define DEBUG 0x01

/* --- global variables --- */
struct ftdi_context ftdic;              /**<  Global FTDI context */
char nvmkey[] = {0x12, 0x89, 0xAB, 0x45, 0xCD, 0xD8, 0x88, 0xFF};


/* --- function prototypes --- */

/* --- main --- */

/* --- functions --- */
/** @brief Intialize the ftdi programmer 
*/
int ftdi_programmer_init()        //Initialize the ftdi device
{
	int result=0;
	if (ftdi_init(&ftdic) <0) 
        {
		fprintf( stderr, "Can't init context (code: %d)\n", result);
		return -1;
	}
	

	result = ftdi_usb_open(&ftdic, VENDOR_ID, PRODUCT_ID);
	if(result < 0 ) 
	{
		fprintf( stderr, "Can't open device (code: %d)\n", result);
		return -1;
	}
	return result;
}
int ftdi_set_pin_direction(unsigned char *direction)	// set ftdi pin directions
{
	int result=0;
	result = ftdi_set_bitmode(&ftdic, *direction, BITMODE_BITBANG); // set pin directions and bitbang mode
	if(result < 0 ) 
	{
		fprintf( stderr, "Can't set bitmode and mask (code: %d)\n", result);
		return -1;
	}
	
	return result;
        
}
/** @brief Even parity calculator over 9 bits
* Parameters
* @param *c     pointer to char to have parity calculated
* Return values
* @return p     returns even parity of param *c 
*/
char tpi_parity(char *c)
{
	char p=0; /* Parity char and return character */
	p=*c;
	p ^= p >> 4;
	p ^= p >> 2;
	p ^= p >> 1;
	p ^= 0;
	p &= 0x01;
	return p;
}


int tpi_write_frame(unsigned char *data)	// write byte to tpi bus
{
	unsigned char direction=0;
	int i=0;
	uint8_t buff=0;
	uint8_t parity=0;
	uint16_t frame=0;
        char pins=0;
	int result=0;
	

	direction = (0x0 | TPIRST | TPICLK | TPIDAT) & ~TPITST; // rst, clk, dat output, tst input
	ftdi_set_pin_direction(&direction);

	buff = *data;
	frame = (uint16_t)buff;
        frame = frame <<1;
	parity = tpi_parity(data);
	frame = frame | (((uint16_t)parity)<<9);
	frame = frame | (0x0f<<10);


	printf("Write data %02x , Write frame %04x\n", *data, frame);

	// send two idles
	pins =  (( 0x0 | TPIDAT) & (~TPIRST) & (~TPICLK)) & 0x0f; // set dat high for idle
	for (i=0; i<2; i++)
	{
		result = ftdi_write_data(&ftdic, &pins, 1); // write pin values
		usleep(TPI_HALF_CLK); // wait half cycle

		pins = pins ^ TPICLK; //toggle clock	
		result = ftdi_write_data(&ftdic, &pins, 1); // write pin values
		usleep(TPI_HALF_CLK); // wait half cycle
	}	 

	// send 12 bit frame
	for (i=0; i<12 ;i++)
	{
		pins =  (( 0x0 | TPIDAT) & (~TPIRST) & (~TPICLK)) & 0x0f; // set dat high
		pins = ((frame & (1<<i)) ? pins : pins ^ TPIDAT);
	
		result = ftdi_write_data(&ftdic, &pins, 1); // write pin values
		usleep(TPI_HALF_CLK); // wait half cycle

		pins = pins ^ TPICLK; //toggle clock	
		result = ftdi_write_data(&ftdic, &pins, 1); // write pin values
		usleep(TPI_HALF_CLK); // wait half cycle
		
        }


}
/** @brief store 16 nit address in the tpi pointer register
*/
int tpi_pr(uint16_t address)
{
	int result=0;
	unsigned char data=0;

	data=SSTPR0;	//command to load low byte
	tpi_write_frame(&data);
	data=(address & 0x00ff);	// lower address
	tpi_write_frame(&data);

	
	data=SSTPR1;	//command to load low byte
	tpi_write_frame(&data);
	data=((address>>8) & 0x00ff);	// upper address
	tpi_write_frame(&data);

	return result;
}
/** @brief reads a data bit from tpi and stores 1 or 0 in data
*/
int tpi_read_bit(unsigned char *data)
{
	unsigned char pins=0;
	unsigned char buff=0;
	int result=0;
	
	//set clock low
	pins =  (( 0x0 ) & (~TPIRST) & (~TPICLK)) & 0x0f; // clk low, rst low 
	#ifdef DEBUG
	debug_gen_test_data(1, &pins);	// debug test stream get bit
	#endif
	result = ftdi_write_data(&ftdic, &pins, 1); // write pin values
	usleep(TPI_HALF_CLK); // wait half cycle
	
	// set clock high and read data
	pins = pins ^ TPICLK;	// toggle clock
	result = ftdi_write_data(&ftdic, &pins, 1); // write pin values
	result = ftdi_read_pins(&ftdic, &buff);	// read pins
	usleep(TPI_HALF_CLK); // wait half cycle

	//  Set daya = 1 id TPIDATA is high
	*data = ((buff & TPIDAT) > 0 ? 1 : 0);
	#ifdef DEBUG
	if (*data > 0)
	{
		printf("read 1 \n");
	}
	else
	{
		printf("read 0 \n");
	}
	#endif

	return result;
}
/** @brief reads data frame from tpi store byte in data, checks idle, parity and stop bits
*/
int tpi_read_frame(unsigned char *data)
{
	int i=0;
	unsigned char buff=0;
	unsigned char direction=0;
	unsigned char calc_parity=0;
	unsigned char read_parity=0;
	int result=0;

	// change direction to read
	direction = (0x0 | TPIRST | TPICLK | TPITST) & (~TPIDAT); // rst, clk, tst output, dat input
	ftdi_set_pin_direction(&direction);
	
	// skip idle bits (1) looking for start bit (0)
	i = TPI_READ_GUARD_TIME_MAX;
	while (i > 0)
	{
		result=tpi_read_bit(&buff);
                if (buff == 0)
		{
			//found start bit
			break;
		}
		#ifdef DEBUG
		else
		{
			printf("Read idle bit, i = %d \n", i);
		}
		#endif
		i--;

	}
	if (i < 1)	// idle count exceeeded
	{
		// something's wrong
		result = -1;
		fprintf(stderr, "Read idle bit gaurd time exceeded %d bits (code %d)\n", \
TPI_READ_GUARD_TIME_MAX, result);
		return result;
	}
	
	// read data byte
	*data=0;
	for (i=0; i<8; i++)	// GET DATA BYTE
	{
		
		result=tpi_read_bit(&buff);
		*data=(buff>0 ? *data | 1<<i : *data );
	}
	printf("Read byte %02x\n",*data);

	// read parity
	result=tpi_read_bit(&read_parity);
        calc_parity=tpi_parity(data);
	if ((0x01 & read_parity) != (0x01 & calc_parity))	// parity mismatch
	{	
		result = -2;
		fprintf(stderr, "Parity Error, Read %d, Calculated %d, (code %d\n)", read_parity, calc_parity, result);
	}
		
	// check two stop bits
	for (i=0; i<2; i++)
	{
		result=tpi_read_bit(&buff);
		if (buff<1)	//stop bits should be 1 so therewas an error
		{
			result = -3;
			fprintf(stderr, "Error: Stop bit #%d is 0 should be 1 (code %d) \n", (i+1), result); 
		}
	}
	
	return result;
	
}
int tpi_read_data(uint16_t address, unsigned char *data, int len) 	// write byte to tpi bus
{
	unsigned char direction=0;
	int i=0;
	uint8_t buff=0;
	uint8_t parity=0;
	uint16_t frame=0;
        char pins=0;
	int result=0;


	printf("read address %02x \n", address);
        
	// Store tpi pointer address
	result=tpi_pr(address);

	#ifdef DEBUG
	debug_gen_test_data(0, &pins);	//reset debug test stream
	#endif

	for(i=0; i<len; i++)
	{
		//  read comand
		buff=SLDP;	// Serial load Data with post increment
		result=tpi_write_frame(&buff);
		
		// read byte
		result=tpi_read_frame(&data[i]);
		if (result < 0)	//error in frame
		{
			fprintf(stderr, "Error in frame aborting read (code %d)\n", result);
		}
	}



	return result;
}
void debug_gen_test_data(uint16_t reset_or_continue, unsigned char *data)
{
	// testdata[0]= 1111-start-0xca-parity-stop-stop  (tests initial tx to rx idle timei so all 16 bits needed)

		//0b1101100101001111, 	also set testdatacnt to 16 
		//0b0000110110010100,  	also set testdatacnt to 12 
	//  rest of bytes aligned correctly so just send 12 bits
	//  testdata[1]=start-0x54-bad parity 0-stop-stop (tests odd parity error)
	//  testdata[2]=start-0x93-bad parity 1-bad stop-stop (tests bad even parity and bad stop1)
	//  testdata[3]=start-0xbf-parity-stop-bad stop (tests bad stop2)
	static uint16_t testdata[4] = { \
		0b1101100101001111, \
		0b0000110010101000, \
		0b0000101100100110, \
		0b0000011101111110 };
	static uint16_t testdatacnt[4] = {16,12,12,12};	// bit counts for each tesdata word
	static int wordoffset=0;
	static int bitoffset=0;	
	uint16_t buff=0;

	wordoffset=((reset_or_continue > 0) ? wordoffset : 0);	//reset wordoffset to 0
	bitoffset=((reset_or_continue > 0) ? bitoffset : 0);	//reset bitoffset to 0

	*data=((testdata[wordoffset] & (1<<bitoffset)) ? (*data | TPITST) : (*data & (~TPITST)));

	#ifdef MORE_DEBUG
	if ( (*data & TPITST ) > 0)
	{
		printf("TST Bit 1 \n");
	}
	else
	{
		printf("TST Bit 0 \n");
	}
	#endif
	
	buff=testdata[wordoffset];
	buff=buff>>(testdatacnt[wordoffset] -11);
	buff=buff & 0x00ff;

	// printf("Debug Current byte %04x, Current word %04x, Current Wordoffset %04x, bitoffset %04x \n",   buff , testdata[wordoffset], wordoffset, bitoffset);
	
	bitoffset++;
	bitoffset=(bitoffset < testdatacnt[wordoffset] ? bitoffset : 0);  //reset if word done
	wordoffset=(bitoffset > 0 ? wordoffset : wordoffset + 1);	// next word
	wordoffset=(wordoffset < 4 ? wordoffset : 0);	//start over
	// printf("Debug Next Wordoffset %04x, bitoffset %04x \n", wordoffset, bitoffset);

	// repeat reset so everything is ready for the 1st bit 
	wordoffset=((reset_or_continue > 0) ? wordoffset : 0);	//reset wordoffset to 0
	bitoffset=((reset_or_continue > 0) ? bitoffset : 0);	//reset bitoffset to 0
}



/** @brief Initialize the tpi bus on the selected attiny device
*
*  @note To enable tpi bus
*	Set ftdi bitbang mode and pin directions, 
*	Set pins high to start wait a while, 
*	Set reset low wait, and wait trst, 
*	Set data high and run clock for 16 cycles,
*	Set guard time from programmer tx to tpi tx to 0 in CSR,
*	Send Flash Skey,
*	Read device id
*/
int tpi_init()	//init the tpi interface and attiny device
{
	int result=0;
	uint8_t direction=0x00;
	uint8_t pins=0;
        uint8_t i=0;
	unsigned char data=0;
	unsigned char device_id[DEVICE_ID_LEN];

	direction = (0x0 | TPIRST | TPICLK | TPIDAT) & ~TPITST; // rst, clk, dat output, tst input
	ftdi_set_pin_direction(&direction);
	
	pins = 0x0 | TPIRST | TPICLK | TPIDAT | TPITST; // rst, clk, dat high to start
	result = ftdi_write_data(&ftdic, &pins, 1); // write pin values

	usleep(1000); // keep pins high for about a ms

	pins = pins & (~TPIRST);	//set reset low  to statr enable sequence
	result = ftdi_write_data(&ftdic, &pins, 1); // write pin values
	
	usleep(2); // reset should be low for at least 2us according to data sheet

	//  toggle clock  for 16 cycles (32 half clks) times to complete enable sequence
        for( i = 0; i < 16; i++ )
	{
		pins = pins ^ TPICLK; //toggle clock	
		result = ftdi_write_data(&ftdic, &pins, 1); // write pin values
		usleep(TPI_HALF_CLK); // wait half cycle

		pins = pins ^ TPICLK; //toggle clock	
		result = ftdi_write_data(&ftdic, &pins, 1); // write pin values
		usleep(TPI_HALF_CLK); // wait half cycle
	

	}

	// Set tpi csr gaurd time to 0
	data = 0x0 | SSTCS | TPIPCR;	//SSTCS command and TPIPR location
	tpi_write_frame(&data);	// Write command
	data = 0x07;	// 0x07 sets gaurd time to 0
	tpi_write_frame(&data);

	// Send flash SKEY
	data = SKEY;
	tpi_write_frame(&data);	//send skey command
	for (i=0; i<8; i++)
	{
		tpi_write_frame(&nvmkey[i]);	// send skey data	
	}
	
	// get device id
	result=tpi_read_data(DEVICE_ID_BITS_BASE, device_id, DEVICE_ID_LEN);

	

	return result;

}
		

        
       

 

	
	

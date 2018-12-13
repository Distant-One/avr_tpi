/** 
* @file ftdi_tpi.c
* @brief
* @details 
* @version
* @date Thu 13 Dec 2018 03:57:45 PM EST
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
# ifndef S_SPLINT_S
#include <ftdi.h>
#include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>


/* --- defines and constants --- */
#define DEBUG 0x01

/* --- global variables --- */
struct ftdi_context ftdic;              /**<  Global FTDI context */
uint8_t nvmkey[] = {(uint8_t)0x12,(uint8_t)0x89, (uint8_t)0xAB, (uint8_t)0x45, (uint8_t)0xCD, (uint8_t)0xD8, (uint8_t)0x88, (uint8_t)0xFF};


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
int ftdi_set_pin_direction(uint8_t *direction)	// set ftdi pin directions
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
uint8_t tpi_parity(uint8_t *c)
{
	uint8_t p=(uint8_t)0; /* Parity char and return character */
	p=*c;
	p ^= p >> 4;
	p ^= p >> 2;
	p ^= p >> 1;
	p ^= 0;
	p &= 0x01;
	return p;
}
void tpi_write_idle_bits(unsigned int count)
{
	uint8_t pins=0;
	int i=0;
	int result=0;
	
	while (count > 0)
	{
		pins =  (uint8_t)((( 0x0 | TPIDAT) & (~TPIRST) & (~TPICLK)) & 0x0f); // set dat high for idle
		for (i=0; i<2; i++)
		{
			result = ftdi_write_data(&ftdic, &pins, 1); // write pin values
			usleep(TPI_HALF_CLK); // wait half cycle

			pins = pins ^ TPICLK; //toggle clock	
			result = ftdi_write_data(&ftdic, &pins, 1); // write pin values
			usleep(TPI_HALF_CLK); // wait half cycle
		}
		count--;
	}	 

}
/** @brief write one or more bytes
*/
int tpi_write_data(uint16_t address, uint8_t *data, int len)
{
	int i=0;
	uint8_t buff=0;
	int result=0;
	
	printf("write address %04x \n", address);
        
	// Store tpi pointer address
	result=tpi_pr(address);

	for(i=0; i<len; i++)
	{
		//  write comand
		buff=SSTP;	// Serial stor Data with post increment
		result=tpi_write_frame(&buff);
		
		// write byte
		result=tpi_write_frame(&data[i]);
		if (result < 0)	//error in frame
		{
			fprintf(stderr, "Error in frame aborting write (code %d)\n", result);
		}
	}
	return result;
}

int tpi_write_frame(uint8_t *data)	// write byte to tpi bus
{
	uint8_t direction=0;
	uint8_t i=0;
	uint8_t buff=0;
	uint8_t parity=0;
	uint16_t frame=0;
        uint8_t pins=0;
	int result=0;
	

	direction = (uint8_t)((0x0 | TPIRST | TPICLK | TPIDAT) & ~TPITST); // rst, clk, dat output, tst input
	result=ftdi_set_pin_direction(&direction);

	buff = *data;
	frame = (uint16_t)buff;
        frame = frame <<1;
	parity = tpi_parity(data);
	frame = frame | (((uint16_t)parity)<<9);
	frame = frame | (0x0f<<10);


	printf("Write data %02x , Write frame %04x\n", *data, frame);

	// send two idles
	//tpi_write_idle_bits(2);

	// send 12 bit frame
	for (i=0; i<12 ;i++)
	{
		pins =  (uint8_t)((( 0x0 | TPIDAT) & (~TPIRST) & (~TPICLK)) & 0x0f); // set dat high
		pins = (uint8_t)(((frame & (1<<i))>0) ? pins : pins ^ TPIDAT);
	
		result = ftdi_write_data(&ftdic, &pins, 1); // write pin values
		usleep(TPI_HALF_CLK); // wait half cycle

		pins = pins ^ TPICLK; //toggle clock	
		result = ftdi_write_data(&ftdic, &pins, 1); // write pin values
		usleep(TPI_HALF_CLK); // wait half cycle
		
        }

	return result;
}
/** @brief store 16 bit address in the tpi pointer register
*/
int tpi_pr(uint16_t address)
{
	int result=0;
	uint8_t data=0;

	data=SSTPR0;	//command to load low byte
	result=tpi_write_frame(&data);
	data=(address & 0x00ff);	// lower address
	result=tpi_write_frame(&data);

	
	data=SSTPR1;	//command to load low byte
	result=tpi_write_frame(&data);
	data=((address>>8) & 0x00ff);	// upper address
	result=tpi_write_frame(&data);

	return result;
}
/** @brief reads a data bit from tpi and stores 1 or 0 in data
*/
int tpi_read_bit(uint8_t *data)
{
	uint8_t pins=0;
	uint8_t buff=0;
	int result=0;
	
	//set clock low
	pins =  (uint8_t)((( 0x0 ) & (~TPIRST) & (~TPICLK)) & 0x0f); // clk low, rst low 
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
	*data = (uint8_t)((buff & TPIDAT) > 0 ? 1 : 0);
	#ifdef DEBUG
	if (*data > 0)
	{
		printf("1 ");
	}
	else
	{
		printf("0 ");
	}
	#endif

	return result;
}
/** @brief reads data frame from tpi store byte in data, checks idle, parity and stop bits
*/
int tpi_read_frame(uint8_t *data)
{
	int i=0;
	uint8_t j=0;
	uint8_t buff=0;
	uint8_t direction=0;
	uint8_t calc_parity=0;
	uint8_t read_parity=0;
	int result=0;
	
	printf("Read bit "); // begining of reading line
	// change direction to read
	direction = (uint8_t)((0x0 | TPIRST | TPICLK | TPITST) & (~TPIDAT)); // rst, clk, tst output, dat input
	result=ftdi_set_pin_direction(&direction);
	
	// skip idle bits (1) looking for start bit (0)
	i = TPI_READ_GUARD_TIME_MAX;
	while (i > 0)
	{
		result=tpi_read_bit(&buff);
                if (buff == 0)
		{
			//found start bit
			#ifdef DEBUG
			printf("(START) ");
			#endif
			break;
		}
		#ifdef DEBUG
		else
		{
			printf("(IDLE) ");
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
	for (j=0; j<8; j++)	// GET DATA BYTE
	{
		
		result=tpi_read_bit(&buff);
		*data=(buff>0 ? *data | 1<<j : *data );
	}
	printf("(Read byte %02x) ",*data);

	// read parity
	result=tpi_read_bit(&read_parity);
	#ifdef DEBUG
	printf("(PARITY) ");
	#endif
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
	printf("\n");
	
	return result;
	
}
/** @brief read one or more data bytes
*/
int tpi_read_data(uint16_t address, uint8_t *data, int len) 	// write byte to tpi bus
{
	int i=0;
	uint8_t buff=0;
        uint8_t pins=0;
	int result=0;


	printf("read address %04x \n", address);
        
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
void debug_gen_test_data(uint16_t reset_or_continue, uint8_t *data)
{
	// testdata[0]= 1111-start-0xca-parity-stop-stop  (tests initial tx to rx idle timei so all 16 bits needed)

		//0b1101100101001111, 	also set testdatacnt to 16 
		//0b0000110110010100,  	also set testdatacnt to 12 
	//  rest of bytes aligned correctly so just send 12 bits
	//  testdata[1]=start-0x54-bad parity 0-stop-stop (tests odd parity error)
	//  testdata[2]=start-0x93-bad parity 1-bad stop-stop (tests bad even parity and bad stop1)
	//  testdata[3]=start-0xbf-parity-stop-bad stop (tests bad stop2)
	//static uint16_t testdata[4] = { 0b1101100101001111, 0b0000110010101000, 0b0000101100100110, 0b0000011101111110 };
	static uint16_t testdata[4] = { 0xd94f, 0x0ca8, 0x0b26, 0x077e };
	static uint16_t testdatacnt[4] = {16,12,12,12};	// bit counts for each tesdata word
	static int wordoffset=0;
	static unsigned int bitoffset=0;	
	uint16_t buff=0;

	wordoffset=((reset_or_continue > 0) ? wordoffset : 0);	//reset wordoffset to 0
	bitoffset=((reset_or_continue > 0) ? bitoffset : 0);	//reset bitoffset to 0

	*data=(((testdata[wordoffset] & (1<<bitoffset))>0) ? (*data | TPITST) : (*data & (~TPITST)));

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
/** @brief set control and status space registers
*/
int tpi_control_store( uint8_t reg_address, uint8_t reg_value)
{
	int result=0;
	uint8_t data=0;
	
	// Combine SSTC command with Adress and write the command
	data = (0xf0 & SSTCS) | ( 0x0f & reg_address);    //SSTCS command and TPIPR location
	result=tpi_write_frame(&data); // Write command
	data = reg_value;    // write register value
	result=tpi_write_frame(&data);
	
	return result;
}

/** @brief read control and status space registers
*/
int tpi_control_read( uint8_t reg_address, uint8_t *reg_value)
{
	int result =0;
	uint8_t data=0;
	
	// Combine SLDCS command with Adress and write the command
	data = (0xf0 & SLDCS) | ( 0x0f & reg_address);    //SSTCS command and TPIPR location
	result=tpi_write_frame(&data); // Write command

         // send two idles
	tpi_write_idle_bits(2);

	//write guard idles
	// Not needed since init routine sets gaurd to 0

	//read byte
	result=tpi_read_frame(&data);
	*reg_value=data;

	return result;
}
/** @Brief disable external program mode by writimg 0 to nvm bit 
*/
int tpi_disable_external_program_mode()
{
	uint8_t data=0;
	int result=0;

	// Disable nvm
	result=tpi_control_store(TPISR, 0x00);
	result=tpi_control_read(TPISR, &data);
	printf("TPISR returns %02x \n", data);
	if (data > 0)
	{
		fprintf(stderr, "NVM Still Enabled %02x\n", data);
		result = -1;
	}
	else
	{
		printf("External Program Mode (NVM) Disabled %02x\n", data);
		result = 0;
	}
	return result;

}
/** @Brief Enable external program mode by sending SKEY command and data
*/
int tpi_enable_external_program_mode()
{
	int i=0;
	int result=0;
	uint8_t data=0;
		
	data = SKEY;
	result=tpi_write_frame(&data);	//send skey command

	for (i = 8; i > 0; i--)	// Send last byte 1st
	{
		data=nvmkey[i-1];
		result=tpi_write_frame(&data);	// send skey data	
	}

	//After the key has been given, the Non-Volatile Memory Enable (NVMEN) bit in the TPI Status Register (TPISR) must be polled until the Non-Volatile memory has been enabled.

        for( i = 0; i < 32; i++ )
	{
		result=tpi_control_read(TPISR, &data);
		printf("TPISR returns %02x \n", data);
		if ((data & 0x02)>0 )
		{
			printf("External Program Mode (NVM) Enabled %02x\n", data);
			result =0;
			break;
		}
	}
	if (i>31)
	{
		printf ("NVM enable failed\n");
		result = -1;
	}
	return result;
}
/** @brief disable  tpi bus accces on the device
*/
void tpi_disable()
{
	int result=0;
	uint8_t direction=0x00;
	uint8_t pins=0;

	//disable tpi access
	direction = (0x0 | TPIRST | TPICLK | TPIDAT) & ~TPITST; // rst, clk, dat output, tst input
	result=ftdi_set_pin_direction(&direction);
	
	pins = 0x0 | TPIRST | TPICLK | TPIDAT | TPITST; // rst, clk, dat high to start
	result = ftdi_write_data(&ftdic, &pins, 1); // write pin values

	usleep(1000); // keep pins high for about a ms

	printf("TPI Access disabled\n");
}
/** @brief Enable tpi bus accces on the device
*/
int tpi_enable_tpi_access()
{
	int result=0;
	uint8_t direction=0x00;
	uint8_t pins=0;
	uint8_t data=0;
	

	direction = (0x0 | TPIRST | TPICLK | TPIDAT) & ~TPITST; // rst, clk, dat output, tst input
	result=ftdi_set_pin_direction(&direction);
	
	pins = 0x0 | TPIRST | TPICLK | TPIDAT | TPITST; // rst, clk, dat high to start
	result = ftdi_write_data(&ftdic, &pins, 1); // write pin values

	usleep(1000); // keep pins high for about a ms

	pins = pins & (~TPIRST);	//set reset low  to statr enable sequence
	result = ftdi_write_data(&ftdic, &pins, 1); // write pin values
	
	usleep(2); // reset should be low for at least 2us according to data sheet

	//  toggle clock  for 16 cycles (32 half clks) times to complete enable sequence
	tpi_write_idle_bits(16);
	
	// Set tpi csr gaurd time to 0
	result=tpi_control_store(TPIPCR, 0x07);
	
	// Check TPI gaurd time was written
	result=tpi_control_read(TPIPCR, &data);
	printf("TPIPCR, returns %02x \n", data);


	// Check TPI ID
	result=tpi_control_read( TPIIR, &data);
	printf("TPIIR returns %02x \n", data);
	if ((data & 0x80)>0)	//check if tpi id can be read
	{
		printf("TPI Access Enabled %02x\n", data);
		result=0;
	}
	else
	{
		result= -1;
		fprintf(stderr, "TPI Access cannot be enabled (code %d)",result);
	}

	return result;

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
int tpi_init(uint32_t *device_id)	//init the tpi interface and attiny device
{
	int result=0;
	uint8_t buff[DEVICE_ID_LEN]={0,0,0};

	//enable tpi access	
	result=tpi_enable_tpi_access();
	
	//Enable External Program (nvm) mode
	result=tpi_enable_external_program_mode();

	// Get device id	
	/*
	!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	CHANGE SO THAT BUFF BELOW IS PASSED AS POINTER
	!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	*/
	result=tpi_read_data(DEVICE_ID_BITS_BASE, buff, DEVICE_ID_LEN);
	*device_id = buff[0]<<16 | buff[1]<<8 | buff[2];
	printf("Device id is 0x%06x\n",*device_id);

	return result;

}
/** @brief disables external program mode and tpi access
*/
int tpi_stop()
{
	int result=0;
	// exit external tpi programming mode at end of testing
	// Disable nvm
	result=tpi_disable_external_program_mode();
	
	//disable tpi access
	tpi_disable();

	return result;

}

		

        
       

 

	
	

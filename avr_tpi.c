/** 
* @file avr_tpi.c
* @brief TPI Programmer for Attiny4/5/9/10
* @details TPI prgrammer for Microchip (formerly Atmel)AVR ATTINY4/5/9/10 Microporcessorsi using linux/raspian and ft232r bitbang mode.
* These parts can only be programmed through the TPI interface which is not compatible with the typical ISP interface for other AVR devices. 
*
* @version 0001
* @date Fri 16 Nov 2018 10:59:02 PM EST
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
*	@note Using libftdi api (not D2XX)
*	@see https://www.intra2net.com/en/developer/libftdi/documentation/group__libftdi.html
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
*	@note Need to consider timing for this project.
*	
*	Attiny TPI max clock rate is 2MHZ, with no minimum specified
*	
*	I'm not sure what the the max toggle rate is for ft232rl
*
*	I'll start with tpiclk rate of 100kHz (10us period)
*	
*
*/

/* --- include files --- */
#include <stdio.h>
#include <ftdi.h>


/* --- defines and constants --- */

/*	debug defines	*/
#define	SLOWTPI	0x01	/* Slow TPICLK for debugging with LED's */

/*	port pin  and other defines for ft232rl */
#define PIN_TX  0x01  
#define PIN_RX  0x02  
#define PIN_RTS 0x04  
#define PIN_CTS 0x08  
#define PIN_DTR 0x10	/* not brought out on ftdi ttl-232 Cable */
#define PIN_DSR 0x20	/* not brought out on ftdi ttl-232 Cable */
#define PIN_DCD 0x40	/* not brought out on ftdi ttl-232 Cable */
#define PIN_RI  0x80	/* not brought out on ftdi ttl-232 Cable */
#define FT232R_VENDOR_ID	0x0403
#define FT232R_PRODUCT_ID	0x6001
#define VENDOR_ID	FT232R_VENDOR_ID
#define PRODUCT_ID	FT232R_PRODUCT_ID

/* 	Default signal mapping */
#define TPICLK	PIN_TX
#define TPIRESETN	PIN_RX
#define TPIDATA	PIN_RTS

/*	TPI Operation masks */
#define TPI_WRITE_MASK	TPIRESETN | TPICLK | TPIDATA	/* Signals driven for TPI write */
#define TPI_READ_MASK	TPIRESETN | TPICLK 	/* Signals driven for TPI read */
#define TPI_RESETN_MASK	TPIRESTN	/* Signals driven to reset TPI */

/* 	TPI timing */
#ifndef SLOWTPI
	#define TPI_CLK_PERIOD	10	/* 100khz vlock is 10us period */
#endif
#ifdef 	SLOWTPI
	#define TPI_CLK_PERIOD	1000000	/* Set TPICLK to 1 second for debugging with LED's */
#endif
#define TPI_HALF_CLK	TPI_CLK_PERIOD/2
#define	TPI_T_RESET	2	/* TPI treset max is 2us */
#define TPI_ENABLE_COUNT	16	/* TPICLK counts after TPI_T_RESET to enable TPI */
#define TPI_FRAME_LEN	12	/* TPI frame is 12 bits including st, d0-d7, parity, stop1 and stop 2 */


/*	TPI frame values	*/
#define	TPI_IDLE_BIT	0x01	/* Idle bit is 1 */
#define	TPI_ST_BIT	0x00	/* Start bit is 0 */
#define	TPI_SP1_BIT	0x01	/* Stop Bit 1 is 1 */
#define TPI_SP2_BIT	0x01	/* Stop Bit 2 is 1 */
#define TPI_BREAK_BIT	0x00	/* Break bit is 0 */





/* --- global variables --- */

/*	data frame bit sequence	*/
/*
enum tpiFrameSequence {
	START,
	D0, D1, D2, D3, D4, D5, D6, D7,
	PARITY,
	STOP1, STOP2
};
*/


/* --- function prototypes --- */
#ifdef RUNNING
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
#endif


/* --- main --- */

int main()
{
    unsigned char c = 0;
    signed int resultcode = 0;
    struct ftdi_context ftdic;
    unsigned char sdebug[40];		/* string to hold debug messages */

    /* Initialize context for subsequent function calls */
    if (ftdi_init(&ftdic) <0) {
    /*if ((ftdi = ftdi_new()) == 0) {   */
	puts("Can't init context");
	return 1;
    }

    /* Open FTDI device based on vendor & product IDs */
    resultcode = ftdi_usb_open(&ftdic, VENDOR_ID, PRODUCT_ID);
    if(resultcode < 0 ) {
        puts("Can't open device");
	printf("%d", resultcode);
        return 1;
    }

    /* Enable bitbang mode with TPI signals set as outputs */
    
   ftdi_set_bitmode(&ftdic, TPI_WRITE_MASK,BITMODE_BITBANG);
	/*  clear outputs to 0 */
	c =  0x00;
        ftdi_write_data(&ftdic, &c, 1);
	sprintf(sdebug, "clear c is %d", c) ;
	puts(sdebug);
	puts("000");
	usleep(TPI_HALF_CLK);

	/*  T-1a set reset high, clk low, data low */
	c =  ((0x07 & TPIRESETN)  | (0x00 & TPICLK) | (0x00 & TPIDATA));
        ftdi_write_data(&ftdic, &c, 1);
	sprintf(sdebug, "T-1a c is %d", c) ;
	puts(sdebug);
	usleep(TPI_HALF_CLK);

	/*  T-1b set reset high, clk high, data low */
	c =  c | TPICLK;
        ftdi_write_data(&ftdic, &c, 1);
	sprintf(sdebug, "T-1b c is %d", c) ;
	puts(sdebug);
	usleep(TPI_HALF_CLK);


    /* Endless loop: simulate frame */
    for(;;) {
	

	/*  T0a set reset low, clk low, data low */
	c =  ((0x00 & TPIRESETN)  | (0x00 & TPICLK) | (0x00 & TPIDATA));
        ftdi_write_data(&ftdic, &c, 1);
	sprintf(sdebug, "T0a c is %d", c) ;
	puts(sdebug);
	usleep(TPI_HALF_CLK);

	/*  T0b set reset low, clk high, data low */
	c =  c | TPICLK;
        ftdi_write_data(&ftdic, &c, 1);
	sprintf(sdebug, "T0b c is %d", c) ;
	puts(sdebug);
	usleep(TPI_HALF_CLK);

	/*  T1a set reset low, clk low, data high */
	c =  ((0x00 & TPIRESETN)  | (0x00 & TPICLK) | (0x07 & TPIDATA));
        ftdi_write_data(&ftdic, &c, 1);
	sprintf(sdebug, "T1a c is %d", c) ;
	puts(sdebug);
	usleep(TPI_HALF_CLK);

	/*  T1b set reset low, clk high, data high */
	c =  c | TPICLK;
        ftdi_write_data(&ftdic, &c, 1);
	sprintf(sdebug, "T1b c is %d", c) ;
	puts(sdebug);
	usleep(TPI_HALF_CLK);

	/*  T2a set reset low, clk low, data low */
	c =  ((0x00 & TPIRESETN)  | (0x00 & TPICLK) | (0x00 & TPIDATA));
        ftdi_write_data(&ftdic, &c, 1);
	sprintf(sdebug, "T2a c is %d", c) ;
	puts(sdebug);
	usleep(TPI_HALF_CLK);

	/*  T2b set reset low, clk high, data low */
	c =  c | TPICLK;
        ftdi_write_data(&ftdic, &c, 1);
	sprintf(sdebug, "T2b c is %d", c) ;
	puts(sdebug);
	usleep(TPI_HALF_CLK);

	/*  T3a set reset low, clk low, data high */
	c =  ((0x00 & TPIRESETN)  | (0x00 & TPICLK) | (0x07 & TPIDATA));
        ftdi_write_data(&ftdic, &c, 1);
	sprintf(sdebug, "T3a c is %d", c) ;
	puts(sdebug);
	usleep(TPI_HALF_CLK);

	/*  T3b set reset low, clk high, data high */
	c =  c | TPICLK;
        ftdi_write_data(&ftdic, &c, 1);
	sprintf(sdebug, "T3b c is %d", c) ;
	puts(sdebug);
	usleep(TPI_HALF_CLK);

	/*  T4a set reset low, clk low, data low */
	c =  ((0x00 & TPIRESETN)  | (0x00 & TPICLK) | (0x00 & TPIDATA));
        ftdi_write_data(&ftdic, &c, 1);
	sprintf(sdebug, "T4a c is %d", c) ;
	puts(sdebug);
	usleep(TPI_HALF_CLK);

	/*  T4b set reset low, clk high, data low */
	c =  c | TPICLK;
        ftdi_write_data(&ftdic, &c, 1);
	sprintf(sdebug, "T4b c is %d", c) ;
	puts(sdebug);
	usleep(TPI_HALF_CLK);

    }

}

/* --- functions --- */

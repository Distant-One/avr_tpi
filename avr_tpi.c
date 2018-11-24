/** 
* @file avr_tpi.c
* @brief TPI Programmer for Attiny4/5/9/10
* @details TPI prgrammer for Microchip (formerly Atmel)AVR ATTINY4/5/9/10 Microporcessorsi using linux/raspian and ft232r bitbang mode.
* These parts can only be programmed through the TPI interface which is not compatible with the typical ISP interface for other AVR devices. 
*
* @version 0001
* @date Sat 24 Nov 2018 04:14:38 PM EST
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
ty*
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
*	I'll start with tpiclk rate of 100kHz (10us period) and see if that works
*
*	@note General flow of the program
*	- Initialize ft232
*	- Open ftdi usb device
*	- Set ftdi in bitbang writemode
*		-# resetn, tpiclk, tpidata set to outputs
*		-# RESETN	0 
*		-# TPICLK	0
*		-# TPIDATA	0
*	- Enable attiny TPI interface
*	-# ResetN Low for treset
*		-# RESETN	0 
*		-# TPICLK	0
*		-# TPIDATA	0
*	-# toggle tpiclk for 16x cycle
*		-# Cycle  :   0 1 2 3 4 5 6 7 8 9101112131415 
*		-# RESETN :  00000000000000000000000000000000 
*		-# TPICLK :  01010101010101010101010101010101
*		-# TPIDATA:  11111111111111111111111111111111
*	
*	
*	@note REading writing memory
*	- Write Pointer Register High
*	- Write Pointer Register Low
*	- Write/Read data with increment
*	
*
*/

/* --- include files --- */
#include <stdio.h>
#include <ftdi.h>


/* --- defines and constants --- */

/*	debug defines	*/
#define	SLOWTPI	0x01	/* Slow TPICLK for debugging with LED's */
#define DEBUG_MESSAGES 0x01	/* Output debuig message sz to the console */

/*	port pin  and other defines for ft232rl */
#define FT232R_VENDOR_ID	0x0403	/**< FT232RL Vendor ID */
#define FT232R_PRODUCT_ID	0x6001	/**< FT232RL Vendor ID */
#define PIN_TX  0x01 	/**< TX signal port location */ 
#define PIN_RX  0x02  	/**< RX signal port location */
#define PIN_RTS 0x04  	/**< RTS signal port location */
#define PIN_CTS 0x08  	/**< CTS signal port location */
#define PIN_DTR 0x10	/**< not brought out on ftdi ttl-232 Cable */
#define PIN_DSR 0x20	/**< not brought out on ftdi ttl-232 Cable */
#define PIN_DCD 0x40	/**< not brought out on ftdi ttl-232 Cable */
#define PIN_RI  0x80	/**< not brought out on ftdi ttl-232 Cable */

/* 	Default signal mapping */
#define TPICLK	PIN_TX	/**< Assigning TPICLK to FTDI TX PIN */
#define TPIRESETN	PIN_RX 	/**< Assigning TPIRESETN to FTDI RX PIN */
#define TPIDATA	PIN_RTS 	/**< Assigning TPIDATA to FTDI RTS PIN */
#define TESTDATA	PIN_CTS	/**< Assigning TPIDATA to FTDI RTS PIN */

/*	TPI Operation masks */
#define TPI_WRITE_MASK	TPIRESETN | TPICLK | TPIDATA	/**< Signals set to outputs for TPI write */
#define TPI_WRITE_STATE	(0x00 & TPIRESETN) | (0x00 & TPICLK) | (0x00 & TPIDATA)	/**< Signal state for start of TPI write */
#define TPI_WRITE_ONE_SETUP	(0x00 & TPIRESETN) | (0x00 & TPICLK) | (0xff & TPIDATA)	/**< Signal state for setup write 1*/ 
#define TPI_WRITE_ONE_HOLD	(0x00 & TPIRESETN) | (0xff & TPICLK) | (0xff & TPIDATA)	/**< Signal state for hold write 1*/ 
#define TPI_WRITE_ZERO_SETUP	(0x00 & TPIRESETN) | (0x00 & TPICLK) | (0x00 & TPIDATA)	/**< Signal state for setup write 0*/ 
#define TPI_WRITE_ZERO_HOLD	(0x00 & TPIRESETN) | (0xff & TPICLK) | (0x00 & TPIDATA)	/**< Signal state for hold write 0*/ 
#define TPI_READ_MASK	TPIRESETN | TPICLK 	/**< Signals driven for TPI read */
#define TPI_READ_STATE	(0x00 & TPIRESETN) | (0x00 & TPICLK) /**< Signal state for start of TPI read */
#define TPI_RESETN_MASK	TPIRESTN	/**< Signals driven to reset TPI */

/* 	TPI timing */
#ifndef SLOWTPI
	#define TPI_CLK_PERIOD	10	/**< Default TPICLK is 100khz so the period is 10us */
#endif
#ifdef 	SLOWTPI
	#define TPI_CLK_PERIOD	100000	/**< Set TPICLK period to 1 second for debugging with LED's */
#endif
#define TPI_HALF_CLK	TPI_CLK_PERIOD/2	/**< Half period for TPICLK */
#define TPICLK_START	0	/**< TPICLK set to low for 1st half period then invert second half */
#define	TPI_T_RESET	2	/**< TPI treset max is 2us */
#define TPI_RESET_DATA	0	/**< Data value during TPI reset */
#define TPI_RESET_CLK	0	/**< Clock value during TPT reset */
#define TPI_ENABLE_COUNT	16	/**< TPICLK counts after TPI_T_RESET to enable TPI */
#define TPI_ENABLE_SETUP	(0x00 & TPIRESETN) | (0x00 & TPICLK) | (0xff & TPIDATA)	/**< Signal state for setup write 1*/ 
#define TPI_ENABLE_HOLD	(0x00 & TPIRESETN) | (0xff & TPICLK) | (0xff & TPIDATA)	/**< Signal state for hold write 1*/ 
#define TPI_DISABLE	(0xff & TPIRESETN) | (0x00 & TPICLK) | (0x00 & TPIDATA)	/**< Signal state for setup write 1*/ 


/*	TPI frame values	*/
#define TPI_FRAME_LEN	12	/**< TPI frame is 12 bits including st, d0-d7, parity, stop1 and stop 2 */
#define TPI_BREAK_FRAME_MIN_LEN	12	/**< Break frame 12 zero bits minimum, no start/stop/parity. At least one idle bit before and after break frame */ 
#define	TPI_IDLE_BIT	TPI_WRITE_ONE_SETUP	/**< Idle bit is 1 */
#define	TPI_ST_BIT	TPI_WRITE_ZERO_SETUP	/**< Start bit is 0 */
#define	TPI_SP1_BIT	TPI_WRITE_ONE_SETUP	/**< Stop Bit 1 is 1 */
#define TPI_SP2_BIT	TPI_WRITE_ONE_SETUP	/**< Stop Bit 2 is 1 */
#define TPI_BREAK_BIT	TPI_WRITE_ZERO_SETUP	/**< Break bit is 0 */
#define TPI_MAX_FRAME_CNT	0x09	/**< Accomodate SKEY instruction followed by 8 bytes */
#define TPI_READ_GUARD_TIME	130	/**< Idle bits swithcing between programmer output and device output 128 default + 2  */
#define	TPI_WRITE_GUARD_TIME	1	/**< Minumum idle bits switching between device output and programmer output */ 

/*	TPI Instructions	*/
#define SLD	0x20	/**< SLD: data, PR, Serial LoaD from data space using indirect addressing */
#define SLDP	0x24	/**< SLDP: data, PR+, Serial LoaD from data space using indirect addressing and post increment*/
#define SST	0x60	/**< SST: data, PR, Serial STore to  data space using indirect addressing */
#define SSTP	0x64	/**< SSTP: data, PR, Serial STore to  data space using indirect addressing and post increment*/
#define SSTPR0	0x68	/**< SSTPR0: PR, low byte, Serial STore to Pointer Register using direct addressing */
#define SSTPR1	0x69	/**< SSTPR1: PR, high byte, Serial STore to Pointer Register using direct addressing */
#define SIN	0x10	/**< SIN: data, a, Serial IN from data space */
#define SIN_MASK 0x6f	/**< SINMASK: xaax aaaa, bits marked a for 6 bit address */
#define SOUT	0x90		/**< SOUT: aa aaaa, data, Serial OUT to data space */
#define SOUT_MASK 0x6f	/**< SOUTMASK: xaax aaaa, bits marked a for 6 bit address */
#define SLDCS	0x80	/**< SLDCS: data, aaaa, Serial LoaD from Control and Status space using direct addressing */ 
#define SLDCSMASK	0x0f	/**< SLDCS: xxxx aaaa, bits marked a form 4 bit address */ 
#define SSTCS	0xc0	/**< SSTCS: aaaa, data, Serial STore to Control and Status space using direct addressing */
#define SSTCSMASK	0x0f	/**< SSTCS: xxxx aaaa, bits marked a form 4 bit address */ 
#define SKEY	0xe0	/**< SKEY: Key, {8{data}}xi, Serial KEY */


/* --- global variables --- */

/*	NVM Program Key	*/
const char nvmkey[] = {0x12, 0x89, 0xAB, 0x45, 0xCD, 0xD8, 0x88, 0xFF};


/*	data frame bit sequence	*/

enum tpiFrameSequence {
	SEARCHING,
	STILLSEARCHING,
        START,
	D0, D1, D2, D3, D4, D5, D6, D7,
	PARITY,
	STOP1, STOP2
};
const char* tpiFrameSequenceNames[] = {"Start","D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7", "Parity", "Stop1", "Stop2"};
enum test { FALSE, TRUE };

int vendorID = FT232R_VENDOR_ID;		/**< Default Vendor ID assignment for the ft232 chip used */
int productID = FT232R_PRODUCT_ID;	/**< Default Product ID assignment for the ft232 chip used */
struct ftdi_context ftdic;		/**< FTDI context */

unsigned int	tpiHalfClk=TPI_HALF_CLK;	/**< Default half perdiod of clock */	
unsigned int	tpiClkStart=TPICLK_START;	/**< Default initial low/high state of  clock*/	
unsigned int	tpiResetTime=TPI_T_RESET;	/**< Default reset time prior to starting the clk during tpi enable */
unsigned int	tpiEmableCount=TPI_ENABLE_COUNT;	/**< Default number of clocks for tpi enable sequence */




/* --- function prototypes --- */
void tpi_state_message(unsigned char *c);
int programmer_direction(unsigned char bitmask, unsigned char mode);
int tpi_enable_access();
int programmer_write_data(unsigned char* buf,int size);
int programmer_set_bitmode(unsigned char bitmask, unsigned char mode);
int programmer_read_data(unsigned char* buf);
char tpi_parity(char *c);
int tpi_write_data(unsigned char *buf, unsigned int size);
int tpi_read_data(unsigned char *buf, unsigned int size);
void accumulate_and_display_frame(unsigned char *pinmask, unsigned char *datapins, int accumulate_or_display);
#ifdef RUNNING
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
    unsigned char sdebug[40];		/* string to hold debug messages */
    int result=0;	/* return value */
    unsigned char cnt=0;
    char buf[TPI_MAX_FRAME_CNT];


    /* Initialize context for subsequent function calls */
    if (ftdi_init(&ftdic) <0) {
    /*if ((ftdi = ftdi_new()) == 0) {   */
        sprintf(sdebug, "Can't init context") ;
	puts(sdebug);
	return -1;
    }

    /* Open FTDI device based on vendor & product IDs */
    result = ftdi_usb_open(&ftdic, vendorID, productID);
    if(result < 0 ) {
        sprintf(sdebug, "Can't open device %d", result) ;
        puts(sdebug);
        return -1;
    }

    #ifdef RUNNING
    /* Enable TPI Interface */
    result = tpi_enable_access(); /* Enable tpi access on avr device */
    #endif

    buf[0]=SKEY;
    for (cnt = 1; cnt < TPI_MAX_FRAME_CNT; cnt++)
    {
        buf[cnt]=nvmkey[cnt-1];
    }

    /* tpi_write_data(&buf[0], TPI_MAX_FRAME_CNT); */
    /* tpi_write_data(&buf[0], 1); */
    tpi_read_data(&buf[0], 4);
    #ifdef RUNNING
    c=0xab;
    tpi_write_data(&c, 0x1);
    #endif

    return result;

}


/* --- functions --- */
/*! @brief quick function to send state of reset, clock and data to console 
*   @param *c	Pointer to signal state written to tpi bus
*/
void tpi_state_message(unsigned char *c)
{
    unsigned char rst=0;	/* bit holding rst state 1 or 0 */
    unsigned char tclk=0;	/* bit holding tclk state 1 or 0 */
    unsigned char tdata=0;	/* bit holding tdata state 1 or 0*/
    unsigned char sdebug[10];	/* string to hold debug messages */

    if ((*c & TPIRESETN) > 0)
    {
       rst=1;
    }
    if ((*c & TPICLK) > 0)
    {
       tclk=1;
    }
    if ((*c & TPIDATA) > 0)
    {
       tdata=1;
    }

    puts("rcd");
    sprintf(sdebug, "%d%d%d", rst,tclk,tdata) ;
    puts(sdebug);
}

/*! @brief Native programmer write function  
* Parameters
* @param *buff	pointer to buffer to write
* @param size of buffer to write
*
* Return values
* @return 0	all fine
* @return i<0	failed
*/
int programmer_write_data(unsigned char* buf,int size)
{
    int result=0;	/* return value */
    result = ftdi_write_data(&ftdic, buf , size);
    return result;
}
int programmer_read_data(unsigned char* buf)
{
    int result=0;	/* return value */
    result = ftdi_read_pins(&ftdic, buf);
    return result;
}


/*! @brief Native programmer set bit operation and read/write mode
* Parameters
* @param bitmask  char signals set to 1 will be outputs
* @param mode char value to set programmer bitmode
*
* Return values
* @return 0	all fine
* @return i<0	failed
*/
int programmer_set_bitmode(unsigned char bitmask, unsigned char mode)
{

    int result=0;	/* return value */
    result = ftdi_set_bitmode(&ftdic, bitmask, mode);
    return result;
}
	

/*! @brief programmer read or write set up Sequence 
*	Sequence used to set the ftdi part up for writing to the tpi bus
* 
* Parameters
* @param ftdi	pointer to ftdi_context
* @param bitmask	Bitmask to configure lines. HIGH/ON value configures a line as output.
* @param mode	Bitbang mode: use the values defined in ftdi_mpsse_mode
*
* Return values
* @return 0	all fine
* @return i<0	failed
*
*/
int programmer_direction(unsigned char bitmask, unsigned char mode)
{	
    unsigned char c=0;	/* character value used to write to ftdi device */  
    int result=0;	/* return vbalue */	
    unsigned char sdebug[60];		/* string to hold debug messages */

    /* Set up bit bang mode using write mask */
    result = programmer_set_bitmode(bitmask, mode);
    #ifdef DEBUG_MESSAGES 
    sprintf(sdebug, "Set FTDI bitmask to %02x and bitbang mode %02x", bitmask, mode) ;
    puts(sdebug);
    tpi_state_message(&bitmask);
    #endif
    if (result < 0)	/* failed */
    {
        sprintf(sdebug, "Set Bitbang mode failed %d", result);
        puts(sdebug);
    }

    return result;

}

/*! @brief Start TPI Write Sequence  
* Parameters
* @param ftdi	pointer to ftdi_context
* Return values
* @return 0	all fine
* @return i<0	failed
*/
int tpi_enable_access()
{
    unsigned char c=0;	/* character value used to write to ftdi device */  
    int result=0;	/* return vbalue */	
    unsigned char sdebug[40];		/* string to hold debug messages */
    int cycleCnt=0;	/* counts tpi cycles */


    /* set programmer to write mode */
    result = programmer_direction(TPI_WRITE_MASK, BITMODE_BITBANG);  /* Set ftdi bitbang mode and set default signal state */
    
    /* Make sure tpi is disabled to start */
    c=TPI_DISABLE;
    result = programmer_write_data(&c, 1);
    #ifdef DEBUG_MESSAGES 
    sprintf(sdebug, "Make sure tpi is disabled to start  %d", c) ;
    puts(sdebug);
    tpi_state_message(&c);
    #endif
    usleep(tpiHalfClk);	/* Wait half cycle  time */
    usleep(tpiHalfClk);	/* Wait half cycle  time */

    /* Reset Portion of enable sequence */
    c=TPI_WRITE_STATE;	
    /* result = ftdi_write_data(&ftdic, &c, 1); */
    result = programmer_write_data(&c, 1);
    #ifdef DEBUG_MESSAGES 
    sprintf(sdebug, "Set signals to reset defaults %d", c) ;
    puts(sdebug);
    tpi_state_message(&c);
    #endif
    usleep(TPI_T_RESET);	/* Wait reset time */

    /* Clock active portion of the enable sequence */
    while ( cycleCnt < TPI_ENABLE_COUNT)
    {
        c=TPI_ENABLE_SETUP;		/* set tpidata high for setup cycle */
        result = programmer_write_data(&c, 1);
        #ifdef DEBUG_MESSAGES 
        sprintf(sdebug, "Cycle %d Setup signals %d", cycleCnt, c) ;
        puts(sdebug);
        tpi_state_message(&c);
        #endif
        usleep(tpiHalfClk);	/* Wait half cycle  time */
        
        c=TPI_ENABLE_HOLD;		/* set tpidata high for hold cycle */
        result = programmer_write_data(&c, 1);
        #ifdef DEBUG_MESSAGES 
        sprintf(sdebug, "Setup signals  %d", c) ;
        puts(sdebug);
        tpi_state_message(&c);
        #endif
        usleep(tpiHalfClk);	/* Wait half cycle  time */
        
        cycleCnt++;
    }
    return result;
}


/*! @brief Even parity calculator over 9 bits
* Parameters
* @param *c 	pointer to char to have parity calculated
* Return values
* @return p	returns even parity of param *c	
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


/** @brief Sequence for writing data bytes on the TPI interface */
int tpi_write_data(unsigned char *buf, unsigned int size)
{
    int offset=0;	/* offset into buf string */
    enum tpiFrameSequence frameCycle;	/* Clock cycle in frame */
    unsigned char c=0;	/* character value used to write to ftdi device */  
    unsigned char d=0;	/* character value used to hold data byte at buff +offset */  
    int result=0;	/* return vbalue */	
    unsigned char sdebug[40];		/* string to hold debug messages */
    int cnt=0;
    char rst[TPI_FRAME_LEN*2];
    char clk[TPI_FRAME_LEN*2];
    char dat[TPI_FRAME_LEN*2];


    if (size > TPI_MAX_FRAME_CNT)
    {
        sprintf(sdebug, "TPI Write Date Buffer size %d exceeds max %d", size, TPI_MAX_FRAME_CNT);
	puts(sdebug);
        return -1;
    }
    
    /* Set up bit bang mode using write mask */
    result = programmer_direction(TPI_WRITE_MASK, BITMODE_BITBANG);  

    while (offset < size)
    {
       	
        d=buf[offset];
        #ifdef DEBUG_MESSAGES 
        sprintf(sdebug, "Writing byte #%d, Value %02x", offset, d) ;
        puts(sdebug);
        #endif
        cnt=0;
        for (frameCycle = START; frameCycle <= STOP2; frameCycle++)
        {
            switch(frameCycle)
            {
                case START:
		    c=TPI_ST_BIT;
                    break;
                case D0: 
                case D1: 
                case D2: 
                case D3: 
                case D4: 
                case D5: 
                case D6: 
                case D7:
                    c=(((d & 0x01) == 0x01) ? TPI_WRITE_ONE_SETUP : TPI_WRITE_ZERO_SETUP  );
                    d = d >> 1;
                    break;
                case PARITY: 
                    c=((tpi_parity(&buf[offset]) == 0x01) ? TPI_WRITE_ONE_SETUP : TPI_WRITE_ZERO_SETUP );
                    break;
                case STOP1: 
                    c=TPI_SP1_BIT;
                    break;
                case STOP2: 
                    c=TPI_SP2_BIT;
                    break;
                default:
                    puts("Big Problems in Framebit count!");
                    result = -1;
            }
            result = programmer_write_data(&c, 1);
            #ifdef MORE_DEBUG_MESSAGES 
            sprintf(sdebug, "Frame sequence number %d", frameCycle) ;
            puts(sdebug);
            sprintf(sdebug, "Frame Setup signals  %d", c) ;
            puts(sdebug);
            tpi_state_message(&c);
            #endif
            
            #ifdef DEBUG_MESSAGES 
            rst[cnt]=0x30;	/* ascii for "0" */
            if ((c & TPIRESETN) > 0)
            {
               rst[cnt]=0x31;	/* ascii for "1" */
            }
            clk[cnt]=0x30;
            if ((c & TPICLK) > 0)
            {
               clk[cnt]=0x31;
            }
            dat[cnt]=0x30;
            if ((c & TPIDATA) > 0)
            {
               dat[cnt]=0x31;
            }
            cnt++;
            #endif
            usleep(tpiHalfClk);	/* Wait half cycle  time */

            c = (c ^ TPICLK);	/* Invert Clock */	   
            result = programmer_write_data(&c, 1);
            #ifdef MORE_DEBUG_MESSAGES 
            sprintf(sdebug, "Frame Hold signals  %d", c) ;
            puts(sdebug);
            tpi_state_message(&c);
            
            #endif
            #ifdef DEBUG_MESSAGES 
            rst[cnt]=0x30;	/* ascii for "0" */
            if ((c & TPIRESETN) > 0)
            {
               rst[cnt]=0x31;	/* ascii for "1" */
            }
            clk[cnt]=0x30;
            if ((c & TPICLK) > 0)
            {
               clk[cnt]=0x31;
            }
            dat[cnt]=0x30;
            if ((c & TPIDATA) > 0)
            {
               dat[cnt]=0x31;
            }
            cnt++;
            #endif
            usleep(tpiHalfClk);	/* Wait half cycle  time */
        }
        offset++;
        rst[cnt]=0;
        sprintf(sdebug, "rst: %s",rst);
        puts(sdebug);
        clk[cnt]=0;
        sprintf(sdebug, "clk: %s",clk);
        puts(sdebug);
        dat[cnt]=0;
        sprintf(sdebug, "dat: %s",dat);
        puts(sdebug);
    }
    return result;

}


/** @brief Sequence for reading data bytes on the TPI interface */
int tpi_read_data(unsigned char *buf, unsigned int size)
{
    int offset=0;	/* offset into buf string */
    enum tpiFrameSequence frameCycle;	/* Clock cycle in frame */
    unsigned char c=0;	/* char value of ftdi signals used to write to ftdi device */  
    unsigned char r=0;	/* char value read from ftdi device */  
    int result=0;	/* return value */	
    unsigned char sdebug[60];		/* string to hold debug messages */
    int cnt=0;
    char rst[(TPI_FRAME_LEN + TPI_READ_GUARD_TIME)*2];
    char clk[(TPI_FRAME_LEN + TPI_READ_GUARD_TIME)*2];
    char dat[(TPI_FRAME_LEN + TPI_READ_GUARD_TIME)*2];
    char tst[(TPI_FRAME_LEN + TPI_READ_GUARD_TIME)*2];
    int t[]={0xffff,0xfc8e,0x936f,0x72af,0xc5af,0xc5af,0xc5af,0xc5af,0xc5af}; 	/* test to find start, read value, test parity, stop1 and stop 2  */
    #ifdef RUNNING
    int t[]={0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xfc8e}; /*  test to check start search  not timeout */ 
    int t[]={0xc5af,0xfc8e,0x936f,0x72af,0xc5af,0xc5af,0xc5af,0xc5af,0xc5af}; 	/* test to find start, read value, test parity, stop1 and stop 2  */
    int t[]={0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xfc8e}; /* test to check start search timeout */
    int t[]={0xffffffff,0xffffffff,0xffffffff,0xf853}; /*  test to check start search  not timeout */ 
    int t[]={0xffffffff,0xfc8e}; /*  test to check start search  not timeout */ 
    #endif
    int tbuffbitcnt=0;
    int tbuffoffset=0;
    char maskit=0;	/* pin mask for ft232 */
    int timeoutcnt=0;	/* frameCycles to wait for read start bit before bailing out */
    enum test foundstartbiti=FALSE;
    char p=0;	/* parity holder */


    if (size > TPI_MAX_FRAME_CNT)
    {
        sprintf(sdebug, "TPI Read Date Buffer size %d exceeds max %d", size, TPI_MAX_FRAME_CNT);
	puts(sdebug);
        return -1;
    }
    
    /* Set up bit bang mode using read mask */
    #ifdef TESTDATA
        /* result = programmer_direction((TESTDATA | TPI_READ_MASK), BITMODE_BITBANG);  */
        maskit=TESTDATA | TPI_READ_MASK;
    #endif
    #ifndef TESTDATA
        /* enable reset and clcok but set tpidata as input */
        maskit= TPI_READ_MASK;
    #endif
    result = programmer_direction(maskit, BITMODE_BITBANG);
    sprintf(sdebug, "mask = %02x, mod = %02x", maskit, BITMODE_BITBANG );
    puts(sdebug);

    tbuffoffset=0;
    timeoutcnt=TPI_READ_GUARD_TIME+1;
    while (offset < size)
    {
        foundstartbiti = FALSE;
       	buf[offset]=0;
        cnt=0;
        tbuffbitcnt=0;
        
        for (frameCycle = SEARCHING; frameCycle <= STOP2; frameCycle++)
        {
            c=TPI_WRITE_ZERO_SETUP;
            sprintf(sdebug, "test bit %04x, word offset %04dx, test word %04x",tbuffbitcnt, tbuffoffset, t[tbuffoffset]);
	    puts(sdebug);
            if ((0x0001 & t[tbuffoffset]) > 0) 
            {
                c = c | TESTDATA;
            }
            else
            {
                c = c & ~TESTDATA;
            }
            tbuffbitcnt++;
            if ( tbuffbitcnt > 15)
            {
                tbuffbitcnt=0;
                tbuffoffset++;
            }
            t[tbuffoffset]=t[tbuffoffset]>>1;
            result = programmer_write_data(&c, 1);	/* set clock and reset */
            result = programmer_read_data(&r);	/* read data */
            #ifdef MORE_DEBUG_MESSAGES 
            sprintf(sdebug, "Frame sequence :%02x, Write Data:%02x, Read Data %02x", frameCycle, c, (r&TPIDATA) );
            puts(sdebug);
            tpi_state_message(&c);
            #endif
            
            #ifdef DEBUG_MESSAGES 
            accumulate_and_display_frame(&c, &r, 0x01);
            #endif
            usleep(tpiHalfClk);	/* Wait half cycle  time */

            c = (c ^ TPICLK);	/* Invert Clock */	   
            result = programmer_write_data(&c, 1);	/* set clock and reset */
            result = programmer_read_data(&r);	/* read data */
            #ifdef MORE_DEBUG_MESSAGES 
            sprintf(sdebug, "Frame sequence :%02x, Write Data:%02x, Read Data %02x", frameCycle, c, (r&TPIDATA) );
            puts(sdebug);
            tpi_state_message(&c);
            #endif
            switch(frameCycle)
            {
                case D0: 
                    buf[offset]=(((r & TPIDATA) > 0x01) ? buf[offset] | 0x01 : buf[offset] & 0xfe  );
                    break;
                case D1: 
                    buf[offset]=(((r & TPIDATA) > 0x01) ? buf[offset] | 0x02 : buf[offset] & 0xfd  );
                    break;
                case D2: 
                    buf[offset]=(((r & TPIDATA) > 0x01) ? buf[offset] | 0x04 : buf[offset] & 0xfb  );
                    break;
                case D3: 
                    buf[offset]=(((r & TPIDATA) > 0x01) ? buf[offset] | 0x08 : buf[offset] & 0xf7  );
                    break;
                case D4: 
                    buf[offset]=(((r & TPIDATA) > 0x01) ? buf[offset] | 0x10 : buf[offset] & 0xef  );
                    break;
                case D5: 
                    buf[offset]=(((r & TPIDATA) > 0x01) ? buf[offset] | 0x20 : buf[offset] & 0xdf  );
                    break;
                case D6: 
                    buf[offset]=(((r & TPIDATA) > 0x01) ? buf[offset] | 0x40 : buf[offset] & 0xbf  );
                    break;
                case D7:
                    buf[offset]=(((r & TPIDATA) > 0x01) ? buf[offset] | 0x80 : buf[offset] & 0x7f );
                    break;
                case PARITY: 
                    p=((tpi_parity(&buf[offset]) > 0) ? 0x01 : 0x00);
                    r=(((r & TPIDATA) > 0) ? 0x01 : 0x00);
                    if (p != r)
                    {
                        sprintf(sdebug, "Bad parity. Got Parity %01x Calc Parity  %01x", r , p);
                        puts(sdebug);
                    }
                    break;
                case STOP1: 
                    p=(((r & TPIDATA) > 0) ? 0x01 : 0x00);
                    if ( p < 1 )
                    {
                        puts("Bad STOP1 bit, got 0, should be 1");
                    }
                    break;
                case STOP2: 
                    p=(((r & TPIDATA) > 0) ? 0x01 : 0x00);
                    if ( p < 1 )
                    {
                        puts("Bad STOP2 bit, got 0, should be 1");
                    }
                    break;
            }
            #ifdef DEBUG_MESSAGES 
            accumulate_and_display_frame(&c, &r, 0x01);
            #endif
            usleep(tpiHalfClk);	/* Wait half cycle  time */
            if (foundstartbiti == FALSE)
	    {
                if ((r & TPIDATA) == 0x00)
                {
                    foundstartbiti=TRUE;
                    frameCycle = START;
                    timeoutcnt=TPI_READ_GUARD_TIME+1;
                }
                else
                {
                    frameCycle = SEARCHING;
                    timeoutcnt--;
		    sprintf(sdebug,"search count %d",timeoutcnt);
                    puts(sdebug);
                    if ( timeoutcnt < 1 )
                    {
                        sprintf(sdebug, "Timout exceeded for start bit from device %d",TPI_READ_GUARD_TIME);
                        puts(sdebug);
                        return -1;
                    }
                }
             }
        }
        #ifdef DEBUG_MESSAGES 
        sprintf(sdebug, "Read byte #%02x value %02x", offset, buf[offset]) ;
        puts(sdebug);
        accumulate_and_display_frame(&c, &r, 0x00);
        #endif
        offset++;
        tbuffoffset++;
        puts("\n");
        
    }

}
/** @brief debug function to display frames 
*  @param pinmask pointer to char for control signals, data bit will be ignored 
*  @param datapins pointer to char containing data bit, control signals will be ignored 
*  @param accumulate_or_display sset to 1 will accumulate frame bits. Set to 0 will display accumulated frame
*/
void accumulate_and_display_frame(unsigned char *pinmask, unsigned char *datapins, int accumulate_or_display)
{
    
    static int adcnt=0;
    static char adrst[(TPI_FRAME_LEN + TPI_READ_GUARD_TIME)*2];
    static char adclk[(TPI_FRAME_LEN + TPI_READ_GUARD_TIME)*2];
    static char addat[(TPI_FRAME_LEN + TPI_READ_GUARD_TIME)*2];
    static char adtst[(TPI_FRAME_LEN + TPI_READ_GUARD_TIME)*2];
    static char adsdebug[(TPI_FRAME_LEN + TPI_READ_GUARD_TIME)*2];

    if (accumulate_or_display > 0)	/* then accumulate */
    {
        
        adrst[adcnt]=0x30;	/* ascii for "0" */
        if ((*pinmask & TPIRESETN) > 0)
        {
           adrst[adcnt]=0x31;	/* ascii for "1" */
        }
        adclk[adcnt]=0x30;
        if ((*pinmask & TPICLK) > 0)
        {
           adclk[adcnt]=0x31;
        }
        addat[adcnt]=0x30;
        if ((*datapins & TPIDATA) > 0)
        {
           addat[adcnt]=0x31;
        }
        adtst[adcnt]=0x30;
        if ((*pinmask & TESTDATA) > 0)
        {
           adtst[adcnt]=0x31;
        }
        adcnt++;
    }
    else	/* display accumulated frame */
    {
        adrst[adcnt]=0;
        sprintf(adsdebug, "rst: %s",adrst);
        puts(adsdebug);
        adclk[adcnt]=0;
        sprintf(adsdebug, "clk: %s",adclk);
        puts(adsdebug);
        addat[adcnt]=0;
        sprintf(adsdebug, "dat: %s",addat);
        puts(adsdebug);
        adtst[adcnt]=0;
        sprintf(adsdebug, "tst: %s",adtst);
        puts(adsdebug);
        
        adcnt=0;
    }
}





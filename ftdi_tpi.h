/** 
* @file ftdi_tpi.h
* @brief
* @details 
* @version
* @date Tue 27 Nov 2018 01:25:10 PM EST
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
#include <stdint.h>

/* --- defines and constants --- */
#ifndef _FTDI_TPI_H
#define _FTDI_TPI_H
#endif

/*      ft232rl ID and port pin  mappings*/
#define FT232R_VENDOR_ID        0x0403  /**< FT232RL Vendor ID */
#define VENDOR_ID FT232R_VENDOR_ID
#define FT232R_PRODUCT_ID       0x6001  /**< FT232RL Vendor ID */
#define PRODUCT_ID FT232R_PRODUCT_ID 
#define PIN_TX  0x01    /**< TX signal port location */ 
#define PIN_RX  0x02    /**< RX signal port location */
#define PIN_RTS 0x04    /**< RTS signal port location */
#define PIN_CTS 0x08    /**< CTS signal port location */
#define PIN_DTR 0x10    /**< not brought out on ftdi ttl-232 Cable */
#define PIN_DSR 0x20    /**< not brought out on ftdi ttl-232 Cable */
#define PIN_DCD 0x40    /**< not brought out on ftdi ttl-232 Cable */
#define PIN_RI  0x80    /**< not brought out on ftdi ttl-232 Cable */

/*	TPI signal to FT232RL Port Pin Mappings */
#ifndef TPICLK	
#define TPICLK	PIN_TX  /**< TPICLK (O) to FTDI TX PIN */
#endif
#ifndef TPIRST	
#define TPIRST	PIN_RX  /**< TPIRST (O) to FTDI RX PIN */
#endif
#ifndef TPIDAT	
#define TPIDAT	PIN_RTS /**< TPIDAT (I/O) to FTDI RTS PIN */
#endif
#ifndef TPITST	
#define TPITST	PIN_CTS /**< TPITST (O/I) used to monitor/test TPIDAT for debugging to FTDI CTS PIN */
#endif

/*	Signal Hookup
* FT232RL             Attiny4/5/9/10 (SOT-23 Pins)
*      TX-------------TPICLK (3)
*      RX-------------/RESET (6)
*     CTS-----------+-TPIDAT (1)
*                   |  <-- remove RTS connection when using an actual  device
*     RTS--resistor-+  <-- Only used for debud. Don't use when attached to a device
*
* 
*/

/*	TPI Command Words */
#define SSTCS   0xc0    /**< SSTCS: aaaa, data, Serial STore to Control and Status space using dir    ect addressing */
#define SLDCS	0x80	/**< SLDCS: aaaa, data, Serial LoaD data from Control and Status space using direct addressing */
#define TPIPCR 	0x02	/**< TPIPCR: location in CSR of the rx to tx gaurd wait time */
#define TPIIR	0x0f	/**< TPI Identification Register. Reading this location returns 0x80 */
#define SKEY    0xe0    /**< SKEY: Key, {8{data}}xi, Serial KEY */
#define SSTPR0  0x68    /**< SSTPR0: PR, low byte, Serial STore to Pointer Register using direct a    ddressing */
#define SSTPR1  0x69    /**< SSTPR1: PR, high byte, Serial STore to Pointer Register using direct     addressing */
#define SLDP    0x24    /**< SLDP: data, PR+, Serial LoaD from data space using indirect addressin    g and post increment*/
#define SSTP    0x64    /**< SSTP: data, PR, Serial STore to  data space using indirect addressing     and post increment*/



/*	TPI Clock */
#define TPI_HALF_CLK 5000 /**< set Half tpi clock period to in microseconds */
#define TPI_READ_GUARD_TIME_MAX     10     /**< Idle bits swithcing between programmer output and dev    ice output 128 default + 2  */


/*	ATtiny memory map
*/
#define IO_SPACE_BASE	0x0000
#define IO_LEN	(0x003f -IO_SPACE_BASE )
#define DEVICE_ID_BITS_BASE	0x3FC0
#define DEVICE_ID_LEN	(0x3FC3 - DEVICE_ID_BITS_BASE )




/* --- global variables --- */

/* --- function prototypes --- */
int ftdi_programmer_init();	//Initialize the ftdi device
int ftdi_set_pin_direction(uint8_t *direction);	// set ftdi pin directions
char tpi_parity(char *c); //even parity calculator
int tpi_write_frame(unsigned char *data);	// write byte to tpi bus
int tpi_pr(uint16_t address);	//load tpi pointer register
void debug_gen_test_data(uint16_t reset_or_continue, unsigned char *data);
int tpi_read_bit(unsigned char *data);
int tpi_read_frame(unsigned char *data);
int tpi_read_data(uint16_t address, unsigned char *data, int len);       // write byte to tpi bus
int tpi_control_store( unsigned char reg_address, unsigned char reg_value);
int tpi_control_read( unsigned char reg_address, unsigned char *reg_value);	//read control reg*/
int tpi_init();	//Initialize the tpi interface

/* --- main --- */

/* --- functions --- */

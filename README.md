The avr_tpi is used to program Microchip (formerly Atmel) attiny4/5/9/10 microcontrollers via the Tiny Programming Interface (TPI) interfaces since they do not support the standard isp interface.  

A few points of interest:
1. The Tiny Programming Interface (TPI) requires three signals:
   /RESET: Tiny Programming Interface enable input to device and output from programmer.
   TPICLK: Tiny Programming Interface clock input to device and output from programmer.
   TPIDATA: Tiny Programming Interface data input/output to device and output/input from the programmer.
2. Attiny devices need 5V power for programming even though the devices can run 1.8V to 5.5V.
   One caviot, if the RSTDISBL has been set, 12V is required on the /reset line. No plans to support this in this programmer solution.
3. The TPI interface is not similar to SPI nor I2C so a bitbang solution is required.
4. For debugging the programmer code, I've added 4th signal TPITST.  TPITST can be used to generate output to test read functions

The application environment will be linux (ubuntu and raspbian) with a ft232 usb to serial bridge used in bitbang mode.

/*      Signal Hookup to device
* FT232RL             Attiny4/5/9/10 (SOT-23 Pins)
*      TX-------------TPICLK (3)
*      RX-------------/RESET (6)
*     CTS-------------TPIDAT (1), Data can be read/written on this signal
* 
*/



/*      Signal Hookup for programmer code debuging
* FT232RL              Breadboard
*      TX--------------(TPICLK LED) |>|--resistor--GND
*      RX--------------(TPIRST LED) |>|--resistor--GND
*     CTS------------+-(TPIDAT LED) |>|--resistor--GND
*                    |  
*     RTS-+-resistor-+ 
*         |
*         +-resistor---(TPITST LED) |>|--resistor--GND 
*
*   resistor - I've been testing with 550 Ohms, but 1k should work too.
*/



The ft232 usb serial bridge is the now retired ProtoBoard - Diprotodon (USB+Mix) since it offers 5v logic levels and bitbang mode
https://www.sparkfun.com/products/retired/8723

 

/*
 * CC1111EM Setup functions
 *
 * Copyright 2010 Dave
 * http://daveshacks.blogspot.com/2010/01/im-me-lcd-interface-hacked.html
 *
 * Copyright 2010 Michael Ossmann
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include <cc1110.h>
#include "ioCCxx10_bitdef.h"
#include "setup.h"
#include "bits.h"
#include "types.h"

void sleepMillis(int ms) {
	int j;
	while (--ms > 0) { 
		for (j=0; j<1200;j++); // about 1 millisecond
	};
}

void xtalClock() { // Set system clock source to 26 Mhz
    SLEEP &= ~SLEEP_USB_EN; // Disable USB
    SLEEP &= ~SLEEP_OSC_PD; // Turn both high speed oscillators on
    while( !(SLEEP & SLEEP_XOSC_S) ); // Wait until xtal oscillator is stable
    CLKCON = (CLKCON & ~(CLKCON_CLKSPD | CLKCON_OSC)) | CLKSPD_DIV_1; // Select xtal osc, 26 MHz
    while (CLKCON & CLKCON_OSC); // Wait for change to take effect
    SLEEP |= SLEEP_OSC_PD; // Turn off the other high speed oscillator (the RC osc)
}

void setIOPorts() {
	// Setup LED and Button Switch
    // Turn on LED
	P1DIR |= BIT1;
//#ifdef BUTTON_ON
    // Turn on BUTTON
	P1DIR |= BIT2;
//#endif
	// Turn off LED
	CC1111EM_LED = LOW;
	// Activate BUTTON - Do we need this?
	CC1111EM_BUTTON = HIGH;
}

//Toggle the CC1111EM LED
void tglLED() {
	CC1111EM_LED ^= HIGH;
}

// Set a baudrate 9600 for 24 MHz Xtal clock
//#define USART0_BAUD_M  163
//#define USART0_BAUD_E  8
// Set a baudrate 57600 for 24 MHz Xtal clock
#define USART0_BAUD_M  59
#define USART0_BAUD_E  11
// Set a clock rate of approx. 2.5 Mbps for 26 MHz Xtal clock
#define SPI_BAUD_M  170
#define SPI_BAUD_E  16

void configureSPI() {
	U0CSR = 0;  //Set SPI Master operation
	U0BAUD =  SPI_BAUD_M; // set Mantissa
	U0GCR = U0GCR_ORDER | SPI_BAUD_E; // set clock on 1st edge, -ve clock polarity, MSB first, and exponent
}

void configUART(){
    //Turn on USART0 ALT1

    //Clear PERCFG_U0CFG for ALT1 and set USART0 as precedence
    PERCFG &= (~BIT0);
    //PERCFG &= (~PERCFG_U0CFG);
    //P2DIR &= (~P2DIR_PRIP0);

    //Enable USART Interrupts IEN0
    //IEN0 = BIT1 | BIT3;
    
    //Timer1 use ALT2
    //PERCFG |= PERCFG_T1CFG;
    
    // Set USAT Control and Status
	//U0CSR = (BIT7 | BIT6);   //Clear U0CSR and set for UART Mode with TX/RX
	U0CSR = BIT7;   //Clear U0CSR and set for UART Mode with TX

    // Set UART Control
	//U0UCR = BIT7;   //FLUSH UART
	U0UCR = 0;      //Clear U0CSR and configure UART
	//U0UCR = BIT6;      //Clear U0CSR, set FLOW control, and configure UART

    // Set BAUDRATE
	U0BAUD = USART0_BAUD_M; // set Mantissa

    // Set Generic Control
    U0GCR = 0;      //Clear U0GCR
	//U0GCR = U0GCR_ORDER | USART0_BAUD_E; // set clock on 1st edge, -ve clock polarity, MSB first, and exponent
	U0GCR = USART0_BAUD_E; // set clock on 1st edge, -ve clock polarity, MSB first, and exponent

    // Ground       - Dongle Pin = 2
    // BIT2 = RX    - Dongle Pin = 6
    // BIT3 = TX    - Dongle Pin = 5
    // BIT4 = CT    - Dongle Pin = 4
    // BIT5 = RT    - Dongle Pin = 3
    //P0DIR = (BIT2 | BIT3);
    //P0DIR = (BIT4 | BIT5);
    //P0DIR |= (BIT2 | BIT3 | BIT4 | BIT5);
    P0SEL = (BIT2 | BIT3);
    //P0SEL = (BIT2 | BIT3 | BIT4 | BIT5);

}

// Transmit a byte via UART
//void serialTX(unsigned char x){
void serialTX(u8 x){
    //int cnt = 0;
    //last byte written not transmitted
    while (P0_5 != LOW);
    //while (!(U0CSR & U0CSR_TX_BYTE));
    //while (IRCON2 & BIT1); //TEST IRCON2.UTXxIF
    U0DBUF = x;
    //while (U0CSR & BIT0)
        //led_tick(3,XSH_PAUSE); //TEST IRCON2.UTXxIF
    //last byte written not transmitted
    //while (!(U0CSR & U0CSR_TX_BYTE));
    led_tick(1,XSH_PAUSE);

}

void led_tick(int cnt, int pause){
    int i;
    for (i = cnt;i;i--){
        tglLED();
        sleepMillis(pause);
        tglLED();
        sleepMillis(pause);
    }
}

//Setup USB
void setUSB(){
    SLEEP |= SLEEP_USB_EN; //Enable USB Controller
}

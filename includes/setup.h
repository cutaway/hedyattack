/*
 * IM-Me display functions
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

#include "types.h"

#define BIT0 0x01
#define BIT1 0x02
#define BIT2 0x04
#define BIT3 0x08
#define BIT4 0x10
#define BIT5 0x20
#define BIT6 0x40
#define BIT7 0x80

#define FALSE 0
#define TRUE 1
#define LOW 0
#define HIGH 1

#define WIDTH  132
#define HEIGHT 65

#define LG_PAUSE  0x0BB8 //3000
#define PAUSE     0x03E8 //1000
#define SH_PAUSE  0x01F4 //500
#define XSH_PAUSE 0x00FF //256
#define SXSH_PAUSE 0x000F //15
#define RADIO_PAUSE 0x0002 //2

void sleepMillis(int ms);

void xtalClock();

// IO Port Definitions:
//#define A0 P0_2
//#define SSN P0_4
#define CC1111EM_LED P1_1
#define LED_GREEN CC1111EM_LED
#define CC1111EM_BUTTON P1_2
#define BUTTON CC1111EM_BUTTON
#define USART0_RX P0_2
#define USART0_TX P0_3
#define USART0_CT P0_4
#define USART0_RT P0_5

// plus SPI ports driven from USART0 are:
// MOSI P0_3
// SCK P0_5

void setIOPorts();
void configUART();
void configureSPI();
void tglLED();
void led_tick(int cnt, int pause);
//void serialTX(unsigned char x);
void serialTX(u8 x);

/*
HedyAttack - Tools for identifying and analyzing frequency hopping spread spectrum(fhss) implementations.
Copyright (C) 2011  Cutaway, Q, and Atlas

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Please leave comments and recommendations at http://code.google.com/p/hedyattack
 */

#include <cc1110.h>
#include "types.h"
#include "ioCCxx10_bitdef.h"
#include "setup.h"
#include "stdio.h"
#include "pm.h"
#include "cc1111emk.h"


void sendTestReadings(){
    // Ground       - Dongle Pin = 2
    // BIT2 = RX    - Dongle Pin = 6
    // BIT3 = TX    - Dongle Pin = 5
    // BIT4 = CT    - Dongle Pin = 4
    // BIT5 = RT    - Dongle Pin = 3

    //u8 hd0 = 0xAA;
    //u8 hd1 = 0xBB;
    //u8 hd2 = 0xCC;
    //u8 hd3 = 0xDD;
    //u8 hd4 = 0xEE;
    //int ft = 0xEF;
    //int cnt = 0;
    //led_tick(3,PAUSE);

    U0DBUF = 0xAA;
    led_tick(1,XSH_PAUSE);
    U0DBUF = 0xBB;
    led_tick(1,PAUSE);
    U0DBUF = 0xCC;
    led_tick(1,XSH_PAUSE);
    U0DBUF = 0xDD;
    led_tick(1,PAUSE);
    U0DBUF = 0xEE;
    led_tick(1,XSH_PAUSE);

    //serialTX(hd0);
    //serialTX(hd1);
    //serialTX(hd2);
    //serialTX(hd3);
    //serialTX(hd4);

    /*
    for (cnt = 0; cnt < 100; cnt++)
        serialTX(cnt);
    serialTX(ft);
    serialTX(ft);
    */
}


void main(void) {
    xtalClock();
    setIOPorts();
    configUART();

    while (1) {
        sendTestReadings();
        sleepMillis(50);
    }
}

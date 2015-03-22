// #include "cc1111rf.h"
#include "global.h"

#ifdef VIRTUAL_COM
    #include "cc1111.h"
    #include "cc1111_vcom.h"
#else
    #include "cc1111usb.h"
#endif

/*************************************************************************************************
 * Application Code - these first few functions are what should get overwritten for your app     *
 ************************************************************************************************/

/*
 * Copyright 2010 Don C. Weber, InGuardians, Inc.
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

#define NUM_CHANNELS_83

// Only do 902-926
//#define FULL_900

// Do not store RSSI values
//#define INC_CH_RSSI

// specan-usb is app 0x43
#define SPECAN_APP 0x43
// specan-usb commands - these are the same for client and firmware side
#define SPECAN_TOGGLE 0x00
#define SPECAN_DATA 0x01

#ifdef VIRTUAL_COM
    #include "cc1111.h"
    #include "cc1111_vcom.h"
#else
    #include "cc1111usb.h"
#endif

//#include <cc1110.h>
//#include "types.h"
//#include "ioCCxx10_bitdef.h"
//#include "setup.h"
#include "global.h"
#include "stdio.h"
#include "pm.h"
#include "cc1111emk.h"

/* globals */
static xdata channel_info chan_table[NUM_CHANNELS];
static xdata u8 ss[NUM_CHANNELS];
static xdata u8 *temp_ss;
static xdata u8 mode = 0;
bit txdata_rdy = 0;
u8 min_chan;
u8 max_chan;

void radio_setup() {
    //Set up like SmartRF Register View

    //IOCFG2   = 0x00;
    //IOCFG1   = 0x00;
    //IOCFG0   = 0x00;

    // Preamble for Data Transfer
    //SYNC1   = 0x0B;
    //SYNC0   = 0x0B;

    //PKTLEN   = 0xFF;
    PKTCTRL1 = 0xE5;
    PKTCTRL0 = 0x04;
    //ADDR     = 0x00;
    //CHANNR   = 0x00;

    // IF of 457.031 kHz 
    //FSCTRL1 = 0x06;
    //FSCTRL0 = 0x00;
	// IF of 457.031 kHz 
	FSCTRL1 = 0x12;
	FSCTRL0 = 0x00;

    // Configure Frequency - math included
    // freq    = ??
    // hz = freq * 1000000;
    // the frequency setting is in units of 396.728515625 Hz 
    // u32 setting = (u32) (freq * .0025206154);
    // 920 = 0x26 0x55 0x55
    // 902 = 0x25 0x95 0x55
    // FREQ2 = 0x26;
    // FREQ1 = 0x55;
    // FREQ0 = 0x55;
    
    //Data Rate
    // 270.833333 kHz: 0xE5; 0xA3; 0x10; 0x23; 0x11; 
    // 499.511719 kHz: 0xEE; 0x55; 0x73; 0x23; 0x55;
    //MDMCFG4 = 0xEE; 
    //MDMCFG3 = 0x55; 
    //MDMCFG2 = 0x73; 
    //MDMCFG1 = 0x23; 
    //MDMCFG0 = 0x55; 

    //DEVIATN  = 0x16;

	// no automatic frequency calibration 
	MCSM0 = 0;
    // frequency calibration 
    //MCSM2  = 0x07;
    //MCSM1  = 0x30;
    //MCSM0  = 0x18;
    //FOCCFG = 0x17;
    //BSCFG  = 0x6C;

	// disable 3 highest DVGA settings 
	AGCCTRL2 |= AGCCTRL2_MAX_DVGA_GAIN;
    //AGCCTRL2 = 0x03;
    //AGCCTRL1 = 0x40;
    //AGCCTRL0 = 0x91;

    //FREND1    = 0x56;   // Front end RX configuration.
    //FREND0    = 0x10;   // Front end RX configuration.

    // frequency synthesizer calibration 
    FSCAL3 = 0xEA;
    FSCAL2 = 0x2A;
    FSCAL1 = 0x00;
    FSCAL0 = 0x1F;

    // "various test settings" 
    TEST2 = 0x88;
    TEST1 = 0x31;
    TEST0 = 0x09;

    // PA output power setting.
    //PA_TABLE7 = 0x00;   
    //PA_TABLE6 = 0x00;   
    //PA_TABLE5 = 0x00;   
    //PA_TABLE4 = 0x00;   
    //PA_TABLE3 = 0x00;   
    //PA_TABLE2 = 0x00;   
    //PA_TABLE1 = 0x00;   
    //PA_TABLE0 = 0xC0;   
}

/* freq in Hz */
void calibrate_freq(u32 freq, u8 ch) {

        /* the frequency setting is in units of 396.728515625 Hz */
        //u32 setting = (u32) (freq * .0025206154);
        u32 setting = (u32) (freq * FREQ_MULTIPLIER);

        FREQ2 = (setting >> 16) & 0xff;
        FREQ1 = (setting >> 8) & 0xff;
        FREQ0 = setting & 0xff;

        RFST = RFST_SCAL;
        RFST = RFST_SRX;

        /* wait for calibration */
        sleepMillis(2);

        /* store frequency/calibration settings */
        chan_table[ch].freq2 = FREQ2;
        chan_table[ch].freq1 = FREQ1;
        chan_table[ch].freq0 = FREQ0;
        chan_table[ch].fscal3 = FSCAL3;
        chan_table[ch].fscal2 = FSCAL2;
        chan_table[ch].fscal1 = FSCAL1;

        RFST = RFST_SIDLE;
}

/* Calibrate the 900 MHz Radio */
void radio_calibration(void) {
    u32 min_hz;
    u8 chan;

    /* doing everything in Hz from here on */
    min_hz = MIN_900 * 1e6;

    for (chan = 0; chan <= NUM_CHANNELS; chan++){
        calibrate_freq((min_hz + (HZ_CHAN_SPACE * chan)),chan);
    }
    
    // Long flash to show we are running
    //led_tick(1,PAUSE);
}

#define UPPER(a, b, c)  ((((a) - (b) + ((c) / 2)) / (c)) * (c))
#define LOWER(a, b, c)  ((((a) + (b)) / (c)) * (c))

/* tune the radio using stored calibration */
void tune(u8 ch) {
    FREQ2 = chan_table[ch].freq2;
    FREQ1 = chan_table[ch].freq1;
    FREQ0 = chan_table[ch].freq0;

    FSCAL3 = chan_table[ch].fscal3;
    FSCAL2 = chan_table[ch].fscal2;
    FSCAL1 = chan_table[ch].fscal1;
}

/* Toggle Mode for RESET */
void toggle_mode(){
    mode ^= 1;
    if (mode){
        debug("toggle_mode on");
    } else {
        debug("toggle_mode off");
    }
}

/* Toggle TXDATA */
/*
    This function controls if whether or not the client
    is ready to receive in coming data. Until it tells is
    so we need to hold off sending any data.  We may or 
    may not want to process any information
*/
void toggle_txdata(){
    txdata_rdy ^= 1;
    if (txdata_rdy){
        debug("toggle_txdata on");
    } else {
        debug("toggle_txdata off");
    }
}

void appMainLoop(){

    u8 ch;
    u16 i;

    while (1) {

        /// process usb events each loop
        usbProcessEvents();
       
        // If somebody is pressing the reset button then...well...reset
        if (BUTTON == LOW){
            // Set Reset
            toggle_mode();
            txdata_rdy = 0;
        }

        /* Reset everything */
        if (mode == 0){
            //reset:
            min_chan = 0;
            //max_chan = NUM_CHANNELS - 1;
            max_chan = NUM_CHANNELS;
            temp_ss = (xdata u8*)&ss[0];

            radio_setup();
            radio_calibration();

            toggle_mode();
            //txdata_rdy = 0;
        }

        if (txdata_rdy){
            //REALLYFASTBLINK();


            for (ch = min_chan; ch < max_chan; ch++) {

                /* tune radio */
                tune(ch);
                //sleepMillis(config_interval);
                // Start RX
                RFST = RFST_SRX;
                sleepMillis(2);

                // read RSSI
                *temp_ss++ = (RSSI ^ 0x80);

                /* end RX */
                RFST = RFST_SIDLE;

            }

            //debug("txdata send");
            txdata(SPECAN_APP, SPECAN_DATA, NUM_CHANNELS, (xdata u8*)&ss[0]);
            temp_ss = (xdata u8*)&ss[0];

            //sleepMillis(50);

        }else{
            blink(30,30);
            //REALLYFASTBLINK();
        }
    }

}/* End appMainLoop */

/* appMainInit() is called *before Interrupts are enabled* for various initialization things. */
void appMainInit(void)
{
}

/* appHandleEP5 gets called when a message is received on endpoint 5 from the host.  this is the 
 * main handler routine for the application as endpoint 0 is normally used for system stuff.
 *
 * important things to know:
 *  * your data is in ep5iobuf.OUTbuf, the length is ep5iobuf.OUTlen, and the first two bytes are
 *      going to be \x40\xe0.  just craft your application to ignore those bytes, as i have ni
 *      puta idea what they do.  
 *  * transmit data back to the client-side app through txdatai().  this function immediately 
 *      xmits as soon as any previously transmitted data is out of the buffer (ie. it blocks 
 *      while (ep5iobuf.flags & EP_INBUF_WRITTEN) and then transmits.  this flag is then set, and 
 *      cleared by an interrupt when the data has been received on the host side.                */
int appHandleEP5()
{   // not used by VCOM
#ifndef VIRTUAL_COM
    u8 app, cmd;
    u16 len;
    xdata u8 *buf;

    app = ep5iobuf.OUTbuf[4];
    cmd = ep5iobuf.OUTbuf[5];
    buf = &ep5iobuf.OUTbuf[6];
    len = (u16)*buf;
    buf += 2;                                               // point at the address in memory
    // ep5iobuf.OUTbuf should have the following bytes to start:  <app> <cmd> <lenlow> <lenhigh>
    // check the application
    //  then check the cmd
    //   then process the data
    switch (cmd)
    {
        case SPECAN_TOGGLE:
            /* Toggle whether or not we are ready to receive. */
            /* Also perform a reset to start at the beginning. */
            toggle_txdata();
            toggle_mode();
            txdata(app, cmd, 1, '\x00');
            break;
        default:
            break;
    }
    ep5iobuf.flags &= ~EP_OUTBUF_WRITTEN;                       // this allows the OUTbuf to be rewritten... it's saved until now.
#endif
    return 0;
}

/* in case your application cares when an OUT packet has been completely received on EP0.       */
void appHandleEP0OUTdone(void)
{
}

/* called each time a usb OUT packet is received */
void appHandleEP0OUT(void)
{
#ifndef VIRTUAL_COM
    u16 loop;
    xdata u8* dst;
    xdata u8* src;

    // we are not called with the Request header as is appHandleEP0.  this function is only called after an OUT packet has been received,
    // which triggers another usb interrupt.  the important variables from the EP0 request are stored in ep0req, ep0len, and ep0value, as
    // well as ep0iobuf.OUTlen (the actual length of ep0iobuf.OUTbuf, not just some value handed in).

    // for our purposes, we only pay attention to single-packet transfers.  in more complex firmwares, this may not be sufficient.
    switch (ep0req)
    {
        case 1:     // poke
            
            src = (xdata u8*) &ep0iobuf.OUTbuf[0];
            dst = (xdata u8*) ep0value;

            for (loop=ep0iobuf.OUTlen; loop>0; loop--)
            {
                *dst++ = *src++;
            }
            break;
    }

    // must be done with the buffer by now...
    ep0iobuf.flags &= ~EP_OUTBUF_WRITTEN;
#endif
}

/* this function is the application handler for endpoint 0.  it is called for all VENDOR type    *
 * messages.  currently it implements a simple debug, ping, and peek functionality.              *
 * data is sent back through calls to either setup_send_ep0 or setup_sendx_ep0 for xdata vars    *
 * theoretically you can process stuff without the IN-direction bit, but we've found it is better*
 * to handle OUT packets in appHandleEP0OUTdone, which is called when the last packet is complete*/
int appHandleEP0(USB_Setup_Header* pReq)
{
#ifdef VIRTUAL_COM
    pReq = 0;
#else
    if (pReq->bmRequestType & USB_BM_REQTYPE_DIRMASK)       // IN to host
    {
        switch (pReq->bRequest)
        {
            case 0:
                setup_send_ep0(&lastCode[0], 2);
                break;
            case 1:
                setup_sendx_ep0((xdata u8*)USBADDR, 40);
                break;
            case 2:
                setup_sendx_ep0((xdata u8*)pReq->wValue, pReq->wLength);
                break;
            case 3:     // ping
                setup_send_ep0((u8*)pReq, pReq->wLength);
                break;
            case 4:     // ping
                setup_sendx_ep0((xdata u8*)&ep0iobuf.OUTbuf[0], 16);//ep0iobuf.OUTlen);
                break;
        }
    }
#endif
    return 0;
}



/*************************************************************************************************
 *  here begins the initialization stuff... this shouldn't change much between firmwares or      *
 *  devices.                                                                                     *
 *************************************************************************************************/

static void appInitRf(void)
{
}

/* initialize the IO subsystems for the appropriate dongles */
static void io_init(void)
{
#ifdef IMMEDONGLE   // CC1110 on IMME pink dongle
    // IM-ME Dongle.  It's a CC1110, so no USB stuffs.  Still, a bit of stuff to init for talking 
    // to it's own Cypress USB chip
    P0SEL |= (BIT5 | BIT3);     // Select SCK and MOSI as SPI
    P0DIR |= BIT4 | BIT6;       // SSEL and LED as output
    P0 &= ~(BIT4 | BIT2);       // Drive SSEL and MISO low

    P1IF = 0;                   // clear P1 interrupt flag
    IEN2 |= IEN2_P1IE;          // enable P1 interrupt
    P1IEN |= BIT1;              // enable interrupt for P1.1

    P1DIR |= BIT0;              // P1.0 as output, attention line to cypress
    P1 &= ~BIT0;                // not ready to receive
    
#else       // CC1111
#ifdef DONSDONGLES
    // CC1111 USB Dongle
    // turn on LED and BUTTON
    P1DIR |= 3;
    // Activate BUTTON - Do we need this?
    //CC1111EM_BUTTON = 1;

#else
    // CC1111 USB (ala Chronos watch dongle), we just need LED
    P1DIR |= 3;

#endif      // CC1111

#endif      // conditional config


#ifndef VIRTUAL_COM
    // Turn off LED
    LED = 0;
#endif
}


void clock_init(void){
    //  SET UP CPU SPEED!  USE 26MHz for CC1110 and 24MHz for CC1111
    // Set the system clock source to HS XOSC and max CPU speed,
    // ref. [clk]=>[clk_xosc.c]
    SLEEP &= ~SLEEP_OSC_PD;
    while( !(SLEEP & SLEEP_XOSC_S) );
    CLKCON = (CLKCON & ~(CLKCON_CLKSPD | CLKCON_OSC)) | CLKSPD_DIV_1;
    while (CLKCON & CLKCON_OSC);
    SLEEP |= SLEEP_OSC_PD;
    while (!IS_XOSC_STABLE());
}

void initBoard(void)
{
    clock_init();
    io_init();
}


/*************************************************************************************************
 * main startup code                                                                             *
 *************************************************************************************************/
void main (void)
{
    /* Initialize board and USB interface */
    initBoard();
    initUSB();

    /* init_RF();  we don't need usb RF for this */
    appMainInit();

    /* Start USB */
    usb_up();

    /* Enable interrupts */
    EA = 1;

    /* Wait for USB setup to complete */
    waitForUSBsetup();

    while (1)
    {  
        /* keeps USB alive */
        usbProcessEvents();

        /* Go do what we are here to do */
        appMainLoop();
    }
}


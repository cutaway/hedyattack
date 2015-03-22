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

#define NUM_CHANNELS_53

#include <cc1110.h>
#include "types.h"
#include "ioCCxx10_bitdef.h"
#include "setup.h"
#include "stdio.h"
#include "string.h"
#include "pm.h"
#include "bits.h"
#include "cc1111emk.h"

/* globals */
/*
---------------------------------------------------------------------
|   Mini Scan XDATA Breakdown                                       | 
---------------------------------------------------------------------
|   XDATA == 3596 bytes                                             | 
|   0xF000 thru 0xFFFF                                              |
---------------------------------------------------------------------
| channel_table* | channel_mins | channel_modes | loop cnt | Extra  |
| 53 x 6 = 318   |      53      |       3000    |   2      | Data   |
|     bytes      |      bytes   |       bytes   |   bytes  | Buffer |
---------------------------------------------------------------------
------------------------------------------------------------------------------
| Channel Modes                                                              |
------------------------------------------------------------------------------
| Marker | MSB Loop | LSB Loop | channel | channel | ... | channel | channel |
| 0xFE   | Count    | Count    | number  | rssi    | ... | number  | rssi    |
------------------------------------------------------------------------------
* Does not include RSSI and MAX_RSSI data
From cc1111emk-minscan.map

#######################
Hexadecimal

Area                               Addr   Size   Decimal Bytes (Attributes)
--------------------------------   ----   ----   ------- ----- ------------
XSEG                               F000   0F30 =   3888. bytes (REL,CON,XDATA)

      Value  Global
   --------  --------------------------------
  0D:F000    _chan_table
  0D:F13E    _chan_mins
  0D:F173    _chan_mods
  0D:FF1F    _chan_loop
#######################

*/
xdata channel_info chan_table[NUM_CHANNELS];
xdata channel_mins chan_mins[NUM_CHANNELS];
//xdata u8 chan_mins[NUM_CHANNELS];
// Set this fixed size so we know where data is located 
// for access via debugger.  
// Plus, we don't want to overrun XDATA
#define MAX_MODS 3000
// Channel marker separates passes by 0xFE or 254
#define CHAN_MARK 254
xdata u8 chan_mods[MAX_MODS];
xdata u16 chan_loop;
xdata u8 appstatus;

#define SCAN_MASK 0
#define RESET_MASK BIT4
#define CONF_MINS_MASK BIT5
#define TRANS_USB_MASK BIT6

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
    //FREQ2 = 0x26;
    //FREQ1 = 0x55;
    //FREQ0 = 0x55;
    
    //Data Rate
    // 270.833333 kHz: 0xE5; 0xA3; 0x10; 0x23; 0x11; 
    // 499.511719 kHz: 0xEE; 0x55; 0x73; 0x23; 0x55;
    MDMCFG4 = 0x3E; // 500 kBaud Data Rate + (500 kHz channel * .8 filter space)
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
    min_hz = MIN_900 * 1000000;

    for (chan = 0; chan <= NUM_CHANNELS; chan++){
        calibrate_freq((min_hz + (HZ_CHAN_SPACE * chan)),chan);
    }
    
    // Long flash to show we are running
    //led_tick(1,PAUSE);
}

/* tune the radio using stored calibration */
void tune(u8 ch) {
    FREQ2 = chan_table[ch].freq2;
    FREQ1 = chan_table[ch].freq1;
    FREQ0 = chan_table[ch].freq0;

    FSCAL3 = chan_table[ch].fscal3;
    FSCAL2 = chan_table[ch].fscal2;
    FSCAL1 = chan_table[ch].fscal1;
}

void main(void) {

    //delays
    u16 action_pause;
    u16 sh_action_pause;
    u16 config_interval;

    // Counters
    u8 mkd = 0;
    u16 conf_loop = 0;
    u16 chan_loop_cnt = 0;
    u16 chan_mods_cnt = 0;
    u16 chan_reset_cnt = 0;
    
    // Channel data
    u8 min_diff = 70;

    // channel storage
    u8 ch = 0;
    u8 *chan_mods_ptr = 0;
    u8 chan_rssi = 0;

    u8 min_chan;
    u8 max_chan;

    xtalClock();
    setIOPorts();
    radio_setup();

    chan_loop_cnt = 0;  //Total number of times we have been through

    //delays
    action_pause = 50;
    sh_action_pause = 10;
    config_interval = 5;

    // Number of channels
    min_chan = 0;
    max_chan = NUM_CHANNELS; // shouldn't subtract one because we want 0 thru 52
                

    // Initial setup uses RESET
    appstatus |= RESET_MASK;

    while (1) {
       
        // If somebody is pressing the reset button then...well...reset
        if (BUTTON == LOW){
            // Set Reset
            appstatus |= RESET_MASK;
        }

        switch (appstatus & 0xf0){
            // Initialize or Reset radio
            case RESET_MASK:
            //case '1':

                // Long flash to show we are resetting
                led_tick(1,LG_PAUSE);

                // Calibrate the radio
                radio_calibration();

                //Total number of bytes written to chan_mods array
                chan_mods_cnt = 0;  

                // sets up pointer to chan_mod memory location
                chan_mods_ptr = &chan_mods[0];
                // TODO: memset is broken - why?
                //memset(chan_mods_ptr,0xFF,sizeof(chan_mods));
                for (chan_reset_cnt = 0; chan_reset_cnt < MAX_MODS; chan_reset_cnt++){
                    *chan_mods_ptr = 0xFF;
                    (chan_mods_ptr)++;
                }
                chan_mods_ptr = &chan_mods[0];

                // Reset reset
                appstatus &= (~RESET_MASK);
                // Set Config Mins
                appstatus |= CONF_MINS_MASK;

                break;

            // Grab the minimum RSSI value for all channels
            case CONF_MINS_MASK:
            //case '2':

                // Reset
                for (ch = min_chan; ch < max_chan; ch++) {
                    chan_mins[ch].min = 0xFF;
                }

                //Loop Through and determine typical minimum value
                for (conf_loop = 0; conf_loop < config_interval; conf_loop++) {
                    for (ch = min_chan; ch < max_chan; ch++) {

                        /* tune radio */
                        tune(ch);
                        // Start RX
                        RFST = RFST_SRX;
                        //sleepMillis(sh_action_pause);
                        sleepMillis(2);

                        // Find minimum value of RSSI for each channel and store
                        chan_mins[ch].min = MIN((RSSI ^ 0x80), chan_mins[ch].min);

                        // Quick flash to show we are configuring mins
                        led_tick(1,SXSH_PAUSE);

                        /* end RX */
                        RFST = RFST_SIDLE;
                        
                        // Sleep necessary to allow cc1111 to sync
                        sleepMillis(sh_action_pause);
                    }
                }

                // Set up chan_mins so they include the minimum difference for detection
                // This lets us do less math later when speed counts
                for (ch = min_chan; ch < max_chan; ch++) {
                    chan_mins[ch].min = chan_mins[ch].min + min_diff;
                }

                // UnSet Config Mins
                appstatus &= (~CONF_MINS_MASK);
                
                break;

            // Spit data our USB
            case TRANS_USB_MASK:
            //case '4':

                // Set Reset
                appstatus &= (~TRANS_USB_MASK);
                appstatus |= RESET_MASK;

                break;

            // Default case is to SCAN BABY SCAN
            default:

                // Track number of times we have been through the channels
                chan_loop_cnt++;
                //chan_loop = (chan_loop_cnt >> 8) & 0xff;
                //chan_loop = chan_loop_cnt & 0xff;
                chan_loop = chan_loop_cnt;

                mkd = FALSE;
                for (ch = min_chan; ch < max_chan; ch++) {

                    /* tune radio */
                    tune(ch);
                    // Start RX
                    RFST = RFST_SRX;
                    //sleepMillis(sh_action_pause);

                    sleepMillis(2);

                    // Quick flash to show we are running
                    //led_tick(1,PAUSE);

                    // Test to see if current state is outside the defined range
                    chan_rssi = RSSI ^ 0x80;
                    if (chan_rssi > chan_mins[ch].min){
                        if (mkd == FALSE){
                            mkd = TRUE;
                            /*
                            ------------------------------------------------------------------------------
                            | Channel Modes                                                              |
                            ------------------------------------------------------------------------------
                            | Marker | MSB Loop | LSB Loop | channel | channel | ... | channel | channel |
                            | 0xFE   | Count    | Count    | number  | rssi    | ... | number  | rssi    |
                            ------------------------------------------------------------------------------
                            */
                            // TODO: Can we move this to the inner loop and do a quick test
                            // TODO: to see if the marker has been set.  This would save storage
                            // TODO: and probably help speed
                            // Add channel marker so we know where a pass end
                            *chan_mods_ptr = CHAN_MARK;
                            (chan_mods_ptr)++;
                            chan_mods_cnt++;
                            // Add the loop count - this eats space but helps us track
                            *chan_mods_ptr = (chan_loop_cnt >> 8) & 0xff;
                            (chan_mods_ptr)++;
                            chan_mods_cnt++;
                            *chan_mods_ptr = chan_loop_cnt & 0xff;
                            (chan_mods_ptr)++;
                            chan_mods_cnt++;
                        }

                        // Store channel number
                        *chan_mods_ptr = ch;
                        (chan_mods_ptr)++;
                        chan_mods_cnt++;

                        // Store channel channel's rssi
                        *chan_mods_ptr = chan_rssi;
                        (chan_mods_ptr)++;
                        chan_mods_cnt++;

                        // Short flash to show we have detected a change
                        led_tick(2,SXSH_PAUSE);
                    }

                    /* end RX */
                    RFST = RFST_SIDLE;
                    
                    // Quick flash to show we are running
                    //led_tick(1,XSH_PAUSE);

                    // Sleep necessary to allow cc1111 to sync
                    // Removed due to LED flash
                    //sleepMillis(sh_action_pause);
                }

                // Monitor to see if we have filled up the array
                // for modified channels and reset
                // Also monitor the number of times we have looped
                // through all of the channels as we don't want to overrun
                // or stop the marker
                if ( ((chan_mods_cnt + NUM_CHANNELS + 3) >= MAX_MODS) || (chan_loop_cnt == 0xFDFF) ){
                    // TODO: Transmit via USB or UART HERE
                    // Long flash to show we are resetting
                    led_tick(1,LG_PAUSE);
                    if (chan_loop_cnt == 0xFDFF){
                        chan_loop_cnt = 0;
                    }

                    // Set TRANS_USB
                    appstatus |= TRANS_USB_MASK;
                }

                break;
        }
    }
}

/*
 * Copyright 2010 atlas, cutaway, and q
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




// TODO:  make work with xhci_hcd on linux_x64  
//          "WARN: short transfer on control ep" 
//          "ERROR: unexpected command completion code 0x11." 
//          "can't set config #1, error -22"
// FIXME:   channel-spacing automated detection is still not working correctly
//
// TODO:    
// TODO:    predictive analysis versions 1 and 2:
//              wait on a channel and time whenever a hit occurs with the right sync word mark the time.
//              wait on a channel until hit, then systematically hop to another channel to see if the next packet occurs there.  wash rinse repeat 30-50 times per channel combination
//
//

#include "cc1111usb.h"
#include "hedyattack.h"
#include "xstorage.h"
#include <cc1110.h>
#include "ioCCxx10_bitdef.h"

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

//  from SmartRF Studio...
code u8 radio_modem_cfg = (0x0e, 0x3b, 0x73, 0x43, 0x3b, 0x00, 0xb6,0x11);

void setup_radio_modem(u8 *pCfg){
    //MDMCFG4 = (2<<6) | (0<<4) | (0xc);  // CHANBW_E, CHANBW_M, DRATE_E
    MDMCFG4 = *pCfg++;
    //MDMCFG3 = 0x22;                     // DRATE_M  such that Rate=(((256.+0x22)*(2**12))/(2**28)) * 26000000
    MDMCFG3 = *pCfg++;
    //MDMCFG2 = (0<<7) | (MOD_2FSK<<4) | (0<<3) | 2;  // DEM_DCFILE_OFF, MOD_FORMAT, MANCHESTER_EN, SYNC_MODE
    MDMCFG2 = *pCfg++;
    //MDMCFG1 = (0<<7) | (2<<4) | 2;      // FEC_EN, NUM_PREAMBLE, CHANSPC_E
    MDMCFG1 = *pCfg++;
    //MDMCFG0 = (0xf8);                   // CHANSPC_M such that deltaFchan = (Fref/2**18)*(256+CHANSPC_M)*(2**CHANSPC_E)
    MDMCFG0 = *pCfg++;
    //DEVIATN = (4<<4) | (7);             // DEVIATION_E, DEVIATION_M
    DEVIATN = *pCfg++;
    //FREND1 =  (1<<6) | (1<<4) | (1<<2) | 2; // LNA_CURRENT, LNA2MIX_CURRENT, LODIV_BUG_CURRENT_RX, MIX_CURRENT
    //FREND1      = 0xB6;         // SmartRF - 250kHz channels base at 902MHz
    FREND1 = *pCfg++;
    //FREND0 =  (1<<4) | (1);             // LODIV_BUF_CURRENT_TX, PA_POWER
    //FREND0      = 0x11;         // SmartRF - 250kHz channels base at 902MHz (|1 for PA_TABLE1 instead of PA_TABLE0)
    FREND0 = *pCfg++;
}

    // * no automatic frequency calibration 
    // * disable 3 highest DVGA settings 
code u8 radio_signal_cfg = (0x07,0x20,0x00, 0x1d,0x1c, 0xc7|AGCCTRL2_MAX_DVGA_GAIN,0x00,0xb0, 0x04, 0x05, 0xff, 0xaa,0xaa);
void setup_radio_signal(u8 *pCfg){
    //MCSM2 =   (0<<4) | (0<<3) | (7);    // RX_TIME_RSSI, RX_TIME_QUAL, RX_TIME
    MCSM2 = *pCfg++;
    //MCSM1 =   (3<<4) | (0<<2) | (0);    // CCA_MODE, RXOFF_MODE, TXOFF_MODE
    MCSM1 = *pCfg++;
    //MCSM0 =   (0<<4) | (1<<2) | (0);    // FS_AUTOCAL, CLOSE_IN_RX
    //MCSM0       = 0x08;
    MCSM0 = *pCfg++;
    //FOCCFG =  (1<<6) | (1<<5) | (2<<3) | (1<<2) | 2; // FOC_BS_CS_GATE/PRE_K/POST_K/LIMIT
    //FOCCFG      = 0x1d;


    FOCCFG = *pCfg++;
    //BSCFG =   (1<<6) | (2<<4) | (1<<3) | (1<<2) | 0; // BS_PRE_K/PRE_KP/POST_KI/POST_KP/LIMIT
    //BSCFG       = 0x1c;
    BSCFG = *pCfg++;
    //FSCTRL0     = 0x00;
    //AGCCTRL2 =(0<<6) | (0<<3) | (3);    // MAX_DVGA, MAX_LNA_GAIN, MAGN_TARGET
    //AGCCTRL2    = 0xC7;        // SmartRF - 250kHz channels base at 902MHz
    AGCCTRL2 = *pCfg++;
    //AGCCTRL1 =(1<<6) | (0<<4) | (0);    // AGC_LNA_PRIORITY, CARRIER_SENSE_REL_THR/ABS_THR
    //AGCCTRL1    = 0;           // SmartRF - 250kHz channels base at 902MHz
    AGCCTRL1 = *pCfg++;
    //AGCCTRL0 =(2<<6) | (1<<4) | (0<<2) | 1; // HYST_LEVEL, WAIT_TIME, AGC_FREEZE, FILTER_LENGTH
    //AGCCTRL0    = 0xb0;        // SmartRF - 250kHz channels base at 902MHz
    AGCCTRL0 = *pCfg++;
    //PKTCTRL1 = (0<<5) | (0<<2) | 0; //PQT, APPEND_STATUS, ADR_CHK
    //PKTCTRL1    = 0x04;        // SmartRF - 250kHz channels base at 902MHz
    PKTCTRL1 = *pCfg++;
    //PKTCTRL0 = (0<<6) | (0<<4) | (0<<2) | 0;    // WHITE_DATA, PKT_FORMAT, CRC_EN, LENGTH_CONFIG
    //PKTCTRL0    = 0x05;        // SmartRF - 250kHz channels base at 902MHz
    PKTCTRL0 = *pCfg++;
    PKTLEN      =0xFF;    // hack - this is just so we get the first few bytes
    PKTLEN = *pCfg++;
    //SYNC0       =0xaa;     // hack - same as 10101010
    SYNC0 = *pCfg++;
    //SYNC1       =0xaa;
    SYNC1 = *pCfg;
}

code u8 radio_freq_cfg = (0x40,0xc0, 0x22,0xb1,0x3b,0x00);
void setup_radio_freq_cfg(u8 *pCfg) {
    // Frequency Settings

    // PA_TABLE0   = 0x40;   // Low power (Bit0) in OOK mode
    PA_TABLE0 = *pCfg++;
    // PA_TABLE1   = 0xC0;   // Power Setting = 10db and Bit1 in OOK     -- switching from 0xc0 to 0x8e helped
    PA_TABLE1 = *pCfg++;
    //FREQ2       = 0x22;   // 902000000 / 396.728515625 == 0x22b13b
    FREQ2 = *pCfg++;
    //FREQ1       = 0xb1;
    FREQ1 = *pCfg++;
    //FREQ0       = 0x3b;
    FREQ0 = *pCfg++;
    //CHANNR      = 0;     // CHANNR is multiplied by channel spacing and added to base freq
    CHANNR = *pCfg++;
}

void setup_radio_freq_cal(u8 *pCfg)
{
    //setup_radio_250kHz();

    //setModulation(MOD_MSK);
    //current_modulation = 3;

    // * frequency synthesizer calibration 
    //FSCAL3      = 0xEA;
    FSCAL3 = *pCfg++;
    //FSCAL2      = 0x2A;
    FSCAL2 = *pCfg++;
    //FSCAL1      = 0x00;
    FSCAL1 = *pCfg++;
    //FSCAL0      = 0x1F;
    FSCAL0 = *pCfg++;

    // * "various test settings" 
    //TEST2       = 0x88;
    TEST2 = *pCfg++;
    //TEST1       = 0x31;
    TEST1 = *pCfg++;
    //TEST0       = 0x09;
    TEST0 = *pCfg++;


}

/* freq in Hz */
void calibrate_freq(volatile  u32 freq, u8 ch) {
        /* the frequency setting for CC1110 is in units of 396.728515625 Hz */
        /* the frequency setting for CC1111 is in units of 366.2109375 Hz */
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
    u16 chan;

    /* doing everything in Hz from here on */
    min_hz = MIN_900 * 1000000;

    for (chan = 0; chan <= NUM_CHANNELS; chan++){
        calibrate_freq((min_hz + (chan_spacing * chan)),chan);
        //usbProcessEvents();
    }
    
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

    // Counters
    u16 chan_reset_cnt=0;
    u8 chan_cnt=0;
    
    // Channel data
    u8 ch=0;
    u8 chan_diff=0;
    u8 chan_rssi=0;

    // channel storage
    channel_hop *chan_hops_ptr=0;

    // channel analysis 
    u8 chans_per_loop=0;
    u16 max_hit_cnt=0;
    u8 chan_width=0;
    u16 cur_hit_cnt=0;
    u8 rssi_tmp=0;
    u8 start_of_peak=0;

    // Number of channels to start with
    u8 min_chan=0;

    chan_loop_cnt=0;

    chan_hops_cnt=0;
    next_mode_cnt=0;

    initBoard();
    initUSB();

    radio_setup();

    // let usb configure and settle
    waitForUSBsetup();
    //while (1) usbProcessEvents();

    // Start in Reset Mode
    mode = MODE_RESET;

    while (1) 
    {
        if (chans_per_loop > 8)
            pos_chan_threshold++;
        chans_per_loop               = 0;
       
        // If somebody is pressing the reset button then...well...reset
        if (BUTTON == LOW){
            // Set Reset
            mode = MODE_RESET;
        }

        /// process usb events each loop
        usbProcessEvents();

        // Initialize or Reset radio
        if (mode== MODE_RESET)
        {
            // Long flash to show we are resetting
            blink(LG_PAUSE,LG_PAUSE);
            // Counters
            chan_reset_cnt          = 0;
            chan_cnt                = 0;
            
            // Channel data
            chan_diff               = 0;
            ceiling                 = 0;
            floor                   = 0xff;
            //pos_chan_threshold      = 0;  // set this later automatically, or manually, before use

            // channel storage
            ch                      = 0;
            chan_rssi               = 0;
            chan_hops_ptr           = 0;

            // channel analysis 
            max_hit_cnt             = 0;
            chan_width              = 0;
            cur_hit_cnt             = 0;
            start_of_peak           = 0;

            // Number of channels to start with
            min_chan                = 0;

            chan_loop_cnt           = 0;    //Total number of times we have been through

            appstatus               = 0;
            chan_hops_cnt           = 0;
            next_mode_cnt           = 0;
            lastCode[0]             = 0;
            lastCode[1]             = 0;


            // Track number of times we have been through the channels

            // reset the channel spacing upon reset
            //chan_spacing = CHAN_SPACE / chan_num;
            chan_spacing = 50000;                   // let's start here.  SCAN_CHANS will identify the real spacing using this spacing...
            chan_num = NUM_CHANNELS;                // start at MAX channels.  SCAN_CHANS will reduce this number so SCAN_HOPS can only hit the *real* channels

            // Reset Channel RSSI Floor
            //mode = MODE_INIT_FLOOR;                 // FIXME: should we go to MODE_LAZY and wait for the client to kick off operations?  yeah, for now.
            //mode = MODE_INIT_FLOOR;                 //  FIXME: this is the manual way of operating the tool
            mode = nextMode[MODE_RESET];
        }
        chan_loop_cnt++;

        switch (mode)
        {
            case MODE_LAZY:
                lastCode[0] = 0x3f;
                next_mode_cnt = 0;                    // if 0, the init will set this
                break;
            // Grab the minimum RSSI value for all channels
            case MODE_INIT_FLOOR:
                lastCode[0] = 0x40;

                // Calibrate all radio channels
                radio_calibration();

                // Set up chan_str so they start maxed out
                for (chan_cnt = min_chan; chan_cnt < chan_num; chan_cnt++) {
                    chan_str[chan_cnt] = 0xff;
                    chan_maxs[chan_cnt] = 0;
                }

                if (next_mode_cnt==0)
                    next_mode_cnt = REPS_SCAN_FLOOR;
                chan_loop_cnt = 0;
                mode = MODE_SCAN_FLOOR;
                // no break, just carry into the next case and start the next mode
                // cause we can grab mins too and save a pass
            case MODE_SCAN_FLOOR:
                //Loop Through channels for each thing.
                // FIXME: I think this should be <=, fix if not - cutaway
                for (ch = min_chan; ch <= chan_num; ch++) 
                {

                    // make sure we don't lose usb  (FIXME:  should we just have this on the timer?)
                    usbProcessEvents();

                    /* tune radio */
                    tune(ch);
                    RFST = RFST_SRX;
                    sleepMicros(200);

                    // Find minimum value of RSSI for each channel and store
                    //    smart floor finder: find the top of the floor

                    chan_rssi = (RSSI ^ 0x80);                  // why is this ^0x80?

                    if (ceiling < chan_rssi)
                        ceiling = chan_rssi+1;
                    else if (floor > chan_rssi)
                        floor = chan_rssi;

                    rssi_tmp = chan_str[ch];
                    if (chan_rssi > rssi_tmp)       // which is bigger?  rssi?
                    {       //rssi is bigger
                        if ((chan_rssi - rssi_tmp) < MINSCAN_MAX_THRESHOLD){       // don't change if *much* higher, only nudge the floor  
                            chan_str[ch] = chan_rssi;   // if the increase in rssi is below the threshold, we must have found a higher floor.
                        }
                    } else {    // previous min is bigger
                        if ((rssi_tmp - chan_rssi) > MINSCAN_MIN_THRESHOLD){
                            chan_str[ch] = chan_rssi;   // if the drop in rssi from previous mins is > threshold, we must have found a more floory floor.
                        }
                    }

                    if (chan_rssi < chan_str[ch]){
                        chan_str[ch] = chan_rssi;
                    }

                    if (chan_rssi > chan_str[ch])
                    {
                        if ((chan_rssi - chan_str[ch]) > MINSCAN_MAX_THRESHOLD)
                            chan_str[ch] = chan_rssi;  // if the drop in rssi from previous mins is > threshold, we must have found a more floory floor.
                    } else {
                        if ((chan_str[ch] - chan_rssi) < MINSCAN_MIN_THRESHOLD)
                            chan_str[ch] = chan_rssi;  // if the increase in rssi is below the threshold, we must have found a higher floor.

                    }

                    // also do maxes in this run
                    if ((u16) chan_rssi > chan_maxs[ch]){      // simple max will do here. FIXME: should we do an average max above a certain threshold?
                        chan_maxs[ch] = (u16) chan_rssi;
                    }

                    // We are done determining floor and ceiling, transmit and move on
                    if (chan_loop_cnt >= next_mode_cnt)
                    {
                        // FIXME: determine median of min and max for each channel, and store that in mins
                        //txdata(2, 2, 4, "hi!!");
                        debug("sending mins");
                        txdata(2, DATA_MINS, chan_num-min_chan, &chan_str[0]);
                        debug("sending maxs");
                        txdata(2, DATA_MAXS, (chan_num-min_chan), &chan_maxs[0]);
                        debug("modding mins to be threshold");
                        /*for (chan_reset_cnt=0; chan_reset_cnt<chan_num; chan_reset_cnt++)
                        {
                            rssi_tmp = chan_str[chan_reset_cnt];
                            if (rssi_tmp == 0xff || chan_maxs[chan_reset_cnt] == 0)
                            {
                                debug("continuing SCAN_MINS, something's not done");
                                break;      // if we still have unmodified channel info, we obiously are not done in this phase.
                            }
                            large_num += rssi_tmp + (((u8)chan_maxs[chan_reset_cnt] - rssi_tmp+5)/2);
                        }
                        pos_chan_threshold = (u8)(large_num / chan_num);
                        */
                        pos_chan_threshold = floor + ((ceiling-floor)*7/8);

                        debug("sending threshold");
                        //txdata(2, DATA_THRESHOLDS, chan_num, (u8*)&chan_str[0]);
                        txdata(2, DATA_THRESHOLD, 1, (u8*)&pos_chan_threshold);


                        //blink(SH_PAUSE,SH_PAUSE);       // let 'em know we're done
                        next_mode_cnt = 0;                  // if 0, the init will set this
                        mode = nextMode[MODE_SCAN_FLOOR];
                        //mode = MODE_LAZY;
                    }
                    RFST = RFST_SIDLE;
                }
                break;
                
            // Analyze Channel Spacing
            case MODE_INIT_CHANS:
                lastCode[0] = 0x41;
                // reinitialize the chan_maxs array.  it should currently have 'maxes' which are no longer necessary.  it's about to hold hit-counts
                for (chan_cnt=min_chan+chan_width; chan_cnt<chan_num; chan_cnt++){
                    chan_maxs[chan_cnt] = 0;
                }
                radio_calibration();
                next_mode_cnt = REPS_SCAN_CHANS;
                chan_loop_cnt = 0;
                start_of_peak = 0;
                mode = MODE_SCAN_CHANS;
                // no break, just carry into the next case and start the next mode
            case MODE_SCAN_CHANS:
                //Loop Through channels for each thing.
                // FIXME: I think this should be <=, fix if not - cutaway
                for (ch = min_chan; ch <= chan_num; ch++) 
                {
                    // make sure we don't lose usb  (FIXME:  should we just have this on the timer?)
                    usbProcessEvents();

                    /* tune radio */
                    tune(ch);
                    RFST = RFST_SRX;
                    sleepMicros(200);

                    // debugging, "training" the tool
                    //if (ch==0){
                    //    txdata(2, DATA_CHAN_RSSI, (chan_num-min_chan), (u8*)&chan_str[0]);
                    //}

                    // when we're done here, we will be able to determine the likeliest channel spacing
                    chan_rssi = (RSSI ^ 0x80);                  // why is this ^0x80?
                    chan_str[ch] = chan_rssi;
                    if (chan_rssi > chan_maxs[ch]) {
                        chan_maxs[ch] = chan_rssi;
                    }

                    if (chan_rssi > pos_chan_threshold)           // is rssi above the threshold for this channel?
                    {
                        if (start_of_peak==0) {
                            start_of_peak = ch;
                            debug("===start of peak===");
                            debughex(chan_rssi);
                        }

                    } else {
                        if (start_of_peak > 0) {
                            // just dropped below threshold.  split the difference to find the channel center
                            chan_diff = ch - (u8)((ch - start_of_peak)/2);            // current channel - half the distance between start and end threshold
                            // now increment the center counter.
                            if (chan_maxs[chan_diff]<0xff)
                                chan_maxs[chan_diff]++;
                            debug("==start of peak");
                            debughex(start_of_peak);
                            debug("==middle of peak");
                            debughex(chan_diff);
                            debug("==end of peak");
                            debughex(ch);
                            debug("===end of peak===");
                            start_of_peak = 0;

                        }
                    }

                    // exit strategy...  (analysis)
                    if (chan_loop_cnt >= next_mode_cnt)
                    {
                        //blink(SH_PAUSE,SH_PAUSE);       // let 'em know we're done
                        debug("sending chanspc0");
                        txdata(2, DATA_CHANMAX, (chan_num-min_chan)*2, (u8*)&chan_maxs[0]);
                        chan_loop_cnt = 0;
                        next_mode_cnt = 0;                    // if 0, the init will set this
                        mode = MODE_LAZY;
                    }

                    /******************* THIS CODE IS FUCKED UP ************************
                    // Track how many times we have seen a change
                    if (chan_rssi > (chan_str[ch]))             // current rssi is over the threshold, thus we call it a hit
                        chan_maxs[ch]++;

                    // exit strategy...  (analysis)
                    if (chan_loop_cnt >= next_mode_cnt)
                    {
                        //blink(SH_PAUSE,SH_PAUSE);       // let 'em know we're done
                        debug("sending chanspc0");
                        txdata(2, DATA_CHANMAX, (chan_num-min_chan)*2, (u8*)&chan_maxs[0]);

                        // analyze and reduce channels for HOPS... or should that be done in INIT_HOPS?
                        //
                        // at our minimum, 50khz should be able to identify 1Mhz channels at 20 slots apart.
                        // loop 1-24 and watch for hitwidth
                        // track count of 1-width channels?
                        // otherwise just grab the greatest incidents of each spacing
                        max_hit_cnt = 0;
                        max_width   = 0;
                        cur_hit_cnt = 0;
                        chan_loop_cnt = 0;
                        chan_rssi = 0;
                        for (chan_width=2; chan_width < MAX_CHAN_WIDTH; chan_width++)
                        {
                            for (chan_cnt=min_chan+chan_width; chan_cnt<chan_num; chan_cnt+=chan_width) // skip through channels chan_width between, and look for peak
                            {
            //usbProcessEvents();
                                if (chan_maxs[chan_cnt] > chan_maxs[chan_cnt-1] && chan_maxs[chan_cnt] > chan_maxs[chan_cnt+1])
                                {
                                    cur_hit_cnt++;
                                }
                                //

                                if (chan_maxs[chan_cnt] > (CHAN_MIN_HITS) && chan_maxs[chan_cnt] < (CHAN_MAX_HITS)
                                    && (chan_maxs[chan_cnt-chan_width] > (CHAN_MIN_HITS) && chan_maxs[chan_cnt-chan_width] < (CHAN_MAX_HITS)))

                                //if (chan_maxs[chan_cnt] > (CHAN_MIN_HITS) && chan_maxs[chan_cnt] < (CHAN_MAX_HITS)
                                //    && (chan_maxs[chan_cnt-chan_width] > (CHAN_MIN_HITS) && chan_maxs[chan_cnt-chan_width] < (CHAN_MAX_HITS)))
                                    // cross the threshold to be considered more than just annoying noise
                                    // but no so many hits as to be considered solid noise.
                                    // do this for both the current channel and the one "chan_width" previous.
                                    // if both match, increment the width counter
                                {
                                    cur_hit_cnt++;
                                }//

                            }

                            if (cur_hit_cnt>max_hit_cnt)
                            {
                                if ((cur_hit_cnt - max_hit_cnt) < (cur_hit_cnt>>3))
                                {
                                    max_hit_cnt = cur_hit_cnt;
                                    max_width = chan_width;
                                }
                            }

                        }
                        if (max_width==1)                   // our best channel spacing is 1 width apart.
                        {
                            debug("sending chanspc1");
                            txdata(2, DATA_CHANSPC, 4, (u8*)&chan_spacing);
                            mode = MODE_INIT_HOPS;
                        } else {
                            if (max_width==0){
                                debug("wtf, max_width is 0!");
                                mode = MODE_INIT_CHANS;
                                break;
                            }
                            chan_spacing *= max_width;
                            if (chan_spacing > CHAN_SPACE){
                                chan_spacing = 50000;
                            }
                            debughex32(max_width);
                            mode = MODE_INIT_FLOOR;          // once again to the breach!  let's do this again.
                            debug("sending chanspc2");
                            txdata(2, DATA_CHANSPC, 4, (u8*)&chan_spacing);
                            blink(10,100);
                            mode = MODE_SCAN_CHANS;

                        }
                        chan_loop_cnt = 0;
                    }*/
                    RFST = RFST_SIDLE;
                }
                break;

            case MODE_INIT_HOPS:
                lastCode[0] = 0x42;
                //Total number of bytes written to chan_hops array
                chan_hops_cnt = 0;  

                // sets up pointer to chan_mod memory location
                chan_hops_ptr = &chan_hops[0];

                for (chan_reset_cnt = 0; chan_reset_cnt < MAX_HOPS; chan_reset_cnt++){
                    chan_hops_ptr->loop = 0xFFFF;
                    chan_hops_ptr->chan = 0xFF;
                    chan_hops_ptr->rssi = 0xFF;
                    (chan_hops_ptr)++;
                }
                chan_hops_ptr = &chan_hops[0];

                radio_calibration();
                chan_loop_cnt = 0;
                mode = nextMode[MODE_INIT_HOPS];
                //pos_chan_threshold = 0x88;

            //  SCAN BABY SCAN
            case MODE_SCAN_HOPS:
                //Loop Through channels for each thing.
                // FIXME: I think this should be <=, fix if not - cutaway
                for (ch = min_chan; ch <= chan_num; ch++) 
                {
                    // make sure we don't lose usb  (FIXME:  should we just have this on the timer?)
                    usbProcessEvents();

                    /* tune radio */
                    tune(ch);
                    RFST = RFST_SRX;
                    sleepMicros(200);

                    // channel identification will take a threshold, and identifying *the middle* of where the rssi values cross over the threshold.

                    // Test to see if current state is outside the defined range
                    chan_rssi = RSSI ^ 0x80;

                    if (chan_rssi > pos_chan_threshold)             // is rssi above the threshold for this channel?  (foobar  FIXME: make one threshold for the range)
                    {
                        //debug("over threshold");
                        //debughex(ch);
                        //debughex(chan_rssi);
                        //debughex(threshold);
                        if (start_of_peak==0) {                     // if we are just crossing the threshold on this peak
                            start_of_peak = ch;
                        } else {
                            if (chan_rssi > rssi_tmp){              // now we track the peak rssi value for this channel hit
                                rssi_tmp = chan_rssi;
                            }
                            else 
                            {
                                //  
                                debughex(rssi_tmp);
                            }
                        }

                    } else {                            // if we're lower than the threshold...  
                        //debug("under threshold");
                        //debughex(ch);
                        //debughex(chan_rssi);
                        //debughex((u8)pos_chan_threshold);
                        if (start_of_peak > 0) {
                            // just dropped below threshold.  split the difference to find the channel center
                            chan_diff =   start_of_peak + (u8)((ch - start_of_peak+1)/2);
                            chan_hops_ptr = &chan_hops[chan_hops_cnt++];
                            chan_hops_ptr->rssi = rssi_tmp;             // here we store the peak rssi value.
                            chan_hops_ptr->chan = chan_diff;            // here we store the center of the channel
                            chan_hops_ptr->loop = chan_loop_cnt;        // which loop did we find this?
                            start_of_peak = 0;                          // RESET OUR JUNK FOR THE NEXT PEAK!
                            rssi_tmp = 0;                               // RESET OUR JUNK FOR THE NEXT PEAK!
                            chans_per_loop++;
                            //debug("adding channel");
                            //debughex(chan_diff);
                        }
                    }

                    // Monitor to see if we have filled up XDATA
                    // for modified channels and reset when needed
                    if ((chan_hops_cnt) >= MAX_HOPS)
                    {
                        blink(SH_PAUSE,SH_PAUSE);       // let 'em know we're done
                        debug("sending hops");
                        txdata(2, DATA_HOPS, (chan_hops_cnt), (u8*)&chan_hops[0]);
                        //mode = MODE_INIT_HOPS;                  // loop back on ourselves... just keep scanning.
                        //  OR just skip chaning modes altogether and keep scanning!
                        chan_hops_cnt = 0;

                    }
                    if (chan_loop_cnt == 0xFDFF)
                        chan_loop_cnt = 0;
                    RFST = RFST_SIDLE;
                }
                break;
            case MODE_INIT_SPECAN:
                radio_calibration();
                mode = MODE_SPECAN;
            case MODE_SPECAN:
                //Loop Through channels for each thing.
                // FIXME: I think this should be <=, fix if not - cutaway
                for (ch = min_chan; ch <= chan_num; ch++) 
                {
                    // make sure we don't lose usb  (FIXME:  should we just have this on the timer?)
                    usbProcessEvents();

                    /* tune radio */
                    tune(ch);
                    RFST = RFST_SRX;
                    sleepMicros(200);

                    if (ch==0){
                        txdata(2, DATA_CHAN_RSSI, (chan_num-min_chan), (u8*)&chan_str[0]);
                    }

                    // when we're done here, we will be able to determine the likeliest channel spacing
                    chan_rssi = (RSSI ^ 0x80);                  // why is this ^0x80?
                    chan_str[ch] = chan_rssi;
                    RFST = RFST_SIDLE;
                }
                break;

        }   //switch

            /* end RX */
    }       // while 
}






///////////////////////////////// APP-USB CODE BEGINS HERE ///////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
 *      cleared by an interrupt when the data has been received on the host side.                
 *  * xdata is kinda tricky to manipulate
 *  */
//int appHandleEP5(xdata USB_EP_IO_BUF* epiobuf)
int appHandleEP5()
{
    u8 app, cmd;
    u16 len;
    xdata u8 *buf;


    //buf = &epiobuf->OUTbuf[4];
    buf = &usb_ep5_OUTbuf[4];
    //buf = &ep5iobuf.OUTbuf[4];                            // why can i not get this static global structure to cross file boundaries?
    //blink_binary_baby_lsb((u16)&ep5iobuf,16);
    //blink_binary_baby_lsb((u16)epiobuf,16);
    app = *(buf++);
    cmd = *(buf++);
    len = (u16)*(buf);
    buf += 2;                                               // point at the address in memory
    //blink_binary_baby_lsb(ep5iobuf.OUTbuf[0],8);
    //blink_binary_baby_lsb(ep5iobuf.OUTbuf[1],8);

    /*app = ep5iobuf.OUTbuf[4];
    cmd = ep5iobuf.OUTbuf[5];
    buf = &ep5iobuf.OUTbuf[6];
    len = (u16)*buf;
    buf += 2;                                               // point at the address in memory
    */
    // ep5iobuf.OUTbuf should have the following bytes to start:  <app> <cmd> <lenlow> <lenhigh>
    // check the application
    //  then check the cmd
    //   then process the data
    switch (cmd)
    {
        case FHSS_CMD_RESET:
            mode = MODE_RESET;
            txdata(app, cmd, 1, buf);
            break;
        case FHSS_CMD_DUMP_MINS:
            txdata(app, cmd, sizeof(chan_str), (xdata u8*)&chan_str[0]);
            break;
        case FHSS_CMD_DUMP_MAXS:
            txdata(app, cmd, sizeof(chan_maxs), (xdata u8*)&chan_maxs[0]);
            break;
        case FHSS_CMD_DUMP_CHANSPACING:
            txdata(app, cmd, 4, (u8*)&chan_spacing);
            break;
        case FHSS_CMD_DUMP_HOPS:
            txdata(app, cmd, chan_hops_cnt*sizeof(channel_hop), (xdata u8*)&chan_hops[0]);
            break;
        case FHSS_CMD_GET_HOP_CNT:
            txdata(app, cmd, 2, (xdata u8*)&chan_hops_cnt);
            break;
        case FHSS_CMD_CONFIG_MINS:
            txdata(app, cmd, 1, buf);
            break;
        case FHSS_CMD_CONFIG_MAXS:
            txdata(app, cmd, 1, buf);
            break;
        case FHSS_CMD_CONFIG_CHANS:
            txdata(app, cmd, 1, buf);
            break;
        case FHSS_CMD_CONFIG_RADIO:                     // read in a string of bytes where the placement is important, interpret accordingly and set radio registers
            txdata(app, cmd, 1, buf);
            break;
        case FHSS_CMD_SET_NUMCHANS:
            //if (len == 2){
                //blink(100,50);
                mode = MODE_LAZY;
                chan_num = (u16)(*buf);
                chan_spacing = CHAN_SPACE / chan_num;    // this is a hack that requires 902-928
                txdata(app, cmd, 2, buf);
            //}
            break;
        case FHSS_CMD_SET_CHANSPACING:                  // FIXME:  this is broken.  no time to fix now.
            //if (len == 4){
                //blink(100,50);
                mode = MODE_LAZY;
                chan_spacing =  (u32)(*buf++);
                chan_spacing += (u32)(*buf++)<<8;
                chan_spacing += (u32)(*buf++)<<16;
                chan_spacing += (u32)(*buf)<<24;
                //chan_spacing = (xdata u32)(*buf);
                chan_num = CHAN_SPACE / chan_spacing;   // this is a hack that requires 902-928
                if (chan_num > NUM_CHANNELS){
                    chan_num = NUM_CHANNELS;
                }
                txdata(app, cmd, 4, (xdata u8*)&chan_spacing);
            //}
            break;
        case FHSS_CMD_CONFIG_HOPS:
            // reallY?  what am i supposed to do here?  sniff?  not by this deadline.  mebbe next rev.
            txdata(app, cmd, 15, "NOT IMPLEMENTED");
            break;

        case FHSS_CMD_SET_MODE:
            mode = (*buf++);
            if (len == 5)
                next_mode_cnt = (u16)(*buf);
            chan_loop_cnt = 0;
            txdata(app, cmd, 1, --buf);
            break;
        case FHSS_CMD_GET_MODE:
            txdata(app, cmd, 1, (u8*)&mode);
            break;

        case FHSS_CMD_GET_THRESHOLD:
            txdata(app, cmd, 1, (u8*)&pos_chan_threshold);
            break;
        case FHSS_CMD_SET_THRESHOLD:
            pos_chan_threshold = (xdata u8)(*buf++);            // odd, but it looks like the (xdata u8) cast is required.  freakin weird, since the u32 above didn't work the same.
            txdata(app, cmd, 1, (u8*)&pos_chan_threshold);
            break;

        case RADIO_CMD_CONFIG_FREQ:
            break;
        default:
            blink_binary_baby_lsb(app,8);
            blink_binary_baby_lsb(cmd,8);
            blink_binary_baby_lsb(len,16);

    }
    ep5iobuf.flags &= ~EP_OUTBUF_WRITTEN;                       // this allows the OUTbuf to be rewritten... it's saved until now.
    //epiobuf->flags &= ~EP_OUTBUF_WRITTEN;                       // this allows the OUTbuf to be rewritten... it's saved until now.
    //debughex(pos_chan_threshold);
    return 0;

}


/* in case your application cares when an OUT packet has been completely received.               */
void appHandleEP0OUTdone(void)
{
}

/* this function is the application handler for endpoint 0.  it is called for all VENDOR type    *
 * messages.  it currently bounces back the last location and last error bytes                  */
int appHandleEP0(USB_Setup_Header* pReq)
{
    if (pReq->bmRequestType & USB_BM_REQTYPE_DIRMASK)       // IN to host
    {
        {
            REALLYFASTBLINK();
            //if (pReq->wIndex&0xf)
            //{
                //setup_send_ep(&ep0iobuf, &pingstorage[0], pingsize); // haxed for RAM usage
                //setup_send_ep0(&ep0iobuf.OUTbuf[0], ep0iobuf.OUTlen);
            //}
            setup_send_ep0(&lastCode[0], 2);
        }
    } else                                                  // OUT from host
    {
        if (pReq->wIndex&0xf)                               // EP0 receive.  
        {
            //usb_recv_epOUT(0, &ep0iobuf);
            usb_recv_ep0OUT();
            //ptr1 = &pingstorage[0];                           // haxed for RAM usage
            //ptr2 = &ep0iobuf.OUTbuf[0];                       // haxed for RAM usage
            //pingsize = (u8)ep0iobuf.OUTlen;                   // haxed for RAM usage
            //for (loop=ep0iobuf.OUTlen;loop>0; loop--){        // haxed for RAM usage
            //    *ptr1++ = *ptr2++;                            // haxed for RAM usage
            //}                                                 // haxed for RAM usage
            ep0iobuf.flags &= ~EP_OUTBUF_WRITTEN;
        }
    }
    return 0;
}

/*************************************************************************************************
 *  here begins the initialization stuff... this shouldn't change much between firmwares or      *
 *  devices.                                                                                     *
 *************************************************************************************************/

/* initialize the IO subsystems for the appropriate dongles */
static void io_init(void)
{
#ifdef IMMEDONGLE
    // IM-ME Dongle.  It's a CC1110, so no USB stuffs.  Still, a bit of stuff to init for talking to it's own Cypress USB chip
    P0SEL |= (BIT5 | BIT3);     // Select SCK and MOSI as SPI
    P0DIR |= BIT4 | BIT6;       // SSEL and LED as output
    P0 &= ~(BIT4 | BIT2);       // Drive SSEL and MISO low
        
    P1IF = 0;                   // clear P1 interrupt flag
    IEN2 |= IEN2_P1IE;          // enable P1 interrupt
    P1IEN |= BIT1;              // enable interrupt for P1.1
        
    P1DIR |= BIT0;              // P1.0 as output, attention line to cypress
    P1 &= ~BIT0;                // not ready to receive
        
#elif defined DONSDONGLES
    // CC1111 USB Dongle
    // turn on LED and BUTTON
    P1DIR |= 3;
    // Turn off LED
    LED = 0;
    // Activate BUTTON - Do we need this?
    //CC1111EM_BUTTON = 1;
        
#else
    // CC1111 USB (ala Chronos watch dongle), we just need LED
    P1DIR |= 3;
    LED = 0;
#endif
        
        
}
       
        
/*************************************************************************************************
 * main startup code                                                                             *
 *************************************************************************************************/
void initBoard(void)
{
    clock_init();
    io_init();
}


/*
 * Copyright 2010 Michael Ossmann, Don C. Weber
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

/*
 * There is one channel per column of the display.  The radio is tuned to one
 * channel at a time and RSSI is displayed for that channel.
 */

// 900 MHz - 52 channels 0.5 MHz per channel from 902 to 928
//#define NUM_CHANNELS 53 
#define NUM_CHANNELS 90 

// hz between channels
#define CHAN_SPACE 26500000     // 902 thru 928 inclusive
#define HZ_CHAN_SPACE 500000    // These are for 52 channels (plus chan 0)
#define KHZ_CHAN_SPACE 500
#define MHZ_CHAN_SPACE .5
#define INITIAL_CHAN_SPACING    50000   // for channel identification, start small and look at just a small portion of the spectrum

#ifdef IMMEDONGLE
    #define FREQ_MULTIPLIER 0.0025206154
#else
    #define FREQ_MULTIPLIER 0.0027306667
#endif

////#include "cc1111usb.h"

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
#define ACTION_PAUSE 50
#define SH_ACTION_PAUSE 10
#define CONFIG_INTERVAL 5
#define RADIO_PAUSE 2

#define MINSCAN_MAX_THRESHOLD   3
#define MINSCAN_MIN_THRESHOLD   10

// BITS
#define BIT0 0x01
#define BIT1 0x02
#define BIT2 0x04
#define BIT3 0x08
#define BIT4 0x10
#define BIT5 0x20
#define BIT6 0x40
#define BIT7 0x80

/************************ STATE MACHINE *****************************
 * start off in:
 *     MODE_RESET, which sets everything up and traverses to
 *     -> SCAN_LOOP.  scan loop has it's own states
 *              -> INIT_FLOOR - sets up the chan_mins array
 *                  calls CONFIG_CHANS - configures the radio with spacing based on global variable chan_spacing (inits as FIXME:X)
 *              -> SCAN_FLOOR - identifies the RSSI floor over a set of 135 channels  (loops ~ 1000)
 *              -> INIT_CHANS - initializes the chan_maxs array
 *              -> SCAN_CHANS - identifies RSSI peaks over a set of 135 channels, identifying the channel spacing (loops ~10000)
 *              -> INIT_HOPS  - set up the channel list and radio stuff
 *                  calls CONFIG_CHANS - configures the radio with spacing based on global variable chan_spacing
 *              -> SCAN_HOPS  - identifies channel pattern over a limited number of channels (determined in channel spacing)
 *
 */
// Modes - MODE_MASK is used to clear
#define MODE_MASK           0x0F
#define MODE_LAZY           0xf
#define MODE_RESET          0
#define MODE_INIT_FLOOR     1
#define MODE_SCAN_FLOOR     2
#define MODE_INIT_CHANS     3
#define MODE_SCAN_CHANS     4
#define MODE_INIT_HOPS      5
#define MODE_SCAN_HOPS      6
#define MODE_INIT_SPECAN    7
#define MODE_SPECAN         8

#define DATA_MASK           0x0F
#define DATA_MINS           2
#define DATA_MAXS           3
#define DATA_CHANMAX        4
#define DATA_CHANSPC        5
#define DATA_HOPS           6
#define DATA_THRESHOLDS     7
#define DATA_THRESHOLD      8
#define DATA_CHAN_RSSI      9

// commands from client
#define FHSS_CMD_RESET          0x00
#define FHSS_CMD_DUMP_MINS      0x01
#define FHSS_CMD_DUMP_MAXS      0x02
#define FHSS_CMD_DUMP_CHANSPACING 0x03
#define FHSS_CMD_DUMP_HOPS      0x04
#define FHSS_CMD_CONFIG_MINS    0x10
#define FHSS_CMD_CONFIG_MAXS    0x11
#define FHSS_CMD_CONFIG_CHANS   0x12
#define FHSS_CMD_CONFIG_HOPS    0x13
#define FHSS_CMD_CONFIG_RADIO   0x14
#define FHSS_CMD_CONFIG_BASE_FREQ 0x15
#define FHSS_CMD_GET_BASE_FREQ  0x16
#define FHSS_CMD_SET_MODE       0x20
#define FHSS_CMD_GET_MODE       0x21
#define FHSS_CMD_SET_NUMCHANS   0x22
#define FHSS_CMD_SET_CHANSPACING 0x23
#define FHSS_CMD_SET_THRESHOLD  0x24
#define FHSS_CMD_GET_THRESHOLD  0x25
#define FHSS_CMD_GET_HOP_CNT    0x26
#define FHSS_CMD_GET_NUMCHANS   0x27
#define RADIO_CMD_CONFIG_FREQ   0x44


#define REPS_SCAN_FLOOR     1200
#define REPS_SCAN_CHANS     1200

#define CHAN_MIN_HITS       5                      // FIXME: foobar
#define CHAN_MAX_HITS       REPS_SCAN_CHANS*3/4     // FIXME: foobar
#define MAX_CHAN_WIDTH      25

#define MAX_HOPS 400

typedef struct {
	/* frequency setting */
	u8 freq2;
	u8 freq1;
	u8 freq0;
	
	/* frequency calibration */
	u8 fscal3;
	u8 fscal2;
	u8 fscal1;
    
} channel_info;

typedef struct {
    u16 loop;
    u8 chan;
    u8 rssi;
} channel_hop;


code u8 nextMode[] = {
    MODE_LAZY,
    MODE_SCAN_FLOOR,
    MODE_LAZY,
    MODE_SCAN_CHANS,
    MODE_LAZY,
    MODE_SCAN_HOPS,
    MODE_SCAN_HOPS,
    MODE_LAZY
    };

void sleepMillis(int ms);
void sleepMicros(int us);


// IO Port Definitions:
//#define A0 P0_2
//#define SSN P0_4
#define CC1111EM_LED P1_1
//  #define LED_GREEN CC1111EM_LED
#define CC1111EM_BUTTON P1_2
#define BUTTON CC1111EM_BUTTON
#define USART0_RX P0_2
#define USART0_TX P0_3
#define USART0_CT P0_4
#define USART0_RT P0_5

/*
 * wide mode (default): 44 MHz on screen, 333 kHz per channel
 * narrow mode: 6.6 MHz on screen, 50 kHz per channel
 */
#define WIDE 0
#define NARROW 1
#define ULTRAWIDE 2

/*
 * short mode (default): displays RSSI >> 2
 * tall mode: displays RSSI
 */
#define SHORT 0
#define TALL 1

/* vertical scrolling */
#define SHORT_STEP  16
#define TALL_STEP   4
#define MAX_VSCROLL 208
#define MIN_VSCROLL 0

/* frequencies in MHz */
#define DEFAULT_FREQ     915
#define WIDE_STEP        5
#define NARROW_STEP      1
#define ULTRAWIDE_STEP   20
#define WIDE_MARGIN      13
#define NARROW_MARGIN    3
#define ULTRAWIDE_MARGIN 42

/* frequency bands supported by device */
#define BAND_300 0
#define BAND_400 1
#define BAND_900 2

/* band limits in MHz */
#define MIN_300  281
#define MAX_300  361
#define MIN_400  378
#define MAX_400  481
#ifdef FULL_900
    #define MIN_900  749
    #define MAX_900  962
#else
    //Modified for Freq Hopping Test
    #define MIN_900  902
    #define MAX_900  928
#endif

/* band transition points in MHz */
#define EDGE_400 369
#define EDGE_900 615

/* VCO transition points in Hz */
#define MID_300  318000000
#define MID_400  424000000
#define MID_900  848000000

/* channel spacing in Hz */
#define WIDE_SPACING      199952
#define NARROW_SPACING    49988
#define ULTRAWIDE_SPACING 666504

#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

/* Keeping track of all this for each channel allows us to tune faster. */
//void tglLED();
void radio_setup();
void set_filter();
void set_radio_freq(u32 freq);
//void calibrate_freq(u32 freq);
void calibrate_freq(u32 freq, u8 ch);
u16 set_center_freq(u16 freq);
void tune(u8 ch);
void set_width(u8 w);
void appmain(void);


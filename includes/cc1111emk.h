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
#ifdef NUM_CHANNELS_132
    #define NUM_CHANNELS 132
#endif
#ifdef NUM_CHANNELS_53
    #define NUM_CHANNELS 53
#endif
#ifdef NUM_CHANNELS_83
    #define NUM_CHANNELS 83
#endif

// Different for cc1110 and cc1111 due to crystal
// IMME is cc1110
#ifdef IMMEDONGLE
    #define FREQ_MULTIPLIER 0.0025206154
#else
    #define FREQ_MULTIPLIER 0.0027306667
#endif


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

// hz between channels
#define HZ_CHAN_SPACE 500000
#define KHZ_CHAN_SPACE 500
#define MHZ_CHAN_SPACE .5

#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

/* Keeping track of all this for each channel allows us to tune faster. */
typedef struct {
	/* frequency setting */
	u8 freq2;
	u8 freq1;
	u8 freq0;
	
	/* frequency calibration */
	u8 fscal3;
	u8 fscal2;
	u8 fscal1;
    
#ifdef INC_CH_RSSI
	/* signal strength */
	u8 ss;
	u8 max;
#endif
} channel_info;

typedef struct {
	/* recognized minimum signal strength */
	u8 min; //Changed max to min
} channel_mins;

void tglLED();
void radio_setup();
void set_filter();
void set_radio_freq(u32 freq);
//void calibrate_freq(u32 freq);
void calibrate_freq(u32 freq, u8 ch);
u16 set_center_freq(u16 freq);
void tune(u8 ch);
void set_width(u8 w);
void main(void);

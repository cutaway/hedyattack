#ifndef GLOBAL_H
#define GLOBAL_H

#include "cc1111.h"
#include "cc1111usbdebug.h"
#include "bits.h"

// used for debugging and tracing execution.  see client's ".getDebugCodes()"
extern xdata u8 lastCode[2];

//////////////  DEBUG   //////////////
//#define VIRTUAL_COM
//#define RADIO_EU 
//#define TRANSMIT_TEST
//#define RECEIVE_TEST
//////////////////////////////////////
#define LC_USB_INITUSB                  0x2
#define LC_MAIN_RFIF                    0xd
#define LC_USB_DATA_RESET_RESUME        0xa
#define LC_USB_RESET                    0xb
#define LC_USB_EP5OUT                   0xc
#define LC_RF_VECTOR                    0x10
#define LC_RFTXRX_VECTOR                0x11

#define LCE_USB_EP5_TX_WHILE_INBUF_WRITTEN      0x1
#define LCE_USB_EP0_SENT_STALL                  0x4
#define LCE_USB_EP5_OUT_WHILE_OUTBUF_WRITTEN    0x5
#define LCE_USB_EP5_LEN_TOO_BIG                 0x6
#define LCE_USB_EP5_GOT_CRAP                    0x7
#define LCE_USB_EP5_STALL                       0x8
#define LCE_USB_DATA_LEFTOVER_FLAGS             0x9
#define LCE_RF_RXOVF                            0x10


/* board-specific defines */
#ifdef IMME
    // CC1110 IMME pink dongle - 26mhz
    #define LED_RED   P2_3
    #define LED_GREEN P2_4
    #define SLEEPTIMER  1100
    #define PLATFORM_CLOCK_FREQ 26
    
 #include "immedisplay.h"
 #include "immekeys.h"
 #include "immeio.h"
 //#include "pm.h"

#elif defined DONSDONGLES
    // CC1111 USB Dongle with breakout debugging pins (EMK?) - 24mhz
    #define LED_RED   P1_1
    #define LED_GREEN P1_1
    #define SLEEPTIMER  800
    #define CC1111EM_BUTTON P1_2
    #define PLATFORM_CLOCK_FREQ 24
    #define BUTTON CC1111EM_BUTTON

#elif defined CHRONOSDONGLE
    // CC1111 USB Chronos watch dongle - 24mhz
    #define LED_RED   P1_0
    #define LED_GREEN P1_0
    #define SLEEPTIMER  730
    #define PLATFORM_CLOCK_FREQ 24
#endif

#define LED     LED_GREEN
#define LOW 0
#define HIGH 1


#define REALLYFASTBLINK()        { LED=1; sleepMillis(2); LED=0; sleepMillis(10); }
#define blink( on_cycles, off_cycles)  {LED=1; sleepMillis(on_cycles); LED=0; sleepMillis(off_cycles);}
#define BLOCK()     { while (1) { REALLYFASTBLINK() ; usbProcessEvents(); }  }
#define LE_WORD(x) ((x)&0xFF),((u8) (((u16) (x))>>8))

/* function declarations */
void sleepMillis(int ms);
void sleepMicros(int us);
//void blink(u16 on_cycles, u16 off_cycles);
void blink_binary_baby_lsb(u16 num, char bits);
int strncmp(const char *s1, const char *s2, u16 n);
#endif

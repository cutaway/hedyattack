#include "cc1111rf.h"
#include "global.h"

#include <string.h>

/* Rx buffers */
volatile xdata u8 rfRxCurrentBuffer;
volatile xdata u8 rfrxbuf[BUFFER_AMOUNT][BUFFER_SIZE];
volatile xdata u8 rfRxCounter[BUFFER_AMOUNT];
volatile xdata u8 rfRxProcessed[BUFFER_AMOUNT];
/* Tx buffers */
volatile xdata u8 rftxbuf[BUFFER_SIZE];
volatile xdata u8 rfTxCounter = 0;

u8 rfif;
volatile xdata u8 rf_status;

xdata DMA_DESC rfdma;

/*************************************************************************************************
* RF init stuff                                                                                 *
************************************************************************************************/
void init_RF(void)
{
    rf_status = RF_STATE_IDLE;

    DMA0CFGH = ((u16)&rfdma)>>8;
    DMA0CFGL = ((u16)&rfdma)&0xff;

    /* clear buffers */
    memset(rfrxbuf,0,(BUFFER_AMOUNT * BUFFER_SIZE));

    appInitRf();

    /* Setup interrupts */
    RFTXRXIE = 1;                   // FIXME: should this be something that is enabled/disabled by usb?
    RFIM = 0xff;
    RFIF = 0;
    rfif = 0;
    IEN2 |= IEN2_RFIE;

    /* Put radio into idle state */
    setRFIdle();

}

void setRFIdle(void)
{
    RFST = RFST_SIDLE;
    while(!(MARCSTATE & MARC_STATE_IDLE));
    rf_status = RF_STATE_IDLE;
}

/************************** never used.. *****************************
int waitRSSI()
{
    u16 u16WaitTime = 0;
    while(u16WaitTime < RSSI_TIMEOUT_US)
    {
        if(PKTSTATUS & (PKTSTATUS_CCA | PKTSTATUS_CS))
        {
            return 1;
        }
        else
        {
            sleepMicros(50);
            u16WaitTime += 50;
        }
    }
    return 0;
}
***********************************************************************/

u8 transmit(xdata u8* buf, u16 len)
{
    /* Put radio into idle state */
    setRFIdle();

    /* Clean tx buffer */
    //memset(rftxbuf,0,BUFFER_SIZE);

    if (len == 0)
        len = buf[0];

    /* Copy userdata to tx buffer */
    memcpy(rftxbuf, buf, len);

    /* Reset byte pointer */
    rfTxCounter = 0;

    //pDMACfg = buf;                //wtf was i thinking here?!?

    /* Strobe to rx */
    //RFST = RFST_SRX;
    //while(!(MARCSTATE & MARC_STATE_RX));
    /* wait for good RSSI (clear channel) */   ///// unnecessary when radio is in RX mode by default.
    //while(!(PKTSTATUS & (PKTSTATUS_CCA | PKTSTATUS_CS)));
    //while(1)
    //{
    //    if(PKTSTATUS & (PKTSTATUS_CCA | PKTSTATUS_CS))
    //    {
    //        break;
    //    }
    //}


    /* Put radio into tx state */
    RFST = RFST_STX;
    while(!(MARCSTATE & MARC_STATE_TX));

    return 0;
}


/*  FIXME: this is old code... however, it was failing may prove useful later.  this didn't work, but it was
*  failing during the "SRC/DST_DMA_INC" bug.  may try again.
    // configure DMA for transmission
    *pDMACfg++  = (u16)buf>>8;
    *pDMACfg++  = (u16)buf&0xff;
    *pDMACfg++  = (u16)X_RFD>>8;
    *pDMACfg++  = (u16)X_RFD&0xff;
    *pDMACfg++  = RF_DMA_VLEN_1;
    *pDMACfg++  = RF_DMA_LEN;
    *pDMACfg++  = RF_DMA_WORDSIZE | RF_DMA_TMODE | RF_DMA_TRIGGER;
    *pDMACfg++  = RF_DMA_SRC_INC | RF_DMA_IRQMASK | RF_DMA_M8 | RF_DMA_PRIO_LOW;

    DMAARM |= 1;                    // using DMA 0

    RFST = RFST_STX;                //  triggers the DMA

    while (!(RFIF | RFIF_IRQ_DONE));//  wait for DMA to complete

    RFIF &= ~RFIF_IRQ_DONE;

    if (rf_status == RF_STATE_RX)
        startRX();
    else
        stopRX();

    return (retval);
}
*/

void startRX(void)
{
    memset(rfrxbuf,0,BUFFER_SIZE);

    /* Set both byte counters to zero */
    rfRxCounter[FIRST_BUFFER] = 0;
    rfRxCounter[SECOND_BUFFER] = 0;

    /*
    * Process flags, set first flag to false in order to let the ISR write bytes into the buffer,
    *  The second buffer should flag processed on initialize because it is empty.
    */
    rfRxProcessed[FIRST_BUFFER] = RX_UNPROCESSED;
    rfRxProcessed[SECOND_BUFFER] = RX_PROCESSED;

    /* Set first buffer as current buffer */
    rfRxCurrentBuffer = 0;

    S1CON &= ~(S1CON_RFIF_0|S1CON_RFIF_1);
    RFIF &= ~RFIF_IRQ_DONE;

    RFST = RFST_SRX;

    RFIM |= RFIF_IRQ_DONE;
}
/*  FIXME: this is old code... however, it was failing may prove useful later.  this didn't work, but it was
*  failing during the "SRC/DST_DMA_INC" bug.  may try again.
* {
    volatile xdata u8* pDMACfg = rftxbuf;
    volatile xdata u8* loop;

    // configure DMA for transmission
    *pDMACfg++  = (u16)X_RFD>>8;
    *pDMACfg++  = (u16)X_RFD&0xff;
    *pDMACfg++  = (u16)rfrxbuf>>8;
    *pDMACfg++  = (u16)rfrxbuf&0xff;
    *pDMACfg++  = RF_DMA_VLEN_3;
    *pDMACfg++  = RF_DMA_LEN;
    *pDMACfg++  = RF_DMA_WORDSIZE | RF_DMA_TMODE | RF_DMA_TRIGGER;
    *pDMACfg++  = RF_DMA_DST_INC | RF_DMA_IRQMASK | RF_DMA_M8 | RF_DMA_PRIO_LOW;

    DMAARM |= 0x81;                 // ABORT anything on DMA 0

    loop=(volatile xdata u8*)rfrxbuf+(BUFFER_AMOUNT*BUFFER_SIZE)-1;
    for (;loop+1==&rfrxbuf[0][0]; loop--)
        *loop = 0;

    DMAARM |= 0x01;                 // enable DMA 0

    S1CON &= ~(S1CON_RFIF_0|S1CON_RFIF_1);
    RFIF &= ~RFIF_IRQ_DONE;

    RFST = RFST_SRX;

    RFIM |= RFIF_IRQ_DONE;
}*/

void stopRX(void)
{
    RFIM &= ~RFIF_IRQ_DONE;
    setRFIdle();

    DMAARM |= 0x81;                 // ABORT anything on DMA 0

    DMAIRQ &= ~1;

    S1CON &= ~(S1CON_RFIF_0|S1CON_RFIF_1);
    RFIF &= ~RFIF_IRQ_DONE;
}
/*
void resetRf(void)
{
    stopRX();
    if (rf_status == RF_STATE_RX)
    {
            startRX();
    }

}
*/

void RxMode(void)
{
    if (rf_status != RF_STATE_RX)
    {
        //MCSM1 &= 0xf0;
        //MCSM1 |= MCSM1_RXOFF_MODE_RX | MCSM1_TXOFF_MODE_RX;
        rf_status = RF_STATE_RX;
        startRX();
    }
}

void IdleMode(void)
{
    if (rf_status == RF_STATE_RX)
    {
        //MCSM1 &= 0xf0;
        //MCSM1 |= MCSM1_RXOFF_MODE_IDLE | MCSM1_TXOFF_MODE_IDLE;
        stopRX();
        rf_status = RF_STATE_IDLE;
    }
}


void rfTxRxIntHandler(void) interrupt RFTXRX_VECTOR  // interrupt handler should transmit or receive the next byte
{   // currently dormant, in favor of DMA transfers
    lastCode[0] = 17;

    if(MARCSTATE == MARC_STATE_RX)
    {   // Receive Byte
        rfrxbuf[rfRxCurrentBuffer][rfRxCounter[rfRxCurrentBuffer]++] = RFD;
        if(rfRxCounter[rfRxCurrentBuffer] >= BUFFER_SIZE)
        {
            rfRxCounter[rfRxCurrentBuffer] = 0;
        }
    }
    else if(MARCSTATE == MARC_STATE_TX)
    {  // Transmit Byte
        if(rftxbuf[rfTxCounter] != 0)
        {
            RFD = rftxbuf[rfTxCounter++];
        }
    }
    //else
    //{
        // WTFO!??  should never get here.  FIXME: what's causing this interrupt to execute this code?!?
        //debug("rfTxRxIntHandler: unknown MARCSTATE");
        //lastCode[1] = MARCSTATE;
    //}

    RFTXRXIF = 0;
}

void rfIntHandler(void) interrupt RF_VECTOR  // interrupt handler should trigger on rf events
{
    lastCode[0] = 16;
    S1CON &= ~(S1CON_RFIF_0 | S1CON_RFIF_1);
    rfif |= RFIF;

    // contingency - RX Overflow
    if(RFIF & RFIF_IRQ_DONE)
    {
        if(rf_status == RF_STATE_TX)
        {
            DMAARM |= 0x81;
        }
        else
        {
            if(rfRxProcessed[!rfRxCurrentBuffer] == RX_PROCESSED)
            {
                // EXPECTED RESULT - RX complete.
                //
                /* Clear processed buffer */
                memset(rfrxbuf[!rfRxCurrentBuffer],0,BUFFER_SIZE);
                /* Switch current buffer */
                rfRxCurrentBuffer ^= 1;
                rfRxCounter[rfRxCurrentBuffer] = 0;
                /* Set both buffers to unprocessed */
                rfRxProcessed[FIRST_BUFFER] = RX_UNPROCESSED;
                rfRxProcessed[SECOND_BUFFER] = RX_UNPROCESSED;
            }
            else
            {
                // contingency - Packet Not Handled!
                /* Main app didn't process previous packet yet, drop this one */
                REALLYFASTBLINK();
                memset(rfrxbuf[rfRxCurrentBuffer],0,BUFFER_SIZE);
                rfRxCounter[rfRxCurrentBuffer] = 0;
            }
        }
        //RFIF &= ~RFIF_IRQ_DONE;
    }

    if(RFIF & RFIF_IRQ_RXOVF)
    {
        REALLYFASTBLINK();
        /* RX overflow, only way to get out of this is to restart receiver */
        //resetRf();
        stopRX();
        startRX();
        //RFIF &= ~RFIF_IRQ_RXOVF;
    }
    // contingency - TX Underflow
    if(RFIF & RFIF_IRQ_TXUNF)
    {
        /* Put radio into idle state */
        setRFIdle();
        //resetRf();
        //RFIF &= ~RFIF_IRQ_TXUNF;
    }

    RFIF = 0;
}


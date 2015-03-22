#!/usr/bin/env ipython
import sys, usb, threading, time, struct, select
from chipcondefs import *

EP_TIMEOUT_IDLE     = 400
EP_TIMEOUT_ACTIVE   = 10


USB_BM_REQTYPE_TGTMASK          =0x1f
USB_BM_REQTYPE_TGT_DEV          =0x00
USB_BM_REQTYPE_TGT_INTF         =0x01
USB_BM_REQTYPE_TGT_EP           =0x02

USB_BM_REQTYPE_TYPEMASK         =0x60
USB_BM_REQTYPE_TYPE_STD         =0x00
USB_BM_REQTYPE_TYPE_CLASS       =0x20
USB_BM_REQTYPE_TYPE_VENDOR      =0x40
USB_BM_REQTYPE_TYPE_RESERVED    =0x60

USB_BM_REQTYPE_DIRMASK          =0x80
USB_BM_REQTYPE_DIR_OUT          =0x00
USB_BM_REQTYPE_DIR_IN           =0x80

USB_GET_STATUS                  =0x00
USB_CLEAR_FEATURE               =0x01
USB_SET_FEATURE                 =0x03
USB_SET_ADDRESS                 =0x05
USB_GET_DESCRIPTOR              =0x06
USB_SET_DESCRIPTOR              =0x07
USB_GET_CONFIGURATION           =0x08
USB_SET_CONFIGURATION           =0x09
USB_GET_INTERFACE               =0x0a
USB_SET_INTERFACE               =0x11
USB_SYNCH_FRAME                 =0x12

APP_GENERIC                     = 0x01
APP_DEBUG                       = 0xfe
APP_SYSTEM                      = 0xff


SYS_CMD_PEEK                    = 0x80
SYS_CMD_POKE                    = 0x81
SYS_CMD_PING                    = 0x82
SYS_CMD_STATUS                  = 0x83
SYS_CMD_POKE_REG                = 0x84
SYS_CMD_RESET                   = 0x8f

EP0_CMD_GET_DEBUG_CODES         = 0x00
EP0_CMD_GET_ADDRESS             = 0x01
EP0_CMD_POKEX                   = 0x01
EP0_CMD_PEEKX                   = 0x02
EP0_CMD_PING0                   = 0x03
EP0_CMD_PING1                   = 0x04
EP0_CMD_RESET                   = 0xfe


DEBUG_CMD_STRING                = 0xf0
DEBUG_CMD_HEX                   = 0xf1
DEBUG_CMD_HEX16                 = 0xf2
DEBUG_CMD_HEX32                 = 0xf3
DEBUG_CMD_INT                   = 0xf4

EP5OUT_MAX_PACKET_SIZE          = 64

SYNCM_NONE                      = 0
SYNCM_15_of_16                  = 1
SYNCM_16_of_16                  = 2
SYNCM_30_of_32                  = 3
SYNCM_CARRIER                   = 4
SYNCM_CARRIER_15_of_16          = 5
SYNCM_CARRIER_16_of_16          = 6
SYNCM_CARRIER_30_of_32          = 7

RF_STATE_RX                     = 1
RF_STATE_TX                     = 2
RF_STATE_IDLE                   = 3

RF_SUCCESS                      = 0

MODES = {}
lcls = locals()
for lcl in lcls.keys():
    if lcl.startswith("MARC_STATE_"):
        MODES[lcl] = lcls[lcl]
        MODES[lcls[lcl]] = lcl


"""  MODULATIONS
Note that MSK is only supported for data rates above 26 kBaud and GFSK,
ASK , and OOK is only supported for data rate up until 250 kBaud. MSK
cannot be used if Manchester encoding/decoding is enabled.
"""
MOD_2FSK                        = 0x00
MOD_GFSK                        = 0x10
MOD_ASK_OOK                     = 0x30
MOD_MSK                         = 0x70
MANCHESTER                      = 0x08

MODULATIONS = {
        MOD_2FSK    : "2FSK",
        MOD_GFSK    : "GFSK",
        MOD_ASK_OOK : "ASK/OOK",
        MOD_MSK     : "MSK",
        MOD_2FSK | MANCHESTER    : "2FSK/Manchester encoding",
        MOD_GFSK | MANCHESTER    : "GFSK/Manchester encoding",
        MOD_ASK_OOK | MANCHESTER : "ASK/OOK/Manchester encoding",
        MOD_MSK  | MANCHESTER    : "MSK/Manchester encoding",
        }

# FIXME: make these auto-generated
SYNCMODES = {
        SYNCM_NONE: "None",
        SYNCM_15_of_16: "15 of 16 bits must match",
        SYNCM_16_of_16: "16 of 16 bits must match",
        SYNCM_30_of_32: "30 of 32 sync bits must match",
        SYNCM_CARRIER: "Carrier Detect",
        SYNCM_CARRIER_15_of_16: "Carrier Detect and 15 of 16 sync bits must match",
        SYNCM_CARRIER_16_of_16: "Carrier Detect and 16 of 16 sync bits must match",
        SYNCM_CARRIER_30_of_32: "Carrier Detect and 30 of 32 sync bits must match",
        }

NUM_PREAMBLE = [2, 3, 4, 6, 8, 12, 16, 24 ]

ADR_CHK_TYPES = [
        "No address check",
        "Address Check, No Broadcast",
        "Address Check, 0x00 is broadcast",
        "Address Check, 0x00 and 0xff are broadcast",
        ]



PKT_FORMATS = [
        "Normal mode",
        "reserved...",
        "Random TX mode",
        "reserved",
        ]

LENGTH_CONFIGS = [
        "Fixed Packet Mode",
        "Variable Packet Mode (len=first byte after sync word)",
        "reserved",
        "reserved",
        ]
LC_USB_INITUSB                = 0x2
LC_MAIN_RFIF                  = 0xd
LC_USB_DATA_RESET_RESUME      = 0xa
LC_USB_RESET                  = 0xb
LC_USB_EP5OUT                 = 0xc
LC_RF_VECTOR                  = 0x10
LC_RFTXRX_VECTOR              = 0x11

LCE_USB_EP5_TX_WHILE_INBUF_WRITTEN    = 0x1
LCE_USB_EP0_SENT_STALL                = 0x4
LCE_USB_EP5_OUT_WHILE_OUTBUF_WRITTEN  = 0x5
LCE_USB_EP5_LEN_TOO_BIG               = 0x6
LCE_USB_EP5_GOT_CRAP                  = 0x7
LCE_USB_EP5_STALL                     = 0x8
LCE_USB_DATA_LEFTOVER_FLAGS           = 0x9
LCE_RF_RXOVF                          = 0x10
LCE_RF_TXUNF                          = 0x11

LCS = {}
LCES = {}
lcls = locals()
for lcl in lcls.keys():
    if lcl.startswith("LCE_"):
        LCES[lcl] = lcls[lcl]
        LCES[lcls[lcl]] = lcl
    if lcl.startswith("LC_"):
        LCS[lcl] = lcls[lcl]
        LCS[lcls[lcl]] = lcl


class USBDongle:
    ######## INITIALIZATION ########
    def __init__(self, idx=0, debug=False):
        self.rsema = None
        self.xsema = None
        self._do = None
        self.idx = idx
        self.cleanup()
        self._debug = debug
        self._threadGo = False
        self.radiocfg = RadioConfig()
        self.resetup()
        self.recv_thread = threading.Thread(target=self.run)
        self.recv_thread.setDaemon(True)
        self.recv_thread.start()

    def cleanup(self):
        self._usberrorcnt = 0;
        self.recv_queue = ''
        self.recv_mbox  = {}
        self.xmit_queue = []
        self.trash = []
    
    def setup(self, console=True):
        global dongles

        idx = self.idx
        dongles = []

        for bus in usb.busses():
            for dev in bus.devices:
                if dev.idProduct == 0x4715:
                    if console: print >>sys.stderr,(dev)
                    do = dev.open()
                    iSN = do.getDescriptor(1,0,50)[16]
                    sn = do.getString(iSN, 50)
                    dongles.append((sn, dev, do))

        dongles.sort()
        self.serialnun, self._d, self._do = dongles[idx]

        self.rsema = threading.Lock()
        self.xsema = threading.Lock()
        self._do.claimInterface(0)
        self._threadGo = True
        self.ep5timeout = EP_TIMEOUT_ACTIVE

    def resetup(self, console=True):
        '''try:
            self._do.releaseInterface()
        except:
            pass
        '''
        self._do=None
        if console or self._debug: print >>sys.stderr,("waiting (resetup)")
        while (self._do==None):
            try:
                self.setup(console)
                self._flush_recv_mbox()

            except Exception, e:
                if console: sys.stderr.write('.')
                if console or self._debug: print >>sys.stderr,(repr(e))
                time.sleep(.4)



    ########  BASE FOUNDATIONAL "HIDDEN" CALLS ########
    def _sendEP0(self, request=0, buf=None, value=0x200, index=0, timeout=1000):
        if buf == None:
            buf = 'HELLO THERE'
        #return self._do.controlMsg(USB_BM_REQTYPE_TGT_EP|USB_BM_REQTYPE_TYPE_VENDOR|USB_BM_REQTYPE_DIR_OUT, request, "\x00\x00\x00\x00\x00\x00\x00\x00"+buf, value, index, timeout), buf
        return self._do.controlMsg(USB_BM_REQTYPE_TGT_EP|USB_BM_REQTYPE_TYPE_VENDOR|USB_BM_REQTYPE_DIR_OUT, request, buf, value, index, timeout), buf

    def _recvEP0(self, request=0, length=64, value=0, index=0, timeout=100):
        retary = ["%c"%x for x in self._do.controlMsg(USB_BM_REQTYPE_TGT_EP|USB_BM_REQTYPE_TYPE_VENDOR|USB_BM_REQTYPE_DIR_IN, request, length, value, index, timeout)]
        if len(retary):
            return ''.join(retary)
        return ""

    def _sendEP5(self, buf=None, timeout=1000):
        if (buf==None):
            buf = "\xff\x82\x07\x00ABCDEFG"
        while (len(buf)>0):
            if (buf > EP5OUT_MAX_PACKET_SIZE-4):
                drain = buf[:EP5OUT_MAX_PACKET_SIZE-4]
                buf = buf[EP5OUT_MAX_PACKET_SIZE-4:]
            else:
                drain = buf[:]
            if self._debug: print >>sys.stderr,"XMIT:"+repr(drain)
            self._do.bulkWrite(5, "\x00\x00\x00\x00" + drain, timeout)

    def _recvEP5(self, timeout=100):
        retary = ["%c"%x for x in self._do.bulkRead(0x85, 500, timeout)]
        if self._debug: print >>sys.stderr,"RECV:"+repr(retary)
        if len(retary):
            return ''.join(retary)
            #return retary
        return ''

    def _flush_recv_mbox(self):
        if self._debug:
            print >>sys.stderr,("_flush_recv_mbox")
        for key in self.recv_mbox.keys():
            self.trash.extend(self.recvAll(key))
        self.trash.append(self.recv_queue)
        self.recv_queue = ''

    ######## TRANSMIT/RECEIVE THREADING ########
    def run(self):
        msg = ''
        self.threadcounter = 0

        while True:
            if (not self._threadGo): 
                time.sleep(.04)
                continue

            self.threadcounter = (self.threadcounter + 1) & 0xffffffff

            #### transmit stuff.  if any exists in the xmit_queue
            msgsent = False
            msgrecv = False
            try:
                if len(self.xmit_queue):
                    self.xsema.acquire()
                    msg = self.xmit_queue.pop(0)
                    self.xsema.release()
                    self._sendEP5(msg)
                    msgsent = True
                else:
                    if self._debug>3: sys.stderr.write("NoMsgToSend ")
            #except IndexError:
                #if self._debug==3: sys.stderr.write("NoMsgToSend ")
                #pass
            except:
                sys.excepthook(*sys.exc_info())


            #### handle debug application
            try:
                q = None
                b = self.recv_mbox.get(APP_DEBUG, None)
                if (b != None):
                    for cmd in b.keys():
                        q = b[cmd]
                        if len(q):
                            buf = q.pop(0)
                            #cmd = ord(buf[1])
                            if self._debug > 1: print >>sys.stderr,("buf length: %x\t\t cmd: %x\t\t(%s)"%(len(buf), cmd, repr(buf)))
                            if (cmd == DEBUG_CMD_STRING):
                                if (len(buf) < 4):
                                    if (len(q)):
                                        buf2 = q.pop(0)
                                        buf += buf2
                                    q.insert(0,buf)
                                    if self._debug: sys.stderr.write('*')
                                else:
                                    length, = struct.unpack("<H", buf[2:4])
                                    if self._debug >1: print >>sys.stderr,("len=%d"%length)
                                    if (len(buf) < 4+length):
                                        if (len(q)):
                                            buf2 = q.pop(0)
                                            buf += buf2
                                        q.insert(0,buf)
                                        if self._debug: sys.stderr.write('&')
                                    else:
                                        printbuf = buf[4:4+length]
                                        requeuebuf = buf[4+length:]
                                        if len(requeuebuf):
                                            if self._debug>1:  print >>sys.stderr,(" - DEBUG..requeuing %s"%repr(requeuebuf))
                                            q.insert(0,requeuebuf)
                                        print >>sys.stderr,("DEBUG: (%.3f) %s" % (time.time(), repr(printbuf)))
                            elif (cmd == DEBUG_CMD_HEX):
                                #print >>sys.stderr, repr(buf[4:])
                                print >>sys.stderr, "DEBUG: %x"%(struct.unpack("B", buf[4:]))
                            elif (cmd == DEBUG_CMD_HEX16):
                                #print >>sys.stderr, repr(buf[4:])
                                print >>sys.stderr, "DEBUG: %x"%(struct.unpack("<H", buf[4:]))
                            elif (cmd == DEBUG_CMD_HEX32):
                                #print >>sys.stderr, repr(buf[4:])
                                print >>sys.stderr, "DEBUG: %x"%(struct.unpack("<L", buf[4:]))
                            elif (cmd == DEBUG_CMD_INT):
                                print >>sys.stderr, "DEBUG: %d"%(struct.unpack("<L", buf[4:]))
                            else:
                                print >>sys.stderr,('DEBUG COMMAND UNKNOWN: %x (buf=%s)'%(cmd,repr(buf)))

            except:
                sys.excepthook(*sys.exc_info())

            #### receive stuff.
            try:
                #### first we populate the queue
                msg = self._recvEP5(timeout=self.ep5timeout)
                if len(msg) > 0:
                    self.recv_queue += msg
                    msgrecv = True
            except usb.USBError, e:
                #sys.stderr.write(repr(self.recv_queue))
                #sys.stderr.write(repr(e))
                try:
                    self.rsema.release()                            # THREAD SAFETY DANCE COMPLETE
                except: 
                    pass

                if self._debug>4: print >>sys.stderr,repr(sys.exc_info())
                if ('No such device' in repr(e)):
                    self._threadGo = False
                    self.resetup(False)
                self._usberrorcnt += 1
                pass


            #### parse, sort, and deliver the mail.
            try:
                if len(self.recv_queue):
                    idx = self.recv_queue.find('@')
                    if (idx==-1):
                        if self._debug:
                            sys.stderr.write('@')
                    else:
                        if (idx>0):
                            if self._debug: print >>sys.stderr,("run(): idx>0?")
                            self.trash.append(self.recv_queue[:idx])
                            self.recv_queue = self.recv_queue[idx:]
                   
                        msg = self.recv_queue[1:]                           # pop off the leading "@"   really?  here?  before we know we're done?
                        msglen = len(msg)
                        if (msglen>=4):                                      # if not enough to parse length... we'll wait.
                            app = ord(msg[0])
                            cmd = ord(msg[1])
                            length, = struct.unpack("<H", msg[2:4])

                            if self._debug>1: print>>sys.stderr,("app=%x  cmd=%x  len=%x"%(app,cmd,length))

                            if (msglen >= length+4):
                                #### if the queue has enough characters to handle the next message... chop it and put it in the appropriate recv_mbox
                                msg = self.recv_queue[1:length+5]                   # drop the initial '@' and chop out the right number of chars
                                self.recv_queue = self.recv_queue[length+5:]        # chop it out of the queue

                                b = self.recv_mbox.get(app,None)
                                self.rsema.acquire()                            # THREAD SAFETY DANCE
                                if (b == None):
                                    b = {}
                                    self.recv_mbox[app] = b
                                self.rsema.release()                            # THREAD SAFETY DANCE COMPLETE
                               
                                q = b.get(cmd)
                                self.rsema.acquire()                            # THREAD SAFETY DANCE
                                if (q is None):
                                    q = []
                                    b[cmd] = q

                                q.append(msg)
                                self.rsema.release()                            # THREAD SAFETY DANCE COMPLETE
                            else:            
                                if self._debug:     sys.stderr.write('=')
                        else:
                            if self._debug:     sys.stderr.write('.')
            except:
                try:
                    self.rsema.release()                            # THREAD SAFETY DANCE COMPLETE
                except:
                    pass
                sys.excepthook(*sys.exc_info())


            if not (msgsent or msgrecv or len(msg)) :
                #time.sleep(.1)
                self.ep5timeout = EP_TIMEOUT_IDLE
            else:
                self.ep5timeout = EP_TIMEOUT_ACTIVE
                if self._debug > 5:  sys.stderr.write(" %s:%s:%d .-P."%(msgsent,msgrecv,len(msg)))


                




    ######## APPLICATION API ########
    def recv(self, app, cmd=None, wait=100):
        for x in xrange(wait):
            try:
                b = self.recv_mbox.get(app)
                if cmd is None:
                    keys = b.keys()
                    if len(keys):
                        cmd = b.keys()[-1]
                if b is not None:
                    q = b.get(cmd)
                    #print >>sys.stderr,"debug(recv) q='%s'"%repr(q)
                    self.rsema.acquire(False)
                    resp = q.pop(0)
                    self.rsema.release()
                    return resp
            except IndexError:
                #sys.excepthook(*sys.exc_info())
                try:
                    self.rsema.release()
                except:
                    pass
                pass
            except AttributeError:
                #sys.excepthook(*sys.exc_info())
                try:
                    self.rsema.release()
                except:
                    pass
                pass
            except:
                try:
                    self.rsema.release()
                except:
                    pass
                sys.excepthook(*sys.exc_info())
            time.sleep(.001)                                      # only hits here if we don't have something in queue

    def recvAll(self, app, cmd=None):
        retval = self.recv_mbox.get(app,None)
        if retval is not None:
            if cmd is not None:
                b = retval
                self.rsema.acquire()
                retval = b.get(cmd)
                b[cmd]=[]
                self.rsema.release()
            else:
                self.rsema.acquire()
                self.recv_mbox[app]={}
                self.rsema.release()
            return retval

    def send(self, app, cmd, buf, wait=10000):
        msg = "%c%c%s%s"%(app,cmd, struct.pack("<H",len(buf)),buf)
        self.xsema.acquire()
        self.xmit_queue.append(msg)
        self.xsema.release()
        return self.recv(app, cmd, wait)

    def getDebugCodes(self, timeout=100):
        x = self._recvEP0(timeout=timeout)
        if (x != None and len(x)==2):
            return struct.unpack("BB", x)
        else:
            return x

    def ep0GetAddr(self):
        addr = self._recvEP0(request=EP0_CMD_GET_ADDRESS)
        return addr
    def ep0Reset(self):
        x = self._recvEP0(request=0xfe, value=0x5352, index=0x4e54)
        return x

    def ep0Peek(self, addr, length, timeout=100):
        x = self._recvEP0(request=EP0_CMD_PEEKX, value=addr, length=length, timeout=timeout)
        return x#x[3:]

    def ep0Poke(self, addr, buf='\x00', timeout=100):
        x = self._sendEP0(request=EP0_CMD_POKEX, buf=buf, value=addr, timeout=timeout)
        return x

    def ep0Ping(self, count=10):
        good=0
        bad=0
        for x in range(count):
            #r = self._recvEP0(3, 10)
            r = self._recvEP0(request=2, value=count, length=count, timeout=1000)
            print "PING: %d bytes received: %s"%(len(r), repr(r))
            if r==None:
                bad+=1
            else:
                good+=1
        return (good,bad)

    def debug(self):
        while True:
            """
            try:
                print >>sys.stderr, ("DONGLE RESPONDING:  mode :%x, last error# %d"%(self.getDebugCodes()))
            except:
                pass
            print >>sys.stderr,('recv_queue:\t\t (%d bytes) "%s"'%(len(self.recv_queue),repr(self.recv_queue)[:len(self.recv_queue)%39+20]))
            print >>sys.stderr,('trash:     \t\t (%d bytes) "%s"'%(len(self.trash),repr(self.trash)[:len(self.trash)%39+20]))
            print >>sys.stderr,('recv_mbox  \t\t (%d keys)  "%s"'%(len(self.recv_mbox),repr(self.recv_mbox)[:len(repr(self.recv_mbox))%79]))
            for x in self.recv_mbox.keys():
                print >>sys.stderr,('    recv_mbox   %d\t (%d records)  "%s"'%(x,len(self.recv_mbox[x]),repr(self.recv_mbox[x])[:len(repr(self.recv_mbox[x]))%79]))
                """
            print self.reprRadioState()
            print self.reprClientState()
            x,y,z = select.select([sys.stdin],[],[],0)
            if sys.stdin in x:
                break
            time.sleep(1)

    def ping(self, count=10, buf="ABCDEFGHIJKLMNOPQRSTUVWXYZ", wait=1000):
        good=0
        bad=0
        start = time.time()
        for x in range(count):
            istart = time.time()
            r = self.send(APP_SYSTEM, SYS_CMD_PING, buf, wait)
            istop = time.time()
            print "PING: %d bytes transmitted, received: %s (%f seconds)"%(len(buf), repr(r), istop-istart)
            if r==None:
                bad+=1
            else:
                good+=1
        stop = time.time()
        return (good,bad,stop-start)

    def RESET(self):
        r = self.send(APP_SYSTEM, SYS_CMD_RESET, "RESET_NOW\x00")
    def peek(self, addr, bytecount=1):
        r = self.send(APP_SYSTEM, SYS_CMD_PEEK, struct.pack("<HH", bytecount, addr))
        return r[4:]

    def poke(self, addr, data):
        r = self.send(APP_SYSTEM, SYS_CMD_POKE, struct.pack("<H", addr) + data)
        return r[4:]
    
    def pokeReg(self, addr, data):
        r = self.send(APP_SYSTEM, SYS_CMD_POKE_REG, struct.pack("<H", addr) + data)
        return r[4:]
            
    def getInterruptRegisters(self):
        regs = {}
        # IEN0,1,2
        regs['IEN0'] = self.peek(0xdf00 + IEN0,1)
        regs['IEN1'] = self.peek(IEN1,1)
        regs['IEN2'] = self.peek(IEN2,1)
        # TCON
        regs['TCON'] = self.peek(TCON,1)
        # S0CON
        regs['S0CON'] = self.peek(S0CON,1)
        # IRCON
        regs['IRCON'] = self.peek(IRCON,1)
        # IRCON2
        regs['IRCON2'] = self.peek(IRCON2,1)
        # S1CON
        regs['S1CON'] = self.peek(S1CON,1)
        # RFIF
        regs['RFIF'] = self.peek(RFIF,1)
        # DMAIE
        regs['DMAIE'] = self.peek(DMAIE,1)
        # DMAIF
        regs['DMAIF'] = self.peek(DMAIF,1)
        # DMAIRQ
        regs['DMAIRQ'] = self.peek(DMAIRQ,1)
        return regs

    ######## RADIO METHODS #########
    ### radio recv
    def getMARCSTATE(self):
        mode = ord(self.peek(MARCSTATE))
        return (MODES[mode], mode)

    def setModeTX(self):
        self.poke(X_RFST, "%c"%RFST_STX)

    def setModeRX(self):
        self.poke(X_RFST, "%c"%RFST_SRX)

    def setModeIDLE(self):
        self.poke(X_RFST, "%c"%RFST_SIDLE)

    def setModeTXRXON(self):
        self.poke(X_RFST, "%c"%RFST_SFSTXON)

    def setModeCAL(self):
        self.poke(X_RFST, "%c"%RFST_SCAL)


    ### radio config
    def getRadioConfig(self):
        bytedef = self.peek(0xdf00, 0x3e)
        self.radiocfg.vsParse(bytedef)
        return bytedef

    def setRadioConfig(self):
        bytedef = self.radiocfg.vsEmit()
        self.setModeIDLE()
        self.poke(0xdf00, bytedef)
        self.setModeRX()
        return bytedef

    def setFreq(self, freq=902000000, mhz=24):
        freqmult = (0x10000 / 1000000.0) / mhz
        num = int(freq * freqmult)
        self.radiocfg.freq2 = num >> 16
        self.radiocfg.freq1 = (num>>8) & 0xff
        self.radiocfg.freq0 = num & 0xff
        self.setModeIDLE()
        self.poke(FREQ2, struct.pack("3B", self.radiocfg.freq2, self.radiocfg.freq1, self.radiocfg.freq0))
        self.setModeRX()

    def getFreq(self, mhz=24, radiocfg=None):
        freqmult = (0x10000 / 1000000.0) / mhz
        if radiocfg==None:
            radiocfg = self.radiocfg
            bytedef = self.peek(FREQ2, 3)
            if (len(bytedef) != 3):
                raise(Exception("unknown data returned for getFreq(): %s"%repr(bytedef)))
            (       self.radiocfg.freq2, 
                    self.radiocfg.freq1, 
                    self.radiocfg.freq0) = struct.unpack("3B", bytedef)
        num = (self.radiocfg.freq2<<16) + (self.radiocfg.freq1<<8) + self.radiocfg.freq0
        freq = num / freqmult
        return freq, hex(num)

    def getMdmModulation(self, radiocfg=None):
        if radiocfg == None:
            mdmcfg2 = ord(self.peek(MDMCFG2))
        else:
            mdmcfg2 = radiocfg.mdmcfg2
        mod = (mdmcfg2) & 0x70
        mchstr = (mdmcfg2) & 0x08
        return (mod,mchstr)

    def reprRadioConfig(self, mhz=24, radiocfg=None):
        if radiocfg == None:
            self.getRadioConfig()
            radiocfg = self.radiocfg
        output = []

        output.append( "Frequency Configuration")
        output.append( self.reprFreqConfig(mhz, radiocfg))
        output.append( "\nModem Configuration")
        output.append( self.reprModemConfig(mhz, radiocfg))
        output.append( "\nPacket Configuration")
        output.append( self.reprPacketConfig(radiocfg))
        output.append( "\nRadio Test Signal Configuration")
        output.append( self.reprRadioTestSignalConfig(radiocfg))
        output.append( "\nRadio State")
        output.append( self.reprRadioState(radiocfg))
        output.append("\nClient State")
        output.append( self.reprClientState())
        return "\n".join(output)


    def reprMdmModulation(self, radiocfg=None):
        mod, mchstr = self.getMdmModulation(radiocfg)
        return ("Modulation:           %s" % MODULATIONS[mod | mchstr])

    def setMdmModulation(self, mod, mchstr=0, radiocfg=None):
        if radiocfg == None:
            radiocfg = self.radiocfg

        if (mod|mchstr) & 0x87:
            raise(Exception("Please use constants MOD_FORMAT_* to specify modulation and "))
        radiocfg.mdmcfg2 = ord(self.peek(MDMCFG2)) & 0x87
        radiocfg.mdmcfg2 |= (mod) | (mchstr)
        self.setModeIDLE()
        self.poke(MDMCFG2, struct.pack("<I",radiocfg.mdmcfg2)[0])
        self.setModeRX()

    def getMdmChanSpc(self, mhz=24, radiocfg=None):
        if radiocfg==None:
            radiocfg = self.radiocfg
            radiocfg.chanspc_m = ord(self.peek(MDMCFG0))
            radiocfg.chanspc_e = ord(self.peek(MDMCFG1)) & 3
        spacing = (mhz/0.262144) * (2**radiocfg.chanspc_e) * (256+radiocfg.chanspc_m)
        chanspc = 1000.0 * mhz/pow(2,18) * (256 + chanspc_m) * pow(2, chanspc_e)
        return (spacing)

    def setMdmChanSpc(self, chanspc_khz=None, chanspc_m=None, chanspc_e=None, mhz=24, radiocfg=None):
        '''
        calculates the appropriate exponent and mantissa and updates the correct registers
        chanspc is in kHz.  if you prefer, you may set the chanspc_m and chanspc_e settings 
        directly.

        only use one or the other:
        * chanspc
        * chanspc_m and chanspc_e
        '''
        if radiocfg==None:
            radiocfg = self.radiocfg
        if (chanspc_khz != None):
            for e in range(16):
                m = int((chanspc_khz * pow(2,18) / (1000.0 * mhz * pow(2,e)))-256)
                if m < 256:
                    chanspc_e = e
                    chanspc_m = m
                    break
        if chanspc_e is None or chanspc_m is None:
            raise(Exception("ChanSpc does not translate into acceptable parameters.  Should you be changing this?"))

        chanspc = 1000.0 * mhz/pow(2,18) * (256 + chanspc_m) * pow(2, chanspc_e)
        print "chanspc_e: %x   chanspc_m: %x   chanspc: %f khz" % (e, m, chanspc)
        
        radiocfg.mdmcfg1 = ord(self.peek(MDMCFG1)) & 0xfc  # clear out old exponent value
        radiocfg.mdmcfg1 |= chanspc_e
        radiocfg.mdmcfg0 = chanspc_m
        self.setModeIDLE()
        self.poke(MDMCFG1, chr(radiocfg.mdmcfg1))
        self.poke(MDMCFG0, chr(radiocfg.mdmcfg0))
        self.setModeRX()

    def makePktVLEN(self, maxlen=0xff):
        self.radiocfg.pktctrl0 &= 0xfc
        self.radiocfg.pktctrl0 |= 1
        self.radiocfg.pktlen = maxlen
        self.setModeIDLE()
        self.poke(PKTCTRL0, chr(self.radiocfg.pktctrl0))
        self.poke(PKTLEN, chr(self.radiocfg.pktlen))
        self.setModeRX()

    def makePktFLEN(self, flen=0xff):
        self.radiocfg.pktctrl0 &= 0xfc
        self.radiocfg.pktlen = flen
        self.setModeIDLE()
        self.poke(PKTCTRL0, chr(self.radiocfg.pktctrl0))
        self.poke(PKTLEN, chr(self.radiocfg.pktlen))
        self.setModeRX()

    def setEnablePktCRC(self, enable=True):
        crcE = (0,1)[enable]<<2
        crcM = ~(1<<2)
        self.radiocfg.pktctrl0 &= crcM
        self.radiocfg.pktctrl0 |= crcE
        self.setModeIDLE()
        self.poke(PKTCTRL0, chr(self.radiocfg.pktctrl0))
        self.setModeRX()

    def setEnableDataWhitening(self, enable=True):
        dwE = (0,1)[enable]<<6
        dwM = ~(1<<6)
        self.radiocfg.pktctrl0 &= dwM
        self.radiocfg.pktctrl0 |= dwE
        self.setModeIDLE()
        self.poke(PKTCTRL0, chr(self.radiocfg.pktctrl0))
        self.setModeRX()

    def setPktPQT(self, num=3):
        num &=  7
        num <<= 5
        numM = ~(7<<5)
        self.radiocfg.pktctrl1 &= numM
        self.radiocfg.pktctrl1 |= num
        self.setModeIDLE()
        self.poke(PKTCTRL1, chr(self.radiocfg.pktctrl1))
        self.setModeRX()

    def setEnableMdmFEC(self, enable=True):
        crcE = (0,1)[enable]<<7
        crcM = ~(1<<7)
        self.radiocfg.mdmcfg1 &= crcM
        self.radiocfg.mdmcfg1 |= crcE
        self.setModeIDLE()
        self.poke(MDMCFG1, chr(self.radiocfg.mdmcfg1))
        self.setModeRX()

    def setEnableMdmDCFilter(self, enable=True):
        dcfE = (0,1)[enable]<<7
        dcfM = ~(1<<7)
        self.radiocfg.mdmcfg2 &= dcfM
        self.radiocfg.mdmcfg2 |= dcfE
        self.setModeIDLE()
        self.poke(MDMCFG2, chr(self.radiocfg.mdmcfg2))
        self.setModeRX()

    def getFsIFandOffset(self, mhz=24, radiocfg=None):
        if radiocfg==None:
            radiocfg = self.radiocfg
            radiocfg.channr = ord(self.peek(CHANNR))
            radiocfg.fsctrl1 = ord(self.peek(FSCTRL1))
            radiocfg.fsctrl0 = ord(self.peek(FSCTRL0))

        freq_if = (radiocfg.fsctrl1&0x1f) * (1000000.0 * mhz / pow(2,10))
        freqoff = radiocfg.fsctrl0
        return (freq_if, freqoff)


    def setFsIFandOffset(self, freq_if, if_off, mhz=24):
        '''
        Note that the SmartRF Studio software
        automatically calculates the optimum register
        setting based on channel spacing and channel
        filter bandwidth. (from cc1110f32.pdf)
        '''
        ifBits = freq_if * pow(2,10) / (1000000.0 * mhz)
        ifBits = int(ifBits + .5)

        if ifBits >0x1f:
            raise(Exception("FAIL:  freq_if is too high?  freqbits: %x (must be <0x1f)" % ifBits))
        self.radiocfg.fsctrl1 &= ~(0x1f)
        self.radiocfg.fsctrl1 |= int(ifBits)
        self.radiocfg.fsctrl0 = if_off
        self.setModeIDLE()
        self.poke(FSCTRL1, chr(self.radiocfg.fsctrl1))
        self.poke(FSCTRL0, chr(self.radiocfg.fsctrl0))
        self.setModeRX()

    def getChannel(self):
        self.radiocfg.channr = ord(self.peek(CHANNR))
        return self.radiocfg.channr

    def setChannel(self, channr):
        self.radiocfg.channr = channr
        self.setModeIDLE()
        self.poke(CHANNR, chr(self.radiocfg.channr))
        self.setModeRX()

    def setMdmChanBW(self, bw, mhz=24):
        '''
        For best performance, the channel filter
        bandwidth should be selected so that the
        signal bandwidth occupies at most 80% of the
        channel filter bandwidth. The channel centre
        tolerance due to crystal accuracy should also
        be subtracted from the signal bandwidth. The
        following example illustrates this:

            With the channel filter bandwidth set to 500
            kHz, the signal should stay within 80% of 500
            kHz, which is 400 kHz. Assuming 915 MHz
            frequency and +/-20 ppm frequency uncertainty
            for both the transmitting device and the
            receiving device, the total frequency
            uncertainty is +/-40 ppm of 915 MHz, which is
            +/-37 kHz. If the whole transmitted signal
            bandwidth is to be received within 400 kHz, the
            transmitted signal bandwidth should be
            maximum 400 kHz - 2*37 kHz, which is 326
            kHz.

        '''

        chanbw_e = None
        chanbw_m = None
        for e in range(4):
            m = int((mhz*1000.0 / (bw *pow(2,e) * 8.0 )) - 4)
            if m < 4:
                chanbw_e = e
                chanbw_m = m
                break
        if chanbw_e is None:
            raise(Exception("ChanBW does not translate into acceptable parameters.  Should you be changing this?"))

        bw = 1000.0*mhz / (8.0*(4+chanbw_m) * pow(2,chanbw_e))
        print "chanbw_e: %x   chanbw_m: %x   chanbw: %f kHz" % (e, m, bw)

        self.radiocfg.mdmcfg4 &= 0x0f
        self.radiocfg.mdmcfg4 |= ((chanbw_e<<6) | (chanbw_m<<4))
        self.setModeIDLE()
        self.poke(MDMCFG4, chr(self.radiocfg.mdmcfg4))
        self.setModeRX()


    def setMdmDRate(self, drate_khz, mhz=24):
        drate_e = None
        drate_m = None
        for e in range(16):
            m = int(drate_khz * pow(2,28) / (pow(2,e)* (mhz*1000.0))-256)
            if m < 256:
                drate_e = e
                drate_m = m
                break
        if drate_e is None:
            raise(Exception("DRate does not translate into acceptable parameters.  Should you be changing this?"))

        drate = 1000.0 * mhz * (256+drate_m) * pow(2,drate_e) / pow(2,28)
        print "drate_e: %x   drate_m: %x   drate: %f kHz" % (e, m, drate)
        
        self.radiocfg.mdmcfg4 &= 0xf0
        self.radiocfg.mdmcfg4 |= drate_e
        self.radiocfg.mdmcfg3 = drate_m
        self.setModeIDLE()
        self.poke(MDMCFG3, chr(self.radiocfg.mdmcfg3))
        self.poke(MDMCFG4, chr(self.radiocfg.mdmcfg4))
        self.setModeRX()

    def getMdmDeviatn(self, dev_m, dev_e):
        raise(Exception("Not Implemented!"))

    def setMdmSyncWord(self, word):
        self.setModeIDLE()
        self.poke(SYNC1, chr(word >> 8))
        self.poke(SYNC0, chr(word & 0xff))
        self.setModeRX()

    def getMdmSyncMode(self, radiocfg=None):
        if radiocfg==None:
            radiocfg = self.radiocfg
            radiocfg.mdmcfg2 = ord(self.peek(MDMCFG2))
        return radiocfg.mdmcfg2&0x07

    def setMdmSyncMode(self, syncmode=SYNCM_15_of_16):
        mdmcfg2 = ord(self.peek(MDMCFG2)) & 0xf8
        self.setModeIDLE()
        self.poke(MDMCFG2, "%c" % (mdmcfg2 | syncmode))
        self.setModeRX()

    def reprModemConfig(self, mhz=24, radiocfg=None):
        output = []
        if radiocfg==None:
            radiocfg = self.radiocfg
            radiocfg.mdmcfg4 = ord(self.peek(MDMCFG4))
            radiocfg.mdmcfg3 = ord(self.peek(MDMCFG3))
            radiocfg.mdmcfg2 = ord(self.peek(MDMCFG2))
            radiocfg.mdmcfg1 = ord(self.peek(MDMCFG1))
            radiocfg.mdmcfg0 = ord(self.peek(MDMCFG0))
            reprMdmModulation = self.reprMdmModulation()
            syncmode = self.getMdmSyncMode()
        else:
            reprMdmModulation = self.reprMdmModulation(radiocfg)
            syncmode = self.getMdmSyncMode(radiocfg)

        chanbw_e = radiocfg.mdmcfg4>>6
        chanbw_m = (radiocfg.mdmcfg4>>4) & 0x3
        bw = 1000.0*mhz / (8.0*(4+chanbw_m) * pow(2,chanbw_e))
        output.append("ChanBW:              %f khz"%bw)

        drate_e = radiocfg.mdmcfg4 & 0xf
        drate_m = radiocfg.mdmcfg3
        drate = 1000.0 * mhz * (256+drate_m) * pow(2,drate_e) / pow(2,28)
        output.append("DRate:               %f khz"%drate)

        output.append("DC Filter:           %s" % (("enabled", "disabled")[radiocfg.mdmcfg2>>7]))

        output.append(reprMdmModulation)

        output.append("Sync Mode:           %s" % SYNCMODES[syncmode])

        fec = radiocfg.mdmcfg1>>7
        output.append("Fwd Err Correct:     %s" % (("disabled","enabled")[fec]))
        
        num_preamble = (radiocfg.mdmcfg1>>4)&7
        output.append("Min TX Preamble:     %d bytes" % (NUM_PREAMBLE[num_preamble]) )

        chanspc_e = radiocfg.mdmcfg1&3
        chanspc_m = radiocfg.mdmcfg0
        chanspc = 1000.0 * mhz/pow(2,18) * (256 + chanspc_m) * pow(2, chanspc_e)
        output.append("Chan Spacing:        %f khz" % chanspc)


        return "\n".join(output)

    def getRSSI(self):
        rssi = self.peek(RSSI)
        return rssi

    def getLQI(self):
        lqi = self.peek(LQI)
        return lqi

       
    def reprRadioTestSignalConfig(self, radiocfg=None):
        if radiocfg==None:
            radiocfg = self.radiocfg
            radiocfg.iocfg2 = ord(self.peek(IOCFG2))
            radiocfg.iocfg1 = ord(self.peek(IOCFG1))
            radiocfg.iocfg0 = ord(self.peek(IOCFG0))
            radiocfg.test2 = ord(self.peek(TEST2))
            radiocfg.test1 = ord(self.peek(TEST1))
            radiocfg.test0 = ord(self.peek(TEST0))
        output = []
        output.append("GDO2_INV:            %s" % ("do not Invert Output", "Invert output")[(radiocfg.iocfg2>>6)&1])
        output.append("GDO2CFG:             0x%x" % (radiocfg.iocfg2&0x3f))
        output.append("GDO_DS:              %s" % (("minimum drive (>2.6vdd","Maximum drive (<2.6vdd)")[radiocfg.iocfg1>>7]))
        output.append("GDO1_INV:            %s" % ("do not Invert Output", "Invert output")[(radiocfg.iocfg1>>6)&1])
        output.append("GDO1CFG:             0x%x"%(radiocfg.iocfg1&0x3f))
        output.append("GDO0_INV:            %s" % ("do not Invert Output", "Invert output")[(radiocfg.iocfg0>>6)&1])
        output.append("GDO0CFG:             0x%x"%(radiocfg.iocfg0&0x3f))
        output.append("TEST2:               0x%x"%radiocfg.test2)
        output.append("TEST1:               0x%x"%radiocfg.test1)
        output.append("TEST0:               0x%x"%(radiocfg.test0&0xfd))
        output.append("VCO_SEL_CAL_EN:      0x%x"%((radiocfg.test2>>1)&1))
        return "\n".join(output)


    def reprFreqConfig(self, mhz=24, radiocfg=None):
        output = []
        freq,num = self.getFreq(mhz, radiocfg)
        output.append("Frequency:           %f hz (%s)" % (freq,num))

        if radiocfg==None:
            radiocfg = self.radiocfg
            radiocfg.channr = ord(self.peek(CHANNR))
            radiocfg.fsctrl1 = ord(self.peek(FSCTRL1))
            radiocfg.fsctrl0 = ord(self.peek(FSCTRL0))

        output.append("Channel:             %d" % radiocfg.channr)


        freq_if = (radiocfg.fsctrl1&0x1f) * (1000000.0 * mhz / pow(2,10))
        freqoff = radiocfg.fsctrl0
        
        output.append("Intermediate freq:   %d hz" % freq_if)
        output.append("Frequency Offset:    %d +/-" % freqoff)

        return "\n".join(output)

    def reprPacketConfig(self, radiocfg=None):
        output = []
        if radiocfg==None:
            radiocfg = self.radiocfg
            radiocfg.sync1 = ord(self.peek(SYNC1))
            radiocfg.sync0 = ord(self.peek(SYNC0))
            radiocfg.addr = ord(self.peek(ADDR))
            radiocfg.pktlen = ord(self.peek(PKTLEN))
            radiocfg.pktctrl1 = ord(self.peek(PKTCTRL1))
            radiocfg.pktctrl0 = ord(self.peek(PKTCTRL0))

        output.append("Sync Bytes:      %.2x %.2x" % (radiocfg.sync1, radiocfg.sync0))
        output.append("Packet Length:       %d" % radiocfg.pktlen)
        output.append("Configured Address: 0x%x" % radiocfg.addr)

        pqt = radiocfg.pktctrl1>>5
        output.append("Preamble Quality Threshold: 4 * %d" % pqt)

        append = (radiocfg.pktctrl1>>2) & 1
        output.append("Append Status:       %s" % ("No","Yes")[append])

        adr_chk = radiocfg.pktctrl1&3
        output.append("Rcvd Packet Check:   %s" % ADR_CHK_TYPES[adr_chk])

        whitedata = (radiocfg.pktctrl0>>6)&1
        output.append("Data Whitening:      %s" % ("off", "ON (but only with cc2400_en==0)")[whitedata])

        pkt_format = (radiocfg.pktctrl0>>5)&3
        output.append("Packet Format:       %s" % PKT_FORMATS[pkt_format])

        crc = (radiocfg.pktctrl0>>2)&1
        output.append("CRC:                 %s" % ("disabled", "ENABLED")[crc])

        length_config = radiocfg.pktctrl0&3
        output.append("Length Config:       %s" % LENGTH_CONFIGS[length_config])

        return "\n".join(output)
    """
    SYNC1       = 0xb1;
    SYNC0       = 0x27;
    PKTLEN      = 0xff;
    PKTCTRL1    = 0x04;             // APPEND_STATUS
    PKTCTRL0    = 0x01;             // VARIABLE LENGTH, no crc, no whitening
    ADDR        = 0x00;
    CHANNR      = 0x00;
    FSCTRL1     = 0x0c;             // IF
    FSCTRL0     = 0x00;
    FREQ2       = 0x25;
    FREQ1       = 0x95;
    FREQ0       = 0x55;
    MDMCFG4     = 0x1d;             // chan_bw and drate_e
    MDMCFG3     = 0x55;             // drate_m
    MDMCFG2     = 0x13;             // gfsk, 30/32+carrier sense sync 
    MDMCFG1     = 0x23;             // 4-preamble-bytes, chanspc_e
    MDMCFG0     = 0x11;             // chanspc_m
    DEVIATN     = 0x63;
    MCSM2       = 0x07;             // RX_TIMEOUT
    MCSM1       = 0x30;             // CCA_MODE RSSI below threshold unless currently recvg pkt
    MCSM0       = 0x18;             // fsautosync when going from idle to rx/tx/fstxon
    FOCCFG      = 0x1d;             
    BSCFG       = 0x1c;             // bit sync config
    AGCCTRL2    = 0xc7;
    AGCCTRL1    = 0x00;
    AGCCTRL0    = 0xb0;
    FREND1      = 0xb6;
    FREND0      = 0x10;
    FSCAL3      = 0xea;
    FSCAL2      = 0x2a;
    FSCAL1      = 0x00;
    FSCAL0      = 0x1f;
    TEST2       = 0x88;
    TEST1       = 0x31;
    TEST0       = 0x09;
    PA_TABLE0   = 0x83;
"""

    def reprRadioState(self, radiocfg=None):
        if radiocfg==None:
            #radiocfg = self.radiocfg
            #radiocfg.iocfg2 = ord(self.peek(IOCFG2))
            #radiocfg.iocfg1 = ord(self.peek(IOCFG1))
            #radiocfg.iocfg0 = ord(self.peek(IOCFG0))
            #radiocfg.test2 = ord(self.peek(TEST2))
            #radiocfg.test1 = ord(self.peek(TEST1))
            #radiocfg.test0 = ord(self.peek(TEST0))
            pass
        output = []
        output.append("     MARCSTATE:      %s (%x)" % (self.getMARCSTATE()))
        try:
            output.append("     DONGLE RESPONDING:  mode :%x, last error# %d"%(self.getDebugCodes()))
        except:
            pass

        return "\n".join(output)

    def reprClientState(self):
        output = []
        output.append('     recv_queue:\t\t (%d bytes) "%s"'%(len(self.recv_queue),repr(self.recv_queue)[:len(self.recv_queue)%39+20]))
        output.append('     trash:     \t\t (%d bytes) "%s"'%(len(self.trash),repr(self.trash)[:min(len(self.trash),39)+20]))
        output.append('     recv_mbox  \t\t (%d keys)  "%s"'%(len(self.recv_mbox),repr(self.recv_mbox)[:min(len(repr(self.recv_mbox)),79)]))
        for x in self.recv_mbox.keys():
            output.append('         recv_mbox   0x%x\t (%d records)  "%s"'%(x,len(self.recv_mbox[x]),repr(self.recv_mbox[x])[:min(len(repr(self.recv_mbox[x])),79)]))
        return "\n".join(output)


    ######## APPLICATION METHODS ########
    def setup900MHz(self):
        self.getRadioConfig()
        rc = self.radiocfg
        rc.iocfg0     = 0x06
        rc.sync1      = 0x0b
        rc.sync0      = 0x0b
        rc.pktlen     = 0xff
        rc.pktctrl1   = 0xe5
        rc.pktctrl0   = 0x04
        rc.fsctrl1    = 0x12
        rc.fsctrl0    = 0x00
        rc.addr       = 0x00
        rc.channr     = 0x00
        rc.mdmcfg4    = 0x3e
        rc.mdmcfg3    = 0x55
        rc.mdmcfg2    = 0x73
        rc.mdmcfg1    = 0x23
        rc.mdmcfg0    = 0x55
        rc.mcsm2      = 0x07
        rc.mcsm1      = 0x30
        rc.mcsm0      = 0x00
        rc.deviatn    = 0x16
        rc.foccfg     = 0x17
        rc.bscfg      = 0x6c
        rc.agcctrl2  |= AGCCTRL2_MAX_DVGA_GAIN
        rc.agcctrl2   = 0x03
        rc.agcctrl1   = 0x40
        rc.agcctrl0   = 0x91
        rc.frend1     = 0x56
        rc.frend0     = 0x10
        rc.fscal3     = 0xEA
        rc.fscal2     = 0x2A
        rc.fscal1     = 0x00
        rc.fscal0     = 0x1F
        rc.test2      = 0x88
        rc.test1      = 0x31
        rc.test0      = 0x09
        rc.pa_table0  = 0xc0
        self.setModeIDLE()
        self.setRadioConfig()
        self.setModeRX()

    def setup900MHzHopTrans(self):
        self.getRadioConfig()
        rc = self.radiocfg
        rc.iocfg0     = 0x06
        rc.sync1      = 0x0b
        rc.sync0      = 0x0b
        rc.pktlen     = 0xff
        rc.pktctrl1   = 0x04
        rc.pktctrl0   = 0x05
        rc.addr       = 0x00
        rc.channr     = 0x00
        rc.fsctrl1    = 0x06
        rc.fsctrl0    = 0x00
        rc.mdmcfg4    = 0xee
        rc.mdmcfg3    = 0x55
        rc.mdmcfg2    = 0x73
        rc.mdmcfg1    = 0x23
        rc.mdmcfg0    = 0x55
        rc.mcsm2      = 0x07
        rc.mcsm1      = 0x30
        rc.mcsm0      = 0x18
        rc.deviatn    = 0x16
        rc.foccfg     = 0x17
        rc.bscfg      = 0x6c
        rc.agcctrl2   = 0x03
        rc.agcctrl1   = 0x40
        rc.agcctrl0   = 0x91
        rc.frend1     = 0x56
        rc.frend0     = 0x10
        rc.fscal3     = 0xEA
        rc.fscal2     = 0x2A
        rc.fscal1     = 0x00
        rc.fscal0     = 0x1F
        rc.test2      = 0x88
        rc.test1      = 0x31
        rc.test0      = 0x09
        rc.pa_table0  = 0xc0
        self.setModeIDLE()
        self.setRadioConfig()
        self.setModeRX()

    def setup900MHzContTrans(self):
        self.getRadioConfig()
        rc = self.radiocfg
        rc.iocfg0     = 0x06
        rc.sync1      = 0x0b
        rc.sync0      = 0x0b
        rc.pktlen     = 0xff
        rc.pktctrl1   = 0x04
        rc.pktctrl0   = 0x05
        rc.addr       = 0x00
        rc.channr     = 0x00
        rc.fsctrl1    = 0x06
        rc.fsctrl0    = 0x00
        rc.freq2      = 0x26
        rc.freq1      = 0x55
        rc.freq0      = 0x55
        rc.mdmcfg4    = 0xee
        rc.mdmcfg3    = 0x55
        rc.mdmcfg2    = 0x73
        rc.mdmcfg1    = 0x23
        rc.mdmcfg0    = 0x55
        rc.mcsm2      = 0x07
        rc.mcsm1      = 0x30
        rc.mcsm0      = 0x18
        rc.deviatn    = 0x16
        rc.foccfg     = 0x17
        rc.bscfg      = 0x6c
        rc.agcctrl2   = 0x03
        rc.agcctrl1   = 0x40
        rc.agcctrl0   = 0x91
        rc.frend1     = 0x56
        rc.frend0     = 0x10
        rc.fscal3     = 0xEA
        rc.fscal2     = 0x2A
        rc.fscal1     = 0x00
        rc.fscal0     = 0x1F
        rc.test2      = 0x88
        rc.test1      = 0x31
        rc.test0      = 0x09
        rc.pa_table0  = 0xc0
        self.setModeIDLE()
        self.setRadioConfig()
        self.setModeRX()

    def setup_rfstudio_902PktTx(self):
        self.getRadioConfig()
        rc = self.radiocfg
        rc.iocfg2     = 0x00
        rc.iocfg1     = 0x00
        rc.iocfg0     = 0x06
        rc.sync1      = 0x0b
        rc.sync0      = 0x0b
        rc.pktlen     = 0xff
        rc.pktctrl1   = 0x04
        rc.pktctrl0   = 0x05
        rc.addr       = 0x00
        rc.channr     = 0x00
        rc.fsctrl1    = 0x0c
        rc.fsctrl0    = 0x00
        rc.freq2      = 0x25
        rc.freq1      = 0x95
        rc.freq0      = 0x55
        rc.mdmcfg4    = 0x1d
        rc.mdmcfg3    = 0x55
        rc.mdmcfg2    = 0x13
        rc.mdmcfg1    = 0x23
        rc.mdmcfg0    = 0x11
        rc.mcsm2      = 0x07
        rc.mcsm1      = 0x30
        rc.mcsm0      = 0x18
        rc.deviatn    = 0x63
        rc.foccfg     = 0x1d
        rc.bscfg      = 0x1c
        rc.agcctrl2   = 0xc7
        rc.agcctrl1   = 0x00
        rc.agcctrl0   = 0xb0
        rc.frend1     = 0xb6
        rc.frend0     = 0x10
        rc.fscal3     = 0xEA
        rc.fscal2     = 0x2A
        rc.fscal1     = 0x00
        rc.fscal0     = 0x1F
        rc.test2      = 0x88
        rc.test1      = 0x31
        rc.test0      = 0x09
        rc.pa_table7  = 0x00
        rc.pa_table6  = 0x00
        rc.pa_table5  = 0x00
        rc.pa_table4  = 0x00
        rc.pa_table3  = 0x00
        rc.pa_table2  = 0x00
        rc.pa_table1  = 0x00
        #rc.pa_table0  = 0x8e
        rc.pa_table0  = 0xc0
        self.setModeIDLE()
        self.setRadioConfig()
        self.setModeRX()

    def testTX(self, data="XYZABCDEFGHIJKL"):
        while (sys.stdin not in select.select([sys.stdin],[],[],0)[0]):
            time.sleep(.4)
            print "transmitting %s" % repr(data)
            self.RFxmit(data)
        sys.stdin.read(1)
    def checkRepr(self, matchstr, checkval, maxdiff=0):
        starry = self.reprRadioConfig().split('\n')
        line,val = getValueFromReprString(starry, matchstr)
        try:
            f = checkval.__class__(val.split(" ")[0])
            if abs(f-checkval) <= maxdiff:
                print "  passed: reprRadioConfig test: %s %s" % (repr(val), checkval)
            else:
                print " *FAILED* reprRadioConfig test: %s %s %s" % (repr(line), repr(val), checkval)

        except ValueError, e:
            print "  ERROR checking repr: %s" % e


def unittest(self):
    print "\nTesting USB ping()"
    self.ping(3)
    
    print "\nTesting USB ep0Ping()"
    self.ep0Ping()
    
    print "\nTesting USB enumeration"
    print "getString(0,100): %s" % repr(self._do.getString(0,100))
    
    print "\nTesting USB EP MAX_PACKET_SIZE handling (ep0Peek(0xf000, 100))"
    print repr(self.ep0Peek(0xf000, 100))

    print "\nTesting USB EP MAX_PACKET_SIZE handling (peek(0xf000, 300))"
    print repr(self.peek(0xf000, 400))

    print "\nTesting getValueFromReprString()"
    starry = self.reprRadioConfig().split('\n')
    print repr(getValueFromReprString(starry, 'khz'))

    print "\nTesting reprRadioConfig()"
    print self.reprRadioConfig()

    print "\nTesting Frequency Get/Setters"
    # FREQ
    freq0,freq0str = self.getFreq()

    testfreq = 902000000
    self.setFreq(testfreq)
    freq,freqstr = self.getFreq()
    if abs(testfreq - freq) < 1024:
        print "  passed: %d : %f  (diff: %f)" % (testfreq, freq, testfreq-freq)
    else:
        print " *FAILED* %d : %f  (diff: %f)" % (testfreq, freq, testfreq-freq)

    testfreq = 868000000
    self.setFreq(testfreq)
    freq,freqstr = self.getFreq()
    if abs(testfreq - freq) < 1024:
        print "  passed: %d : %f  (diff: %f)" % (testfreq, freq, testfreq-freq)
    else:
        print " *FAILED* %d : %f  (diff: %f)" % (testfreq, freq, testfreq-freq)

    testfreq = 433000000
    self.setFreq(testfreq)
    freq,freqstr = self.getFreq()
    if abs(testfreq - freq) < 1024:
        print "  passed: %d : %f  (diff: %f)" % (testfreq, freq, testfreq-freq)
    else:
        print " *FAILED* %d : %f  (diff: %f)" % (testfreq, freq, testfreq-freq)
   
    self.checkRepr("Frequency:", float(testfreq), 1024)
    self.setFreq(freq0)

    # CHANNR
    channr0 = self.getChannel()
    for x in range(15):
        self.setChannel(x)
        channr = self.getChannel()
        if channr != x:
            print " *FAILED* get/setChannel():  %d : %d" % (x, channr)
        else:
            print "  passed: get/setChannel():  %d : %d" % (x, channr)
    self.checkRepr("Channel:", channr)
    self.setChannel(channr0)

    # IF and FREQ_OFF
    freq_if, freqoff = self.getFsIFandOffset()
    for fif, foff in ((164062,1),(140625,2),(187500,3)):
        self.setFsIFandOffset(fif,foff)
        nfif,nfoff = self.getFsIFandOffset()
        if abs(nfif - fif) > 5:
            print " *FAILED* get/setFsIFandOffset():  %d : %f (diff: %f)" % (fif,nfif,nfif-fif)
        else:
            print "  passed: get/setFsIFandOffset():  %d : %f (diff: %f)" % (fif,nfif,nfif-fif)

        if foff != nfoff:
            print " *FAILED* get/setFsIFandOffset():  %d : %d (diff: %d)" % (foff,nfoff,nfoff-foff)
        else:
            print "  passed: get/setFsIFandOffset():  %d : %d (diff: %d)" % (foff,nfoff,nfoff-foff)
    self.checkRepr("Intermediate freq:", fif, 11720)
    self.checkRepr("Frequency Offset:", foff)
    
    self.setFsIFandOffset(freq_if, freqoff)

def getValueFromReprString(stringarray, line_text):
    for string in stringarray:
        if line_text in string:
            idx = string.find(":")
            val = string[idx+1:].strip()
            return (string,val)

def mkFreq(freq=902000000, mhz=24):
    freqmult = (0x10000 / 1000000.0) / mhz
    num = int(freq * freqmult)
    freq2 = num >> 16
    freq1 = (num>>8) & 0xff
    freq0 = num & 0xff
    return (num, freq2,freq1,freq0)



if __name__ == "__main__":
    idx = 0
    if len(sys.argv) > 1:
        idx = int(sys.argv.pop())
    d = USBDongle(idx=idx)


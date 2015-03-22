import cc1111client
import threading
import time,sys,struct
import Gnuplot as gp
from chipcondefs import *


# commands
app = 1
FHSS_CMD_RESET =           0x00
FHSS_CMD_DUMP_MINS =       0x01
FHSS_CMD_DUMP_MAXS =       0x02
FHSS_CMD_DUMP_CHANSPACING= 0x03
FHSS_CMD_DUMP_MAXS =       0x04
FHSS_CMD_DUMP_HOPS =       0x06
FHSS_CMD_CONFIG_MINS =     0x10
FHSS_CMD_CONFIG_MAXS =     0x11
FHSS_CMD_CONFIG_CHANS =    0x12
FHSS_CMD_CONFIG_HOPS =     0x13
FHSS_CMD_CONFIG_RADIO =    0x14
FHSS_CMD_CONFIG_BASE_FREQ =0x15
FHSS_CMD_GET_BASE_FREQ =   0x16
FHSS_CMD_SET_MODE =        0x20
FHSS_CMD_GET_MODE =        0x21
FHSS_CMD_SET_NUMCHANS =    0x22
FHSS_CMD_SET_CHANSPACING = 0x23
FHSS_CMD_SET_THRESHOLD =   0x24
FHSS_CMD_GET_THRESHOLD =   0x25
FHSS_CMD_GET_HOP_CNT =     0x26
FHSS_CMD_GET_NUMCHANS =    0x27


DATA_MINS =                 2
DATA_MAXS =                 3
DATA_CHANMAX =              4
DATA_CHANSPC =              5
DATA_HOPS =                 6
DATA_THRESHOLDS =           7
DATA_THRESHOLD =            8
DATA_CHAN_RSSI =            9

MODE_LAZY =            0xf
MODE_RESET =           0
MODE_INIT_FLOOR =      1
MODE_SCAN_FLOOR =      2
MODE_INIT_CHANS =      3
MODE_SCAN_CHANS =      4
MODE_INIT_HOPS =       5
MODE_SCAN_HOPS =       6
MODE_INIT_SPECAN =     7
MODE_SPECAN =          8


datatypes = {
        DATA_MINS       : ("DATA_MINS ",1),
        DATA_MAXS       : ("DATA_MAXS ",1),
        DATA_CHANMAX    : ("DATA_CHANMAX ",1),
        DATA_CHANSPC    : ("DATA_CHANSPC ",4),
        DATA_CHAN_RSSI  : ("DATA_CHAN_RSSI ",1),
        DATA_HOPS       : ("DATA_HOPS ",4),
        DATA_THRESHOLDS : ("DATA_THRESHOLDS",1),
        DATA_THRESHOLD  : ("DATA_THRESHOLD",1),
    }



class FHSS (cc1111client.USBDongle):
    def __init__(self):
        cc1111client.USBDongle.__init__(self)
        self.makeGraphs()
        self.history = [[] for x in range(10)]

    def makeGraphs(self):
        self.graphs = [None for x in xrange(10)]
        self.minplot = gp.Gnuplot()
        self.minplot.ylabel("rssi")
        self.minplot.xlabel("channel")
        self.minplot.title("max rssi / min rssi / etc")
        self.graphs[0] = self.minplot
        self.maxplot = gp.Gnuplot()
        self.maxplot.ylabel("time (scan loops)")
        self.maxplot.xlabel("channel")
        self.maxplot.title("channel hopping over time")
        self.graphs[1] = self.maxplot
        self.rssiplot = gp.Gnuplot()
        self.rssiplot.ylabel("rssi")
        self.rssiplot.xlabel("channel")
        self.rssiplot.set_range('yrange',(0,128))
        self.rssiplot('set style data linespoints')
        self.rssiplot.title("freq spectrum analysis")
        self.graphs[2] = self.rssiplot
        #self.threshplot = gp.Gnuplot()
        #self.threshplot.ylabel("rssi")
        #self.threshplot.xlabel("channel")
        #self.threshplot.title("thresholds")
        #self.graphs[DATA_THRESHOLDS] = self.threshplot
        #self.chanmaxplot = gp.Gnuplot()
        #self.chanmaxplot.ylabel("rssi")
        #self.chanmaxplot.xlabel("channel")
        #self.chanmaxplot.title("chanmaxplot")
        #self.graphs[DATA_MAXS] = self.maxplot

    def interpret(self, delay=.1, stored_data=None, chanslist=None):
        # FIXME: make an interactive version that allows for speeding/slowing and stopping, backing up, etc...
        # FIXME: make timestamp show up on plot.
        idx = 0
        if chanslist is None:
            chanslist = [x/1000000.0 for x in self.getSpecAnChans()]

        while True:
            time.sleep(delay)
            if stored_data is None:
                msg = self.recv(2)
                if (msg is None):
                    sys.stderr.write('i')
                    continue

            else:
                try:
                    msg = stored_data[idx]
                    print idx, repr(msg)
                except IndexError, e:
                    return
                idx += 1

            msg, t = msg
            array = []
            dataarray = []
            cmd = ord(msg[1])
            data = msg[4:]
            res = datatypes.get(cmd, None)
            if res != None:             #plot it! w00t!
                tname,twidth = res
                fmt = (None, "B", "<H", None, "<L")[twidth]
                #plot = self.graphs[cmd]
                if (cmd == DATA_HOPS):
                    plot = self.graphs[1]
                    for x in xrange(0,len(data),twidth):
                        loop, = struct.unpack("<H", data[x:x+2])
                        chan = ord(data[x+2])
                        rssi = ord(data[x+3])
                        print >>sys.stderr,("loop %-5d: chan: %x rssi: %x"%(loop,chan,rssi))
                        array.append((chan,loop))
                    plot.replot(array)

                elif (cmd == DATA_CHAN_RSSI):
                    # spectrum analyzer data
                    plot = self.graphs[2]
                    dataarray = [ struct.unpack(fmt,buffer(data)[x:x+twidth])[0] for x in xrange(0,len(data),twidth) ]
                    array = [ (chanslist[x], ord(data[x])) for x in xrange(len(dataarray)) ]
                    plot.plot(array)
                    plot.refresh()
                
                else:
                    plot = self.graphs[0]
                    dataarray = [ struct.unpack(fmt,buffer(data)[x:x+twidth])[0] for x in xrange(0,len(data),twidth) ]
                    array = [ (hex(x)) for x in dataarray ]
                    self.history[cmd].append(dataarray)
                    print >>sys.stderr,("%20s :  %s"%(tname, repr(array)))
                    if (plot != None):
                        plot.replot([(x,ord(data[x])) for x in xrange(len(data))])
                        print (datatypes[cmd])
                        #plot.refresh()

    def plotHoppingPattern(self, msg):
        if (msg != None):
            array = []
            cmd = ord(msg[1])
            data = msg[4:]
            res = datatypes.get(cmd, None)
            if res != None:             #plot it! w00t!
                tname,twidth = res
                fmt = (None, "B", "<H", None, "<L")[twidth]
                if (cmd == DATA_HOPS):
                    plot = self.graphs[1]
                    for x in xrange(0,len(data),twidth):
                        loop, = struct.unpack("<H", data[x:x+2])
                        chan = ord(data[x+2])
                        rssi = ord(data[x+3])
                        print >>sys.stderr,("loop %-5d: chan: %x rssi: %x"%(loop,chan,rssi))
                        array.append((chan,loop))
                    plot.replot(array)
                else:
                    print >>sys.stderr,("cmd != DATA_HOPS")
            else:
                print >>sys.stderr,("no resource type found")

    def reset(self):
        self.setMode(MODE_RESET)

    def lazy(self):
        self.setMode(MODE_LAZY)

    def dumpMins(self):
        mins,t = self.send(app, FHSS_CMD_DUMP_MINS, '')
        return mins
    def dumpMaxs(self):
        maxs,t = self.send(app, FHSS_CMD_DUMP_MAXS, '')
        return maxs
    def getChanSpacing(self):
        spc,t = self.send(app, FHSS_CMD_DUMP_CHANSPACING, '')
        if spc != None:
            spc, = struct.unpack("<L", spc[4:])
        return spc
    def dumpHops(self):
        hops,t = self.send(app, FHSS_CMD_DUMP_HOPS, '')
        return hops

    def setMode(self, mode, reps=None):
        if self._debug:
            print >>sys.stderr,("SetMode(%x) (reps=%s)" % (mode,reps))

        if (reps==None):
            self.send(app, FHSS_CMD_SET_MODE, struct.pack("<H", mode))
        else:
            self.send(app, FHSS_CMD_SET_MODE, struct.pack("<H", mode)+struct.pack("<H", reps))

    def getMode(self):
        retval,t = self.send(app, FHSS_CMD_GET_MODE, '')
        if retval != None:
            return struct.unpack("B",retval[4:])[0]
        return -1 # should we throw an exception?

    def getNumChans(self):
        return ord(self.send(app, FHSS_CMD_GET_NUMCHANS, '')[0][4:])

    def setNumChans(self, num):
        self.send(app, FHSS_CMD_SET_NUMCHANS, struct.pack("<H", num))

    def setChanSpacing(self, spc):
        self.send(app, FHSS_CMD_SET_CHANSPACING, struct.pack("<L", spc))

    def setBaseFreq(self, frequency):
        self.send(app, FHSS_CMD_CONFIG_BASE_FREQ, struct.pack("<L", frequency))

    def getBaseFreq(self, mhz=24):
        freqmult = (0x10000 / 1000000.0) / mhz
        bytedef,t = self.send(app, FHSS_CMD_GET_BASE_FREQ, '')
        bytedef = bytedef[4:]
        if (len(bytedef) != 4):
            raise(Exception("unknown data returned for getFreq(): %s"%repr(bytedef)))
        return struct.unpack("<I", bytedef)[0]

    def setThreshold(self, thresh):
        self.send(app, FHSS_CMD_SET_MODE, struct.pack("B", thresh))

    def getThreshold(self):
        retval,t = self.send(app, FHSS_CMD_GET_MODE, '')
        if retval != None:
            return struct.unpack("B",retval[4:])[0]
        return -1 # should we throw an exception?

    def doFloorAndCeiling(self):
        self.setMode(MODE_INIT_FLOOR)

    def doChannelIdent(self):
        self.setMode(MODE_INIT_CHANS)

    def doHopTracking(self):
        self.setMode(MODE_INIT_HOPS)

    def doSpecAn(self):
        """
        this will start the spectrum analysis, and begin plotting.  no data is saved.  it returns immediately.
        """
        self.setMode(MODE_INIT_SPECAN)
        try:
            self.t=threading.Thread(target=self.interpret)
            self.t.setDaemon(True)
            self.t.start()
        except:
            self.setMode(MODE_LAZY)

    def getSpecAnChans(self):
        chanlist = []
        chanspc = self.getChanSpacing()
        base_freq = self.getBaseFreq()
        for x in range(self.getNumChans()):
            chanlist.append(base_freq + (chanspc*x))
        return chanlist

    def startSpecAn(self):
        """
        this will start the dumping of spectrum analysis data.  this data will accumulate in the MBOX until picked up.
        """
        self.setMode(MODE_INIT_SPECAN)

    def stopSpecAn(self):
        """
        this will stop the dumping of spectrum analysis data.
        """
        self.setMode(MODE_LAZY)

if __name__ == "__main__":
    d=FHSS()




def derange():
    deranged = {}
    for y in range(len(d.history[9])):
        for x in range(len(d.history[9][0])):
            t = deranged.get(x, None)
            if t == None:
                t = []
                deranged[x] = t
            t.append(d.history[9][y][x])
    return deranged



#!/usr/bin/env python
# GoodFET Chipcon Example
#                                                                                                                                          
# (C) 2009 Travis Goodspeed <travis at radiantmachines.com>
#                                                                                                                                          
# This code dumps the spectrum analyzer data from Mike Ossmann's
# spectrum analyzer firmware.                                                                                                              

import sys;
import Gnuplot as plt

# sys.path.append('/Users/travis/svn/goodfet/trunk/client/')
sys.path.append('/home/cutaway/Hardware/Goodfet/trunk/client/')

from GoodFETCC import GoodFETCC;
from intelhex import IntelHex16bit, IntelHex;
import time;

client=GoodFETCC();
client.serInit();

client.setup();
client.start();
client.CChaltcpu();
time.sleep(1);
client.CCreleasecpu();
time.sleep(1);

bytestart=0xf000;
#maxchan=132;
maxchan=53;
round=0;

g = plt.Gnuplot()

#print "time freq rssi maxrssi";
xlabel = "freq"
ylabel = "rssi"
title = "ccspecan_matplot"
g.xlabel(xlabel)
g.set_range('xrange',(900,930))
g.ylabel(ylabel)
g.set_range('yrange',(40,250))
g.title(title)
g('set style data linespoints')

########################
# Grab Freqs and store
########################
client.CChaltcpu();
freqs = []
mrssi = []
for entry in range(0,maxchan):
    adr=bytestart+entry*8
    freq=((client.CCpeekdatabyte(adr+0)<<16)+
          (client.CCpeekdatabyte(adr+1)<<8)+
          (client.CCpeekdatabyte(adr+2)<<0))
    hz=freq*366.21093303
    freqs.append(hz/1000000.0)
    # prep with the proper number of max rssi values
    mrssi.append(client.CCpeekdatabyte(adr+7))
client.CCreleasecpu();

while 1:
    time.sleep(.2)
    rssi = []
    disp = []
    mdisp = []
    d = 0
    md = 0
    
    round=round+1;
    
    client.CChaltcpu()

    for entry in range(0,maxchan):
        adr=bytestart+entry*8;
        rssi.append(client.CCpeekdatabyte(adr+6))
        #print "%03i %3.3f %03i" % (round,freqs[entry],rssi);

    client.CCreleasecpu();
    
    for entry in range(0,maxchan):
        if rssi[entry] > mrssi[entry]:
            mrssi[entry] = rssi[entry]
        disp.append((freqs[entry],rssi[entry]))
        mdisp.append((freqs[entry],mrssi[entry]))

    d = plt.Data(disp)
    md = plt.Data(mdisp)
    g.title(title+str(round))
    g.plot(d,md)
    if round != 1:
        g.refresh()



#!/usr/bin/env python
# GoodFET HedyAttack Maxscan Example

############################ 
# HedyAttack - Tools for identifying and analyzing frequency hopping spread spectrum(fhss) implementations.
# Copyright (C) 2011  Cutaway, Q, and Atlas
# 2009 Travis Goodspeed <travis at radiantmachines.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Please leave comments and recommendations at http://code.google.com/p/hedyattack
############################  

# This code dumps the maximum radio signal strength signal indication value
# as recorded in the cc1111emk's xdata
# ------------------------------------------------------------------------------
# | Channel Modes                                                              |
# ------------------------------------------------------------------------------
# | Marker | MSB Loop | LSB Loop | channel | channel | ... | channel | channel |
# | 0xFE   | Count    | Count    | number  | rssi    | ... | number  | rssi    |
# ------------------------------------------------------------------------------

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
client.CCreleasecpu();

time.sleep(1);

chanstart=0xf000;
#maxchan=132;
maxchan=53;
round=0;

g = plt.Gnuplot()

#print "time freq rssi maxrssi";
xlabel = "freq"
ylabel = "rssi"
title = "maxscan_plot"
g.xlabel(xlabel)
g.set_range('xrange',(900,930))
g.ylabel(ylabel)
g.set_range('yrange',(40,250))
g.title(title)
g('set style data linespoints')

########################
# Grab Freqs and store
########################
freqs = []
client.CChaltcpu();
for entry in range(0,maxchan):
    adr=chanstart+entry*8
    freq=((client.CCpeekdatabyte(adr+0)<<16)+
          (client.CCpeekdatabyte(adr+1)<<8)+
          (client.CCpeekdatabyte(adr+2)<<0));
    hz=freq*366.21093303
    freqs.append(hz/1000000.0)
client.CCreleasecpu()
time.sleep(1);

########################
# Grab RSSI and MRSSI
########################
run = True
while run == True:
    time.sleep(3)
    client.CChaltcpu()
    rssi = []
    mrssi = []
    d0 = []
    d1 = []
    dn = 0
    dm = 0
    
    round=round+1
    
    #g.show()
    for entry in range(0,maxchan):
        adr=chanstart+entry*8;
        rssi.append(client.CCpeekdatabyte(adr+6));
        mrssi.append(client.CCpeekdatabyte(adr+7));
        #print "%03i %3.3f %03i" % (round,freqs[entry],rssi);
    d0.append((freqs,rssi))
    d1.append((freqs,mrssi))
    dn = plt.Data(d0)
    dm = plt.Data(d1)
    g.title(title+str(round))
    #g.plot(dn)
    g.plot(dn,dm)
    if round != 1:
        g.refresh()
    client.CCreleasecpu();



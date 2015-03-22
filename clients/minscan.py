#!/usr/bin/env python
# GoodFET HedyAttack Minscan Example

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

# This code dumps the mininum radio signal strength signal indication value
# as recorded in the cc1111emk's xdata.  The values are stored according to
# the following formatting.

import sys;
#import Gnuplot as plt

# sys.path.append('/Users/travis/svn/goodfet/trunk/client/')
sys.path.append('/home/cutaway/Hardware/Goodfet/trunk/client/')

from GoodFETCC import GoodFETCC;
from intelhex import IntelHex16bit, IntelHex;
import time;

# Sleep Intervals in Seconds
sshsi = 1       # one second
shsi = 10       # 10 seconds
mdsi = 60       # 1 minute
lgsi = 120      # 2 minutes
xlgsi = 1200    # 20 minutes

client=GoodFETCC();
client.serInit();

client.setup();
client.start();

# Start and stop if previously something failed
client.CChaltcpu();
client.CCreleasecpu();


# Map channel number to approximate frequency
# 0 == 902 thru 52 == 928 with a step of .5 MHz
max_chan=53; # however 0 IS included, so 53
chan_dict = dict([(x,((x*.5)+902)) for x in range(max_chan)])

#bytestart=0xf000;
chan_data_start=0xF000 # 53 chans x 6 bytes
chan_min_start=0xF13E  # 53 chans x 1 byte
chan_chg_start=0xF173  # 3500 bytes
max_chg = 3000
#chan_loops=0xFF1F      # one byte
chan_loops=0xFD2B      # one byte
chg_marker = 0xFE

# how many times have we run through this?
iter_marker = 0

# Running
print("Grabbing Data\n")

while 1:
    # Don't print until AFTER we release CPU so it can continue
    client.CChaltcpu();
    time.sleep(sshsi);

    # Get the current iteration when CPU halted
    #curr_loop = ((client.CCpeekdatabyte(chan_loops)<<8)+
                #(client.CCpeekdatabyte(chan_loops+1)<<0))
    curr_loop = ((client.CCpeekdatabyte(chan_loops)<<0)+
                (client.CCpeekdatabyte(chan_loops+1)<<8))
    #print "curr_loop:",curr_loop
    #print [client.CCpeekdatabyte(chan_loops+1),client.CCpeekdatabyte(chan_loops)]
    

    ###################################################
    # Debugging
    ###################################################
    # Grab and Store FREQ values for each channel
#    print("    Grabbing Channel Frequency values for all Channels.")
#    chan_freq = {}
#    for entry in range(0,max_chan):
#        adr=chan_data_start+entry*6;
#        freq=((client.CCpeekdatabyte(adr+0)<<16)+
#              (client.CCpeekdatabyte(adr+1)<<8)+
#              (client.CCpeekdatabyte(adr+2)<<0));
#        chan_freq[entry] = freq*396.728515625;
    ###################################################
    
    # Grab and Store MIN values for each channel
    print("    Grabbing Minimum RSSI values for all Channels.")
    min_rssi = {}
    for key in chan_dict:
        min_rssi[key] = client.CCpeekdatabyte(chan_min_start + key)

    # Grab and Store values for each changed channel
    print("    Grabbing Channels that changed.")
    chg_chans_array = []
    for cnt in range(max_chg+10):
        chg_chans_array.append(client.CCpeekdatabyte(chan_chg_start + cnt))

    # Restart CPU    
    client.CCreleasecpu()

    ###################################################

    # Done with Run
    print("    Grabbing Data Completed\n")

    # PRINT HERE
#    print("    Printing Channel FREQ values for all Channels.")
#    for key, val in chan_freq.items():
#        print key, val

    mod = 1
    cnt = 0
    chg_out = []
    print("    Printing Minimum RSSI values for all Channels.")
    for key, val in min_rssi.items():
        chg_out.append([key, val])
        if not mod % 10:
            cnt += 1
            #print cnt*10,chg_out
            print chg_out
            chg_out = []
        mod += 1
    print chg_out

    print("    Printing Channels that changed.")
    print("    Curr_loop, Iter_cnt, Store_loop, Chan, RSSI, ...")

#    mod = 1
#    cnt = 0
#    chg_out = []
#    for i in chg_chans_array:
#        chg_out.append(i)
#        if not mod % 10:
#            cnt += 1
#            print cnt*10,chg_out
#            chg_out = []
#        mod += 1
#    print chg_out

    chg_chans   = [] # main
    chg_chan  = [] # reset sub
    mkd = 0
    chg_chans_array.pop(0)
    for i in chg_chans_array:
        if i == 0xFE:
            if mkd >= 2:
                if len(chg_chan):
                    ## Take first two values and concantenate for
                    ## when the loop counter when the data was 
                    ## stored.  Pop those values off then add
                    ## counters

                    #chg_chans.append(chg_chan)
                    try:        # Data Debug can stop ANYWHERE, so be ready to bail
                        lp_cnt = 0
                        lp_cnt = ((chg_chan[0]<<8)+(chg_chan[1]<<0))
                        chg_chan = chg_chan[2:]
                        chg_chan.insert(0,lp_cnt)
                        chg_chan.insert(0,iter_marker)  # how many times has THIS python loop run?
                        chg_chan.insert(0,curr_loop)
                        chg_chans.append(tuple(chg_chan))
                    except:
                        pass    # just let it go
                    ## Start over
                    chg_chan = []
                    mkd = 0
            else:
                chg_chan.append(i)
        else:
            if i != 0xFE:
                mkd += 1
            chg_chan.append(i)

    # one more time through
    iter_marker += 1

    for i in chg_chans:
        print i

    print "    Run Done\n"
    # Give time to keep running
    # May not need if we start printing here
    time.sleep(mdsi)

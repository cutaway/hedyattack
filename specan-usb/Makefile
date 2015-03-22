
#################################################################################
# HedyAttack - Tools for identifying and analyzing frequency hopping spread spectrum(fhss) implementations.
# Copyright (C) 2011  Cutaway, Q, and Atlas
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
#################################################################################

frm = ../firmware
#libs = setup.rel pm.rel cc1111usb.rel global.rel cc1111rf.rel cc1111usbdebug.rel
libs = setup.rel pm.rel cc1111usb.rel global.rel cc1111usbdebug.rel
CC = sdcc
CFLAGS = --no-pack-iram -I../includes
LFLAGS = --xram-loc 0xF000

all: specan-usb.hex

#%.rel : %.c
	#$(CC) $(CFLAGS) -c $<

#specan-usb: specan-usb.hex

specan-usb.hex: specan-usb.rel $(libs)
	sdcc $(LFLAGS) specan-usb.rel $(libs)
	packihx <specan-usb.ihx >$(frm)/specan-usb.hex

specan-usb.rel: specan-usb.c
	$(CC) $(CFLAGS) -DDONSDONGLES -c $<

setup.rel: ../lib/setup.c
	$(CC) $(CFLAGS) -c $<

pm.rel: ../lib/pm.c
	$(CC) $(CFLAGS) -c $<

global.rel: ../lib/global.c
	$(CC) $(CFLAGS) -DDONSDONGLES -c $<

cc1111rf.rel: ../lib/cc1111rf.c
	$(CC) $(CFLAGS) -DDONSDONGLES -c $<

cc1111usb.rel: ../lib/cc1111usb.c
	$(CC) $(CFLAGS) -DDONSDONGLES -c $<

cc1111usbdebug.rel: ../lib/cc1111usbdebug.c
	$(CC) $(CFLAGS) -DDONSDONGLES -c $<

install-specan-usb: specan-usb.hex
	goodfet.cc erase
	goodfet.cc flash $(frm)/specan-usb.hex

verify-specan-usb: specan-usb.hex
	goodfet.cc verify $(frm)/specan-usb.hex

clean:
	rm -f *.hex *.ihx *.rel *.asm *.lst *.rst *.sym *.lnk *.map *.mem

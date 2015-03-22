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
CC=sdcc
#CFLAGS=--no-pack-iram $(CF)
CFLAGS = --no-pack-iram $(CF) -I../includes
LFLAGS=--xram-loc 0xF000 

apps = cc1111rf.rel global.rel
appsc = cc1111rf.relc global.relc
appsd = cc1111rf.reld global.reld
appsi = cc1111rf.reli global.reli

libsvcom = $(apps) cc1111_vcom.rel cc1111_vcomdesc.rel cc1111usbdebug.rel #cc1111usb.rel
libsvcomc = $(appsc) cc1111_vcom.relc cc1111_vcomdesc.relc cc1111usbdebug.relc #cc1111usb.rel
libsvcomd = $(appsd) cc1111_vcom.reld cc1111_vcomdesc.reld cc1111usbdebug.reld #cc1111usb.rel
libsvcomi = $(appsi) cc1111_vcom.reli cc1111_vcomdesc.reli cc1111usbdebug.reli #cc1111usb.rel

libsusb  = $(apps) cc1111usb.rel cc1111usbdebug.rel
libsusbc  = $(appsc) cc1111usb.rc cc1111usbdebug.relc
libsusbd  = $(appsd) cc1111usb.rd cc1111usbdebug.reld
libsusbi  = $(appsi) immeusb.reli immeusbdebug.reli

libsimme  = $(appsi) immeio.reli immekeys.reli immefont.reli   #  immedisplay.rel immeterm.rel
libsi  = $(apps) immeio.rel immekeys.rel immefont.rel   #  immedisplay.rel immeterm.rel
#srcs = $(libs,.rel=.c) 

all: chronosfhss.hex
chronosfhss: chronosfhss.hex
donsfhss: donsfhss.hex
immefhss: immefhss.hex

#hedyattack.rel: hedyattack.c
#	$(CC) $(CFLAGS) -c $<


cc1111usb.rc : cc1111usb.c
	./new_serial.py
	$(CC) $(CFLAGS) -DCHRONOSDONGLE -c cc1111usb.c

cc1111usb.rd : cc1111usb.c
	./new_serial.py
	$(CC) $(CFLAGS) -DDONSDONGLES -c cc1111usb.c

%.relc : %.c
	$(CC) $(CFLAGS) -DCHRONOSDONGLE -c $<

%.reld : %.c
	$(CC) $(CFLAGS) -DDONSDONGLES -c $<

%.reli : %.c
	$(CC) $(CFLAGS) -DIMMEDONGLE -DIMME -c $<

chronosfhss.hex: hedyattack.relc $(libsusbc)
	sdcc $(LFLAGS) hedyattack.rel $(libsusb)
	packihx <hedyattack.ihx >$(frm)/hedyattack.hex

donsfhss.hex: hedyattack.reld $(libsusbd)
	sdcc $(LFLAGS) hedyattack.rel $(libsusb) -DDONSDONGLES
	packihx <hedyattack.ihx >$(frm)/hedyattack.hex

immefhss.hex: hedyattack.reli $(libsusbi)
	sdcc $(LFLAGS) hedyattack.rel $(libsusb) -DIMMEDONGLE
	packihx <hedyattack.ihx >$(frm)/hedyattack.hex



installdonsfhss: donsfhss.hex
	goodfet.cc erase
	goodfet.cc flash $(frm)/hedyattack.hex
	goodfet.cc info

installimmefhss: immefhss.hex
	goodfet.cc erase
	goodfet.cc flash $(frm)/hedyattack.hex
	goodfet.cc info

installchronosfhss: chronosfhss.hex
	goodfet.cc erase
	goodfet.cc flash $(frm)/hedyattack.hex
	goodfet.cc info



verify: hedyattack.hex
	goodfet.cc verify $(frm)/hedyattack.hex

clean:
	rm -f *.ihx *.rel *.hex *.sym *.asm *.lst *.lnk *.map *.mem *.rst

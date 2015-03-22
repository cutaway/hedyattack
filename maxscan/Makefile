
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
libs = setup.rel pm.rel
CC = sdcc
CFLAGS = --no-pack-iram -I../includes
LFLAGS = --xram-loc 0xF000

all: maxscan.hex

#%.rel : %.c
	#$(CC) $(CFLAGS) -c $<

#maxscan: maxscan.hex

maxscan.hex: maxscan.rel $(libs)
	sdcc $(LFLAGS) maxscan.rel $(libs)
	packihx <maxscan.ihx >$(frm)/maxscan.hex

maxscan.rel: maxscan.c
	$(CC) $(CFLAGS) -c $<

setup.rel: ../lib/setup.c
	$(CC) $(CFLAGS) -c $<

pm.rel: ../lib/pm.c
	$(CC) $(CFLAGS) -c $<

install-maxscan: maxscan.hex
	goodfet.cc erase
	goodfet.cc flash $(frm)/maxscan.hex

verify-maxscan: maxscan.hex
	goodfet.cc verify $(frm)/maxscan.hex

clean:
	rm -f *.hex *.ihx *.rel *.asm *.lst *.rst *.sym *.lnk *.map *.mem

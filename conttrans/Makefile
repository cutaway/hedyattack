
frm = ../firmware
libs = setup.rel pm.rel
CC = sdcc
CFLAGS = --no-pack-iram -I../includes
LFLAGS = --xram-loc 0xF000

all: conttrans.hex

#%.rel : %.c
	#$(CC) $(CFLAGS) -c $<

#conttrans: conttrans.hex

conttrans.hex: conttrans.rel $(libs)
	sdcc $(LFLAGS) conttrans.rel $(libs)
	packihx <conttrans.ihx >$(frm)/conttrans.hex

conttrans.rel: conttrans.c
	$(CC) $(CFLAGS) -c $<

setup.rel: ../lib/setup.c
	$(CC) $(CFLAGS) -c $<

pm.rel: ../lib/pm.c
	$(CC) $(CFLAGS) -c $<

install-conttrans: conttrans.hex
	goodfet.cc erase
	goodfet.cc flash $(frm)/conttrans.hex

verify-conttrans: conttrans.hex
	goodfet.cc verify $(frm)/conttrans.hex

clean:
	rm -f *.hex *.ihx *.rel *.asm *.lst *.rst *.sym *.lnk *.map *.mem

#!/usr/bin/env python
#
# Copyright 2011 Jared Boone and "stolen" by cutaway
#
# This file is part of Project CCcat.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.

# http://pyusb.sourceforge.net/docs/1.0/tutorial.html

import usb.core
import struct
import sys
import time
from array import array
import numpy

class CCcat(object):
    STATE_IDLE   = 0
    STATE_ACTIVE = 1
    
    def __init__(self, device):
        self._device = device
        self._device.default_timeout = 3000
        self._device.set_configuration()
        self._state = self.STATE_IDLE

    def _cmd_specan(self):
        self._device.ctrl_transfer(0x43, 0x00, '\x00')
        self._state = self.STATE_ACTIVE

    #def _cmd_specan(self, low_frequency, high_frequency):
        #low_frequency = int(round(low_frequency / 1e6))
        #high_frequency = int(round(high_frequency / 1e6))
        #self._device.ctrl_transfer(0x40, 27, low_frequency, high_frequency)
        #self._state = self.STATE_ACTIVE

    def specan(self, low_frequency, high_frequency):
        #spacing_hz = 1e6
        spacing_hz = 3e5
        chans      = 83
        header     = 4
        pkt        = chans + header
        bin_count = int(round((high_frequency - low_frequency) / spacing_hz)) + 1
        frequency_axis = numpy.linspace(low_frequency, high_frequency, num=bin_count, endpoint=True)
        
        #self._cmd_specan(low_frequency, high_frequency)
        self._cmd_specan()
        
        default_raw_rssi = -128
        rssi_offset = -54
        rssi_values = numpy.empty((bin_count,), dtype=numpy.float32)
        rssi_values.fill(default_raw_rssi + rssi_offset)
        
        data = array('B')
        last_index = None
        cnt = 0
        while True:
            buffer = self._device.read(0x85, pkt)
            data += buffer
            while len(data) >= pkt:
                header, block, data = data[:3], data[3:chans], data[chans:]
                app, cmd, dlen = struct.unpack('<BBH', header)
                while len(block):
                    item, block = block[0], block[1:]
                    if cnt == chans:
                        # We started a new frame, send the existing frame
                        yield (frequency_axis, rssi_values)
                        rssi_values.fill(default_raw_rssi + rssi_offset)
                        cnt = 0
                    rssi_values[index] = raw_rssi_value + rssi_offset
                    cnt += 1
                    
    def close(self):
        if self._state != self.STATE_IDLE:
            #self._device.ctrl_transfer(0x40, 21)
            self._state = self.STATE_IDLE
    
if __name__ == '__main__':
    device = usb.core.find(idVendor=0x0451, idProduct=0x4715)
    if device is None:
        raise Exception('Device not found')

    cccat = CCcat(device)
    frame_source = cccat.specan(0.902e9, 0.926e9)

    try:
        for frame in frame_source:
            print(frame)
    except KeyboardInterrupt:
        pass
    finally:
        cccat.close()

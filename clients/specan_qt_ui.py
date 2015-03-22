#!/usr/bin/env python
#
# Copyright 2011 Jared Boone
#
# This file is part of Project Ubertooth.
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

import sys
import threading

import numpy

import usb.core

from PySide import QtCore, QtGui
from PySide.QtCore import Qt, QPointF, QLineF

from specan_qt_dump import Ubertooth

class SpecanThread(threading.Thread):
    def __init__(self, device, low_frequency, high_frequency, new_frame_callback):
        threading.Thread.__init__(self)
        self.daemon = True
        
        self._device = device
        self._low_frequency = low_frequency
        self._high_frequency = high_frequency
        self._new_frame_callback = new_frame_callback
        self._stop = False
        self._stopped = False

    def run(self):
        frame_source = self._device.specan(self._low_frequency, self._high_frequency)
        for frequency_axis, rssi_values in frame_source:
            self._new_frame_callback(numpy.copy(frequency_axis), numpy.copy(rssi_values))
            if self._stop:
                break
            
    def stop(self):
        self._stop = True
        self.join(3.0)
        self._stopped = True

class RenderArea(QtGui.QWidget):
    def __init__(self, device, parent=None):
        QtGui.QWidget.__init__(self, parent)
        
        self._graph = None
        self._reticle = None
        
        self._device = device
        self._frame = None
        self._persisted_frames = None
        self._persisted_frames_depth = 350
        self._path_max = None
        
        self._low_frequency = 2.400e9
        self._high_frequency = 2.483e9
        self._frequency_step = 1e6
        self._high_dbm = 0.0
        self._low_dbm = -100.0
        
        self._thread = SpecanThread(self._device,
                                    self._low_frequency,
                                    self._high_frequency,
                                    self._new_frame)
        self._thread.start()
        
    def stop_thread(self):
        self._thread.stop()
    
    def _new_graph(self):
        self._graph = QtGui.QPixmap(self.width(), self.height())
        self._graph.fill(Qt.black)
    
    def _new_reticle(self):
        self._reticle = QtGui.QPixmap(self.width(), self.height())
        self._reticle.fill(Qt.transparent)
        
    def _new_persisted_frames(self, frequency_bins):
        self._persisted_frames = numpy.empty((self._persisted_frames_depth, frequency_bins))
        self._persisted_frames.fill(-128 + -54)
        self._persisted_frames_next_index = 0
    
    def minimumSizeHint(self):
        x_points = round((self._high_frequency - self._low_frequency) / self._frequency_step)
        y_points = round(self._high_dbm - self._low_dbm)
        return QtCore.QSize(x_points * 4, y_points * 1)
    
    def _new_frame(self, frequency_axis, rssi_values):
        self._frame = (frequency_axis, rssi_values)
        if self._persisted_frames is None:
            self._new_persisted_frames(len(frequency_axis))
        self._persisted_frames[self._persisted_frames_next_index] = rssi_values
        self._persisted_frames_next_index = (self._persisted_frames_next_index + 1) % self._persisted_frames.shape[0]
        self.update()
    
    def _draw_graph(self):
        if self._graph is None:
            self._new_graph()
        elif self._graph.size() != self.size():
            self._new_graph()
        
        painter = QtGui.QPainter(self._graph)
        try:
            painter.setRenderHint(QtGui.QPainter.Antialiasing)
            painter.fillRect(0, 0, self._graph.width(), self._graph.height(), QtGui.QColor(0, 0, 0, 10))
            
            if self._frame:
                frequency_axis, rssi_values = self._frame
                
                path_now = QtGui.QPainterPath()
                path_max = QtGui.QPainterPath()
                
                bins = range(len(frequency_axis))
                x_axis = self._hz_to_x(frequency_axis)
                y_now = self._dbm_to_y(rssi_values)
                y_max = self._dbm_to_y(numpy.amax(self._persisted_frames, axis=0))
                
                # TODO: Wrapped Numpy types with float() to support old (<1.0) PySide API in Ubuntu 10.10
                path_now.moveTo(float(x_axis[0]), float(y_now[0]))
                for i in bins:
                    path_now.lineTo(float(x_axis[i]), float(y_now[i]))
                
                # TODO: Wrapped Numpy types with float() to support old (<1.0) PySide API in Ubuntu 10.10
                path_max.moveTo(float(x_axis[0]), float(y_max[0]))
                for i in bins:
                    path_max.lineTo(float(x_axis[i]), float(y_max[i]))
                
                painter.setPen(Qt.white)
                painter.drawPath(path_now)
                self._path_max = path_max
        finally:
            painter.end()
            
    def _draw_reticle(self):
        if self._reticle is None or (self._reticle.size() != self.size()):
            self._new_reticle()
            
            dbm_lines = [QLineF(self._hz_to_x(self._low_frequency), self._dbm_to_y(dbm),
                                self._hz_to_x(self._high_frequency), self._dbm_to_y(dbm))
                         for dbm in numpy.arange(self._low_dbm, self._high_dbm, 20.0)]
            dbm_labels = [(dbm, QPointF(self._hz_to_x(self._low_frequency) + 2, self._dbm_to_y(dbm) - 2))
                          for dbm in numpy.arange(self._low_dbm, self._high_dbm, 20.0)]
            
            frequency_lines = [QLineF(self._hz_to_x(frequency), self._dbm_to_y(self._high_dbm),
                                      self._hz_to_x(frequency), self._dbm_to_y(self._low_dbm))
                               for frequency in numpy.arange(self._low_frequency, self._high_frequency, self._frequency_step * 10.0)]
            frequency_labels = [(frequency, QPointF(self._hz_to_x(frequency) + 2, self._dbm_to_y(self._high_dbm) + 10))
                                for frequency in numpy.arange(self._low_frequency, self._high_frequency, self._frequency_step * 10.0)]
            
            painter = QtGui.QPainter(self._reticle)
            try:
                painter.setRenderHint(QtGui.QPainter.Antialiasing)
                
                painter.setPen(Qt.blue)
                
                # TODO: Removed to support old (<1.0) PySide API in Ubuntu 10.10
                #painter.drawLines(dbm_lines)
                for dbm_line in dbm_lines: painter.drawLine(dbm_line)
                # TODO: Removed to support old (<1.0) PySide API in Ubuntu 10.10
                #painter.drawLines(frequency_lines)
                for frequency_line in frequency_lines: painter.drawLine(frequency_line)
                
                painter.setPen(Qt.white)
                for dbm, point in dbm_labels:
                    painter.drawText(point, '%+.0f' % dbm)
                for frequency, point in frequency_labels:
                    painter.drawText(point, '%.0f' % (frequency / 1e6))
                    
            finally:
                painter.end()
    
    def paintEvent(self, event):
        self._draw_graph()
        self._draw_reticle()
        
        painter = QtGui.QPainter(self)
        try:
            painter.setRenderHint(QtGui.QPainter.Antialiasing)
            painter.setPen(QtGui.QPen())
            painter.setBrush(QtGui.QBrush())

            if self._graph:
                painter.drawPixmap(0, 0, self._graph)
            
            if self._path_max:
                painter.setPen(Qt.green)
                painter.drawPath(self._path_max)

            painter.setOpacity(0.5)
            if self._reticle:
                painter.drawPixmap(0, 0, self._reticle)
        finally:
            painter.end()

    def _hz_to_x(self, frequency_hz):
        delta = frequency_hz - self._low_frequency
        range = self._high_frequency - self._low_frequency
        normalized = delta / range
        return normalized * self.width()
                             
    def _dbm_to_y(self, dbm):
        delta = self._high_dbm - dbm
        range = self._high_dbm - self._low_dbm
        normalized = delta / range
        return normalized * self.height()

class Window(QtGui.QWidget):
    def __init__(self, parent=None):
        QtGui.QWidget.__init__(self, parent)

        self._device = self._open_device()
        
        self.render_area = RenderArea(self._device)

        main_layout = QtGui.QGridLayout()
        main_layout.setContentsMargins(0, 0, 0, 0)
        main_layout.addWidget(self.render_area, 0, 0)
        self.setLayout(main_layout)
        
        self.setWindowTitle("Ubertooth Spectrum Analyzer")

    def sizeHint(self):
        return QtCore.QSize(480, 160)
    
    def _open_device(self):
        device = usb.core.find(idVendor=0xFFFF, idProduct=0x0004)
        if device is None:
            raise Exception('Device not found')
        return Ubertooth(device)
    
    def closeEvent(self, event):
        self.render_area.stop_thread()
        self._device.close()
        event.accept()

if __name__ == '__main__':
    app = QtGui.QApplication(sys.argv)
    window = Window()
    window.show()
    sys.exit(app.exec_())

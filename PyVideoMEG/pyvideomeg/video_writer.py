# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014 BioMag Laboratory, Helsinki University Central Hospital

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
"""
from os import path
import struct
import numpy
from .read_data import UnknownVersionError

class OverWriteError(Exception):
    """
    Thrown if trying to overwrite previous file.
    """
    pass

class VideoFile(object):
    """
    .Video.dat file
    """
    def __init__(self, file_name, ver, site_id=None, is_sender=None):
        if path.isfile(file_name):
            raise OverWriteError("Won't allow overwriting. File exists on path:\n" +
                                 file_name)
        else:
            self._file = open(file_name, 'wb')
            self._file.write('ELEKTA_VIDEO_FILE')  # Elekta magic string


            if ver == 1 or ver == 2:
                self._file.write(struct.pack('I', ver))
                self.site_id = -1
                self.is_sender = -1
                self.ver = ver
            elif ver == 3:
                self._file.write(struct.pack('I', ver))
                self._file.write(struct.pack('B', 0) if site_id is None else struct.pack('B', 1))
                self._file.write(struct.pack('B', 0) if is_sender is None else struct.pack('B', 1))
                self.ver = ver
            else:
                raise UnknownVersionError()

            self.timestamps = numpy.array([])
            self._frame_ptrs = []
            self._nframes = 0

    def __del__(self):
        self._file.close()

    def append_frame(self, timestamp, frame):
        """
        Appends a frame to the end of the file.
        """
        self._file.seek(0, 2)
        if self.ver == 1:
            self._file.write(struct.pack('QI', timestamp, len(frame)))
            self._frame_ptrs.append((self._file.tell(), len(frame)))
            self._file.write(frame)
            self._nframes = self._nframes + 1
            self.timestamps = numpy.append(self.timestamps, timestamp)
        else:
            print "Not implemented"

    def get_frame(self, indx):
        """
        Return indx-th frame a jpg image in the memory.
        """
        offset, frame_sz = self._frame_ptrs[indx]
        self._file.seek(offset)
        return self._file.read(frame_sz)

    def check_sanity(self):
        """
        Performs sanity checks against VideoFile, and reports problems.
        """
        for i in range(1, len(self.timestamps)):
            if self.timestamps[i - 1] >= self.timestamps[i]:
                print "Timestamps don't increase expectedly."
                break
        if self._nframes != len(self.timestamps):
            print "Framecount doesn't match the count of timestamps."
        
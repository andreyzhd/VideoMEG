# -*- coding: utf-8 -*-
"""
Created on Fri Dec 21 16:07:07 2012

@author: andrey
"""

import struct
import time
import math
import numpy

class UnknownVersionError(Exception):
    pass

def _read_attrib(data_file, ver):
    '''
    Read data block attributes. If cannot read the attributes (EOF?), return
    -1 in ts
    '''
    if ver == 1:
        attrib = data_file.read(12)
        if len(attrib) == 12:
            ts, sz = struct.unpack('QI', attrib)
        else:
            ts = -1
            sz = -1
        block_id = 0
        total_sz = sz + 12
        
    elif ver == 2 or ver == 3:
        attrib = data_file.read(20)
        if len(attrib) == 20:
            ts, block_id, sz = struct.unpack('QQI', attrib)
        else:
            ts = -1
            block_id = 0
            sz = -1
        total_sz = sz + 20
            
    else:
        raise UnknownVersionError()
        
    return ts, block_id, sz, total_sz
    
    
def ts2str(ts):
    """
    Convert timestamp to human-readable string. Slightly differs from the
    system tools that do the similar thing.
    """
    timestr = time.asctime(time.localtime(math.floor(ts / 1000)))
    yearstr = timestr[-4:len(timestr)]
    timestr = timestr[0:-5] + ('.%03i' % (ts % 1000)) + ' ' + yearstr
    return timestr
    
    
class AudioData:
    """
    To read an audio file initialize AudioData object with file name and get
    the data from the object's variables:
        srate     - nominal sampling rate
        nchan     - number of channels
        ts        - buffers' timestamps
        raw_audio - raw audio data
        buf_sz    - buffer size
    """
    def __init__(self, file_name):
        data_file = open(file_name, 'rb')    
        assert(data_file.read(len('ELEKTA_AUDIO_FILE')) == 'ELEKTA_AUDIO_FILE')  # make sure the magic string is OK 
        self.ver = struct.unpack('I', data_file.read(4))[0]
        
        if self.ver == 1 or self.ver == 2:        
            self.site_id = -1
            self.is_sender = -1            

        elif self.ver == 3:
            self.site_id, self.is_sender = struct.unpack('BB', data_file.read(2))
            
        else:
            raise UnknownVersionError()
            
        self.srate, self.nchan = struct.unpack('II', data_file.read(8))
        
        #get the size of the data part of the file
        begin_data = data_file.tell()
        data_file.seek(0, 2)
        end_data = data_file.tell()
        data_file.seek(begin_data, 0)
        
        ts, block_id, self.buf_sz, total_sz = _read_attrib(data_file, self.ver)
        data_file.seek(begin_data, 0)
        
        n_chunks = (end_data - begin_data) / total_sz
        self.raw_audio = bytearray(n_chunks * self.buf_sz)
        self.ts = numpy.zeros(n_chunks)

        for i in range(n_chunks):
            ts, block_id, sz, total_sz = _read_attrib(data_file, self.ver)
            self.raw_audio[self.buf_sz*i : self.buf_sz*(i+1)] = data_file.read(sz)
            self.ts[i] = ts

        data_file.close()
        
        
class VideoData:
    """
    To read a video file initialize VideoData object with file name. You can
    then get the frame times from the object's ts variable. To get individual
    frames use get_frame function.
    """
    def __init__(self, file_name):
        self._file = open(file_name, 'rb')
        assert(self._file.read(len('ELEKTA_VIDEO_FILE')) == 'ELEKTA_VIDEO_FILE')  # make sure the magic string is OK 
        self.ver = struct.unpack('I', self._file.read(4))[0]
        
        if self.ver == 1 or self.ver == 2:        
            self.site_id = -1
            self.is_sender = -1            

        elif self.ver == 3:
            self.site_id, self.is_sender = struct.unpack('BB', self._file.read(2))
            
        else:
            raise UnknownVersionError()

        self.ts = numpy.array([])
        self._frame_ptrs = []
        ts, block_id, sz, total_sz = _read_attrib(self._file, self.ver)

        while ts != -1:        # we did not reach end of file
            self.ts = numpy.append(self.ts, ts)
            self._frame_ptrs.append((self._file.tell(), sz))
            self._file.seek(sz, 1)
            ts, block_id, sz, total_sz = _read_attrib(self._file, self.ver)
            
    def __del__(self):
        self._file.close()
        
    def get_frame(self, indx):
        """
        Return indx-th frame a jpg image in the memory.
        """
        offset, sz = self._frame_ptrs[indx]
        self._file.seek(offset)
        return(self._file.read(sz))
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
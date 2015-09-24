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

import struct
import time
import math
import numpy

_BYTES_PER_SAMPLE = 2   # for the audio data
_DECODING_FORMAT = 'h'  # for the audio data
_REGR_SEGM_LENGTH = 20  # seconds, should be integer

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
        buf_sz    - buffer size (bytes)
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
        
        # get the size of the data part of the file
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
        
    def format_audio(self):
        """Return the formatted version or self.raw_audio.
        Return:
            audio     - nchan-by-nsamp matrix of the audio data
            audio_ts  - timestamps for all the audio samples
            
        Caution: this function consumes a lot of memory!
        """
        #------------------------------------------------------------------
        # Compute timestamps for all the audio samples
        #
        n_chunks = len(self.ts)
        samp_per_buf = self.buf_sz / (self.nchan * _BYTES_PER_SAMPLE)
        nsamp = samp_per_buf * n_chunks
        samps = numpy.arange(samp_per_buf-1, nsamp, samp_per_buf)
        
        errs = -numpy.ones(n_chunks)
        audio_ts = -numpy.ones(nsamp)
        
        # split the data into segments for piecewise linear regression
        split_indx = range(0, nsamp, _REGR_SEGM_LENGTH * self.srate)
        split_indx[-1] = nsamp  # the last segment might be up to twice as long as the others
        
        for i in range(len(split_indx)-1):
            sel_indx = numpy.where((samps >= split_indx[i]) & (samps < split_indx[i+1]))                                # select one segment
            p = numpy.polyfit(samps[sel_indx], self.ts[sel_indx], 1)                                                       # compute the regression coefficients
            errs[sel_indx] = numpy.abs(numpy.polyval(p, samps[sel_indx]) - self.ts[sel_indx])                           # compute the regression error
            audio_ts[split_indx[i] : split_indx[i+1]] = numpy.polyval(p, numpy.arange(split_indx[i], split_indx[i+1]))   # compute the timestamps with regression

        assert(audio_ts.min() >= 0) # make sure audio_ts was completely filled
        assert(errs.min() >= 0)     # make sure errs was completely filled
        print('AudioData: regression fit errors (abs): mean %f, median %f, max %f' % (errs.mean(), numpy.median(errs), errs.max()))
        
        #------------------------------------------------------------------
        # Parse the raw audio data
        #
        audio = numpy.zeros((self.nchan, nsamp))
        
        # NOTE: assuming the raw audio is interleaved
        for i in range(0, nsamp*self.nchan):
            samp_val, = struct.unpack(_DECODING_FORMAT, self.raw_audio[i*_BYTES_PER_SAMPLE : (i+1)*_BYTES_PER_SAMPLE])
            audio[i % self.nchan, i // self.nchan] = samp_val
        
        return audio, audio_ts
        
        
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
            
        self.nframes = self.ts.size
            
    def __del__(self):
        self._file.close()
        
    def get_frame(self, indx):
        """
        Return indx-th frame a jpg image in the memory.
        """
        offset, sz = self._frame_ptrs[indx]
        self._file.seek(offset)
        return(self._file.read(sz))
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
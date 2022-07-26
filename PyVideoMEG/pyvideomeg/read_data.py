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
    """
    Read data block attributes. If cannot read the attributes (EOF?), return
    -1 in ts
    """
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
    
def repair_file(file_name, fixed_file_name):
    """
    Try to repair a corrupted audio or video file. Assume that the corruption
    happened in the end of file, e.g. the file is valid until certain point
    """
    inp_file = open(file_name, 'rb')   
    is_audio = False
    
    ##---------------------------------------------------------------------
    # Read the header
    #
    
    # Check the magick string
    if inp_file.read(len('ELEKTA_AUDIO_FILE')) == b'ELEKTA_AUDIO_FILE':
        is_audio = True
    else:        
        inp_file.seek(0, 0)
        assert(inp_file.read(len('ELEKTA_VIDEO_FILE')) == b'ELEKTA_VIDEO_FILE')
    
    # Read the file version
    ver = struct.unpack('I', inp_file.read(4))[0]
    if ver < 1 or ver > 3:
        raise UnknownVersionError()        
        
    if ver == 3:
        # Read site_id and is_sender data
        id_sender_data = inp_file.read(2)
        assert(len(id_sender_data) == 2)
        
    if is_audio:
        srate_nchan_data = inp_file.read(8)
        assert(len(srate_nchan_data) == 8)
        
    # Get the file size
    begin_data = inp_file.tell()
    inp_file.seek(0, 2)
    end_data = inp_file.tell()
    inp_file.seek(begin_data, 0)  

    ##---------------------------------------------------------------------
    # Read the first chunk
    # 
    ts, block_id, sz, total_sz = _read_attrib(inp_file, ver)
    assert(ts != -1)
    inp_file.seek(-(total_sz-sz), 1)
    buf = inp_file.read(total_sz)
    assert(len(buf) == total_sz)
        
    ##---------------------------------------------------------------------
    # Recovered enough data, start writing the output file
    #
    out_file = open(fixed_file_name, 'wb')
    
    if is_audio:
        out_file.write(b'ELEKTA_AUDIO_FILE')
    else:
        out_file.write(b'ELEKTA_VIDEO_FILE')
        
    out_file.write(struct.pack('I', ver))
    
    if ver == 3:
        out_file.write(id_sender_data)
        
    if is_audio:
        out_file.write(srate_nchan_data)
        
    out_file.write(buf)
        
    ##---------------------------------------------------------------------
    # Start copying the data
    #
    while inp_file.tell() < end_data:
        ts, block_id, sz, cur_total_sz = _read_attrib(inp_file, ver)
        if ts == -1 or (is_audio and cur_total_sz != total_sz):
            inp_file.close()
            out_file.close()
            return
            
        inp_file.seek(-(cur_total_sz-sz), 1)
        buf = inp_file.read(cur_total_sz)
        if len(buf) != cur_total_sz:
            inp_file.close()
            out_file.close()
            return
            
        out_file.write(buf)
  
    inp_file.close()
    out_file.close()
    
    
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
        assert(data_file.read(len('ELEKTA_AUDIO_FILE')) == b'ELEKTA_AUDIO_FILE')  # make sure the magic string is OK 
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

        assert((end_data - begin_data) % total_sz == 0)
        
        n_chunks = (end_data - begin_data) // total_sz        
        self.raw_audio = bytearray(n_chunks * self.buf_sz)
        self.ts = numpy.zeros(n_chunks)

        for i in range(n_chunks):
            ts, block_id, sz, cur_total_sz = _read_attrib(data_file, self.ver)
            assert(cur_total_sz == total_sz)
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
        samp_per_buf = self.buf_sz // (self.nchan * _BYTES_PER_SAMPLE)
        nsamp = samp_per_buf * n_chunks
        samps = numpy.arange(samp_per_buf-1, nsamp, samp_per_buf)
        
        errs = -numpy.ones(n_chunks)
        audio_ts = -numpy.ones(nsamp)
        
        # split the data into segments for piecewise linear regression
        split_indx = list(range(0, nsamp, _REGR_SEGM_LENGTH * self.srate))
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
        assert(self._file.read(len('ELEKTA_VIDEO_FILE')) == b'ELEKTA_VIDEO_FILE')  # make sure the magic string is OK 
        self.ver = struct.unpack('I', self._file.read(4))[0]
        
        if self.ver == 1 or self.ver == 2:        
            self.site_id = -1
            self.is_sender = -1            

        elif self.ver == 3:
            self.site_id, self.is_sender = struct.unpack('BB', self._file.read(2))
            
        else:
            raise UnknownVersionError()
            
        # get the file size
        begin_data = self._file.tell()
        self._file.seek(0, 2)
        end_data = self._file.tell()
        self._file.seek(begin_data, 0)

        self.ts = numpy.array([])
        self._frame_ptrs = []

        while self._file.tell() < end_data:     # we did not reach end of file
            ts, block_id, sz, total_sz = _read_attrib(self._file, self.ver)
            assert(ts != -1)
            self.ts = numpy.append(self.ts, ts)
            self._frame_ptrs.append((self._file.tell(), sz))
            assert(self._file.tell() + sz <= end_data)
            self._file.seek(sz, 1)
            
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
        

class EvlData:
    """
    Event-list holding Event-class data.
    Can be read from a .evl file with from_file method
    """
    def __init__(self, source_file, events, start, end):
        self.source_file = source_file
        self._events = events
        self.rec_start = start
        self.rec_end = end

    def __new__(cls, source_file, events):
        return cls(source_file, events, 0, 0)

    @classmethod
    def from_file(cls, file_name):
        """
        Read an .evl file for event info.
        :param file_name: File-path to .evl file
        :return: EvlData-class
        """
        print("Using Evl file: {0}".format(file_name))
        f = open(file_name, 'r')
        assert f.read(len("(videomeg::")) == "(videomeg::"
        source_file = ""
        events = []

        read_events = False
        rec_start = 0
        rec_end = 0
        # Only except to find source-file and events
        for line in f:
            if read_events:
                if line[:2] == "))":
                    read_events = False
                else:
                    # Expect form ((:time xxx) (:class :"yyy") (:length zzz) (:annotation "www"))
                    stripped = line[3:-2]
                    pieced = stripped.split(") (")
                    time = pieced[0].rpartition(' ')[2]
                    _class = pieced[1].rpartition(':')[2]
                    length = pieced[2].rpartition(' ')[2]
                    annotation = pieced[3].partition(" \"")[2][:-2]
                    if annotation.lower() == "rec start":
                        rec_start = float(time)
                    elif annotation.lower() == "rec end":
                        rec_end = float(time)
                    else:
                        events.append(Event(time, _class, length, annotation))
            elif line.lstrip().startswith(":source-file"):
                source_file = line.partition(" \"")[2][:-1]
            elif line.lstrip().startswith(":events"):
                read_events = True
        return cls(source_file, events, rec_start, rec_end)

    def __len__(self):
        return len(self._events)

    def get_events(self):
        """
        :return: Array of Event-class objects containing a single event
        """
        return self._events


class Event:
    """
    Contains data from single event.
    Time is floating point.
    """
    def __init__(self, time, _class, length, annotation):
        self.time = float(time)
        self._class = _class
        self.duration = float(length)
        self.annotation = annotation

    def __str__(self):
        return "Event start-time: " + str(self.time) + " and duration: " + str(self.duration)

    def __repr__(self):
        return ("Start-time: " + str(self.time) + "\nClass: " + self._class +
                "\nLength: " + str(self.duration) + "\nAnnotation: " + self.annotation)


class FifData:
    """
    Contains some data from .fif file.
    """
    def __init__(self, file_name, ch):
        import mne
        from pyvideomeg import comp_tstamps

        raw = mne.io.read_raw_fif(fname=file_name, allow_maxshield=True, verbose=50)
        timing_data = mne.pick_types(raw.info, meg=False, include=[ch])
        timings = raw[timing_data, :][0].squeeze()
        self._file_name = file_name
        # Using uint_cast=True to resolve bug with Neuromag acquisition
        # See https://martinos.org/mne/stable/generated/mne.find_events.html
        self.timestamps = comp_tstamps(timings, raw.info['sfreq'])
        self.start_time = self.timestamps[0]
        self.sampling_freq = raw.info['sfreq']

    def get_timestamps(self):
        """
        Get all the timestamps associated.
        :return: numpy.ndarray of timestamps
        """
        return self.timestamps

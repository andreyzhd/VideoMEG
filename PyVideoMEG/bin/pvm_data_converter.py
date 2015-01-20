#!/usr/bin/python -tt

"""
Convert old 4-file video/audio data set to a new format (version 1).

---------------------------------------------------------------------------
Author: Andrey Zhdanov
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
import sys
import numpy

MAGIC_VIDEO_STR	= 'ELEKTA_VIDEO_FILE'
MAGIC_AUDIO_STR	= 'ELEKTA_AUDIO_FILE'
SFREQS = (22050, 44100, 48000, 96000, 128000)


def read_ts(fname):
    ts_file = open(fname, 'rb')
    ts_file.seek(0, 2)
    fsize = ts_file.tell()
    assert(fsize % (8 * 2) == 0)

    ts_file.seek(0,0)
    
    ts = list(struct.unpack('Q', ts_file.read(8)))
    prev_offset = (struct.unpack('Q', ts_file.read(8)))[0]
    lns = []
    
    for i in range(1, fsize/(8*2)):
        ts.append((struct.unpack('Q', ts_file.read(8)))[0])
        cur_offset = (struct.unpack('Q', ts_file.read(8)))[0]
        lns.append(cur_offset-prev_offset)
        prev_offset = cur_offset
        
    lns.append(fsize-prev_offset)
    ts_file.close()

    return (ts, lns)
    

fname = sys.argv[1]
fname_base = ''

if fname.endswith('_videodump.dat'):
    fname_base = fname[:-len('_videodump.dat')]
    
if fname.endswith('_videodump.ts'):
    fname_base = fname[:-len('_videodump.ts')]

if fname.endswith('_audiodump.dat'):
    fname_base = fname[:-len('_audiodump.dat')]
    
if fname.endswith('_audodump.ts'):
    fname_base = fname[:-len('_audiodump.ts')]

assert(fname_base != '')

# video
(ts, lns) = read_ts(fname_base + '_videodump.ts')
dat_file = open(fname_base + '_videodump.dat', 'rb')
out_file = open(fname_base + '.vid', 'wb')
out_file.write(MAGIC_VIDEO_STR)
out_file.write(struct.pack('I', 1))    # file format version

for i in range(len(ts)):
    out_file.write(struct.pack('Q', ts[i]))
    out_file.write(struct.pack('I', lns[i]))
    out_file.write(dat_file.read(lns[i]))
    
dat_file.close()
out_file.close()
    
               
# audio
(ts, lns) = read_ts(fname_base + '_audiodump.ts')
dat_file = open(fname_base + '_audiodump.dat', 'rb')
out_file = open(fname_base + '.aud', 'wb')
out_file.write(MAGIC_AUDIO_STR)
out_file.write(struct.pack('I', 1))    # file format version

# estimate sampling frequency
sfreq = (lns[1] / (2*2)) * 1000. / (numpy.array(ts[1:]) - numpy.array(ts[:-1])).mean()
sfreq_match = SFREQS[numpy.argmin(numpy.abs((numpy.array(SFREQS) - sfreq)))]
print('Estimated sampling frequency is %f, using the closest match %f' % (sfreq, sfreq_match))
print('ALSA period size is %i samples' % (lns[1] / (2*2)))
out_file.write(struct.pack('I', sfreq_match))
out_file.write(struct.pack('I', 2))    # number of channels

for i in range(len(ts)):
    out_file.write(struct.pack('Q', ts[i]))
    out_file.write(struct.pack('I', lns[i]))
    out_file.write(dat_file.read(lns[i]))
    
dat_file.close()
out_file.close()
    

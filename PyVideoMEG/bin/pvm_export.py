#!/usr/bin/python -tt

"""
Export a pair of video and audio files to a standard video format. Requires
mencoder and imagemagick

Usage: pvm_export video_file_name audio_file_name output_file_name. The audio
file name is optional.

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

import sys
import os
import tempfile
import shutil
import numpy

from PIL import Image, ImageDraw, ImageFont
try:                        #Python2.*
    from StringIO import StringIO
except ImportError:         #Python3.*
    from io import StringIO

import pyvideomeg

MENCODER_LOG_FILE = '/dev/null'
FONT_FILE = '/usr/share/fonts/truetype/ubuntu-font-family/UbuntuMono-R.ttf'
FONT_SZ = 20


#--------------------------------------------------------------------------
# Make a temporary folder for storing frames and audio files
#
tmp_fldr = tempfile.mkdtemp()
print('Using \'%s\' for storing temporary files' % tmp_fldr)

fnt = ImageFont.truetype(FONT_FILE, FONT_SZ)


#--------------------------------------------------------------------------
# Read the video file and extract frames to multiple jpg files
#
vid_file = pyvideomeg.VideoData(sys.argv[1])

for i in range(len(vid_file.ts)):
    img = Image.open(StringIO(vid_file.get_frame(i)))
    draw = ImageDraw.Draw(img)
    draw.text((10,0), '%i  :  %s' % (vid_file.ts[i], pyvideomeg.ts2str(vid_file.ts[i])), font=fnt, fill='black')
    img.save('%s/%08i.jpg' % (tmp_fldr, i))
       
       
#--------------------------------------------------------------------------
# If no audio file, encode and exit
#
if len(sys.argv) == 3:
    print('No audio file is specified, using only the video')
    fps = len(vid_file.ts) / (float(vid_file.ts[-1] - vid_file.ts[0]) / 1000)
    print('FPS: %f' % fps)
    vid_opts = 'mf://%s/*.jpg -mf type=jpg:fps=%f' % (tmp_fldr, fps)
    cmd = 'mencoder %s -ovc lavc -lavcopts vcodec=msmpeg4v2 -nosound -o %s > %s' % (vid_opts, sys.argv[2], MENCODER_LOG_FILE)

    print('Executing command \'%s\'' % cmd)
    os.system(cmd)

    #----------------------------------------------------------------------
    # Clean up and exit
    #
    shutil.rmtree(tmp_fldr)
    del(vid_file)
    sys.exit(0)


#--------------------------------------------------------------------------
# Read the audio file
#
aud_file = pyvideomeg.AudioData(sys.argv[2])


#--------------------------------------------------------------------------
# Trim audio and video, so that the resulting pieces match based on the
# timestamps.
#

if aud_file.ts[0] < vid_file.ts[0]:
    first_vid_indx = 0
    first_aud_indx = numpy.argmin(abs(aud_file.ts-vid_file.ts[0]))
else:
    first_aud_indx = 0
    first_vid_indx = numpy.argmin(abs(vid_file.ts-aud_file.ts[0]))

if aud_file.ts[-1] > vid_file.ts[-1]:
    last_vid_indx = len(vid_file.ts) - 1
    last_aud_indx = numpy.argmin(abs(aud_file.ts-vid_file.ts[-1]))
else:
    last_aud_indx = len(aud_file.ts) - 1
    last_vid_indx = numpy.argmin(abs(vid_file.ts-aud_file.ts[-1]))
    
# make sure there is an overlap between video and audio
assert((first_aud_indx != last_aud_indx) & (first_vid_indx != last_vid_indx))

# delete the video frames for which there is no audio
for i in range(len(vid_file.ts)):
    if (i < first_vid_indx) | (i > last_vid_indx):
        os.remove('%s/%08i.jpg' % (tmp_fldr, i))

# dump the relevant part of the audio to the file
out_file = open(tmp_fldr + '/audio.raw', 'wb')
out_file.write(aud_file.raw_audio[(first_aud_indx*aud_file.buf_sz) : ((last_aud_indx+1)*aud_file.buf_sz)])
out_file.close()

# compute some statistics
video_frame_cnt = last_vid_indx - first_vid_indx + 1
audio_frame_cnt = last_aud_indx - first_aud_indx + 1

print('Discarded %i video frames out of %i' % (len(vid_file.ts) - video_frame_cnt, len(vid_file.ts)))
print('Discarded %i audio buffers out of %i' % (len(aud_file.ts) - audio_frame_cnt, len(aud_file.ts)))

fps = video_frame_cnt / (float(vid_file.ts[last_vid_indx] - vid_file.ts[first_vid_indx]) / 1000)
print('FPS: %f' % fps)

wc_srate = (audio_frame_cnt * aud_file.buf_sz / 2 / aud_file.nchan) / (float(aud_file.ts[last_aud_indx] - aud_file.ts[first_aud_indx]) / 1000)
fixed_fps = fps * aud_file.srate / wc_srate      # correct for the difference between soundcard and computer clocks 
print('nominal sampling rate is %i\nwall clock sampling rate is %f\nnumber of channels: %i\nfixed FPS: %f' % (aud_file.srate, wc_srate, aud_file.nchan, fixed_fps))


#--------------------------------------------------------------------------
# Encode to an avi file
#
aud_opts = '-audiofile %s/audio.raw -audio-demuxer 20 -rawaudio rate=%i:channels=%i:samplesize=2' % (tmp_fldr, aud_file.srate, aud_file.nchan)
vid_opts = 'mf://%s/*.jpg -mf type=jpg:fps=%f' % (tmp_fldr, fixed_fps)
cmd = 'mencoder %s %s -ovc lavc -lavcopts vcodec=msmpeg4v2 -oac mp3lame -o %s > %s' % (vid_opts, aud_opts, sys.argv[3], MENCODER_LOG_FILE)

print('Executing command \'%s\'' % cmd)
os.system(cmd)


#--------------------------------------------------------------------------
# Clean up
#
shutil.rmtree(tmp_fldr)

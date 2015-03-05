#!/usr/bin/python -tt

"""
Merge two video files into one (side-by-side) and export the result in a
standard video format. Requires mencoder.

Usage: merge video_file_name_1 video_file_name_2 output_file_name

---------------------------------------------------------------------------
Author: Andrey Zhdanov
Copyright (C) 2015 BioMag Laboratory, Helsinki University Central Hospital

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

import Image, ImageDraw, ImageFont
import StringIO

import pyvideomeg

MENCODER_LOG_FILE = '/dev/null'
FONT_FILE = '/usr/share/fonts/truetype/ubuntu-font-family/UbuntuMono-R.ttf'
FONT_SZ = 20
        

#--------------------------------------------------------------------------
# Make a temporary folder for storing frames
#
tmp_fldr = tempfile.mkdtemp()
print('Using \'%s\' for storing temporary files' % tmp_fldr)


#--------------------------------------------------------------------------
# Read video files and merge the frames
#
vid_file_1 = pyvideomeg.VideoData(sys.argv[1])
vid_file_2 = pyvideomeg.VideoData(sys.argv[2])

# make sure that the files overlap in time
assert((vid_file_1.ts[0] < vid_file_2.ts[-1]) & (vid_file_2.ts[0] < vid_file_1.ts[-1]))

# skip the unmatched frames (if any) at the beginning of video_file_1
i = 0
while min(abs(vid_file_1.ts[i+1] - vid_file_2.ts)) < min(abs(vid_file_1.ts[i] - vid_file_2.ts)):
    i += 1

fnt = ImageFont.truetype(FONT_FILE, FONT_SZ)

indx = []
err = []
first_i = i
while i < len(vid_file_1.ts):
    indx.append(numpy.argmin(abs(vid_file_1.ts[i] - vid_file_2.ts)))
    err.append(vid_file_1.ts[i] - vid_file_2.ts[indx[-1]])
    
    img1 = Image.open(StringIO.StringIO(vid_file_1.get_frame(i)))
    img2 = Image.open(StringIO.StringIO(vid_file_2.get_frame(indx[-1])))
    
    draw = ImageDraw.Draw(img1)
    draw.text((10,0), '%i  :  %s' % (vid_file_1.ts[i], pyvideomeg.ts2str(vid_file_1.ts[i])), font=fnt, fill='black')
    
    draw = ImageDraw.Draw(img2)
    draw.text((10,0), '%i' % err[-1], font=fnt, fill='black')
    
    out_image = Image.new("RGB", (2*img1.size[0],img1.size[1]))
    out_image.paste(img1, (0,0))
    out_image.paste(img2, (img1.size[0],0))
    out_image.save('%s/%08i.jpg' % (tmp_fldr, i))

    i += 1
    
    # if reached the end of the second file, stop
    if indx[-1] == len(vid_file_2.ts)-1:
        break
    
err = numpy.array(err)


#----------------------------------------------------------------------------
# Compute some statistics
#
distinct = 1
skipped = 0
reused = 0

for i in range(len(indx)-1):
    if indx[i] == indx[i+1]:
        reused += 1
    else:
        distinct += 1
        if (indx[i+1] - indx[i]) > 1:
            skipped += 1
        assert(indx[i+1] - indx[i] > 0)

print('File #1: used %i frames out of %i' % (len(indx), len(vid_file_1.ts)))
print('File #2: used %i frames (%i distinct) out of %i, %i skips, %i frames reused' % (len(indx), distinct, len(vid_file_2.ts), skipped, reused))
print('Matching error: average: %f ms, absolute average: %f ms, maximal: %f ms' % (numpy.mean(err), numpy.mean(abs(err)), max(abs(err))))

fps = len(indx) / (float(vid_file_1.ts[i-1] - vid_file_1.ts[first_i]) / 1000)
print('FPS: %f' % fps)


#--------------------------------------------------------------------------
# Encode to an avi file
#
vid_opts = 'mf://%s/*.jpg -mf type=jpg:fps=%f' % (tmp_fldr, fps)
cmd = 'mencoder %s -ovc lavc -lavcopts vcodec=msmpeg4v2 -nosound -o %s > %s' % (vid_opts, sys.argv[3], MENCODER_LOG_FILE)

print('Executing command \'%s\'' % cmd)
os.system(cmd)


#--------------------------------------------------------------------------
# Clean up
#
shutil.rmtree(tmp_fldr)
del(vid_file_1)
del(vid_file_2)





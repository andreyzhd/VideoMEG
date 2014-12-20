# -*- coding: utf-8 -*-
"""An example: merging two video files
    
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

import numpy as np
import PIL
import cStringIO
import struct

import pyvideomeg

FNAME_1 = '/home/andrey/Desktop/test/3/videoMEG_sync_test.video_01.dat'
FNAME_2 = '/home/andrey/Desktop/test/3/videoMEG_sync_test.video_02.dat'

OUT_FNAME = '/home/andrey/Desktop/test/3/videoMEG_sync_test.video_merged.dat'

FRAME_SZ = (640, 480)

file_1 = pyvideomeg.VideoData(FNAME_1)
file_2 = pyvideomeg.VideoData(FNAME_2)

out_file = out_file = open(OUT_FNAME, 'wb')
out_file.write('ELEKTA_VIDEO_FILE')
out_file.write(struct.pack('I', 1)) # file format version

for i in range(len(file_1.ts)):
    # find the closest matching frame from the second file
    file2_indx = np.argmin(np.abs(file_2.ts - file_1.ts[i]))
    
    # merge the images
    im0 = PIL.Image.open(cStringIO.StringIO(file_1.get_frame(i)))
    im1 = PIL.Image.open(cStringIO.StringIO(file_2.get_frame(file2_indx)))
    
    im0.resize((FRAME_SZ[0]//2, FRAME_SZ[1]), PIL.Image.ANTIALIAS)
    im1.resize((FRAME_SZ[0]//2, FRAME_SZ[1]), PIL.Image.ANTIALIAS)
    
    res = PIL.Image.new('RGB', FRAME_SZ)
    res.paste(im0, (0,0))
    res.paste(im1, (FRAME_SZ[0]//2,0))

    # write the merged image to the output file
    buf = cStringIO.StringIO()
    res.save(buf, 'JPEG')
    len(buf.getvalue())
    out_file.write(struct.pack('Q', file_1.ts[i]))
    out_file.write(struct.pack('I', len(buf.getvalue())))
    out_file.write(buf.getvalue())
    buf.close()
    
out_file.close()
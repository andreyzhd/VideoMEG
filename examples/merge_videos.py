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
from PIL import Image
import io
import struct

import pyvideomeg

FNAME_1 = '/home/andrey/data/2014-06-18--11-38-18--p3_instructions--site_2_sender.vid'
FNAME_2 = '/home/andrey/data/2014-06-18--12-37-46--p3_instructions--site_0_sender.vid'

OUT_FNAME = '/home/andrey/data/merged.vid'

FRAME_SZ = (1280, 480)

file_1 = pyvideomeg.VideoData(FNAME_1)
file_2 = pyvideomeg.VideoData(FNAME_2)

out_file = out_file = open(OUT_FNAME, 'wb')
out_file.write(b'ELEKTA_VIDEO_FILE')
out_file.write(struct.pack('I', 1)) # file format version

for i in range(len(file_1.ts)):
    # find the closest matching frame from the second file
    file2_indx = np.argmin(np.abs(file_2.ts - file_1.ts[i]))
    
    # merge the images
    im0 = Image.open(io.BytesIO(file_1.get_frame(i)))
    im1 = Image.open(io.BytesIO(file_2.get_frame(file2_indx)))
    
    im0_r = im0.resize((FRAME_SZ[0]//2, FRAME_SZ[1]), Image.Resampling.LANCZOS)
    im1_r = im1.resize((FRAME_SZ[0]//2, FRAME_SZ[1]), Image.Resampling.LANCZOS)
    
    res = Image.new('RGB', FRAME_SZ)
    res.paste(im0_r, (0,0))
    res.paste(im1_r, (FRAME_SZ[0]//2,0))

    # write the merged image to the output file
    buf = io.BytesIO()
    res.save(buf, 'JPEG')
    len(buf.getvalue())
    out_file.write(struct.pack('Q', int(file_1.ts[i])))
    out_file.write(struct.pack('I', len(buf.getvalue())))
    out_file.write(buf.getvalue())
    buf.close()
    
out_file.close()

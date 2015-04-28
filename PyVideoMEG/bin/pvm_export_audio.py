#!/usr/bin/python -tt

"""
Export an audio file to a standard audio format. Requires scipy.

Usage: pvm_export_audio audio_file_name

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
from scipy.io import wavfile
import numpy as np
from os import path as op

import pyvideomeg

for aud_file in sys.argv[1:]:
    if not op.isfile(aud_file):
        raise IOError('file not found: %s' % aud_file)
    if op.splitext(aud_file)[1] != '.aud':
        raise ValueError('unknown extension "%s"' % op.splitext(aud_file)[1])
    out_file = op.splitext(aud_file)[0] + '.wav'
    if op.isfile(out_file):
        print('Skipping, output file exists: %s' % out_file)
        continue
    print('Creating file: %s' % out_file)
    sys.stdout.flush()
    aud_file = pyvideomeg.AudioData(aud_file)
    rate = aud_file.srate
    n_ch = aud_file.nchan
    aud_file = np.frombuffer(aud_file.raw_audio, '<i2').reshape(-1, n_ch)
    wavfile.write(out_file, rate, aud_file)

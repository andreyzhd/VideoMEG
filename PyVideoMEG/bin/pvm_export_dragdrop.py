#!/usr/bin/python -tt

"""This makes a simple drag-and-drop target to convert recorded files
"""

import sys
from os import path as op
import subprocess


for vid_file in sys.argv[1:]:
    vid_file = sys.argv[1]
    if not op.isfile(vid_file):
        raise RuntimeError('Could not find video file %s' % vid_file)
    aud_file = vid_file[:-12] + 'audio_00.aud'
    if not op.isfile(aud_file):
        raise RuntimeError('Could not find audio file %s' % aud_file)
    out_file = vid_file[:-12] + vid_file.split('_')[-1][:2] + '.avi'
    cmd = ['pvm_export.py', vid_file, aud_file, out_file]
    proc = subprocess.check_call(cmd, stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE)

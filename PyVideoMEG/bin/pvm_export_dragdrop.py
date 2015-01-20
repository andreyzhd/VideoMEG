#!/usr/bin/python -tt

"""This makes a simple drag-and-drop target to convert recorded files
"""

import sys
import os
from os import path as op
import subprocess


for vid_file in sys.argv[1:]:
    if op.splitext(vid_file)[1] == '.aud':
        continue
    vid_file = sys.argv[1]
    if not op.isfile(vid_file):
        raise RuntimeError('Could not find video file %s' % vid_file)
    aud_file = vid_file[:-12] + 'audio_00.aud'
    if not op.isfile(aud_file):
        raise RuntimeError('Could not find audio file %s' % aud_file)
    avi_file = vid_file[:-12] + vid_file.split('_')[-1][:2] + '.avi.tmp'
    cmd = ['pvm_export.py', vid_file, aud_file, avi_file]
    proc = subprocess.check_call(cmd, stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE)
    mov_file = op.splitext(avi_file)[0] + '.mov'
    subprocess.check_call(['avconv', '-i', avi_file, '-f', 'mov', '-strict',
                           'experimental', mov_file + '.tmp'])
    os.remove(avi_file)
    os.rename(mov_file + '.tmp', mov_file)

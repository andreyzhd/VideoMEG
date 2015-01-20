#!/usr/bin/python -tt

import sys
from os import path as op
import subprocess

in_file = sys.argv[1]
if not op.isfile(in_file):
    raise IOError('file not found: "%s"' % in_file)
out_file = op.splitext(in_file)[0] + '.mpeg'
subprocess.check_call(['mencoder', in_file, '-oac', 'copy', '-ovc', 'lavc',
                       '-lavcopts', 'vcodec=mpeg2video:vbitrate=800',
                       '-fps', '30', '-o', out_file])

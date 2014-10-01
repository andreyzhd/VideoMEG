#!/usr/bin/python -tt

"""
Upgrade video or audio file format from version 0 to 1.

The script only changes the format version number stored in the file header
without making any other changes. File to be converted is given as a command
line argument. The converted file is created with the same name as the original
+ suffix '.fixed'

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


MAGIC_VIDEO_STR	= 'ELEKTA_VIDEO_FILE'
MAGIC_AUDIO_STR	= 'ELEKTA_AUDIO_FILE'


in_file = open(sys.argv[1], 'rb')

magic = in_file.read(17)
if (magic != MAGIC_VIDEO_STR) and (magic != MAGIC_AUDIO_STR):
    print('Not a valid Elekta video or audio file')
    exit()

ver = struct.unpack('I', in_file.read(4))[0]
if ver != 0:
    print("Wrong file format version")
    exit()

out_file = open(sys.argv[1] + '.fixed', 'wb')
out_file.write(magic)
out_file.write(struct.pack('I', 1))

out_file.write(in_file.read())
out_file.close()
in_file.close()

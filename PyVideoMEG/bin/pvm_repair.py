#!/usr/bin/python -tt

"""
Try to repair a damaged video or audio file. Assume that the damage is at the
end of the file.

Usage: pvm_repair original_file_name fixed_file_name

---------------------------------------------------------------------------
Author: Andrey Zhdanov
Copyright (C) 2016 BioMag Laboratory, Helsinki University Central Hospital

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

import pyvideomeg

pyvideomeg.repair_file(sys.argv[1], sys.argv[2])
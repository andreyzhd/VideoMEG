#!/usr/bin/env python
"""
    Script to amplify motion in Video-MEG recordings. Will produce file [prefix].video.amp.dat.
    Requires path to .fif file as argument. Other needed files are expected to have same prefix.

    Copyright (C) 2017 BioMag Laboratory, Helsinki University Central Hospital

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
# If Matlab.engine is not imported before numpy, it will crash and inform that installation is corrupted.
# This is not reported to MathWorks as of yet.
from __future__ import print_function

import matlab.engine
import tempfile
import subprocess
import sys
import shutil
# TODO Verify compatibility for 2.7. Was changed from StringIO to io.StringIO
try:
    from StringIO import StringIO
except ImportError:
    from io import StringIO
import pyvideomeg
from PIL import Image
from io import BytesIO
from os import remove, path as op
from math import ceil
from scipy.io import loadmat

__author__ = "Janne Holopainen"


VIDEOMEG_DIR = op.join(op.dirname(__file__), '..', '..', 'pyvideomeg')
MATLAB_AMPLIFY_M = op.join(VIDEOMEG_DIR, 'matlab_scripts', 'amplify.m')
MATLAB_PHASEAMPMOD_M = op.join(VIDEOMEG_DIR, 'matlab_scripts', 'phaseAmplifyMod.m')

if not op.exists(MATLAB_AMPLIFY_M) or not op.exists(MATLAB_PHASEAMPMOD_M):
    raise FileNotFoundError("Required Matlab scripts not found from pyvideomeg/matlab_scripts")


class OverLappingEvents(Exception):
    """
    Raised when overlapping events would produce ambiguous amplification.
    """
    pass


class NonMatchingAmplification(Exception):
    """
    Raised when original and amplified file, don't match according to
    Elekta's variables.
    """
    pass


def _rounded_evl_list(event_list):
    """
    Returns list of (start, end) -timestamps that have been ceiled to
    closest 2 second blocks.
    """
    listed = []
    for event in event_list.events:
        middle = event.time + (event.duration / 2)
        ceiled_duration = float(ceil(event.duration))
        if ceiled_duration % 2 != 0:
            ceiled_duration = ceiled_duration + 1
        listed.append((middle - ceiled_duration / 2, middle + ceiled_duration / 2))
    return listed


def phase_based_amplification(video_file, sample_count, frames_per_sample, engine):
    """
    Calls matlab script with proper parameters, to perform amplification on video_file.
    Matlab saves the resulting video as matrix to /tmp/vid.mat.
    """
    cycles = 1
    print("Invoking amplify.m")
    # TODO verify that amplify.m works with cycles as intented
    amplified_as_matrix = engine.amplify(video_file, sample_count,
                                         frames_per_sample, cycles, nargout=0)
    return amplified_as_matrix

if __name__ == "__main__":

    if len(sys.argv) == 2:
        assert sys.argv[-1] > 4 or sys.argv[-4:] != ".fif"

        FIF = op.split(sys.argv[1])[1]
        TREE = op.split(sys.argv[1])[0]
        FNAME = FIF[:-4]
        VIDEO_FILE = op.join(TREE, FNAME + ".video.dat")

        try:
            FIF = pyvideomeg.read_data.FifData(op.join(TREE, FNAME + ".fif"))
        except IOError:
            print(".fif file was not found. Cannot get accurate timing data - exiting.")
            sys.exit()

        try:
            EVL = pyvideomeg.read_data.EvlData.from_file(op.join(TREE, FNAME + ".evl"))
        except IOError:
            print(".evl file was not found. Using MNE automatic detection.")
            EVL = FIF.get_events()

        if len(EVL) == 0:
            print("No events were found. nothing to amplify - exiting.")
            sys.exit()

        EVENT_LIST = _rounded_evl_list(EVL)

        # Check for overlaps in events
        for i in range(1, len(EVENT_LIST)):
            if EVENT_LIST[i][0] < EVENT_LIST[i-1][1]:
                raise OverLappingEvents("Events " + str(i-1) + " and " + str(i) + " overlap.\n" +
                                        "Amplification would be ambiguous")

        ORIGINAL = pyvideomeg.VideoData(VIDEO_FILE)
        AMPLIFIED = pyvideomeg.VideoFile(op.join(TREE, FNAME + ".video.amp.dat"), ORIGINAL.ver,
                                         site_id=ORIGINAL.site_id, is_sender=ORIGINAL.is_sender)

        FPS = (len(ORIGINAL.ts)-1) * 1000. / (ORIGINAL.ts[-1] - ORIGINAL.ts[0])

        # Start Matlab-engine for amplification
        # Won't work if internet connection is not available - matlab cannot check license.
        try:
            _ENG = matlab.engine.start_matlab()
        # Matlab doesn't specify this exception
        except:
            print("Problem starting Matlab engine. Possible reasons for this include:\n" +
                  " 1 - Is your matlab engine for python installed and accessible?\n" +
                  " 2 - Do you have internet connection, to check your matlab license?\n\n" +
                  "Cannot proceed with amplification. Cleaning up and exiting.")
            remove(op.join(TREE, FNAME + ".video.amp.dat"))
            sys.exit()

        EVENT_NUMBER = 0
        i = 0

        while i < len(ORIGINAL.ts):
            FRAME = ORIGINAL.get_frame(i)
            # TODO Verify that FIF.start_time provides right time for the amplification
            # Evl marks the time from the start of the fif file, which differs from the time
            # calculated from video timestamps.
            # Evl matching time stamp can be found from .fif file. MNE-python can extract that
            # information.
            VIDEO_TIME = (ORIGINAL.ts[i] - FIF.start_time)/1000.0
            # All events handled or first event hasn't started yet
            if EVENT_NUMBER >= len(EVENT_LIST) or VIDEO_TIME < EVENT_LIST[EVENT_NUMBER][0]:
                AMPLIFIED.append_frame(ORIGINAL.ts[i], FRAME)
                i = i + 1
            # Amplify event
            elif EVENT_LIST[EVENT_NUMBER][0] <= VIDEO_TIME <= EVENT_LIST[EVENT_NUMBER][1]:
                TMP_FLDR = tempfile.mkdtemp()
                j = i
                k = 0
                # Read the whole event and save on temporary folder
                while (ORIGINAL.ts[j] - ORIGINAL.ts[0])/1000.0 <= EVENT_LIST[EVENT_NUMBER][1]:
                    IMG = Image.open(StringIO(ORIGINAL.get_frame(j)))
                    IMG.save('%s/%06i.jpg' % (TMP_FLDR, k))
                    j = j + 1
                    k = k + 1

                FFMPEG = subprocess.Popen(["ffmpeg", "-r", str(FPS)+"/1", "-i",
                                           TMP_FLDR+"/%06d.jpg", "-c:v", "mjpeg", "-q:v",
                                           "0", TMP_FLDR+"/vid.avi"])
                FFMPEG.wait()

                SAMPLE_COUNT = round((EVENT_LIST[EVENT_NUMBER][1] -
                                      EVENT_LIST[EVENT_NUMBER][0]) / 2.)
                FRAME_PER_SAMPLE = round(float(k) / SAMPLE_COUNT)
                phase_based_amplification(op.join(TMP_FLDR, "vid.avi"), SAMPLE_COUNT,
                                          FRAME_PER_SAMPLE, _ENG)
                _ENG.clear('all', nargout=0)
                AMPLIFIED_VERSION = loadmat("/tmp/vid.mat")['out']

                # Write amplified event to video.amp.dat.
                # k was treated as a index before.
                for indx in range(k):
                    IMG = Image.fromarray(AMPLIFIED_VERSION[:, :, :, indx])
                    # TODO If possible replace BytesIO() with StringIO for consistency and less
                    # reading from memory.
                    bio = BytesIO()
                    IMG.save(bio, format="JPEG")
                    bio.seek(0)
                    AMPLIFIED.append_frame(ORIGINAL.ts[i + indx], bio.read(-1))

                shutil.rmtree(TMP_FLDR)
                remove("/tmp/vid.mat")

                i = i + k
                EVENT_NUMBER = EVENT_NUMBER + 1
            # Skip if nothing for some reason event none of the above applies
            else:
                i = i + 1

        _ENG.quit()

        if len(ORIGINAL.ts) != len(AMPLIFIED.timestamps):
            raise NonMatchingAmplification("Length of the files differ")
        if ORIGINAL.ver != AMPLIFIED.ver:
            raise NonMatchingAmplification("Files have different versions\nOriginal " +
                                           str(ORIGINAL.ver) + " Amplified " +
                                           str(AMPLIFIED.ver))
        AMPLIFIED.check_sanity()

    else:
        print("Need path to .fif file\nmotion_amplify.py <.fif-file>")

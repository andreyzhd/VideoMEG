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
import getopt
try:
    from StringIO import StringIO
except ImportError:
    from io import StringIO
import pyvideomeg
from PIL import Image, ImageDraw, ImageFont
from io import BytesIO
from os import remove, path as op
from math import ceil
from scipy.io import loadmat

__author__ = "Janne Holopainen"


VIDEOMEG_DIR = op.join(op.dirname(__file__), '..', '..')
MATLAB_SCRIPTS = op.join(VIDEOMEG_DIR, 'matlab_scripts')
MATLAB_AMPLIFY_M = op.join(MATLAB_SCRIPTS, 'amplify.m')
MATLAB_PHASEAMPMOD_M = op.join(MATLAB_SCRIPTS, 'phaseAmplifyMod.m')

if not op.exists(MATLAB_AMPLIFY_M) or not op.exists(MATLAB_PHASEAMPMOD_M):
    raise IOError("Required Matlab scripts not found from pyvideomeg/matlab_scripts")

FONT_FILE = '/usr/share/fonts/truetype/ubuntu-font-family/UbuntuMono-R.ttf'
FONT_SZ = 20
FONT = ImageFont.truetype(FONT_FILE, FONT_SZ)


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
    print("REC START: {0}".format(event_list.rec_start))
    for event in event_list.get_events():
        middle = event.time - event_list.rec_start + (event.duration / 2.0)
        ceiled_duration = max(float(ceil(event.duration)), 2.0)
        if ceiled_duration % 2 != 0:
            ceiled_duration = ceiled_duration + 1.0
        listed.append((middle - ceiled_duration / 2.0, middle + ceiled_duration / 2.0))
    return listed


def phase_based_amplification(video_file, sample_count, frames_per_sample, merge_video, engine):
    """
    Calls matlab script with proper parameters, to perform amplification on video_file.
    Matlab saves the resulting video as matrix to /tmp/vid.mat.
    """
    cycles = 1
    # TODO Taking the scene from both sides of the event might not be the best idea.
    
    # Pyramid can be: 'octave', 'halfOctave', 'smoothHalfOctave', 'quarterOctave'
    pyramid = 'halfOctave'

    amplified_as_matrix = engine.amplify(video_file, sample_count,
                                         frames_per_sample, cycles, pyramid, merge_video, nargout=0)
    return amplified_as_matrix

if __name__ == "__main__":

    try:
        OPTS, ARGS = getopt.getopt(sys.argv[1:], "e:v:mt:", ["evl=", "video=", "merge", "timing="])
    except getopt.GetoptError:
        print("Need path to .fif file\nmotion_amplify.py <.fif-file>\n" +
              "Optionals: --evl, --video, --merge, --timing")

    F_EVL = None
    F_VID = None
    MERGE_VIDEO = False
    TIMING_CH = "STI 006"

    for o, a in OPTS:
        if o in ("-e", "--evl"):
            F_EVL = a
        elif o in ("-v", "--video"):
            F_VID = a
        elif o in ("-m", "--merge"):
            MERGE_VIDEO = True
        elif o in ("-t", "--timing"):
            TIMING_CH = a

    if len(sys.argv) >= 2:
        assert ARGS[0][-1] > 4 or ARGS[0][-4:] != ".fif"

        FIF = op.split(ARGS[0])[1]
        TREE = op.split(ARGS[0])[0]
        FNAME = FIF[:-4]

        if F_VID is not None:
            VIDEO_FILE = op.expanduser(F_VID)
        else:
            VIDEO_FILE = op.join(TREE, FNAME + ".video.dat")

        try:
            FIF = pyvideomeg.read_data.FifData(op.join(TREE, FNAME + ".fif"), TIMING_CH)
        except IOError:
            print(".fif file was not found. Cannot get accurate timing data - exiting.")
            sys.exit()

        try:
            if F_EVL is not None:
                EVL = pyvideomeg.read_data.EvlData.from_file(op.expanduser(F_EVL))
            else:
                EVL = pyvideomeg.read_data.EvlData.from_file(op.join(TREE, FNAME + ".evl"))
        except IOError:
            print(".evl file was not found. Using MNE automatic detection.")
            EVL = FIF.get_events()

        if len(EVL) == 0:
            print("No events were found. nothing to amplify - exiting.")
            sys.exit()

        EVENT_LIST = _rounded_evl_list(EVL)

        # Check for overlaps in events
        # TODO Treat overlapping events as single event?
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

        _ENG.cd(MATLAB_SCRIPTS)

        EVENT_NUMBER = 0
        VIDEO_OFFSET = ORIGINAL.ts[0] - FIF.start_time
        i = 0

        while i < len(ORIGINAL.ts):
            FRAME = ORIGINAL.get_frame(i)
            # TODO Automatically calculate the EVL-video offset.
            # fif-file doesn't appear to have a timestamp that would allow us to calculate th time.
            # Using EVL manual markings with REC START on the start of file instead.
            VIDEO_TIME = (ORIGINAL.ts[i] - ORIGINAL.ts[0])/1000.0
            # All events handled or first event hasn't started yet
            if EVENT_NUMBER >= len(EVENT_LIST) or VIDEO_TIME < EVENT_LIST[EVENT_NUMBER][0]:
                if MERGE_VIDEO is not None:
                    # Resize original video and paste it to the left part of the new video.

                    IMG = Image.new("RGB", (640, 480))
                    ORI = Image.open(StringIO(ORIGINAL.get_frame(i)))
                    ORI = ORI.resize((320, 240), Image.BICUBIC)
                    draw = ImageDraw.Draw(IMG)
                    draw.text((120, 100), "ORIGINAL", fill=(82, 90, 240), font=FONT)
                    draw.text((440, 100), "AMPLIFIED", fill=(82, 90, 240), font=FONT)
                    IMG.paste(ORI, (0,120))
                    bio = BytesIO()
                    IMG.save(bio, format="JPEG")
                    bio.seek(0)
                    AMPLIFIED.append_frame(ORIGINAL.ts[i], bio.read(-1))
                else:
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
                                           "0", TMP_FLDR+"/vid.avi"], stderr=subprocess.PIPE)
                FFMPEG.wait()

                SAMPLE_COUNT = round((EVENT_LIST[EVENT_NUMBER][1] -
                                      EVENT_LIST[EVENT_NUMBER][0]) / 2.)
                FRAME_PER_SAMPLE = round(float(k) / SAMPLE_COUNT)
                phase_based_amplification(op.join(TMP_FLDR, "vid.avi"), SAMPLE_COUNT,
                                          FRAME_PER_SAMPLE, MERGE_VIDEO, _ENG)
                _ENG.clear('all', nargout=0)
                AMPLIFIED_VERSION = loadmat("/tmp/vid.mat")['out']

                # Write amplified event to video.amp.dat.
                # k was treated as a index before.
                for indx in range(k):
                    IMG = Image.fromarray(AMPLIFIED_VERSION[:, :, :, indx])
                    if MERGE_VIDEO is None:
                        draw = ImageDraw.Draw(IMG)
                        draw.text((280, 5), "AMPLIFIED", fill=(82, 90, 240), font=FONT)
                    else:
                        draw = ImageDraw.Draw(IMG)
                        draw.text((120, 100), "ORIGINAL", fill=(82, 90, 240), font=FONT)
                        draw.text((440, 100), "AMPLIFIED", fill=(82, 90, 240), font=FONT)
                    bio = BytesIO()
                    IMG.save(bio, format="JPEG")
                    bio.seek(0)
                    AMPLIFIED.append_frame(ORIGINAL.ts[i + indx], bio.read(-1))

                shutil.rmtree(TMP_FLDR)
                remove("/tmp/vid.mat")

                i = i + k
                EVENT_NUMBER = EVENT_NUMBER + 1
            # Skip, if for some reason none of the above applies
            else:
                i = i + 1

        _ENG.quit()

        if len(ORIGINAL.ts) != len(AMPLIFIED.timestamps):
            print(i)
            print(str(len(ORIGINAL.ts)) + "    " + str(len(AMPLIFIED.timestamps)))
            raise NonMatchingAmplification("Length of the original and amplified video-files" +
                                           " differ")
        if ORIGINAL.ver != AMPLIFIED.ver:
            raise NonMatchingAmplification("Files have different versions\nOriginal " +
                                           str(ORIGINAL.ver) + " Amplified " +
                                           str(AMPLIFIED.ver))
        AMPLIFIED.check_sanity()

    else:
        print("Need path to .fif file\nmotion_amplify.py <options> <.fif-file>")

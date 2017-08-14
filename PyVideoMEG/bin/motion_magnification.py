#!/usr/bin/env python
"""
    Script to amplify motion in Video-MEG recordings. Will produce file [prefix].video.amp.dat.
    Requires path to .fif file as argument. Other needed files are expected to have same prefix.

    Copyright (C) 2017 BIOMag Laboratory, Helsinki University Central Hospital

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
# If Matlab.engine is not imported before numpy, it will crash and inform that
# installation is corrupted.
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

# TODO QOL Format print-statements to use .format
# TODO QOL Add display to show progress of amplification
# TODO QOL Add Logging
# TODO Write more comprehensive help

VIDEOMEG_DIR = op.join(op.dirname(__file__), '..', '..')
MATLAB_SCRIPTS = op.join(VIDEOMEG_DIR, 'matlab_scripts')
MATLAB_AMPLIFY_M = op.join(MATLAB_SCRIPTS, 'amplify.m')
MATLAB_PHASEAMPMOD_M = op.join(MATLAB_SCRIPTS, 'phaseAmplifyMod.m')

if not op.exists(MATLAB_AMPLIFY_M) or not op.exists(MATLAB_PHASEAMPMOD_M):
    raise IOError("Required Matlab scripts not found from pyvideomeg/matlab_scripts")


class OverLappingEventsError(Exception):
    """
    Raised when overlapping events would produce ambiguous amplification.
    """
    pass


class NonMatchingAmplificationError(Exception):
    """
    Raised when original and amplified file, don't match according to
    Elekta's variables.
    """
    pass


def _rounded_evl_list(event_list, duration):
    """
    Returns list of (start, end) -timestamps that have been ceiled to
    closest duration length blocks.

    Note:
        Might result in timestamps that are later than video end.
    """
    listed = []
    for event in event_list.get_events():
        middle = event.time - event_list.rec_start + (event.duration / 2.0)
        ceiled_duration = max(float(ceil(event.duration)), duration)

        # Ceil to next multiple of duration.
        ceiled_duration = ceiled_duration + (ceiled_duration % duration)

        listed.append((middle - ceiled_duration / 2.0, middle + ceiled_duration / 2.0))
    return listed


def phase_based_amplification(video_file, sample_count, frames_per_sample, merge_video, low_cut,
                              high_cut, amp, engine):
    """
    Calls matlab script with proper parameters, to perform amplification on video_file.
    Matlab saves the resulting video as matrix to /tmp/vid.mat.
    """
    cycles = 1
    # TODO Taking the scene from both sides of the event might not be the best idea.
    # TODO Open the parameters to the CLI.
    # TODO Should we drop the cyclic manipulation before the amplification?
    # TODO Can we exclude normal breathing from amplification?

    # Pyramid can be: 'octave', 'halfOctave', 'smoothHalfOctave', 'quarterOctave'
    pyramid = 'octave'
    amplified_as_matrix = engine.amplify(video_file, sample_count, frames_per_sample,
                                         cycles, pyramid, low_cut, high_cut, amp,
                                        merge_video, nargout=0)
    return amplified_as_matrix

if __name__ == "__main__":

    try:
        OPTS, ARGS = getopt.getopt(sys.argv[1:], "e:v:mt:l:h:a:d:", ["evl=", "video=", "merge",
                                                                   "timing=", "low=", "high=",
                                                                   "duration="])
    except getopt.GetoptError:
        print("Need path to .fif file\nmotion_amplify.py <.fif-file>\n" +
              "Optionals: --evl, --video, --merge, --timing")

    F_EVL = None
    F_VID = None
    MERGE_VIDEO = False
    TIMING_CH = "STI 006"
    LOW = 0.3
    HIGH = 1.3
    AMPLIFICATION_FACTOR = 10.0
    DURATION = 4.0

    for o, a in OPTS:
        if o in ("-e", "--evl"):
            F_EVL = a
        elif o in ("-v", "--video"):
            F_VID = a
        elif o in ("-m", "--merge"):
            MERGE_VIDEO = True
        elif o in ("-t", "--timing"):
            TIMING_CH = a
        elif o in ("-d", "--duration"):
            DURATION = float(a)
        elif o in ("-l", "--low"):
            try:
                LOW = float(a)
            except ValueError:
                print("Cannot convert value from {0} to float. Using 0.3".format(o))
        elif o in ("-h", "--high"):
            try:
                HIGH = float(a)
            except ValueError:
                print("Cannot convert value from {0} to float. Using 1.3".format(o))
        elif o == "-a":
            try:
                AMPLIFICATION_FACTOR = float(a)
            except ValueError:
                print("Cannot convert value from {0} to float. Using 10.0 as amplification factor"
                      .format(o))


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

        if not EVL.get_events():
            print("No events were found. nothing to amplify - exiting.")
            sys.exit()

        EVENT_LIST = _rounded_evl_list(EVL, DURATION)

        # Check for overlaps in events
        # TODO Treat overlapping events as single event?
        # TODO Should the length be adjusted by the expeced frequency?
        for i in range(1, len(EVENT_LIST)):
            if EVENT_LIST[i][0] < EVENT_LIST[i-1][1]:
                raise OverLappingEventsError("Events " + str(i-1) + " and " + str(i) +
                                             " overlap.\n" + "Amplification would be ambiguous")

        ORIGINAL = pyvideomeg.VideoData(VIDEO_FILE)
        AMPLIFIED = pyvideomeg.VideoFile(op.join(TREE, FNAME + ".video.amp.dat"), ORIGINAL.ver,
                                         site_id=ORIGINAL.site_id, is_sender=ORIGINAL.is_sender)

        FPS = (len(ORIGINAL.ts)-1) * 1000. / (ORIGINAL.ts[-1] - ORIGINAL.ts[0])

        # Start Matlab-engine for amplification
        # Won't work if internet connection is not available - matlab cannot check license.
        try:
            _ENG = matlab.engine.start_matlab()
        # Matlab doesn't specify this exception
        except Exception:
            print("Problem starting Matlab engine. Possible reasons for this include:\n" +
                  " 1 - Is your matlab engine for python installed and accessible?\n" +
                  " 2 - Do you have internet connection, to check your matlab license?\n\n" +
                  "Cannot proceed with amplification. Cleaning up and exiting.")
            remove(op.join(TREE, FNAME + ".video.amp.dat"))
            sys.exit()

        FONT_FILE = '/usr/share/fonts/truetype/ubuntu-font-family/UbuntuMono-R.ttf'
        FONT_SZ = 20
        FONT = ImageFont.truetype(FONT_FILE, FONT_SZ)

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
                    DRAW = ImageDraw.Draw(IMG)
                    DRAW.text((120, 100), "ORIGINAL", fill=(82, 90, 240), font=FONT)
                    DRAW.text((440, 100), "AMPLIFIED", fill=(82, 90, 240), font=FONT)
                    IMG.paste(ORI, (0, 120))
                    BIO = BytesIO()
                    IMG.save(BIO, format="JPEG")
                    BIO.seek(0)
                    AMPLIFIED.append_frame(ORIGINAL.ts[i], BIO.read(-1))

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
                                      EVENT_LIST[EVENT_NUMBER][0]) / DURATION)
                FRAME_PER_SAMPLE = round(float(k) / SAMPLE_COUNT)
                phase_based_amplification(op.join(TMP_FLDR, "vid.avi"), SAMPLE_COUNT,
                                          FRAME_PER_SAMPLE, MERGE_VIDEO, LOW, HIGH,
                                          AMPLIFICATION_FACTOR, _ENG)
                _ENG.clear('all', nargout=0)
                AMPLIFIED_VERSION = loadmat("/tmp/vid.mat")['out']

                # Write amplified event to video.amp.dat.
                # k was treated as a index before.
                for indx in range(k):

                    IMG = Image.fromarray(AMPLIFIED_VERSION[:, :, :, indx])
                    if MERGE_VIDEO is None:
                        DRAW = ImageDraw.Draw(IMG)
                        DRAW.text((280, 5), "AMPLIFIED", fill=(82, 90, 240), font=FONT)
                    else:
                        DRAW = ImageDraw.Draw(IMG)
                        DRAW.text((120, 100), "ORIGINAL", fill=(82, 90, 240), font=FONT)
                        DRAW.text((440, 100), "AMPLIFIED", fill=(82, 90, 240), font=FONT)
                    BIO = BytesIO()
                    IMG.save(BIO, format="JPEG")
                    BIO.seek(0)
                    AMPLIFIED.append_frame(ORIGINAL.ts[i + indx], BIO.read(-1))

                shutil.rmtree(TMP_FLDR)
                remove("/tmp/vid.mat")

                i = i + k
                EVENT_NUMBER = EVENT_NUMBER + 1
            # Skip, if for some reason none of the above applies
            else:
                i = i + 1

        _ENG.quit()

        if len(ORIGINAL.ts) != len(AMPLIFIED.timestamps):
            raise NonMatchingAmplificationError("Length of the original and amplified " +
                                                "video-files differ")
        if ORIGINAL.ver != AMPLIFIED.ver:
            raise NonMatchingAmplificationError("Files have different versions\nOriginal " +
                                                str(ORIGINAL.ver) + " Amplified " +
                                                str(AMPLIFIED.ver))
        # Perform check for resulting videofile
        AMPLIFIED.check_sanity()

    else:
        print("Need path to .fif file\nmotion_amplify.py <options> <.fif-file>")

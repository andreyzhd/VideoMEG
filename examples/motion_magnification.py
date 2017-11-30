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
import errno
import pyvideomeg
from PIL import Image, ImageDraw, ImageFont
from io import BytesIO
from os import remove, strerror, path as op
from math import ceil
from scipy.io import loadmat

__author__ = "Janne Holopainen"


VIDEOMEG_DIR = op.join(op.dirname(__file__), '..')
MATLAB_SCRIPTS = op.join(VIDEOMEG_DIR, 'Third_Party', 'MIT_video_amplification')
PHASE_BASED_DIR = op.join(MATLAB_SCRIPTS, 'original')
MATLAB_AMPLIFY_M = op.join(MATLAB_SCRIPTS, 'amplify.m')
MATLAB_PHASEAMPMOD_M = op.join(MATLAB_SCRIPTS, 'phaseAmplifyMod.m')

if not op.exists(MATLAB_AMPLIFY_M):
    print("Required Matlab scripts not found from pyvideomeg/matlab_scripts")
    raise IOError(errno.ENOENT, strerror(errno.ENOENT), MATLAB_AMPLIFY_M)
if not op.exists(MATLAB_PHASEAMPMOD_M):
    print("Required Matlab scripts not found from pyvideomeg/matlab_scripts")
    raise IOError(errno.ENOENT, strerror(errno.ENOENT), MATLAB_PHASEAMPMOD_M)
if not op.exists(PHASE_BASED_DIR):
    print("Matlab code for Phase Based motion processing not found. Broken installation?")
    raise IOError(errno.ENOENT, strerror(errno.ENOENT), PHASE_BASED_DIR)

SHORT_HELP = """Motion-magnification.py
Required format: motion-magnification.py [OPTION] ... [.FIF-FILE]
See motion-magnification.py --help for list of options."""
LONG_HELP = """Motion-magnification.py
Required format: motion-magnification.py [OPTION] ... [.FIF-FILE]

Optional arguments:
    --help                   - Prints this help.
    -e | --evl= [FILE]       - Use FILE as source for EVL markings.
    -v | --video= [FILE]     - Use FILE as source for video.
    -m | --merge             - Format resulting video on side-by-side fashion (Where original video
                               is on the left and amplified on right). Default is False.
    -d | --duration [FLOAT]  - Change duration of the amplified pieces to FLOAT.
    -l | --low [FLOAT]       - Change the lower bound for units to be amplified. Units in Hz.
                               Default 0.3.
    -h | --high [FLOAT]      - Change the high bound for units to be amplified. Units in Hz.
                               Default 1.3.
    -a [FLOAT]               - Change the amplification-factor. High values may result in distorted
                               footages. Default 10.0.
    -t | --timing [STRING]   - Name if the timing channel. Default 'STI 006'.
    --attenuate              - Attenuate frequencies outside of the band between --low and --high.
    -p | --pyramid [STRING]  - Change pyramid type used for magnification. Possible values
                               ['octave', 'halfOctave', 'quarterOctave', 'smoothHalfOctave']
                               Default 'octave'.
    """

_PYRAMID_DICT = {"octave": "octave", "halfoctave": "halfOctave",
                 "smoothHalfOctave": "smoothHalfOctave", "quarteroctave": "quarterOctave"}

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

    Returns:
        List    - List of events with start and end-time being eventually around event.
    """
    listed = []
    for event in event_list.get_events():
        middle = event.time - event_list.rec_start + (event.duration / 2.0)
        ceiled_duration = max(float(ceil(event.duration)), duration)

        # Ceil to next multiple of duration.
        ceiled_duration = ceiled_duration + (ceiled_duration % duration)

        listed.append((middle - ceiled_duration / 2.0, middle + ceiled_duration / 2.0))
    return listed


def _phase_based_amplification(video_file, sample_count, frames_per_sample, low_cut,
                               high_cut, amp, att, pyramid, engine):
    """
    Calls matlab script with proper parameters, to perform amplification on video_file.
    Matlab saves the resulting video as matrix to /tmp/vid.mat.

    Attributes:
        video_file (str)        - Path to videofile. Format should be MJPEG.
        sample_count (int)      - In how many frames_per_sample sequences should the video be
                                  processed.
        frames_per_sample (int) - How many frames does a sample contain.
        low_cut (float)         - Movement band-pass low cut
        high_cut (float)        - Movement band-pass high cut
        amp (float)             - Magnification factor
        att (boolean)           - Will frequencies outside the band-pass be attenuated.
        pyramid (str)           - Which pyramid will be used. See: _PYRAMID_DICT
        engine (matlab.engine)  - Matlab engine to used with amplification.

    Returns:
        None    - Resulting video is saved to /tmp/vid.mat

    """
    cycles = 1
    # TODO Should we drop the cyclic manipulation before the amplification?

    engine.amplify(video_file, sample_count, frames_per_sample,
                   cycles, pyramid, low_cut, high_cut, amp,
                   att, PHASE_BASED_DIR, nargout=0)


def merge_frames(left_image, right_image=None):
    """
    Creates a new image with left and right image pasted side-by-side.
    If only left-image is supplied, will have a black box on place of right_image.

    Returns:
        Image   - New Image with picture(s) merged side-by-side.
    """
    (width, height) = left_image.size
    horizontal_middle = width/2
    vertical_top = (height/2)/2

    img = Image.new("RGB", (width, height))
    left = left_image.resize((width/2, height/2), Image.BICUBIC)
    img.paste(left, (0, vertical_top))

    if right_image is not None:
        right = right_image.resize((width/2, height/2), Image.BICUBIC)
        img.paste(right, (horizontal_middle, vertical_top))

    return img

def _add_text_single(text, image, font):
    """
    Add text to the top of the frame.
    Text is drawn to middle of the frame with 5 pixels from the top.

    Returns:
        Image   - Original image, with text appended
    """
    (width, _) = image.size
    text_start = (width/2 - 20, 5)

    draw = ImageDraw.Draw(image)

    draw.text(text_start, text, fill=(82, 90, 240), font=font)

    return image

def _add_text_double(left_text, right_text, image, font):
    """
    Add text to side-by-side videos.
    Will draw the text to around one quarter from the top and also one quarter from sides.

    Returns:
        image   - Original image, with text's appended
    """
    (width, height) = image.size
    # Expect text to take about 40 pixels width
    left_start = (width/4 - 20, height/4 - 20)
    right_start = (3*(width/4) - 20, height/4 - 20)

    draw = ImageDraw.Draw(image)

    draw.text(left_start, left_text, fill=(82, 90, 240), font=font)
    draw.text(right_start, right_text, fill=(82, 90, 240), font=font)

    return image

if __name__ == "__main__":

    try:
        OPTS, ARGS = getopt.getopt(sys.argv[1:], "e:v:mt:l:h:a:d:p:", ["evl=", "video=", "merge",
                                                                       "timing=", "low=", "high=",
                                                                       "duration=", "help",
                                                                       "attenuate", "pyramid="])
    except getopt.GetoptError:
        print(SHORT_HELP)

    F_EVL = None
    F_VID = None
    MERGE_VIDEO = False
    TIMING_CH = "STI 006"
    LOW = 0.3
    HIGH = 1.3
    AMPLIFICATION_FACTOR = 10.0
    DURATION = 4.0
    ATTENUATE = False
    PYRAMID = "octave"

    # Perform first check only for HELP
    for o, _ in OPTS:
        if o == "--help":
            print(LONG_HELP)
            exit(0)

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
        elif o == "--attenuate":
            ATTENUATE = True
        elif o in ("-p", "--pyramid"):
            try:
                PYRAMID = _PYRAMID_DICT[a.lower()]
            except KeyError:
                print("Unknown pyramid type: {0}. Using octave.")

    if len(sys.argv) >= 2:
        assert ARGS[0][-1] > 4 or ARGS[0][-4:] != ".fif"

        FIF = op.split(ARGS[0])[1]
        TREE = op.split(ARGS[0])[0]
        FNAME = FIF[:-4]

        if F_VID is not None:
            VIDEO_FILE = op.expanduser(F_VID)
        else:
            VIDEO_FILE = op.join(TREE, "{0}.video.dat".format(FNAME))

        try:
            FIF = pyvideomeg.read_data.FifData(op.join(TREE, "{0}.fif".format(FNAME)), TIMING_CH)
        except IOError:
            print(".fif file was not found. Cannot get accurate timing data - exiting.")
            sys.exit()

        try:
            if F_EVL is not None:
                EVL = pyvideomeg.read_data.EvlData.from_file(op.expanduser(F_EVL))
            else:
                EVL = pyvideomeg.read_data.EvlData.from_file(op.join(TREE,
                                                                     "{0}.evl".format(FNAME)))
        except IOError:
            print(".evl file was not found.")

        if not EVL.get_events():
            print("No events were found. nothing to amplify - exiting.")
            sys.exit()

        EVENT_LIST = _rounded_evl_list(EVL, DURATION)
        EVENT_LIST = sorted(EVENT_LIST, key=lambda event: event[0])

        # Check for overlaps in events
        # TODO Treat overlapping events as single event?
        for i in range(1, len(EVENT_LIST)):
            if EVENT_LIST[i][0] < EVENT_LIST[i-1][1]:
                raise OverLappingEventsError("Events {0} and {1} overlap.\n".format(i-1, i) +
                                             "Amplification would be ambiguous")

        ORIGINAL = pyvideomeg.VideoData(VIDEO_FILE)
        AMPLIFIED = pyvideomeg.VideoFile(op.join(TREE, "{0}.video.amp.dat".format(FNAME)),
                                         ORIGINAL.ver, site_id=ORIGINAL.site_id,
                                         is_sender=ORIGINAL.is_sender)

        FPS = (len(ORIGINAL.ts)-1) * 1000. / (ORIGINAL.ts[-1] - ORIGINAL.ts[0])

        # Start Matlab-engine for amplification
        # Won't work if internet connection is not available - Matlab cannot check license.
        try:
            _ENG = matlab.engine.start_matlab()
        # Matlab doesn't specify this exception
        except Exception:
            print("Problem starting Matlab engine. Possible reasons for this include:\n" +
                  " 1 - Is your Matlab engine for python installed and accessible?\n" +
                  " 2 - Do you have internet connection, to check your Matlab license?\n\n" +
                  "Cannot proceed with amplification. Cleaning up and exiting.")
            remove(op.join(TREE, "{0}.video.amp.dat".format(FNAME)))
            sys.exit()

        try:
            FONT_FILE = '/usr/share/fonts/truetype/ubuntu-font-family/UbuntuMono-R.ttf'
            FONT = ImageFont.truetype(FONT_FILE, size=20)
        except IOError:
            FONT = ImageFont.truetype(size=20)

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
                if MERGE_VIDEO:
                    # Resize original video and paste it to the left part of the new video.

                    ORI = Image.open(BytesIO(FRAME))
                    IMG = merge_frames(ORI)
                    IMG = _add_text_double("ORIGINAL", "AMPLIFIED", IMG, FONT)

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
                j = i # ORIGINAL FRAME NUMBER
                k = 0 # EVENT FRAME NUMBER
                # Read the whole event and save on temporary folder
                while (ORIGINAL.ts[j] - ORIGINAL.ts[0])/1000.0 <= EVENT_LIST[EVENT_NUMBER][1]:
                    IMG = Image.open(BytesIO(ORIGINAL.get_frame(j)))
                    IMG.save('%s/%06i.jpg' % (TMP_FLDR, k))
                    j = j + 1
                    k = k + 1

                FFMPEG = subprocess.Popen(["ffmpeg", "-r", "{0}/1".format(FPS), "-i",
                                           "{0}/%06d.jpg".format(TMP_FLDR), "-c:v", "mjpeg",
                                           "-q:v", "0", "{0}/vid.avi".format(TMP_FLDR)],
                                          stderr=subprocess.PIPE)
                FFMPEG.wait()

                SAMPLE_COUNT = round((EVENT_LIST[EVENT_NUMBER][1] -
                                      EVENT_LIST[EVENT_NUMBER][0]) / DURATION)
                FRAME_PER_SAMPLE = round(float(k) / SAMPLE_COUNT)
                _phase_based_amplification(op.join(TMP_FLDR, "vid.avi"), SAMPLE_COUNT,
                                           FRAME_PER_SAMPLE, LOW, HIGH,
                                           AMPLIFICATION_FACTOR, ATTENUATE, PYRAMID, _ENG)
                _ENG.clear('all', nargout=0)
                AMPLIFIED_VERSION = loadmat("/tmp/vid.mat")['out']

                # Write amplified event to video.amp.dat.
                # k was treated as a index before.
                for indx in range(k):

                    IMG = Image.fromarray(AMPLIFIED_VERSION[:, :, :, indx])

                    if MERGE_VIDEO:

                        ORI = Image.open(BytesIO(ORIGINAL.get_frame(i + indx)))
                        IMG = merge_frames(ORI, IMG)
                        IMG = _add_text_double("ORIGINAL", "AMPLIFIED", IMG, FONT)

                    else:
                        IMG = _add_text_single("AMPLIFIED", IMG, FONT)

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
            raise NonMatchingAmplificationError(
                "Files have different versions\nOriginal " +
                "{0} Amplified {1}.".format(ORIGINAL.ver, AMPLIFIED.ver))
        # Perform check for resulting video-file
        AMPLIFIED.check_sanity()

    else:
        print(SHORT_HELP)

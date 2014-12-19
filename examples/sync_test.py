# -*- coding: utf-8 -*-
"""An example: code for assessing video/audio/MEG synchronization.
    
    This script takes a triplet (fiff, audio, and video) of files and generates
    a bunch of pictures. Each picture describes a short piece of the
    recordings. The upper pane shows 3 consecutive video frames. The lower pane
    shows the corresponding pieces of audio and a single MEG channel. The black
    vertical lines mark the frame locations.
    
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

import PIL
import cStringIO
import matplotlib.pyplot as plt
import numpy as np
import mne
import pyvideomeg

VIDEO_FNAME = '/home/andrey/Desktop/test/1/tapping_01_raw.video.dat'
AUDIO_FNAME = '/home/andrey/Desktop/test/1/tapping_01_raw.audio.dat'
MEG_FNAME = '/home/andrey/Desktop/test/1/tapping_01_raw.fif'
TIMING_CH = 'STI006'
MEG_CH = 'STI006'
FRAME_SZ = (640, 480)
OUT_FLDR = '/tmp'
WIND_WIDTH = 3  # in frames

#--------------------------------------------------------------------------
# Load the data
#
raw = mne.io.Raw(MEG_FNAME, allow_maxshield=True)

# load the timing channel
picks_timing = mne.pick_types(raw.info, meg=False, include=[TIMING_CH])
dt_timing = raw[picks_timing,:][0].squeeze()

# load the MEG channel
picks_meg = mne.pick_types(raw.info, meg=False, include=[MEG_CH])
meg = raw[picks_meg,:][0].squeeze()

# compute the timestamps for the MEG channel
meg_ts = pyvideomeg.comp_tstamps(dt_timing, raw.info['sfreq'])

vid_file = pyvideomeg.VideoData(VIDEO_FNAME)
aud_file = pyvideomeg.AudioData(AUDIO_FNAME)

audio, audio_ts = aud_file.format_audio()
audio = audio[0,:].squeeze()    # use only the first audio channel


#--------------------------------------------------------------------------
# Make the pics
#
plt.ioff()  # don't pop up the figure windows

ts_scale = np.diff(vid_file.ts).max() * (WIND_WIDTH+0.1)
meg_scale = np.percentile(np.abs(meg), 99.5) * 1.1
aud_scale = np.percentile(np.abs(audio), 99.5) * 1.1

for i in range(1+WIND_WIDTH, len(vid_file.ts)-(1+WIND_WIDTH)):
    # combine 3 frames
    im0 = PIL.Image.open(cStringIO.StringIO(vid_file.get_frame(i-1)))
    im1 = PIL.Image.open(cStringIO.StringIO(vid_file.get_frame(i)))
    im2 = PIL.Image.open(cStringIO.StringIO(vid_file.get_frame(i+1)))
   
    res = PIL.Image.new('RGB', (FRAME_SZ[0]*3, FRAME_SZ[1]))
    res.paste(im0, (0,0))
    res.paste(im1, (FRAME_SZ[0],0))
    res.paste(im2, (FRAME_SZ[0]*2,0))
        
    plt.figure()
    
    # plot the frames
    plt.subplot(2,1,1)
    plt.imshow(np.asarray(res))
    plt.xticks(())
    plt.yticks(())
    
    # plot the traces
    min_ts = vid_file.ts[i] - ts_scale
    max_ts = vid_file.ts[i] + ts_scale
    
    plt.subplot(2,1,2)    
    meg_indx = np.where((meg_ts > min_ts) & (meg_ts < max_ts))
    plt.plot(meg_ts[meg_indx], meg[meg_indx] / meg_scale, 'b')
    
    audio_indx = np.where((audio_ts > min_ts) & (audio_ts < max_ts))
    plt.plot(audio_ts[audio_indx], audio[audio_indx] / aud_scale, 'g')
    
    # mark the frame positions
    plt.plot(vid_file.ts[i-1]*np.ones(2), (-1,1), 'k')
    plt.plot(vid_file.ts[i]*np.ones(2), (-1,1), 'k')
    plt.plot(vid_file.ts[i+1]*np.ones(2), (-1,1), 'k')
    
    plt.xticks(())
    plt.yticks(())
    plt.xlim((min_ts, max_ts))
    plt.ylim((-1, 1))
    
    savefig('%s/frame-%07.f.png' % (OUT_FLDR, i))
    plt.close('all')    

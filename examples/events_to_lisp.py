# -*- coding: utf-8 -*-
from __future__ import print_function
"""

Read VideoMEG event files corresponding to a fiff file. Create a corresponding 
lisp event file that can be used in Graph. 

Python 2.7

Note: graph does not understand utf-8. Thus any event code / comment characters 
beyond ASCII will be wrongly displayed in graph. This might be fixable by 
writing the lisp event file in ISO-8859-1 encoding.

@author: jussi
"""

USAGE = """

Usage: events_to_lisp.py fiff_name marker_path [timing_ch]

fiff_name    name of fiff file to find markers for
marker_path  where to find marker files
timing_ch    MEG video timing channel, e.g. 'STI016'. Defaults to 'STI101' 
"""


import glob
import os
import mne
import pyvideomeg
import sys


DEFAULT_TIMING_CH = 'STI101'
# this glob should match all event files; add extension if it's implemented
EVENTFILES_GLOB = '*'

# string templates for lisp event file
LISP_HEADER_STR = '(videomeg::saved-event-list\n :source-file \"{sourcefile}\"\n :events \'(\n'
LISP_EVENT_STR = '  ((:time {time:.3f}) (:class :manual) (:length 0.0) (:annotation \"{annotation}\"))\n'



def fiff_timerange(fname, timing_ch):
    """ Return start and end time of a fiff file in Unix epoch """
    raw = mne.io.Raw(fname, allow_maxshield=True)
    # load the timing channel
    picks_timing = mne.pick_types(raw.info, meg=False, include=[timing_ch])
    if not picks_timing:
        sys.exit('Cannot find specified timing channel')
    dt_timing = raw[picks_timing,:][0].squeeze()
    # interpolation is useless here, should be fixed if too slow    
    meg_ts = pyvideomeg.comp_tstamps(dt_timing, raw.info['sfreq'])
    return meg_ts[0], meg_ts[-1]

def timestamp(fname):
    """ Return numeric timestamp of event file fn """
    fname = os.path.basename(fname)    
    return int(fname[:fname.find('--')])

def leading_substring(s1, s2):
    """ Find longest common leading substring of s1 and s2 """
    k = 0
    while s1.startswith(s2[:k]) and k <= len(s2):
        k += 1
    return s2[:k-1]
        
def event_filenames(videodata_path, t0, t1):
    """ Return names of event files between times t0 and t1 """
    # find candidate filenames based on timestamp
    pt = leading_substring(str(t0), str(t1))
    fnames = glob.glob(videodata_path+'/'+pt+EVENTFILES_GLOB)
    # filter according to time    
    return [fi for fi in fnames if timestamp(fi) >= t0 and timestamp(fi) <= t1]
    
def event_data(fname):
    """ Return marker type and comment for given event file (1st and 2nd lines) """
    with open(fname, 'r') as f:
        li = f.read().splitlines()
    return li[0], li[1]


if not len(sys.argv) in [3,4]:
    sys.exit(USAGE)
        
fiffname = sys.argv[1]
if not os.path.isfile(fiffname):
    sys.exit('Input file {0} not found'.format(fiffname))
videodata_path = sys.argv[2]

if len(sys.argv) == 4:
    timing_ch = sys.argv[3]
else:
    timing_ch = DEFAULT_TIMING_CH

fiffbase = os.path.basename(fiffname)
outfn = os.path.splitext(fiffname)[0] + '.evl'

(t0,t1) = fiff_timerange(fiffname, timing_ch)
evfiles = event_filenames(videodata_path, t0, t1)

if not evfiles:
    print('\nNo event files corresponding to fiff time range:\n', int(t0), '-', int(t1))
else:
    with open(outfn, 'w') as outfile:
        outfile.write(LISP_HEADER_STR.format(sourcefile=fiffbase))
        for evfile in evfiles:
            t = (timestamp(evfile) - t0)/1.e3
            event, comment = event_data(evfile)
            outfile.write(LISP_EVENT_STR.format(time=t, annotation=event+': '+comment))
        outfile.write('))')  # terminate lisp block
    print('\nWrote',len(evfiles),'events to',outfn)
    
    


    
    



# -*- coding: utf-8 -*-
"""
Read VideoMEG event files corresponding to a fiff file. Create a corresponding 
lisp event file that can be used in Graph. 

Usage: events_to_lisp.py [fiffname]
Python 2.7

Note: graph does not understand utf-8. Thus any event code / comment characters 
beyond ASCII will be wrongly displayed in graph. This might be fixable by 
writing the lisp event file in ISO-8859-1 encoding.

@author: jussi
"""

from __future__ import print_function
import glob
import os
import mne
import pyvideomeg
import sys


VIDEODATA_PATH = '/videodat/markers'  # searched for event files
EVENTFILES_GLOB = '*'  # this glob should match all event files
TIMING_CH = 'STI016'

# string templates for lisp event file
LISP_HEADER_STR = '(videomeg::saved-event-list\n :source-file \"{sourcefile}\"\n :events \'(\n'
LISP_EVENT_STR = '  ((:time {time}) (:class :manual) (:length 0.0) (:annotation \"{annotation}\"))\n'


def fiff_timerange(fname):
    """ Return start and end time of a fiff file in Unix epoch """
    raw = mne.io.Raw(fname, allow_maxshield=True)
   # load the timing channel
    picks_timing = mne.pick_types(raw.info, meg=False, include=[TIMING_CH])
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
        
def event_filenames(t0, t1):
    """ Return names of event files between times t0 and t1 """
    # find candidate filenames
    pt = leading_substring(str(t0), str(t1))
    fnames = glob.glob(VIDEODATA_PATH+'/'+pt+EVENTFILES_GLOB)
    return [fi for fi in fnames if timestamp(fi) >= t0 and timestamp(fi) <= t1]
    
def event_data(fname):
    """ Return marker type and comment for given event file (1st and 2nd lines) """
    with open(fname, 'r') as f:
        li = f.read().splitlines()
    return li[0], li[1]


if len(sys.argv) != 2:
    raise Exception('need one argument: name of fiff file')
fiffname = sys.argv[1]
fiffbase = os.path.basename(fiffname)
outfn = os.path.splitext(fiffname)[0] + '.evl'
(t0,t1) = fiff_timerange(fiffname)
evfiles = event_filenames(t0, t1)
print('')
if not evfiles:
    print('No event files corresponding to fiff time range:\n', int(t0), '-', int(t1))
else:
    with open(outfn, 'w') as outfile:
        outfile.write(LISP_HEADER_STR.format(sourcefile=fiffbase))
        for evfile in evfiles:
            t = (timestamp(evfile) - t0)/1.e3
            event, comment = event_data(evfile)
            outfile.write(LISP_EVENT_STR.format(time=t, annotation=event+': '+comment))
        outfile.write('))')  # terminate lisp block
    print('Wrote',len(evfiles),'events to',outfn)
    
    


    
    



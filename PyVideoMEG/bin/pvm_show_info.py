#!/usr/bin/python -tt

"""
Print to the standard output some info about video or audio file

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

import sys

import pyvideomeg

try:
    vf = pyvideomeg.VideoData(sys.argv[1])
    is_audio = False
except pyvideomeg.UnknownVersionError:
    print('The file %s has unknown version' % sys.argv[1])
    sys.exit(1)
except:
    try:
        af = pyvideomeg.AudioData(sys.argv[1])
        is_audio = True
    except pyvideomeg.UnknownVersionError:
        print('The file %s has unknown version' % sys.argv[1])
        sys.exit(1)
    except:
        print('The file %s seems to be neither video nor audio file' % sys.argv[1])
        sys.exit(1)
        
if is_audio:
    nsamples = len(af.raw_audio) / af.nchan / 2
    wc_srate = (nsamples - (af.buf_sz/2/af.nchan)) * 1000. / (af.ts[-1] - af.ts[0])
    
    print('\n\n\n')
    print('Audio file.')
    print('\tVersion: %i' % af.ver)
    print('\tNominal sampling rate: %i' % af.srate)
    print('\tWall clock sampling rate: %f' % wc_srate)
    print('\tNumber of channels: %i' % af.nchan)
    print('\tNumber of samples: %i' % nsamples)
    print('\tTotal duration (estimated): %f seconds' % (nsamples / wc_srate))
    print('\tBuffer size: %i frames' % (af.buf_sz / 2 / af.nchan))
    print('\tFirst buffer time: %s' % pyvideomeg.ts2str(af.ts[0]))
    print('\tLast buffer time: %s' % pyvideomeg.ts2str(af.ts[-1]))
    
    if af.ver == 3:
        print('\tSite ID: %i' % af.site_id)
        
        if af.is_sender:
            print('\tThe file was recorded by the *sender* process')
        else:
            print('\tThe file was recorded by the *receiver* process')
    
    del(af)

else:
    fps = (len(vf.ts)-1) * 1000. / (vf.ts[-1] - vf.ts[0])

    print('\n\n\n')
    print('Video file.')
    print('\tVersion: %i' % vf.ver)
    print('\tNumber of frames: %i' % len(vf.ts))
    print('\tFPS: %f' % fps)
    print('\tTotal duration (estimated): %f seconds' % (len(vf.ts) / fps))
    print('\tFirst frame time: %s' % pyvideomeg.ts2str(vf.ts[0]))
    print('\tLast frame time: %s' % pyvideomeg.ts2str(vf.ts[-1]))
    
    if vf.ver == 3:
        print('\tSite ID: %i' % vf.site_id)
        
        if vf.is_sender:
            print('\tThe file was recorded by the *sender* process')
        else:
            print('\tThe file was recorded by the *receiver* process')
    
    del(vf)
    
print('\n\n\n')
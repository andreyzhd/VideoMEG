Motion magnification is provided in order to help identify small motion and possible artefact-sources from the video data. Magnification works best with small movement and can result in blurry outcomes if there is a lot of movement present.

Resulting video file will be written to a video.amp.dat file and original files are left __unaltered__.

### Prerequisites
Calculations for the amplification are done in Matlab. In order for the Python-wrapper to work - Matlab engine for Python should be installed by user. [Installing Matlab engine for Python](X)

Original research on the used magnification method was done by MIT, and our code is mostly borrowed from them. Installation of their code is required: [Zip-package for the code](http://people.csail.mit.edu/nwadhwa/phase-video/PhaseBasedRelease_20131023.zip) and [Instructions for configuration](Y)


To save computational time, we will only amplify events marked on .evl-file.

**Also** - in order to the time the amplification with MEG-traces - currently (9.8.17) user needs to add a Event-marking to the start of the recording with annotation: "REC START"

### Usage
Motion magnification script call requires at least path to .fif -file, and in this case it will expect to find .evl & .video.dat file from same path with same prefix.

```
motion-amplify.py [path-to-fif]
```

Additional options and parameters for motion magnification are:
* "-e" | "--evl=", followed by a path to different .evl -file
* "-v" | "--video=", followed by a path to different .video.dat -file
* "-t" | "--timing=", changes timing channel on .fif -file [Default "STI 006"]
* "-l" | "--low=", changes the low cut-off frequency (Hz) for amplification. [Default 0.3]
* "-h" | "--high=", changes the high cur-off frequency (Hz) for amplification. [Default 1.3]
* "-m" | "--merge", no additional parameter needed - changes the resulting video being merged side-by-side with original.

   If **-m** option is not used, script will write amplified sequences in the place of the original.

With additional parameters the call would be:

```
motion-amplify.py [additional-parameters] [path-to-fif]
```

### Resulting video
Resulting file is synced (If REC START event was present in the .evl -file and rightly placed) with the .fif -file given as argument.

Users should note that normal movement - like breathing - might be greatly exaggerated on the resulting video, and extra attention should be paid before making conclusions.
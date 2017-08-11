We still rely greatly on the wonderfull code made by Neal Wadhwa at MIT. Their lisence grants use for non-commercial research. Code can be downloaded here (License included): [Zip-package for the code](http://people.csail.mit.edu/nwadhwa/phase-video/PhaseBasedRelease_20131023.zip)

### Downloading the code
Download the zip-file provided above, and extract the files to a path suitable for you. This path shall be called "Path-to-PhaseAmplify"

### Configuring the motion magnification to use MIT code

#### Configuration before installation
If you have not yet installed the PyVideoMEG with motion magnification, you can do the following change.
1. From the cloned repository of VideoMEG, open amplify.m from path: VideoMEG/PyVideoMEG/matlab_scripts/amplify.m

2. On the following line:
```
PhaseBasedAmpDir = '';
```
* Change to line to point to the folder of the MIT code. For example on windows:
```
PhaseBasedAmpDir = 'C:\MIT_PhaseBased';
```
* Or Mac or Linux
```
PhaseBasedAmpDir = '/home/videomeg/MIT_PhaseBased';
```

3. After running setup.py, no further configuration is needed, unless MIT code is moved, in which case, check the configuration below.

#### Configuration after installation
So you have installed PyVideoMeg with motion magnification. This configuration unfortunately depends on your system setup.

1. Navigate to your Python site-packages folder.
2. Navigate pyvideomeg-* folder.
3. From matlab_scripts/amplify.m change the following line:
```
PhaseBasedAmpDir = '';
```
* Change to line to point to the folder of the MIT code. For example on windows:
```
PhaseBasedAmpDir = 'C:\MIT_PhaseBased';
```
* Or Mac or Linux
```
PhaseBasedAmpDir = '/home/videomeg/MIT_PhaseBased';
```
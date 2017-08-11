Currently the motion magnification uses Matlab for motion calculation. Thus user needs to have Matlab engine installed for the python, which allows python to interact with Matlab. (Works with python 2.7, 3.4, 3.5)

### Setup files
Installation files are shipped with Matlab R2014b and later.

### Intallation without virtualenv
If you are not sure if you are using virtualenv, then you probably are not. You should follow these instuctions.

1. Find out, where you have matlab installed. This can be done by runnign command 'matlabroot' in Matlab. This location is later referenced as matlabroot.

2. With command-line interface run the following commands (Might need root priviledges), where matlabroot is changed to location of matlab intallation:
```
cd "matlabroot\extern\engines\python"
python setup.py install
```

Without sudo access, following should work:
```
cd "matlabroot\extern\engines\python"
python setup.py build --build-base="[Path-to-location-where-you-have-access]" install --user
```
This will install engine to home folder.

Alternatively you can follow instructions [Matlab's instructions for installation](https://se.mathworks.com/help/matlab/matlab_external/install-the-matlab-engine-for-python.html)

### Installation with virtualenv
1. Find out, where you have matlab installed. This can be done by runnign command 'matlabroot' in Matlab. This location is later referenced as matlabroot.

2. Make sure you have your virtualenv activated.

3. With command-line interface run the following commands (Might need root priviledges), where matlabroot is changed to location of matlab intallation:
```
cd "matlabroot\extern\engines\python"
python setup.py install
```
If you don't have sudo access, you can do the installation for example by running setup.py with following command:
```
python setup.py build --build-base="[Path-to-location-where-you-have-access]" install
```
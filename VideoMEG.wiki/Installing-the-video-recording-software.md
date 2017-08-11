Video recording software runs on Linux. It has been tested with Ubuntu 16.04 LTS (64-bit desktop version). It might or might not work with other versions of Linux.

You can use software from different user accounts (however, you cannot run multiple instances of the software simultaneously), with each account having it's own configuration, ownership of the recorded files, etc. Software installation consists of system-wide installation and user-specific configuration for each user account that needs to run videoMEG recordings.

## System-wide installation
### Install the operating system
Install Ubuntu 16.04 for amd64. Set up the system without swap, to avoid potential performance issues.

It is highly recommended not to use any fancy things like lvm, raid, etc. since it will make recovering the data from the disk (if the need arises) easier. 

Make sure the computer is connected to the Internet. You should be able to browse the Internet and run a package manager (e.g. apt or synaptic).

### Do some post-setup configuration (optional, but recommended)
Run the software update:  
`sudo apt-get update`  
`sudo apt-get upgrade`

Install the optimized versiom of the JPEG library:  
`sudo apt-get install libjpeg-turbo8`

### Download the software to your computer
Download the release zip file to to your computer. The rest of this document assumes that you have downloaded the file VideoMEG-0.1.zip to your /tmp folder. If you use another release file, you'll need to modify commands accordingly.

Unzip the file to /opt:  
`cd /opt`  
`sudo unzip /tmp/VideoMEG-0.1.zip`

This should create a folder VideoMEG-0.1 containing all the VideoMEG software in /opt.

### Set up the firewire drivers
For interfacing the video camera we use AVT Fire4Linux package provided by the camera manufacturer (www.alliedvisiontec.com). You can find the package in the Third_Party folder. Fire4Linux seems to contain slightly modified versions of standard Linux libdc1394-\* packages + a test application called cc1394.  
**NOTE: as an alternative, you can try using standard libdc1394-\* packages from Ubuntu repository. It seems to work equally well, however, cc1394 will not work with these. You can use Linux application called coriander instead.**

To install the drivers just install all the deb packages from the tar archive except the ones that have 'dbg' in the name (to reduce the chances of using the debug version instead of the release one by mistake). Note that you also need to install the package 'libraw1394-dev' from the standard Ubuntu repositories as some of the Fire4Linux packages depend on it.

`sudo apt-get install libraw1394-dev`  
`cd /tmp`  
`tar -xf /opt/VideoMEG-0.1/Third_Party/AVTFire4Linux3v0_amd64-Ubuntu.deb.tar`  
`cd AVTFire4Linux3v0_amd64-Ubuntu_deb`  
`rm *dbg*`  
`sudo dpkg -i *.deb`  

After installing the drivers, connect the camera to the firewire card and run cc1394. You should be able to start/stop the camera, see the video feed, etc. Note that by default gain and exposure time are set to minimal possible values, you need to adjust these to see anything.

### Set up process priority permissions
The program needs permissions to elevate process static priorities to work properly. You need to give this permissions to every user that is going to run the program. To achieve this, create a group videomegusers:  
`sudo addgroup videomegusers`  

and give all the users in the videomegusers group permisions to elevate their processes priority. For that, add the line  
`@videomegusers - rtprio 95`  
to /etc/security/limits.conf. 

### Set up SyncDaemon
Make sure that SyncDaemon program works. Run  
`sudo /opt/VideoMEG-0.1/SyncDaemon/SyncDaemon 12304 255`

The number 12304 above (12304 decimal = 3010 hex) specifies the address that is used for accessing the parallel port. The address should be specified in decimal notation. Depending on your hardware, you might want to use 12304, 888 (378 hex), 632 (278 hex), 956 (3BC hex), or something else.  The command `cat /proc/ioports | grep parport` may be useful for finding out the correct number.

Check that:  
1. SyncDaemon produces no error messages  
2. after a short period of time (less than a few minutes) it starts outputting timing pulses (a train of 
pulses every 10 seconds) to the parallel port

If the above works, configure the system to run SyncDaemon automatically at the startup. For that add the line:  
`nohup /opt/VideoMEG-0.1/SyncDaemon/SyncDaemon 12304 255 &`

to /etc/rc.local. After rebooting SyncDaemon should be in the list of running processes and computer's parallel port should output timing pulses.

### Add the VideoMEG to the system path
`sudo ln -s /opt/VideoMEG-0.1/VideoRecStation/Release/VideoRecStation /usr/local/bin/VideoRecStation`

## User-specific configuration
You need to perform the steps below for every user that needs to run the videoMEG software.

### Add the user to the videomegusers group
`sudo adduser <user> videomegusers`

This should give the user \<user\> permissions to elevate process priority (you might need to restart the computer for the changes to take effect).  
**NOTE: If the permissions are not properly set, the videoMEG software won't be able to increase the priority of the critical acquisition threads, potentially leading to all kinds of problems. It will run nevertheless, but will print a warning message to the command line.**

### Log in with the user's credentials
Log into the account of the user that will run the videoMEG software. The steps described below show be performed from that user's account.

### Set up the config file
The config file is the place where you specify different system parameters, such as video compression quality, audio sampling frequency, etc. Some of these parameters, like the compression quality, you can modify freely. Others, like the audio sampling rate, need to match you hardware, otherwise the software will not work.

To set up the config file, first create the directory .config/HelsinkiVideoMEG in the user's home directory:  
`mkdir ~/.config/HelsinkiVideoMEG`

Then copy the default config file to ~/.config/HelsinkiVideoMEG:  
`cp /opt/VideoMEG-0.1/VideoRecStation/VideoRecStation.conf ~/.config/HelsinkiVideoMEG`

Modify the parameter data\_storage\_path in the config file to point to the folder where you want to store the video/audio data. You might also need to modify the parameters 'frames\_per\_period', 'num\_periods', and 'sampling_rate' to make the VideoMEG software work on your system. You can try to guess the correct values for these parameters by looking at the output of openpcm:  
`/opt/VideoMEG-0.1/Third_Party/Alsa/openpcm`
**NOTE: By default the videoMEG software accesses the sound card through pulseaudio daemon. As pulseaudio might introduce additional delay, you might want consider bypassing it and accessing the ALSA layer directly.**

### Run the VideoMEG software
If everything was set up properly you should be able to run the software by executing  
`VideoRecStation`

### Add the VideoMEG to the Unity toolbar
`cp /opt/VideoMEG-0.1/VideoRecStation/VideoRecStation.desktop ~/.local/share/applications`  
Navigate the file browser to `~/.local/share/applications` and double-click the VideoRecStation icon. This should start the application. The VideoRecStation icon should appear on the Unity toolbar. Right-click the icon and select "Lock to Launcher".

### Set up the development environment (optional)
You can skip this step if you are only interested in using the software without modifying it.  
`sudo apt-get install libjpeg8-dev libasound2-dev qtcreator qt5-default`


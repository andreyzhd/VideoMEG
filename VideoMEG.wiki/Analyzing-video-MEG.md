Although the Helsinki VideoMEG Project records audio and video separately from the MEG data, it timestamps all the data streams, which allows aligning the all data to the common timeline.  

Here are your options for analyzing the video-MEG:  
 1. If you are writing your own MATLAB or Python scripts for analysing your data, you can use the libraries provided by the project for importing audio and video and synchronizing it to MEG traces.  
 2. You can try using the [FieldTrip](http://www.fieldtriptoolbox.org/) toolbox&mdash;it provides some support for the Helsinki VideoMEG project.  
 3. If you have Elekta Oy's Triux or Vectorview system, you can try to obtain a version of their data analysis software that supports the Helsinki VideoMEG Project.  
 4. If none of the above works for you, you can at least export the video and audio to the *.avi format so you can review them without the MEG traces.


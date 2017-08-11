Audio and video is stored in *.aud and *.vid files; if there are multiple video cameras, each camera's video is stored in a separate *.vid file. All the data in the files is packed in the little-endian convention. The file content is described below in the order of it's appearance in the file. The general structure of the file is:  
 1. Magic string. The string is “ELEKTA_VIDEO_FILE” for video files and “ELEKTA_AUDIO_FILE” for audio files.  
 2. Format version number. This is a 32 bit integer specifying the format version.  
Format of the rest of the file is version-dependent.  

## Version 0
The audio file also has 2 more 32-bit integers describing sampling rate and number of channels respectively. The rest of the both types of files is filled with data blocks. Each data block consists of:  
 1. 64-bit integer containing the block's timestamp. The timestamp is the number of milliseconds since the UNIX epoch.  
 2. 32-bit integer containing the length of the payload in bytes.  
 3. The payload.  
  - For audio files the payload is the raw sample data. Each sample is encoded by a 16-bit signed integer. The channels are interleaved. For audio, all the blocks are of the same length.  
  - For video the payload is a single frame compressed in jpeg format. If you save the payload as a separate file you will get a normal jpeg file that you can open in an image viewer, etc.  

## Version 1
Identical to version 0.

/*
 * This file contains definitions that are shared between video recording and playing software
 *
 * ------------------------------------------------------------------------
 * Author: Andrey Zhdanov
 * Copyright (C) 2014 BioMag Laboratory, Helsinki University Central Hospital
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef COMMON_H_

// Camera configuration
#define VIDEO_HEIGHT        480
#define VIDEO_WIDTH         640
#define MAX_CAMERAS         6

// Audio configuration
#define AUDIO_FORMAT        SND_PCM_FORMAT_S16_LE   // from <alsa/asoundlib.h>
#define AUDIO_DATA_TYPE     int16_t                 // should match AUDIO_FORMAT
#define MAX_AUDIO_VAL       INT16_MAX               // should match AUDIO_FORMAT

#define AUDIO_FILE_VERSION  1
#define VIDEO_FILE_VERSION  1

#define MAGIC_VIDEO_STR     "ELEKTA_VIDEO_FILE"
#define MAGIC_AUDIO_STR     "ELEKTA_AUDIO_FILE"

// Markers configuration
#define MAX_MARKERS         4
#define KEY_REP_SUPPRES     1000                    // milliseconds

#define COMMON_H_


#endif /* COMMON_H_ */


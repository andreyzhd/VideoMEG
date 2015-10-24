/*
 * settings.h
 *
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

#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <QRect>
#include "config.h"

//! Application-wide settings preserved across multiple invocations.
/*!
 * This class follows singleton pattern. It encapsulates application-wide
 * settings stored on the disc. To read the settings simply get the instance of
 * this class with getSettings and read the values from the corresponding
 * public variables. You can modify the values, upon destruction these will be
 * saved to the disk. To make sure that the instance is not destroyed
 * prematurely, always pass it by reference (Settings&)---avoid pointers.
 *
 * This class should be made completely thread-safe (CURRENTLY IT IS NOT).
 */
class Settings {
    // TODO: make the class thread-safe
public:
    // video
    int             jpgQuality;
    bool            color;

    // audio
    unsigned int    sampRate;
    unsigned int    framesPerPeriod;
    unsigned int    nPeriods;
    unsigned int    spkBufSz;
    QString         inpAudioDev;
    QString         outAudioDev;
    bool            useFeedback;
    QRect           controllerRect;
    QRect           videoRects[MAX_CAMERAS];
    unsigned int    videoShutters[MAX_CAMERAS];
    unsigned int    videoGains[MAX_CAMERAS];
    unsigned int    videoUVs[MAX_CAMERAS];
    unsigned int    videoVRs[MAX_CAMERAS];
    bool            videoLimits[MAX_CAMERAS];

    // misc
    QString         storagePath;
    bool            dummyMode;
    bool            controlOnTop;
    double          lowDiskSpaceWarning;
    bool            confirmStop;
    bool            metersUseDB;

    // markers
    unsigned int    markerKeySym[MAX_MARKERS]; // codes (e.g. for XK_F8, XK_F9, etc) taken from <X11/Xutil.h>
    QString         markerType[MAX_MARKERS];
    QString         markersStoragePath;

    static Settings&    getSettings();

private:
    Settings();
    ~Settings();

    // true if one instance of the class already exists---used to prevent multiple instances
    static volatile bool existsInstance;
};

#endif /* SETTINGS_H_ */

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
#include <common.h>

//! Application-wide settings preserved across multiple invocations.
/*!
 * This class contains application-wide settings read from disc. To read the
 * settings simply create the instance of this class and read the values from
 * the corresponding public variables of the class. You can modify the values,
 * upon destruction these will be saved to the disk. Only one instance of the
 * class is allowed, if you try to create the second instance before
 * destroying the existing one, the constructor will trow an exception. This
 * class should be made completely thread-safe (CURRENTLY IT IS NOT).
 */
class Settings {
    // TODO: make the class thread-safe
public:
    Settings();
    ~Settings();

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

private:
    // true if one instance of the class already exists---used to prevent multiple instances
    static volatile bool existsInstance;
};

#endif /* SETTINGS_H_ */

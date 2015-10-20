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
 * the corresponding public variables of the class. The settings are supposed
 * to be read only; they can be changed by manually editing the text file
 * between program invocations but they should stay constant for the whole
 * lifetime of a single program instance. The class should be completely
 * thread-safe (CURRENTLY IT IS NOT).
 */
class Settings {
    // TODO: make member variables immutable as much as possible
    // TODO: make the class thread-safe
    // TODO: implement singleton pattern?
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
    quint32         markerVirtKey[MAX_MARKERS];
    QString         markerType[MAX_MARKERS];
    QString         markersStoragePath;
};

#endif /* SETTINGS_H_ */

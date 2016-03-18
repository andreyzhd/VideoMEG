/*
 * settings.cpp
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

#include <stdio.h>
#include <QSettings>
#include <QRect>
#include <QException>
#include <QDebug>

#include "settings.h"
#include "config.h"

volatile bool Settings::existsInstance = false;

Settings::Settings()
{
    qDebug() << "Settings constructor called";

    // Don't allow to create more than one instance. Because on the atomicity
    // of test-and-set is not guaranteed, it might fail in (hopefully) very rare
    // cases of race condition.
	Q_ASSERT(!existsInstance);
    existsInstance = true;
	
    QSettings settings(ORG_NAME, APP_NAME);

    //---------------------------------------------------------------------
    // Video settings
    //

    // JPEG quality
    jpgQuality = settings.value("video/jpeg_quality", 80).toInt();

    // Use color mode
    color = settings.value("video/color", true).toBool();

    cameraFrontend = settings.value("video/camera_frontend", "dc1394").toString();

    //---------------------------------------------------------------------
    // Capture settings
    //
    for (unsigned int i=0; i<MAX_CAMERAS; i++)
    {
        videoShutters[i] = settings.value(QString("video/camera_%1_shutter").arg(i+1), SHUTTER_MIN_VAL).toUInt();
        videoGains[i] = settings.value(QString("video/camera_%1_gain").arg(i+1), GAIN_MIN_VAL).toUInt();
        videoUVs[i] = settings.value(QString("video/camera_%1_UV").arg(i+1), UV_MIN_VAL).toUInt();
        videoVRs[i] = settings.value(QString("video/camera_%1_VR").arg(i+1), VR_MIN_VAL).toUInt();
        videoRects[i] = settings.value(QString("control/viewer_%1_window").arg(i+1), QRect(-1, -1, -1, -1)).toRect();
        videoLimits[i] = settings.value(QString("control/viewer_%1_limit_display_size").arg(i+1), false).toBool();
    }

    // Window pos and size
    controllerRect = settings.value("control/controller_window", QRect(-1, -1, -1, -1)).toRect();
    controlOnTop = settings.value("control/controller_on_top", false).toBool();
    lowDiskSpaceWarning = settings.value("control/low_disk_space_warning", 0).toDouble();
    confirmStop = settings.value("control/confirm_on_stop", false).toBool();
    metersUseDB = settings.value("control/meters_use_db", false).toBool();

    //---------------------------------------------------------------------
    // Audio settings
    //

    // Sampling rate
    sampRate = settings.value("audio/sampling_rate", 44100).toInt();

    // Frames per period
    framesPerPeriod = settings.value("audio/frames_per_period", 940).toInt();

    // Number of periods
    nPeriods = settings.value("audio/num_periods", 10).toInt();

    // Enable/disable speaker feedback
    useFeedback = settings.value("audio/use_speaker_feedback", true).toBool();

    // Speaker buffer size (in frames)
    spkBufSz = settings.value("audio/speaker_buffer_size", 4).toInt();

    // Input/output audio devices
    inpAudioDev = settings.value("audio/input_audio_device", "default").toString();
    outAudioDev = settings.value("audio/output_audio_device", "default").toString();

    //---------------------------------------------------------------------
    // Misc settings
    //

    // Data storage folder
    storagePath = settings.value("misc/data_storage_path", "/tmp").toString();

    // Markers
    for (unsigned int i=0; i<MAX_MARKER_TYPES; i++)
    {
        markerKeySym[i] = settings.value(QString("markers/KeySym_%1").arg(i+1), 0xffc6+i).toUInt(); // 0xffc6 corresponds to XK_F9
        markerType[i] = settings.value(QString("markers/type_%1").arg(i+1), QString("marker %1").arg(i+1)).toString();
    }

    markersStoragePath = settings.value("markers/storage_path", "/videodat/markers").toString();
}

Settings::~Settings()
{
    qDebug() << "Settings destructor called";

    QSettings settings(ORG_NAME, APP_NAME);

    settings.setValue("video/jpeg_quality", jpgQuality);
    settings.setValue("video/color", color);
    settings.setValue("video/camera_frontend", cameraFrontend);

    for (unsigned int i=0; i<MAX_CAMERAS; i++)
    {
        settings.setValue(QString("video/camera_%1_shutter").arg(i+1), videoShutters[i]);
        settings.setValue(QString("video/camera_%1_gain").arg(i+1), videoGains[i]);
        settings.setValue(QString("video/camera_%1_UV").arg(i+1), videoUVs[i]);
        settings.setValue(QString("video/camera_%1_VR").arg(i+1), videoVRs[i]);
        settings.setValue(QString("control/viewer_%1_window").arg(i+1), videoRects[i]);
        settings.setValue(QString("control/viewer_%1_limit_display_size").arg(i+1), videoLimits[i]);
    }
    settings.setValue("control/controller_window", controllerRect);
    settings.setValue("control/controller_on_top", controlOnTop);
    settings.setValue("control/low_disk_space_warning", lowDiskSpaceWarning);
    settings.setValue("control/confirm_on_stop", confirmStop);
    settings.setValue("control/meters_use_db", metersUseDB);

    settings.setValue("audio/sampling_rate", sampRate);
    settings.setValue("audio/frames_per_period", framesPerPeriod);
    settings.setValue("audio/num_periods", nPeriods);
    settings.setValue("audio/use_speaker_feedback", useFeedback);
    settings.setValue("audio/speaker_buffer_size", spkBufSz);

    settings.setValue("audio/input_audio_device", inpAudioDev);
    settings.setValue("audio/output_audio_device", outAudioDev);

    settings.setValue("misc/data_storage_path", storagePath);

    // Markers
    for (unsigned int i=0; i<MAX_MARKER_TYPES; i++)
    {
        settings.setValue(QString("markers/KeySym_%1").arg(i+1), markerKeySym[i]);
        settings.setValue(QString("markers/type_%1").arg(i+1), markerType[i]);
    }

    settings.setValue("markers/storage_path", markersStoragePath);

    settings.sync();
    existsInstance = false;
}

Settings& Settings::getSettings()
{
    static Settings globalInstance;
    return(globalInstance);
}

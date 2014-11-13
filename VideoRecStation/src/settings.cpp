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

#include "settings.h"
#include "config.h"

Settings::Settings()
{
	QSettings settings(ORG_NAME, APP_NAME);


	//---------------------------------------------------------------------
	// Video settings
	//

	// JPEG quality
	if(!settings.contains("video/jpeg_quality"))
	{
		settings.setValue("video/jpeg_quality", 80);
		jpgQuality = 80;
	}
	else
	{
		jpgQuality = settings.value("video/jpeg_quality").toInt();
	}

	// Use color mode
	if(!settings.contains("video/color"))
	{
		settings.setValue("video/color", false);
		color = true;
	}
	else
	{
		color = settings.value("video/color").toBool();
	}


	//---------------------------------------------------------------------
	// Audio settings
	//

	// Sampling rate
	if(!settings.contains("audio/sampling_rate"))
	{
		settings.setValue("audio/sampling_rate", 44100);
		sampRate = 44100;
	}
	else
	{
		sampRate = settings.value("audio/sampling_rate").toInt();
	}

	// Frames per period
	if(!settings.contains("audio/frames_per_period"))
	{
		settings.setValue("audio/frames_per_period", 940);
		framesPerPeriod = 940;
	}
	else
	{
		framesPerPeriod = settings.value("audio/frames_per_period").toInt();
	}

	// Number of periods
	if(!settings.contains("audio/num_periods"))
	{
		settings.setValue("audio/num_periods", 10);
		nPeriods = 10;
	}
	else
	{
		nPeriods = settings.value("audio/num_periods").toInt();
	}

	// Enable/disable speaker feedback
	if(!settings.contains("audio/use_speaker_feedback"))
	{
		settings.setValue("audio/use_speaker_feedback", true);
		useFeedback = true;
	}
	else
	{
		useFeedback = settings.value("audio/use_speaker_feedback").toBool();
	}

	// Speaker buffer size (in frames)
	if(!settings.contains("audio/speaker_buffer_size"))
	{
		settings.setValue("audio/speaker_buffer_size", 4);
		spkBufSz = 4;
	}
	else
	{
		spkBufSz = settings.value("audio/speaker_buffer_size").toInt();
	}

	// Input audio device
	if(!settings.contains("audio/input_audio_device"))
	{
		settings.setValue("audio/input_audio_device", "default");
		sprintf(inpAudioDev, "default");
	}
	else
	{
		sprintf(inpAudioDev, settings.value("audio/input_audio_device").toString().toLocal8Bit().data());
	}

	// Output audio device
	if(!settings.contains("audio/output_audio_device"))
	{
		settings.setValue("audio/output_audio_device", "default");
		sprintf(outAudioDev, "default");
	}
	else
	{
		sprintf(outAudioDev, settings.value("audio/output_audio_device").toString().toLocal8Bit().data());
	}


	//---------------------------------------------------------------------
	// Misc settings
	//

	// Data storage folder
	if(!settings.contains("misc/data_storage_path"))
	{
		settings.setValue("misc/data_storage_path", "/videodat");
		sprintf(storagePath, "/videodat");
	}
	else
	{
		sprintf(storagePath, settings.value("misc/data_storage_path").toString().toLocal8Bit().data());
	}
}


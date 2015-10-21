/*
 * microphonethread.cpp
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

#include <time.h>
#include <sched.h>
#include <iostream>

#include "microphonethread.h"
#include "config.h"

using namespace std;

MicrophoneThread::MicrophoneThread(CycDataBuffer* _cycBuf, Settings* _settings)
{
    int                     rc;
    snd_pcm_hw_params_t*    params;
    unsigned int            val;

    cycBuf = _cycBuf;
    settings = _settings;

    /* Open PCM device for recording (capture). */
    rc = snd_pcm_open(&pcmHandle, settings->inpAudioDev.toLocal8Bit().data(), SND_PCM_STREAM_CAPTURE, 0);
    if (rc < 0)
    {
        cerr << "unable to open pcm device: " << snd_strerror(rc) << endl;
        abort();
    }

    /* Allocate a hardware parameters object. */
    snd_pcm_hw_params_alloca(&params);

    /* Fill it in with default values. */
    if (snd_pcm_hw_params_any(pcmHandle, params) < 0)
    {
        cerr << "Can not configure PCM device: " << settings->inpAudioDev.toLocal8Bit().data() << endl;
        abort();
    }

    /* Set the desired hardware parameters. */

    /* Interleaved mode */
    snd_pcm_hw_params_set_access(pcmHandle, params, SND_PCM_ACCESS_RW_INTERLEAVED);

    /* Signed 16-bit little-endian format */
    snd_pcm_hw_params_set_format(pcmHandle, params, AUDIO_FORMAT);

    /* Specify the number of channels */
    snd_pcm_hw_params_set_channels(pcmHandle, params, N_CHANS);

    /* Set sampling rate */
    val = settings->sampRate;
    snd_pcm_hw_params_set_rate_near(pcmHandle, params, &val, NULL);

    /* Set period size (in frames) */
    framesPerPeriod = settings->framesPerPeriod;
    snd_pcm_hw_params_set_period_size_near(pcmHandle, params, &framesPerPeriod, NULL);

    /* Set number of periods */
    if (snd_pcm_hw_params_set_periods(pcmHandle, params, settings->nPeriods, 0) < 0)
    {
      cerr << "Error setting periods" << endl;
      abort();
    }

    /* Write the parameters to the driver */
    rc = snd_pcm_hw_params(pcmHandle, params);
    if (rc < 0)
    {
        cerr << "unable to set hw parameters: " << snd_strerror(rc) << endl;
        abort();
    }

    snd_pcm_hw_params_get_rate(params, &val, NULL);
    if (val != settings->sampRate)
    {
        cout << "unable to set sampling rate: requested " << settings->sampRate << ", actual " << val << endl;
        abort();
    }

    snd_pcm_hw_params_get_period_size(params, &framesPerPeriod, NULL);
    if (settings->framesPerPeriod != framesPerPeriod)
    {
        cout << "unable to set frames per period: requested " << settings->framesPerPeriod << ", actual " << framesPerPeriod << endl;
        abort();
    }

    /* Use a buffer large enough to hold one period */
    periodBuffer = (unsigned char*)malloc(framesPerPeriod * N_CHANS * sizeof(AUDIO_DATA_TYPE));
    if (!periodBuffer)
    {
        cerr << "Failed to allocate period buffer" << endl;
        abort();
    }
}


MicrophoneThread::~MicrophoneThread()
{
    snd_pcm_drain(pcmHandle);
    snd_pcm_close(pcmHandle);
    free(periodBuffer);
}


void MicrophoneThread::stoppableRun()
{
    int                 rc;
    struct timespec     timestamp;
    uint64_t            msec;
    struct sched_param  sch_param;
    ChunkAttrib         chunkAttrib;

    // Set priority
    sch_param.sched_priority = MIC_THREAD_PRIORITY;
    if (sched_setscheduler(0, SCHED_FIFO, &sch_param))
    {
        cerr << "Cannot set microphone thread priority. Continuing nevertheless, but don't blame me if you experience any strange problems." << endl;
    }

    // Start the acquisition loop
    while(true)
    {
        rc = snd_pcm_readi(pcmHandle, periodBuffer, framesPerPeriod);
        clock_gettime(CLOCK_REALTIME, &timestamp);

        if (rc == -EPIPE)
        {
            // EPIPE means overrun
            cerr << "Overrun occurred" << endl;
            snd_pcm_prepare(pcmHandle);
        }
        else if (rc < 0)
        {
            cerr << "Error from read: " << snd_strerror(rc) << endl;
        }
        else if (rc != (int)framesPerPeriod)
        {
            cerr << "short read, read " << rc << " frames instead of " << framesPerPeriod << endl;
        }

        msec = timestamp.tv_nsec / 1000000;
        msec += timestamp.tv_sec * 1000;

        chunkAttrib.chunkSize = settings->framesPerPeriod * N_CHANS * sizeof(AUDIO_DATA_TYPE);
        chunkAttrib.timestamp = msec;

        cycBuf->insertChunk(periodBuffer, chunkAttrib);
    }
}

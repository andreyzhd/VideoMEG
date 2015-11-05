/*
 * microphonethread.h
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

#ifndef MICROPHONETHREAD_H_
#define MICROPHONETHREAD_H_

// Use the newer ALSA API
#define ALSA_PCM_NEW_HW_PARAMS_API

#include <alsa/asoundlib.h>
#include "stoppablethread.h"
#include "cycdatabuffer.h"
#include "settings.h"

class MicrophoneThread : public StoppableThread
{
public:
    MicrophoneThread(CycDataBuffer* _cycBuf);
    virtual ~MicrophoneThread();

protected:
    virtual void stoppableRun();

private:
    CycDataBuffer*      cycBuf;
    snd_pcm_t*          pcmHandle;
    snd_pcm_uframes_t   framesPerPeriod;
    unsigned char*      periodBuffer;
    Settings&           settings = Settings::getSettings();
};

#endif /* MICROPHONETHREAD_H_ */

/*
 * dummycamera.cpp
 *
 * Author: Andrey Zhdanov
 * Copyright (C) 2016 BioMag Laboratory, Helsinki University Central Hospital
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

#include <QDebug>
#include "dummycamera.h"
#include "settings.h"

DummyCamera::DummyCamera()
{
    qDebug() << "DummyCamera constructor is called";

    Settings& settings = Settings::getSettings();

    shouldStop = false;
    color = settings.color;
}


void DummyCamera::setBuffer(CycDataBuffer *_cycBuf)
{
    cycBuf = _cycBuf;
}


DummyCamera::~DummyCamera()
{
    qDebug() << "DummyCamera destructor is called";
}


void DummyCamera::stoppableRun()
{

    struct sched_param      sch_param;
    struct timespec         timestamp;
    ChunkAttrib             chunkAttrib;
    unsigned int            chunkSize;
    unsigned char*          fakeImage;

    Q_ASSERT(cycBuf);

    chunkSize = VIDEO_HEIGHT * VIDEO_WIDTH * (color ? 3 : 1);
    chunkAttrib.chunkSize = chunkSize;

    // Set priority
    sch_param.sched_priority = CAM_THREAD_PRIORITY;
    if (sched_setscheduler(0, SCHED_FIFO, &sch_param))
    {
        qWarning() << "Cannot set camera thread priority. Continuing nevertheless, but don't blame me if you experience any strange problems.";
    }

    fakeImage = new unsigned char[chunkSize];

    while (!shouldStop)
    {
        msleep(33);
        clock_gettime(CLOCK_REALTIME, &timestamp);
        chunkAttrib.timestamp = timestamp.tv_nsec / 1000000 + timestamp.tv_sec * 1000;

        for(unsigned int i=0; i<chunkSize; i++)
        {
            fakeImage[i] = (unsigned char) qrand();
        }

        cycBuf->insertChunk(fakeImage, chunkAttrib);
    }

    delete fakeImage;
    return;
}


void DummyCamera::start()
{
    ::StoppableThread::start();
}


void DummyCamera::stop()
{
    ::StoppableThread::stop();
}


void DummyCamera::setShutter(int _newVal)
{
    qDebug() << "setShutter called, new value is: " << _newVal;
}


void DummyCamera::setGain(int _newVal)
{
    qDebug() << "setGain called, new value is: " << _newVal;
}


void DummyCamera::setUV(int _newVal)
{
    qDebug() << "setUV called, new value is: " << _newVal;
}


void DummyCamera::setVR(int _newVal)
{
    qDebug() << "setVR called, new value is: " << _newVal;
}

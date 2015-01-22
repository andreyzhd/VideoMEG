/*
 * camerathread.cpp
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

#include <iostream>
#include <sched.h>
#include <time.h>
#include <QCoreApplication>
#include <stdlib.h>
#include <QTime>

#include "camerathread.h"
#include "config.h"

using namespace std;


CameraThread::CameraThread(dc1394camera_t* _camera, CycDataBuffer* _cycBuf, bool _color)
{
    dc1394error_t err;

    cycBuf = _cycBuf;
    color = _color;
    shouldStop = false;

    camera = _camera;

    // Dummy mode
    if (camera == NULL)
    {
        return;
    }

    /*-----------------------------------------------------------------------
     *  setup capture
     *-----------------------------------------------------------------------*/
    err = dc1394_video_set_operation_mode(camera, DC1394_OPERATION_MODE_1394B);
    if (err != DC1394_SUCCESS)
    {
        cerr << "Could not set operation mode" << endl;
        abort();
    }

    err = dc1394_video_set_iso_speed(camera, DC1394_ISO_SPEED_800);
    if (err != DC1394_SUCCESS)
    {
        cerr << "Could not set iso speed" << endl;
        abort();
    }

    err = dc1394_video_set_mode(camera, (color ? DC1394_VIDEO_MODE_640x480_RGB8 : DC1394_VIDEO_MODE_640x480_MONO8));
    if (err != DC1394_SUCCESS)
    {
        cerr << "Could not set video mode" << endl;
        abort();
    }

    err = dc1394_video_set_framerate(camera, DC1394_FRAMERATE_30);
    if (err != DC1394_SUCCESS)
    {
        cerr << "Could not set framerate" << endl;
        abort();
    }

    err = dc1394_capture_setup(camera, N_CAMERA_BUFFERS, DC1394_CAPTURE_FLAGS_DEFAULT);
    if (err != DC1394_SUCCESS)
    {
        cerr << "Could not setup camera-" << endl \
             << "make sure that the video mode and framerate are" << endl \
             << "supported by your camera" << endl;
        abort();
    }
}


CameraThread::~CameraThread()
{
    dc1394error_t err;
    if (!camera)
    {
        return;
    }

    err = dc1394_capture_stop(camera);
    if (err != DC1394_SUCCESS)
    {
        cerr << "Could not stop video capture" << endl;
        abort();
    }
}


void CameraThread::stoppableRun()
{
    dc1394error_t           err;
    dc1394video_frame_t*    frame;
    struct sched_param      sch_param;
    struct timespec         timestamp;
    ChunkAttrib             chunkAttrib;
    unsigned int            chunkSize;
    unsigned char*          fakeImage;
    QTime                   time;

    chunkSize = VIDEO_HEIGHT * VIDEO_WIDTH * (color ? 3 : 1);
    chunkAttrib.chunkSize = chunkSize;

    // Set priority
    sch_param.sched_priority = CAM_THREAD_PRIORITY;
    if (sched_setscheduler(0, SCHED_FIFO, &sch_param))
    {
        cerr << "Cannot set camera thread priority. Continuing nevertheless, but don't blame me if you experience any strange problems." << endl;
    }

    /*-----------------------------------------------------------------------
     *  dummy mode
     *-----------------------------------------------------------------------*/
    if (!camera)
    {
        fakeImage = new unsigned char[chunkSize];
        time = QTime::currentTime();
        while (!shouldStop)
        {
            msleep(33);
            clock_gettime(CLOCK_REALTIME, &timestamp);
            chunkAttrib.timestamp = timestamp.tv_nsec / 1000000 + timestamp.tv_sec * 1000;
            for(unsigned int i=0; i < chunkSize; i++)
                fakeImage[i] = (unsigned char) qrand();
            cycBuf->insertChunk(fakeImage, chunkAttrib);
        }
        delete fakeImage;
        return;
    }

    /*-----------------------------------------------------------------------
     *  have the camera start sending us data
     *-----------------------------------------------------------------------*/
    err = dc1394_video_set_transmission(camera, DC1394_ON);
    if (err != DC1394_SUCCESS)
    {
        cerr << "Could not start camera iso transmission" << endl;
        abort();
    }

    // Start the acquisition loop
    while (!shouldStop)
    {
        err = dc1394_capture_dequeue(camera, DC1394_CAPTURE_POLICY_WAIT, &frame);
        clock_gettime(CLOCK_REALTIME, &timestamp);

        if (err != DC1394_SUCCESS)
        {
            cerr << "Error dequeuing a frame" << endl;
            abort();
        }

        chunkAttrib.timestamp = timestamp.tv_nsec / 1000000 + timestamp.tv_sec * 1000;
        cycBuf->insertChunk(frame->image, chunkAttrib);

        err = dc1394_capture_enqueue(camera, frame);
        if (err != DC1394_SUCCESS)
        {
            cerr << "Error re-enqueuing a frame" << endl;
            abort();
        }
    }

    /*-----------------------------------------------------------------------
     *  have the camera stop sending us data
     *-----------------------------------------------------------------------*/
    err = dc1394_video_set_transmission(camera, DC1394_OFF);
    if (err != DC1394_SUCCESS)
    {
        cerr << "Could not stop camera iso transmission" << endl;
        abort();
    }
}


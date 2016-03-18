/*
 * dc1394camera.cpp
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

#include <sched.h>
#include <time.h>
#include <QCoreApplication>
#include <stdlib.h>
#include <QDebug>

#include "dc1394camera.h"
#include "settings.h"

using namespace std;


dc1394Camera::dc1394Camera(dc1394camera_t* _camera)
{
    qDebug() << "dc1394Camera constructor is called";

    dc1394error_t err;
    Settings& settings = Settings::getSettings();

    shouldStop = false;
    camera = _camera;
    color = settings.color;

    /*-----------------------------------------------------------------------
     *  setup capture
     *-----------------------------------------------------------------------*/
    err = dc1394_video_set_operation_mode(camera, DC1394_OPERATION_MODE_1394B);
    if (err != DC1394_SUCCESS)
    {
        qFatal("Could not set operation mode");
    }

    err = dc1394_video_set_iso_speed(camera, DC1394_ISO_SPEED_800);
    if (err != DC1394_SUCCESS)
    {
        qFatal("Could not set iso speed");
    }

    err = dc1394_video_set_mode(camera, (color ? DC1394_VIDEO_MODE_640x480_RGB8 : DC1394_VIDEO_MODE_640x480_MONO8));
    if (err != DC1394_SUCCESS)
    {
        qFatal("Could not set video mode");
    }

    err = dc1394_video_set_framerate(camera, DC1394_FRAMERATE_30);
    if (err != DC1394_SUCCESS)
    {
        qFatal("Could not set framerate");
    }

    err = dc1394_capture_setup(camera, N_CAMERA_BUFFERS, DC1394_CAPTURE_FLAGS_DEFAULT);
    if (err != DC1394_SUCCESS)
    {
        qFatal("Could not setup camera. Make sure that the video mode and framerate are supported by your camera");
    }
}


void dc1394Camera::setBuffer(CycDataBuffer *_cycBuf)
{
    cycBuf = _cycBuf;
}


dc1394Camera::~dc1394Camera()
{
    qDebug() << "dc1394Camera destructor is called";

    dc1394error_t err;

    err = dc1394_capture_stop(camera);
    if(err != DC1394_SUCCESS)
    {
        qFatal("Error stopping the video capture");
    }
}


void dc1394Camera::stoppableRun()
{
    dc1394error_t           err;
    dc1394video_frame_t*    frame;
    struct sched_param      sch_param;
    struct timespec         timestamp;
    ChunkAttrib             chunkAttrib;
    unsigned int            chunkSize;
    //unsigned char*          fakeImage;

    Q_ASSERT(cycBuf);

    chunkSize = VIDEO_HEIGHT * VIDEO_WIDTH * (color ? 3 : 1);
    chunkAttrib.chunkSize = chunkSize;

    // Set priority
    sch_param.sched_priority = CAM_THREAD_PRIORITY;
    if (sched_setscheduler(0, SCHED_FIFO, &sch_param))
    {
        qWarning() << "Cannot set camera thread priority. Continuing nevertheless, but don't blame me if you experience any strange problems.";
    }

    /*-----------------------------------------------------------------------
     *  have the camera start sending us data
     *-----------------------------------------------------------------------*/
    err = dc1394_video_set_transmission(camera, DC1394_ON);
    if(err != DC1394_SUCCESS)
    {
        qFatal("Error starting up video transmission");
    }

    // Start the acquisition loop
    while (!shouldStop)
    {
        err = dc1394_capture_dequeue(camera, DC1394_CAPTURE_POLICY_WAIT, &frame);
        clock_gettime(CLOCK_REALTIME, &timestamp);

        if (err != DC1394_SUCCESS)
        {
            qFatal("Error dequeuing a frame");
        }

        chunkAttrib.timestamp = timestamp.tv_nsec / 1000000 + timestamp.tv_sec * 1000;
        cycBuf->insertChunk(frame->image, chunkAttrib);

        err = dc1394_capture_enqueue(camera, frame);
        if (err != DC1394_SUCCESS)
        {
            qFatal("Error re-enqueuing a frame");
        }
    }

    /*-----------------------------------------------------------------------
     *  have the camera stop sending us data
     *-----------------------------------------------------------------------*/
    err = dc1394_video_set_transmission(camera, DC1394_OFF);
    if (err != DC1394_SUCCESS)
    {
        qFatal("Error stopping video transmission");
    }
}


void dc1394Camera::start()
{
    ::StoppableThread::start();
}


void dc1394Camera::stop()
{
    ::StoppableThread::stop();
}


uint32_t dc1394Camera::scale(int _inp, uint32_t _minVal, uint32_t _maxVal)
{
    if(_inp < ::Camera::MIN_VAL)
        _inp = ::Camera::MIN_VAL;

    if(_inp > ::Camera::MAX_VAL)
        _inp = ::Camera::MAX_VAL;

    return(uint32_t(round(((double(_inp - ::Camera::MIN_VAL) / double(::Camera::MAX_VAL - ::Camera::MIN_VAL)) * (_maxVal - _minVal)) + _minVal)));
}


void dc1394Camera::setShutter(int _newVal)
{
    dc1394error_t   err;

    err = dc1394_set_register(camera, SHUTTER_ADDR, scale(_newVal, SHUTTER_MIN_VAL, SHUTTER_MAX_VAL) + SHUTTER_OFFSET);

    if (err != DC1394_SUCCESS)
    {
        qWarning() << "Could not set shutter register";
    }
}


void dc1394Camera::setGain(int _newVal)
{
    dc1394error_t   err;

    err = dc1394_set_register(camera, GAIN_ADDR, scale(_newVal, GAIN_MIN_VAL, GAIN_MAX_VAL) + GAIN_OFFSET);

    if (err != DC1394_SUCCESS)
    {
        qWarning() << "Could not set gain register";
    }
}


void dc1394Camera::setUV(int _newVal)
{
    dc1394error_t   err;
    uv = _newVal;

    // Since UV and VR live in the same register, we need to take care of both
    err = dc1394_set_register(camera, WHITEBALANCE_ADDR, scale(_newVal, UV_MIN_VAL, UV_MAX_VAL) * UV_REG_SHIFT + vr + WHITEBALANCE_OFFSET);

    if (err != DC1394_SUCCESS)
    {
        qWarning() << "Could not set white balance register";
    }
}


void dc1394Camera::setVR(int _newVal)
{
    dc1394error_t   err;
    vr = _newVal;

    // Since UV and VR live in the same register, we need to take care of both
    err = dc1394_set_register(camera, WHITEBALANCE_ADDR, scale(_newVal, VR_MIN_VAL, VR_MAX_VAL) + UV_REG_SHIFT * uv + WHITEBALANCE_OFFSET);

    if (err != DC1394_SUCCESS)
    {
        qWarning() << "Could not set white balance register";
    }
}

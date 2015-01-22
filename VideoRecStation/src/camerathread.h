/*
 * camerathread.h
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

#ifndef CAMERATHREAD_H_
#define CAMERATHREAD_H_

#include <dc1394/dc1394.h>

#include "stoppablethread.h"
#include "cycdatabuffer.h"

//! This thread acquires and timestamps frames for a single libdc1394 video camera.
class CameraThread : public StoppableThread
{
public:
    CameraThread(dc1394camera_t* _camera, CycDataBuffer* _cycBuf, bool _color);
    virtual ~CameraThread();

protected:
    virtual void stoppableRun();

private:
    dc1394camera_t* camera;
    CycDataBuffer*  cycBuf;
    bool            color;
};

#endif /* CAMERATHREAD_H_ */

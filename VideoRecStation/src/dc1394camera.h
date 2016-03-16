/*
 * dc1394camera.h
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

#ifndef DC1394CAMERA_H
#define DC1394CAMERA_H

#include <dc1394/dc1394.h>

#include "camera.h"
#include "stoppablethread.h"

//! This thread acquires and timestamps frames for a single libdc1394 video camera.
class dc1394Camera : public Camera, StoppableThread
{
public:
    dc1394Camera(dc1394camera_t* _camera, CycDataBuffer* _cycBuf);
    virtual ~dc1394CameraThread();
    void Camera::start();
    void Camera::stop();

protected:
    void stoppableRun() override;

private:
    dc1394camera_t* camera;
    CycDataBuffer*  cycBuf;
};

#endif // DC1394CAMERA_H


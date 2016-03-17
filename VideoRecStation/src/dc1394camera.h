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

#include "settings.h"
#include "camera.h"
#include "stoppablethread.h"

//! This thread acquires and timestamps frames for a single libdc1394 video camera.
class dc1394Camera : public Camera, StoppableThread
{
public:
    dc1394Camera(dc1394camera_t* _camera);
    void setBuffer(CycDataBuffer* _cycBuf);
    virtual ~dc1394Camera();
    void start();
    void stop();

    void setShutter(int _newVal);
    void setGain(int _newVal);
    void setUV(int _newVal);
    void setVR(int _newVal);

protected:
    void stoppableRun() override;

private:
    //! Scale a variable from [camera::MIN_VAL, camera::MAX_VAL] to [_minVal, _maxVal]
    uint32_t scale(int _inp, uint32_t _minVal, uint32_t _maxVal);
    dc1394camera_t* camera;
    CycDataBuffer*  cycBuf = NULL;
    bool color;

    // values of UV and VR color balance
    uint32_t uv = UV_MIN_VAL;
    uint32_t vr = VR_MIN_VAL;
};

#endif // DC1394CAMERA_H


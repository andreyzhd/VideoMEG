/*
 * dummycamera.h
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


#ifndef DUMMYCAMERA_H
#define DUMMYCAMERA_H

#include "camera.h"
#include "stoppablethread.h"

//! test implementation of the camera interface-generates random images
class DummyCamera : public Camera, StoppableThread
{
public:
    DummyCamera();  
    void setBuffer(CycDataBuffer* _cycBuf);
    virtual ~DummyCamera();
    void start();
    void stop();

    void setShutter(int _newVal);
    void setGain(int _newVal);
    void setUV(int _newVal);
    void setVR(int _newVal);

protected:
    void stoppableRun() override;

private:
    CycDataBuffer*  cycBuf = NULL;
    bool color;
};

#endif // DUMMYCAMERA_H

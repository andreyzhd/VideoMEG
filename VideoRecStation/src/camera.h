/*
 * camera.h
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

#ifndef CAMERA_H_
#define CAMERA_H_

#include "cycdatabuffer.h"

//! This is an interface abstracting a single camera.
/*!
 * Only call start() and stop() once, in that particular order.
 */
class Camera
{
public:
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void setBuffer(CycDataBuffer* _cycBuf) = 0;
    virtual ~Camera() {};

    virtual void setShutter(int _newVal) = 0;
    virtual void setGain(int _newVal) = 0;
    virtual void setUV(int _newVal) = 0;
    virtual void setVR(int _newVal) = 0;

    // minimal and maximal values for setXXX methods' parameters
    static const int MIN_VAL = 0;
    static const int MAX_VAL = 99;
};

#endif /* CAMERA_H_ */

/*
 * cameracollection.h
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

#ifndef CAMERACOLLECTION_H
#define CAMERACOLLECTION_H

#include "cycdatabuffer.h"
#include "camera.h"

//! This is an interface abstracting a collection of all the cameras on the computer.
/*!
 * The client code should check the number of cameras available via the
 * camCount method. For each number i between 0 and ((number of cameras) - 1),
 * the client can get a pointer to the i-th camera by calling
 * getCamera(i, cycBuf), where cycBuf is the buffer where the camera writes
 * the data. The client must delete the camera object before calling
 * getCamera(i, ...) once again for the same i. The client should delete all
 * the camera objects once it no longer needs them. Before deleting the camera,
 * the client should call stop() on the camera object. Once stop() returns,
 * the client can delete the buffer.
 */
class CameraCollection
{
public:
    virtual int camCount() = 0;
    virtual Camera* getCamera(int _cameraId) = 0;
    virtual QString getCameraModel(int _cameraId) = 0;
    virtual ~CameraCollection() {};
};

#endif // CAMERACOLLECTION_H


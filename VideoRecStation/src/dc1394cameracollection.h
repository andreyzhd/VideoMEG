/*
 * dc1394cameracollection.h
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


#ifndef DC1394CAMERACOLLECTION_H
#define DC1394CAMERACOLLECTION_H

#include <dc1394/dc1394.h>
#include "config.h"
#include "cameracollection.h"

//! dc1394-based implementation of CameraCollection
/*!
 * Never create more than one instance of this class. Destroy all the camera
 * objects obtained with getCamera() before calling the destructor.
 */
class dc1394CameraCollection : public CameraCollection
{
public:
    dc1394CameraCollection();
    ~dc1394CameraCollection();
    int camCount();
    Camera* getCamera(int _cameraId);
    QString getCameraModel(int _cameraId);

private:
    dc1394_t*           dc1394Context;
    dc1394camera_t*     cameras[MAX_CAMERAS];
    int                 numCameras;
};

#endif // DC1394CAMERACOLLECTION_H

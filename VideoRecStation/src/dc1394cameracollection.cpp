/*
 * dc1394cameracollection.cpp
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
#include "dc1394cameracollection.h"
#include "dc1394camera.h"

dc1394CameraCollection::dc1394CameraCollection()
{
    dc1394camera_list_t*    camList;
    dc1394error_t           err;

    dc1394Context = dc1394_new();
    if(!dc1394Context)
    {
        qFatal("Cannot initialize!");
    }

    err = dc1394_camera_enumerate(dc1394Context, &camList);
    if (err != DC1394_SUCCESS)
    {
        qFatal("Failed to enumerate cameras");
    }
    qDebug() << camList->num << " camera(s) found";
    numCameras = MAX_CAMERAS < camList->num ? MAX_CAMERAS : camList->num;

    // Initialize the cameras
    for (int i=0; i<numCameras; i++)
    {
        cameras[i] = dc1394_camera_new(dc1394Context, camList->ids[i].guid);
        if (!cameras[i])
        {
            qFatal("Failed to initialize camera with guid %li", camList->ids[i].guid);
        }
        qDebug() << "Using camera with GUID " << cameras[i]->guid;
    }
    dc1394_camera_free_list(camList);
}


dc1394CameraCollection::~dc1394CameraCollection()
{
    // Free the cameras
    for (int i=0; i<numCameras; i++)
    {
        dc1394_camera_free(cameras[i]);
    }

    dc1394_free(dc1394Context);
}


int dc1394CameraCollection::camCount()
{
    return(numCameras);
}


Camera* dc1394CameraCollection::getCamera(int _cameraId)
{
    return(new dc1394Camera(cameras[_cameraId]));
}


QString dc1394CameraCollection::getCameraModel(int _cameraId)
{
    return(QString(cameras[_cameraId]->model));
}

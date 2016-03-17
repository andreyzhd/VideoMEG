/*
 * dummycameracollection.h
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


#ifndef DUMMYCAMERACOLLECTION_H
#define DUMMYCAMERACOLLECTION_H

#include"cameracollection.h"

//! test implementation of CameraCollection
class DummyCameraCollection : public CameraCollection
{
public:
    DummyCameraCollection();
    virtual ~DummyCameraCollection();
    int camCount();
    Camera* getCamera(int _cameraId);
    QString getCameraModel(int _cameraId);
};

#endif // DUMMYCAMERACOLLECTION_H

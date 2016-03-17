/*
 * dummycameracollection.cpp
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

#include "config.h"
#include "dummycamera.h"
#include "dummycameracollection.h"

DummyCameraCollection::DummyCameraCollection()
{
}


DummyCameraCollection::~DummyCameraCollection()
{
}


int DummyCameraCollection::camCount()
{
    return(MAX_CAMERAS);
}


Camera* DummyCameraCollection::getCamera(int _cameraId)
{
    return(new DummyCamera());
}


QString DummyCameraCollection::getCameraModel(int _cameraId)
{
    return(QString("Dummy no. ") + QString(_cameraId));
}

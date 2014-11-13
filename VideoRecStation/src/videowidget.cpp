/*
 * videowidget.cpp
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

#include <iostream>
#include <math.h>

#include "config.h"
#include "videowidget.h"
#include "cycdatabuffer.h"

using namespace std;


VideoWidget::VideoWidget(QWidget* parent)
    : QLabel(parent)
{
	rotate = false;
}


void VideoWidget::onDrawFrame(unsigned char* _jpegBuf)
{
	ChunkAttrib chunkAttrib;
    QPixmap     pixMap;
    QTransform  transform;
    QTransform  trans = transform.rotate(rotate ? 180 : 0);

	chunkAttrib = *((ChunkAttrib*)(_jpegBuf-sizeof(ChunkAttrib)));

    pixMap.loadFromData(_jpegBuf, chunkAttrib.chunkSize);

    // before displaying, scale the pixmap to preserve the aspect ratio
    this->setPixmap(pixMap.scaled(this->width(), this->height(), Qt::KeepAspectRatio).transformed(trans));
}


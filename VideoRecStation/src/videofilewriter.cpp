/*
 * videofilewriter.cpp
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
#include <jpeglib.h>
#include <stdlib.h>

#include "config.h"
#include "videofilewriter.h"

using namespace std;


VideoFileWriter::VideoFileWriter(CycDataBuffer* _cycBuf, const char* _path, int _camId)
    :   FileWriter(_cycBuf, _path, "_video", "vid", _camId)
{
    uint32_t ver = VIDEO_FILE_VERSION;

    bufLen = strlen(MAGIC_VIDEO_STR) + sizeof(uint32_t);
    buf = (unsigned char*)malloc(bufLen);

    if(!buf)
    {
        cerr << "Error allocating memory!" << endl;
        abort();
    }

    memcpy(buf, MAGIC_VIDEO_STR, strlen(MAGIC_VIDEO_STR));          // string identifying the file type
    memcpy(buf + strlen(MAGIC_VIDEO_STR), &ver, sizeof(uint32_t));  // version of file format
}


VideoFileWriter::~VideoFileWriter()
{
    free(buf);
}


unsigned char* VideoFileWriter::getHeader(int* _len)
{
    *_len = bufLen;
    return(buf);
}

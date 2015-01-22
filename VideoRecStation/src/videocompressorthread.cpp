/*
 * videocompressorthread.cpp
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

#include <cstdlib>
#include <stdio.h>
#include <jpeglib.h>

#include "config.h"
#include "videocompressorthread.h"

VideoCompressorThread::VideoCompressorThread(CycDataBuffer* _inpBuf, CycDataBuffer* _outBuf, bool _color, int _jpgQuality)
{
    inpBuf = _inpBuf;
    outBuf = _outBuf;
    color = _color;
    jpgQuality = _jpgQuality;
}


VideoCompressorThread::~VideoCompressorThread()
{
}


void VideoCompressorThread::stoppableRun()
{
    while(!shouldStop)
    {
        // JPEG-related stuff
        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr       jerr;
        JSAMPROW                    row_pointer;
        unsigned char*              jpgBuf=NULL;
        unsigned long               jpgBufLen=0;

        unsigned char*              data;
        ChunkAttrib                 chunkAttrib;

        // Get raw image from the input buffer
        data = inpBuf->getChunk(&chunkAttrib);

        // Initialize JPEG
        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_compress(&cinfo);
        jpeg_mem_dest(&cinfo, &jpgBuf, &jpgBufLen);

        // Set the parameters of the output file
        cinfo.image_width = VIDEO_WIDTH;
        cinfo.image_height = VIDEO_HEIGHT;
        cinfo.input_components = (color ? 3 : 1);
        cinfo.in_color_space = (color ? JCS_RGB : JCS_GRAYSCALE);

        // Use default compression parameters
        jpeg_set_defaults(&cinfo);
        jpeg_set_quality(&cinfo, jpgQuality, TRUE);

        // Do the compression
        jpeg_start_compress(&cinfo, TRUE);

        // write one row at a time
        while(cinfo.next_scanline < cinfo.image_height)
        {
            row_pointer = (data + (cinfo.next_scanline * cinfo.image_width * (color ? 3 : 1)));
            jpeg_write_scanlines(&cinfo, &row_pointer, 1);
        }

        // clean up after we're done compressing
        jpeg_finish_compress(&cinfo);


        // Insert compressed image into the output buffer
        chunkAttrib.chunkSize = jpgBufLen;
        outBuf->insertChunk(jpgBuf, chunkAttrib);

        // The output buffer needs to be explicitly freed by the libjpeg client
        free(jpgBuf);
        jpeg_destroy_compress(&cinfo);
    }
}

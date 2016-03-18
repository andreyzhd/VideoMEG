/*
 * filewriter.cpp
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
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <QFileInfo>
#include <QDebug>

#include "filewriter.h"

using namespace std;

FileWriter::FileWriter(CycDataBuffer* _cycBuf, const char* _path, const char* _suffix, const char* _ext, int _streamId)
{
    cycBuf = _cycBuf;
    streamId = _streamId;

    path = (char*)malloc(strlen(_path)+1);
    if(!path)
    {
        qFatal("Cannot allocate memory!");
    }

    suffix = (char*)malloc(strlen(_suffix)+1);
    if(!suffix)
    {
        qFatal("Cannot allocate memory!");
    }

    ext = (char*)malloc(strlen(_ext)+1);
    if(!ext)
    {
        qFatal("Cannot allocate memory!");
    }

    strcpy(path, _path);
    strcpy(suffix, _suffix);
    strcpy(ext, _ext);
}


FileWriter::~FileWriter()
{
    free(ext);
    free(suffix);
    free(path);
}


void FileWriter::stoppableRun()
{
    unsigned char*  databuf;
    bool            prevIsRec=false;
    ofstream        outData;
    char            nameBuf[500];
    time_t          timeNow;
    struct tm*      timeNowParsed;
    ChunkAttrib     chunkAttrib;
    uint32_t        chunkSz;

    unsigned char*  header;
    int             headerLen;

    while (true)
    {
        databuf = cycBuf->getChunk(&chunkAttrib);
        if (chunkAttrib.isRec)
        {
            if (!prevIsRec)
            {
                // TODO: replace with code that uses Qt functions
                timeNow = time(NULL);
                timeNowParsed = localtime(&timeNow);
                sprintf(nameBuf, "%s/%04i-%02i-%02i--%02i-%02i-%02i%s_%02i.%s",
                        path,
                        timeNowParsed->tm_year+1900,
                        timeNowParsed->tm_mon+1,
                        timeNowParsed->tm_mday,
                        timeNowParsed->tm_hour,
                        timeNowParsed->tm_min,
                        timeNowParsed->tm_sec,
                        suffix,
                        streamId,
                        ext);
                outData.open(nameBuf, ios_base::out | ios_base::binary | ios_base::trunc);
                if(outData.fail())
                {
                    // TODO: Add more elaborate error checking
                    qFatal("Error opening the file %s", nameBuf);
                }
                readableFileName = QFileInfo(nameBuf).fileName();
                header = getHeader(&headerLen);
                outData.write((const char*)header, headerLen);
            }

            chunkSz = chunkAttrib.chunkSize;
            outData.write((const char*)(&(chunkAttrib.timestamp)), sizeof(quint64));
            outData.write((const char*)(&chunkSz), sizeof(uint32_t));
            outData.write((const char*)databuf, chunkAttrib.chunkSize);
        }
        else
        {
            if (prevIsRec)
            {
                outData.close();
                if (chmod(nameBuf, S_IRUSR | S_IRGRP | S_IROTH))
                {
                    qWarning() << "Could net set file read-only";
                }
            }
        }

        prevIsRec = chunkAttrib.isRec;

        if(shouldStop)
        {
            if(prevIsRec)
            {
                outData.close();
                if (chmod(nameBuf, S_IRUSR | S_IRGRP | S_IROTH))
                {
                    qWarning() << "Could net set file read-only";
                }
            }
            return;
        }
    }
}

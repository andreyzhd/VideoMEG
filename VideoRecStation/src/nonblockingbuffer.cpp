/*
 * nonblockingbuffer.cpp
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
#include <stdlib.h>
#include <string.h>
#include <QMutexLocker>

#include "nonblockingbuffer.h"

using namespace std;


NonBlockingBuffer::NonBlockingBuffer(int _bufSize, long _chunkSize)
{
    insertPtr = 0;              // if getPtr == insertPtr the buffer is empty
    getPtr = 0;
    bufSize = _bufSize+1;       // to store N items we use N+1 slots to simplify
                                // head/tail pointer arithmetics
    chunkSize = _chunkSize;
    buffMutex = new QMutex();

    // Allocate buffers
    dataBuf = (char*)malloc(bufSize * chunkSize);
    if (!dataBuf)
    {
        cerr << "Cannot allocate memory for non-blocking buffer" << endl;
        abort();
    }

    zeroChunk = (char*)malloc(chunkSize);
    if (!zeroChunk)
    {
        cerr << "Cannot allocate memory for the chunk of zeros" << endl;
        abort();
    }

    memset(zeroChunk, 0, chunkSize);
}


NonBlockingBuffer::~NonBlockingBuffer()
{
    free(zeroChunk);
    free(dataBuf);
    delete(buffMutex);
}


void NonBlockingBuffer::insertChunk(void* _data)
{
    QMutexLocker locker(buffMutex);

    // if the buffer is full discard the data
    if((insertPtr+1) % bufSize == getPtr)
    {
        // cerr << "Non-blocking buffer overflow, discarding the data" << endl;
        return;
    }

    // insert the data into the circular buffer
    memcpy(dataBuf + chunkSize * insertPtr, _data, chunkSize);
    insertPtr = (insertPtr+1) % bufSize;
}


void* NonBlockingBuffer::getChunk()
{
    void*           res;
    QMutexLocker    locker(buffMutex);

    if(insertPtr==getPtr)
    {
        // cerr << "Non-blocking buffer underflow, returning zeros" << endl;
        return(zeroChunk);
    }
    else
    {
        res = dataBuf + chunkSize * getPtr;
        getPtr = (getPtr+1) % bufSize;
        return(res);
    }
}

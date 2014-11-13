/*
 * nonblockingbuffer.h
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

#ifndef NONBLOCKINGBUFFER_H_
#define NONBLOCKINGBUFFER_H_

#include <QMutex>

class NonBlockingBuffer {
public:
	NonBlockingBuffer(int _bufSize, long _chunkSize);
	virtual ~NonBlockingBuffer();
	void insertChunk(void* _data);

	// Acquire a chunk and return a pointer to it. The chunk is implicitly
	// released next time getChunk is called.
	void* getChunk();

private:
	QMutex*	buffMutex;
    char*	dataBuf;
    char*	zeroChunk;
	int		bufSize;
	long	chunkSize;
	int		insertPtr;
	int		getPtr;
};

#endif /* NONBLOCKINGBUFFER_H_ */

/*
 * filewriter.h
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

#ifndef FILEWRITER_H_
#define FILEWRITER_H_

#include <QString>
#include "stoppablethread.h"
#include "cycdatabuffer.h"

//! Base class for audio/video stream writers
/*!
 * This base class implements functionality that is common to audio and video
 * stream writers - writing from a cyclic buffer to a file, creating/closing
 * files as necessary. Derived classes should implement getHeader() that should
 * return specific file header appropriate for the given file. Typically the
 * header would contain some string identifying file type and parameters (e.g.
 * sampling rate, etc.)
 *
 * When stopping the thread always call the stop() method while chunks are
 * still being continuously inserted into the CycDataBuffer associated with
 * the object - otherwise stop() will hang forever.
 */
class FileWriter : public StoppableThread
{
protected:
    FileWriter(CycDataBuffer* _cycBuf, const char* _path, const char* _suffix, const char* _ext, int _streamId);
    virtual ~FileWriter();
    virtual void stoppableRun();

    //! Return the header to be written at the beginning of the file.
    virtual unsigned char* getHeader(int* _len) = 0;

private:
    CycDataBuffer*  cycBuf;
    char*           path;
    char*           suffix;
    char*           ext;
    int             streamId;

public:
    QString readableFileName;
};

#endif /* FILEWRITER_H_ */

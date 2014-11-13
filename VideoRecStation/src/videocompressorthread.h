/*
 * videocompressorthread.h
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

#ifndef VIDEOCOMPRESSORTHREAD_H_
#define VIDEOCOMPRESSORTHREAD_H_

#include "stoppablethread.h"
#include "cycdatabuffer.h"

class VideoCompressorThread : public StoppableThread
{
public:
	VideoCompressorThread(CycDataBuffer* _inpBuf, CycDataBuffer* _outBuf, bool _color, int _jpgQuality);
	virtual ~VideoCompressorThread();

protected:
	virtual void stoppableRun();

private:
	CycDataBuffer*	inpBuf;
	CycDataBuffer*	outBuf;
	bool			color;
	int				jpgQuality;
};

#endif /* VIDEOCOMPRESSORTHREAD_H_ */

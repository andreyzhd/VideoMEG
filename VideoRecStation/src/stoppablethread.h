/*
 * stoppablethread.h
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

#ifndef STOPPABLETHREAD_H_
#define STOPPABLETHREAD_H_

#include <QThread>

//! Extension of QThread with added simple mechanism for stopping the thread.
/*!
 * This class adds a simple mechanism for stopping the thread. The thread is
 * stopped by calling stop() method which sets shouldStop protected variable
 * to true. User should override stoppableRun() method to do the actual job
 * (similar to the run() method of Qthread). User implementation of
 * stoppableRun() should check shouldStop variable regularly and return from
 * stoppableRun() if it is true. The stoppableRun() method should only be
 * called once in the object's lifetime.
 *
 * This class prints logging messages to the standard output when a new
 * object is created/run/stopped/destroyed. The code for these messages is
 * not properly synchronized by semaphores, so under certain (probably quite
 * rare) race conditions these messages can be complete garbage. However,
 * this should not affect other functionality of the class.
 */
class StoppableThread : public QThread
{
public:
    void stop();

protected:
    StoppableThread();
    virtual ~StoppableThread();
    virtual void run();
    virtual void stoppableRun() = 0;
    volatile bool shouldStop;

private:
    static int nextId;
    int id;
};

#endif /* STOPPABLETHREAD_H_ */

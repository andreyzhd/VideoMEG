/*
 * stoppablethread.cpp
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


#include <QDebug>
#include "stoppablethread.h"

using namespace std;

int StoppableThread::nextId = 0;

StoppableThread::StoppableThread()
{
    id = nextId++;
    shouldStop = false;

    qDebug() << "StoppableThread no. " << id << " is created";
}


StoppableThread::~StoppableThread()
{
    qDebug() << "StoppableThread no. " << id << " is destroyed";
}


void StoppableThread::run()
{
    qDebug() << "Running StoppableThread no. " << id << " ...";
    stoppableRun();
}


void StoppableThread::stop()
{
    qDebug() << "Stopping StoppableThread no. " << id << " ...";

    shouldStop = true;
    this->wait();

    qDebug() << "    ....done";
}


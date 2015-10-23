/*
 * camerathread.h
 *
 * Author: Andrey Zhdanov
 * Copyright (C) 2015 BioMag Laboratory, Helsinki University Central Hospital
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

#ifndef KEYMONITOR_H
#define KEYMONITOR_H

#include <stdint.h>
#include <QObject>

#include "stoppablethread.h"
#include "settings.h"

#include <X11/Xlib.h>   // These two files have to be included next to each
#include "fixx11h.h"    // other in this specific order. Otherwise things
                        // break.

//! A class for monitoring key presses.
/*!
 * This class monitors the key presses using X11-specific XGrabKey function.
 * This alows grabbing the key presses even when the application does not have
 * the input focus. Only one instance of the class is allowed, if you try to
 * create the second instance before destroying the existing one, the
 * constructor will trow an exception.
 */
class KeyMonitor : public StoppableThread
{
    Q_OBJECT

public:
    KeyMonitor(Settings* _settings);
    virtual ~KeyMonitor();

signals:
    /*!
     * This signal is raised when user presses a key to create a marker.
     */
    void keyPressed(QString _markerType, uint64_t _timestamp);

protected:
    virtual void stoppableRun();

private:
    Display*        dpy;
    unsigned int    keyCodes[MAX_MARKERS];
    QString         keyTypes[MAX_MARKERS];
    unsigned int    modifiers;
    Window          rootWnd;

    // true if one instance of the class already exists---used to prevent multiple instances
    static volatile bool existsInstance;
};

#endif // KEYMONITOR_H

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

#include <time.h>
#include <QException>
#include <QDebug>

#include "keymonitor.h"
#include "settings.h"

volatile bool KeyMonitor::existsInstance = false;

KeyMonitor::KeyMonitor()
{
    // Don't allow to create more than one instance. Because on the atomicity
    // of test-and-set is not guaranteed, it might fail in (hopefully) very rare
    // cases of race condition.
    Q_ASSERT(!existsInstance);
    existsInstance = true;

    int         i;
    Settings&   settings = Settings::getSettings();

    dpy       = XOpenDisplay(0);
    rootWnd   = DefaultRootWindow(dpy);
    modifiers = 0;  // grab only the keys pressed without any modifiers (like
                    // Ctrl, Shift, etc.). Note that no keys will be grabbed if
                    // NumLock is on.

    for (i=0; i<MAX_MARKER_TYPES; i++)
    {
        keyTypes[i] = settings.markerType[i];
        keyCodes[i] = XKeysymToKeycode(dpy, settings.markerKeySym[i]);
        XGrabKey(dpy, keyCodes[i], modifiers, rootWnd, true, GrabModeAsync, GrabModeAsync);
    }

    XSelectInput(dpy, rootWnd, KeyPressMask);
}


void KeyMonitor::stoppableRun()
{
    int             i;
    XEvent          ev;
    struct timespec utimestamp;
    quint64         timestamp;
    quint64         lastSentTimestamp = 0;

    while(!shouldStop)
    {
        XNextEvent(dpy, &ev);

        // ignore anything other than keypress events
        if(ev.type != KeyPress)
        {
            continue;
        }

        // find the marker type
        for(i=0; i<MAX_MARKER_TYPES; i++)
        {
            if((((XKeyEvent&)ev).keycode) == keyCodes[i])
            {
                clock_gettime(CLOCK_REALTIME, &utimestamp);
                timestamp = utimestamp.tv_nsec / 1000000 + utimestamp.tv_sec * 1000;

                // repetition suppression
                if(timestamp - lastSentTimestamp > KEY_REP_SUPPRES)
                {
                    emit keyPressed(keyTypes[i], timestamp);
                    lastSentTimestamp = timestamp;
                }
                break;  // if keyCodes has several identical entries, create only 1 event
            }
        }
    }
}


KeyMonitor::~KeyMonitor()
{
    int i;

    for(i=0; i<MAX_MARKER_TYPES; i++)
    {
        XUngrabKey(dpy, keyCodes[i], modifiers, rootWnd);
    }

    XCloseDisplay(dpy);
}

/*
 * videodialog.h
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


#ifndef VIDEODIALOG_H
#define VIDEODIALOG_H

#include <QDialog>
#include "ui_videodialog.h"
#include "camera.h"
#include "cycdatabuffer.h"
#include "videofilewriter.h"
#include "videocompressorthread.h"
#include "settings.h"


class VideoDialog : public QDialog
{
    Q_OBJECT

public:
    VideoDialog(Camera* _camera, int _cameraId, QWidget *parent = 0);
    virtual ~VideoDialog();
    void setIsRec(bool _isRec);

    //! Stop all the threads associated with the dialog.
    /*!
     * Stop all the threads associated with the dialog so that the dialog can
     * be safely destroyed. This method should always be called before the
     * destructor.
     */
    void stopThreads();

public slots:
    void onShutterChanged(int _newVal);
    void onGainChanged(int _newVal);
    void onUVChanged(int _newVal);
    void onVRChanged(int _newVal);
    void onNewFrame(unsigned char* _jpegBuf);
    void onLdsBoxToggled(bool _checked);

private:
    Ui::VideoDialogClass ui;

    unsigned int            cameraIdx;
    Camera*                 camera;
    CycDataBuffer*          cycVideoBufRaw;
    CycDataBuffer*          cycVideoBufJpeg;
    VideoFileWriter*        videoFileWriter;
    VideoCompressorThread*  videoCompressorThread;
    Settings&               settings = Settings::getSettings();

    // These variables are used for showing the FPS
    u_int64_t               prevFrameTstamp;
    int                     frameCnt;
};

#endif // VIDEODIALOG_H

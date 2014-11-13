/*
 * maindialog.h
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


#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#include <QMainWindow>

#include "config.h"
#include "ui_maindialog.h"
#include "camerathread.h"
#include "microphonethread.h"
#include "cycdatabuffer.h"
#include "videofilewriter.h"
#include "audiofilewriter.h"
#include "speakerthread.h"
#include "videocompressorthread.h"
#include "videodialog.h"
#include "settings.h"

class MainDialog : public QMainWindow
{
    Q_OBJECT

public:
    MainDialog(QWidget *parent = 0);
    ~MainDialog();

public slots:
    void onStartRec();
    void onStopRec();
    void onExit();
    void onAudioUpdate(unsigned char* _data);
    void onCam1Toggled(bool _state);
    void onCam2Toggled(bool _state);

private:
    void initVideo();

    Ui::MainDialogClass ui;

    dc1394camera_t*		camera1;
    dc1394camera_t*		camera2;
	VideoDialog*		videoDialog1;
	VideoDialog*		videoDialog2;

    MicrophoneThread*	microphoneThread;
    CycDataBuffer*		cycAudioBuf;
    AudioFileWriter*	audioFileWriter;

    // Data structures for volume indicator. volMaxvals is a cyclic buffer
    // that stores maximal values for the last N_BUF_4_VOL_IND periods for
    // all channels in an interleaved fashion.
	AUDIO_DATA_TYPE		volMaxvals[N_CHANS * N_BUF_4_VOL_IND];
	int					volIndNext;
	SpeakerThread*		speakerThread;
	NonBlockingBuffer*	speakerBuffer;
	Settings 			settings;
};

#endif // MAINDIALOG_H

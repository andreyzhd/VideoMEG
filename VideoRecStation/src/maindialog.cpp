/*
 * maindialog.cpp
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
#include <sys/statvfs.h>
#include <math.h>

#include <QStorageInfo>
#include <QDebug>

#include "config.h"
#include "maindialog.h"

using namespace std;


MainDialog::MainDialog(QWidget *parent)
    : QMainWindow(parent)
{
    Qt::WindowFlags flags = Qt::WindowTitleHint;

    if (settings.controlOnTop)
    {
        flags = flags | Qt::WindowStaysOnTopHint;
    }
    setWindowFlags(flags);

    ui.setupUi(this);

    // Set up status bar
    ui.statusBar->setSizeGripEnabled(false);
    statusLeft = new QLabel("", this);
    statusRight = new QLabel("", this);
    ui.statusBar->addPermanentWidget(statusLeft, 1);
    ui.statusBar->addPermanentWidget(statusRight, 0);
    updateDiskSpace();
    updateTimer = new QTimer(this);
    updateElapsed = new QTime();
    updateTimer->setInterval(500);  // every half second
    connect(updateTimer, SIGNAL(timeout()), this, SLOT(updateRunningStatus()));
    ui.clipLabel->setStyleSheet("QLabel { background-color : red; color : black; }");
    ui.clipLabel->setVisible(false);

    // Set up video recording
    initVideo();

    // Set up audio recording
    cycAudioBuf = new CycDataBuffer(CIRC_AUDIO_BUFF_SZ);
    microphoneThread = new MicrophoneThread(cycAudioBuf);
    audioFileWriter = new AudioFileWriter(cycAudioBuf, settings.storagePath.toLocal8Bit().data());
    QObject::connect(cycAudioBuf, SIGNAL(chunkReady(unsigned char*)), this, SLOT(onAudioUpdate(unsigned char*)));

    // Initialize volume indicator history
    memset(volMaxvals, 0, N_CHANS * N_BUF_4_VOL_IND * sizeof(AUDIO_DATA_TYPE));
    volIndNext = 0;

    // Initialize speaker
    if(settings.useFeedback)
    {
        speakerBuffer = new NonBlockingBuffer(settings.spkBufSz, settings.framesPerPeriod*N_CHANS*sizeof(AUDIO_DATA_TYPE));
        speakerThread = new SpeakerThread(speakerBuffer);
    }
    else
    {
        speakerBuffer = NULL;
        speakerThread = NULL;
    }

    if (settings.metersUseDB)
    {
        ui.levelLeft->setRange(-60, 0);
        ui.levelRight->setRange(-60, 0);
        ui.levelBottomLabel->setText("-60 dB");
        ui.levelTopLabel->setText("0 dB");
    }
    else
    {
        ui.levelLeft->setMaximum(MAX_AUDIO_VAL);
        ui.levelRight->setMaximum(MAX_AUDIO_VAL);
    }

    // Start audio running
    audioFileWriter->start();
    microphoneThread->start();

    // Start speaker thread
    if(speakerThread)
    {
        speakerThread->start();
    }
    if (settings.controllerRect.isValid())
        this->setGeometry(settings.controllerRect);
}


double MainDialog::freeSpaceGB()
{
    QStorageInfo storageInfo(settings.storagePath);
    return double(storageInfo.bytesAvailable()) / 1073741824.0;
}


void MainDialog::updateRunningStatus()
{
    int secs = updateElapsed->elapsed() / 1000;
    int mins = (secs / 60) % 60;
    int hours = secs / 3600;
    secs %= 60;
    if (hours > 0)
        statusLeft->setText(QString("Recording (%1:%2:%3)").arg(hours, 2, 10, QLatin1Char('0')).arg(mins, 2, 10, QLatin1Char('0')).arg(secs, 2, 10, QLatin1Char('0')));
    else
        statusLeft->setText(QString("Recording (%1:%2)").arg(mins, 2, 10, QLatin1Char('0')).arg(secs, 2, 10, QLatin1Char('0')));
    updateDiskSpace();
}

void MainDialog::updateDiskSpace()
{
    statusRight->setText(QString("%1 GB free").arg(freeSpaceGB(), 0, 'f', 1));
}


MainDialog::~MainDialog()
{
    // TODO: Implement proper destructor
    delete statusLeft;
    delete statusRight;
    delete updateTimer;
    delete updateElapsed;
}


void MainDialog::onStartRec()
{
    double freeSpace = freeSpaceGB();
    if (freeSpace < settings.lowDiskSpaceWarning)
    {
        if (QMessageBox::warning(this, "Low disk space",
                                 QString("Disk space is low (%1 GB). Do you really want to start recording?").arg(freeSpace, 0, 'f', 1),
                                 QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok)
            != QMessageBox::Ok)
        {
            return;
        }
    }
    ui.stopButton->setEnabled(true);
    ui.startButton->setEnabled(false);
    ui.exitButton->setEnabled(false);

    for (unsigned int i=0; i<numCameras; i++)
    {
        camCheckBoxes[i]->setEnabled(false);
        if (camCheckBoxes[i]->isChecked())
        {
            videoDialogs[i]->setIsRec(true);
        }
    }
    cycAudioBuf->setIsRec(true);
    updateElapsed->start();
    updateTimer->start();
}


void MainDialog::onStopRec()
{
    if (settings.confirmStop &&
        QMessageBox::warning(this, "Confirm recording end",
                             "Are you sure you want to stop recording?",
                             QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Ok)
        return;
    updateTimer->stop();
    ui.stopButton->setEnabled(false);
    ui.startButton->setEnabled(true);
    ui.exitButton->setEnabled(true);

    for (unsigned int i=0; i<numCameras; i++)
    {
        camCheckBoxes[i]->setEnabled(cameras[i] != NULL || settings.dummyMode);
        if(camCheckBoxes[i]->checkState() == Qt::Checked)
        {
            videoDialogs[i]->setIsRec(false);
        }
    }

    cycAudioBuf->setIsRec(false);
    QString fileName = QString(audioFileWriter->readableFileName);
    fileName.chop(13);
    statusLeft->setText(QString("Saved %1...").arg(fileName));
}


void MainDialog::setupVideoDialog(unsigned int idx)
{
    videoDialogs[idx] = new VideoDialog(cameras[idx], idx);
    if(settings.videoRects[idx].isValid())
        videoDialogs[idx]->setGeometry(settings.videoRects[idx]);
    videoDialogs[idx]->findChild<QSlider*>("shutterSlider")->setValue(settings.videoShutters[idx]);
    videoDialogs[idx]->findChild<QSlider*>("gainSlider")->setValue(settings.videoGains[idx]);
    videoDialogs[idx]->findChild<QSlider*>("uvSlider")->setValue(settings.videoUVs[idx]);
    videoDialogs[idx]->findChild<QSlider*>("vrSlider")->setValue(settings.videoVRs[idx]);
    videoDialogs[idx]->findChild<QCheckBox*>("ldsBox")->setChecked(settings.videoLimits[idx]);
    videoDialogs[idx]->show();
}


void MainDialog::cleanVideoDialog(unsigned int idx)
{
    videoDialogs[idx]->stopThreads();
    settings.videoRects[idx] = videoDialogs[idx]->geometry();
    settings.videoShutters[idx] = videoDialogs[idx]->findChild<QSlider*>("shutterSlider")->value();
    settings.videoGains[idx] = videoDialogs[idx]->findChild<QSlider*>("gainSlider")->value();
    settings.videoUVs[idx] = videoDialogs[idx]->findChild<QSlider*>("uvSlider")->value();
    settings.videoVRs[idx] = videoDialogs[idx]->findChild<QSlider*>("vrSlider")->value();
    settings.videoLimits[idx] = videoDialogs[idx]->findChild<QCheckBox*>("ldsBox")->isChecked();
    delete videoDialogs[idx];
}


void MainDialog::onExit()
{
    for (unsigned int i=0; i<numCameras; i++)
        if(camCheckBoxes[i]->isChecked())
            this->cleanVideoDialog(i);
    settings.controllerRect = this->geometry();
    close();
}


void MainDialog::onAudioUpdate(unsigned char* _data)
{
    unsigned int    i=0;
    unsigned int    j;
    AUDIO_DATA_TYPE maxvals[N_CHANS]={0};
    AUDIO_DATA_TYPE curval;

    // Update the history
    memset(&(volMaxvals[volIndNext]), 0, N_CHANS * sizeof(AUDIO_DATA_TYPE));
    while(i < settings.framesPerPeriod * N_CHANS)
    {
        for(j=0; j<N_CHANS; j++)
        {
            curval = abs(((AUDIO_DATA_TYPE*)_data)[i++]);
            volMaxvals[volIndNext + j] = (volMaxvals[volIndNext + j] >= curval) ? volMaxvals[volIndNext + j] : curval;
        }
    }

    volIndNext += N_CHANS;
    volIndNext %= (N_CHANS * N_BUF_4_VOL_IND);

    // Compute maxima for all channels
    i = 0;
    while(i < N_CHANS * N_BUF_4_VOL_IND)
    {
        for(j=0; j<N_CHANS; j++)
        {
            curval = volMaxvals[i++];
            maxvals[j] = (maxvals[j] >= curval) ? maxvals[j] : curval;
        }
    }

    // Update only two level bars
    if (settings.metersUseDB)
    {
        ui.levelLeft->setValue(20 * log10((double)maxvals[0] / MAX_AUDIO_VAL));
        ui.levelRight->setValue(20 * log10((double)maxvals[1] / MAX_AUDIO_VAL));
    }
    else
    {
        ui.levelLeft->setValue(maxvals[0]);
        ui.levelRight->setValue(maxvals[1]);
    }
    if (maxvals[0] >= MAX_AUDIO_VAL || maxvals[1] >= MAX_AUDIO_VAL)
        ui.clipLabel->setVisible(true);
    else
        ui.clipLabel->setVisible(false);

    // Feed to the speaker
    if(speakerBuffer)
    {
        speakerBuffer->insertChunk(_data);
    }
}


void MainDialog::initVideo()
{
    dc1394_t*               dc1394Context;
    dc1394camera_list_t*    camList;
    dc1394error_t           err;

    for (unsigned int i=0; i < MAX_CAMERAS; i++)
        cameras[i] = NULL;

    if (settings.dummyMode)
    {
        numCameras = MAX_CAMERAS;
    }
    else
    {
        dc1394Context = dc1394_new();
        if(!dc1394Context)
        {
            cerr << "Cannot initialize!" << endl;
            abort();
        }

        err = dc1394_camera_enumerate(dc1394Context, &camList);
        if (err != DC1394_SUCCESS)
        {
            cerr << "Failed to enumerate cameras" << endl;
            abort();
        }
        cerr << camList->num << " camera(s) found" << endl;
        numCameras = MAX_CAMERAS < camList->num ? MAX_CAMERAS : camList->num;

        // use the first camera in the list
        for (unsigned int i=0; i < numCameras; i++)
        {
            cameras[i] = dc1394_camera_new(dc1394Context, camList->ids[i].guid);
            if (!cameras[i])
            {
                cerr << "Failed to initialize camera with guid " << camList->ids[0].guid << endl;
                abort();
            }
            cout << "Using camera with GUID " << cameras[0]->guid << endl;
        }
        dc1394_camera_free_list(camList);
    }

    // Construct and populate camera check boxes
    for (unsigned int i=0; i < numCameras; i++)
    {
        QString name = settings.dummyMode ? "Dummy" : cameras[i]->model;
        if (name.length() > 20)
        {
            name.truncate(17);
            name.append("...");
        }
        camCheckBoxes[i] = new QCheckBox(QString("%1: %2").arg(i+1).arg(name), this);
        ui.videoVerticalLayout->addWidget(camCheckBoxes[i]);
        camCheckBoxes[i]->setEnabled(true);
        connect(camCheckBoxes[i], SIGNAL(toggled(bool)), this, SLOT(onCamToggled(bool)));
    }
    statusLeft->setText(QString("Found %1 camera%2").arg(numCameras).arg(numCameras != 1 ? "s" : ""));
    vertSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui.videoVerticalLayout->addItem(vertSpacer);
}


void MainDialog::onCamToggled(bool _state)
{
    int idx = -1;
    for (unsigned int i=0; i < numCameras; i++)
    {
        if (sender() == camCheckBoxes[i])
        {
            idx = i;
            break;
        }
    }
    if (idx < 0)
    {
        cerr << "Could not ID camera" << endl;
        abort();
    }

    if(_state)
    {
        this->setupVideoDialog(idx);
    }
    else
    {
        this->cleanVideoDialog(idx);
    }
}


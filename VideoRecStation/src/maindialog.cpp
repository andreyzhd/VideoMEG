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

#include "config.h"
#include "maindialog.h"

using namespace std;


MainDialog::MainDialog(QWidget *parent)
    : QMainWindow(parent)
{

	ui.setupUi(this);
	setWindowFlags(Qt::WindowTitleHint);

	// Set up video recording
	initVideo();

    // Set up audio recording
    cycAudioBuf = new CycDataBuffer(CIRC_AUDIO_BUFF_SZ);
    microphoneThread = new MicrophoneThread(cycAudioBuf);
    audioFileWriter = new AudioFileWriter(cycAudioBuf, settings.storagePath);
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

    ui.levelLeft->setMaximum(MAX_AUDIO_VAL);
    ui.levelRight->setMaximum(MAX_AUDIO_VAL);

    // Start audio running
    audioFileWriter->start();
    microphoneThread->start();

    // Start speaker thread
    if(speakerThread)
    {
    	speakerThread->start();
    }
}


MainDialog::~MainDialog()
{
	// TODO: Implement proper destructor
}


void MainDialog::onStartRec()
{
    ui.stopButton->setEnabled(true);
    ui.startButton->setEnabled(false);
    ui.exitButton->setEnabled(false);

    ui.cam1CheckBox->setEnabled(false);
    ui.cam2CheckBox->setEnabled(false);

    if(ui.cam1CheckBox->checkState() == Qt::Checked)
    {
    	videoDialog1->setIsRec(true);
    }

    if(ui.cam2CheckBox->checkState() == Qt::Checked)
    {
    	videoDialog2->setIsRec(true);
    }

    cycAudioBuf->setIsRec(true);
}


void MainDialog::onStopRec()
{
    ui.stopButton->setEnabled(false);
    ui.startButton->setEnabled(true);
    ui.exitButton->setEnabled(true);

    ui.cam1CheckBox->setEnabled(camera1 != NULL);
    ui.cam2CheckBox->setEnabled(camera2 != NULL);

    if(ui.cam1CheckBox->checkState() == Qt::Checked)
    {
    	videoDialog1->setIsRec(false);
    }

    if(ui.cam2CheckBox->checkState() == Qt::Checked)
    {
    	videoDialog2->setIsRec(false);
    }

    cycAudioBuf->setIsRec(false);
}


void MainDialog::onExit()
{
    if(ui.cam1CheckBox->checkState() == Qt::Checked)
    {
		videoDialog1->stopThreads();
		delete videoDialog1;
    }

    if(ui.cam2CheckBox->checkState() == Qt::Checked)
    {
		videoDialog2->stopThreads();
		delete videoDialog2;
    }

    close();
}


void MainDialog::onAudioUpdate(unsigned char* _data)
{
	unsigned int 	i=0;
	unsigned int	j;
	AUDIO_DATA_TYPE	maxvals[N_CHANS]={0};
	AUDIO_DATA_TYPE	curval;

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
	ui.levelLeft->setValue(maxvals[0]);
	ui.levelRight->setValue(maxvals[1]);

	// Feed to the speaker
	if(speakerBuffer)
	{
		speakerBuffer->insertChunk(_data);
	}
}


void MainDialog::initVideo()
{
    dc1394_t*				dc1394Context;
    dc1394camera_list_t*	camList;
    dc1394error_t			err;

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

    camera1 = NULL;
    camera2 = NULL;

    if (camList->num == 0)
    {
        cerr << "No cameras found" << endl;
        return;
    }

    // use the first camera in the list
    camera1 = dc1394_camera_new(dc1394Context, camList->ids[0].guid);
    if (!camera1)
    {
        cerr << "Failed to initialize camera with guid " << camList->ids[0].guid << endl;
        abort();
    }
    cout << "Using camera with GUID " << camera1->guid << endl;
    ui.cam1CheckBox->setEnabled(true);

    // use the second camera in the list
    if(camList->num > 1)
    {
    	camera2 = dc1394_camera_new(dc1394Context, camList->ids[1].guid);
    	if(!camera2)
    	{
    		cerr << "Failed to initialize camera with guid " << camList->ids[1].guid << endl;
    		abort();
    	}
    	cout << "Using camera with GUID " << camera2->guid << endl;
    	ui.cam2CheckBox->setEnabled(true);
    }

    dc1394_camera_free_list(camList);
}


void MainDialog::onCam1Toggled(bool _state)
{
	if(_state)
	{
		videoDialog1 = new VideoDialog(camera1, 1);
		videoDialog1->show();
	}
	else
	{
		videoDialog1->stopThreads();
		delete videoDialog1;
	}
}


void MainDialog::onCam2Toggled(bool _state)
{
	if(_state)
	{
		videoDialog2 = new VideoDialog(camera2, 2);
		videoDialog2->show();
	}
	else
	{
		videoDialog2->stopThreads();
		delete videoDialog2;
	}
}

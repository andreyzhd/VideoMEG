/*
 * videodialog.cpp
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

#include "videodialog.h"
#include "config.h"
#include "settings.h"

using namespace std;

VideoDialog::VideoDialog(dc1394camera_t* _camera, int _cameraIdx, QWidget *parent)
    : QDialog(parent)
{
	char		winCaption[500];
    Settings	settings;
    cameraIdx = _cameraIdx;

	ui.setupUi(this);
	setWindowFlags(Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint| Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint);
    sprintf(winCaption, "Camera %i", cameraIdx + 1);
	setWindowTitle(winCaption);
	camera = _camera;

	// Set up video recording
	cycVideoBufRaw = new CycDataBuffer(CIRC_VIDEO_BUFF_SZ);
	cycVideoBufJpeg = new CycDataBuffer(CIRC_VIDEO_BUFF_SZ);
    cameraThread = new CameraThread(camera, cycVideoBufRaw, settings.color);
    videoFileWriter = new VideoFileWriter(cycVideoBufJpeg, settings.storagePath, cameraIdx + 1);
	videoCompressorThread = new VideoCompressorThread(cycVideoBufRaw, cycVideoBufJpeg, settings.color, settings.jpgQuality);

    QObject::connect(cycVideoBufJpeg, SIGNAL(chunkReady(unsigned char*)), ui.videoWidget, SLOT(onDrawFrame(unsigned char*)));
    QObject::connect(cycVideoBufJpeg, SIGNAL(chunkReady(unsigned char*)), this, SLOT(onNewFrame(unsigned char*)));

	// Setup gain/shutter sliders
    ui.shutterSlider->setMinimum(SHUTTER_MIN_VAL);
    ui.shutterSlider->setMaximum(SHUTTER_MAX_VAL);
    ui.shutterSlider->setValue(settings.videoShutters[cameraIdx]);

    ui.gainSlider->setMinimum(GAIN_MIN_VAL);
    ui.gainSlider->setMaximum(GAIN_MAX_VAL);
    ui.gainSlider->setValue(settings.videoGains[cameraIdx]);

    ui.uvSlider->setMinimum(UV_MIN_VAL);
    ui.uvSlider->setMaximum(UV_MAX_VAL);
    ui.uvSlider->setValue(settings.videoUVs[cameraIdx]);

    ui.vrSlider->setMinimum(VR_MIN_VAL);
    ui.vrSlider->setMaximum(VR_MAX_VAL);
    ui.vrSlider->setValue(settings.videoVRs[cameraIdx]);

    ui.uvSlider->setEnabled(settings.color);
    ui.vrSlider->setEnabled(settings.color);
    ui.uvLabel->setEnabled(settings.color);
    ui.vrLabel->setEnabled(settings.color);
    ui.wbLabel->setEnabled(settings.color);

    // Start video running
    videoFileWriter->start();
    videoCompressorThread->start();
    cameraThread->start();
}


VideoDialog::~VideoDialog()
{
    delete cycVideoBufRaw;
	delete cycVideoBufJpeg;
	delete cameraThread;
	delete videoFileWriter;
	delete videoCompressorThread;
}


void VideoDialog::stopThreads()
{
	// The piece of code stopping the threads should execute fast enough,
	// otherwise cycVideoBufRaw or cycVideoBufJpeg buffer might overflow. The
	// order of stopping the threads is important.
	videoFileWriter->stop();
	videoCompressorThread->stop();
	cameraThread->stop();
}


void VideoDialog::onShutterChanged(int _newVal)
{
	dc1394error_t	err;

    Settings settings;
    settings.videoShutters[cameraIdx] = _newVal;
    if (!cameraThread || !camera)
	{
		return;
	}

	err = dc1394_set_register(camera, SHUTTER_ADDR, _newVal + SHUTTER_OFFSET);

    if (err != DC1394_SUCCESS)
    {
        cerr << "Could not set shutter register" << endl;
        //abort();
    }
}


void VideoDialog::onGainChanged(int _newVal)
{
	dc1394error_t	err;

    Settings settings;
    settings.videoGains[cameraIdx] = _newVal;
    if (!cameraThread || !camera)
	{
		return;
	}

	err = dc1394_set_register(camera, GAIN_ADDR, _newVal + GAIN_OFFSET);

    if (err != DC1394_SUCCESS)
    {
        cerr << "Could not set gain register" << endl;
        //abort();
    }
}


void VideoDialog::onUVChanged(int _newVal)
{
	dc1394error_t	err;

    if (!cameraThread || !camera)
	{
		return;
	}

	// Since UV and VR live in the same register, we need to take care of both
	err = dc1394_set_register(camera, WHITEBALANCE_ADDR, _newVal * UV_REG_SHIFT + ui.vrSlider->value() + WHITEBALANCE_OFFSET);

    if (err != DC1394_SUCCESS)
    {
        cerr << "Could not set white balance register" << endl;
        //abort();
    }
}


void VideoDialog::onVRChanged(int _newVal)
{
	dc1394error_t	err;

    if (!cameraThread || !camera)
	{
		return;
	}

	// Since UV and VR live in the same register, we need to take care of both
	err = dc1394_set_register(camera, WHITEBALANCE_ADDR, _newVal + UV_REG_SHIFT * ui.uvSlider->value() + WHITEBALANCE_OFFSET);

    if (err != DC1394_SUCCESS)
    {
        cerr << "Could not set white balance register" << endl;
        //abort();
    }
}


void VideoDialog::onNewFrame(unsigned char* _jpegBuf)
{
    ChunkAttrib chunkAttrib;
    float       fps;
    char        fpsLabelBuff[100];

    chunkAttrib = *((ChunkAttrib*)(_jpegBuf-sizeof(ChunkAttrib)));

    if (prevFrameTstamp)
    {
        fps = 1 / (float(chunkAttrib.timestamp - prevFrameTstamp) / 1000);
        sprintf(fpsLabelBuff, "FPS: %02.01f", fps);

        if(frameCnt == 10)
        {
            ui.fpsLabel->setText(fpsLabelBuff);
            frameCnt = 0;
        }
    }

    prevFrameTstamp = chunkAttrib.timestamp;
    frameCnt++;
}


void VideoDialog::setIsRec(bool _isRec)
{
	cycVideoBufJpeg->setIsRec(_isRec);
}

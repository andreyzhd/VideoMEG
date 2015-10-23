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

using namespace std;

VideoDialog::VideoDialog(dc1394camera_t* _camera, int _cameraIdx, Settings* _settings, QWidget *parent)
    : QDialog(parent)
{
    settings = _settings;
    cameraIdx = _cameraIdx;
    prevFrameTstamp = 0;
    frameCnt = 0;

    ui.setupUi(this);
    ui.videoWidget->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    setWindowFlags(Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint| Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint);
    setWindowTitle(QString("Camera %1").arg(cameraIdx + 1));
    camera = _camera;

    // Set up video recording
    cycVideoBufRaw = new CycDataBuffer(CIRC_VIDEO_BUFF_SZ);
    cycVideoBufJpeg = new CycDataBuffer(CIRC_VIDEO_BUFF_SZ);
    cameraThread = new CameraThread(camera, cycVideoBufRaw, settings->color);
    videoFileWriter = new VideoFileWriter(cycVideoBufJpeg, settings->storagePath.toLocal8Bit().data(), cameraIdx + 1);
    videoCompressorThread = new VideoCompressorThread(cycVideoBufRaw, cycVideoBufJpeg, settings->color, settings->jpgQuality);

    QObject::connect(cycVideoBufJpeg, SIGNAL(chunkReady(unsigned char*)), ui.videoWidget, SLOT(onDrawFrame(unsigned char*)));
    QObject::connect(cycVideoBufJpeg, SIGNAL(chunkReady(unsigned char*)), this, SLOT(onNewFrame(unsigned char*)));

    // Setup sliders' limits
    ui.shutterSlider->setMinimum(SHUTTER_MIN_VAL);
    ui.shutterSlider->setMaximum(SHUTTER_MAX_VAL);

    ui.gainSlider->setMinimum(GAIN_MIN_VAL);
    ui.gainSlider->setMaximum(GAIN_MAX_VAL);

    ui.uvSlider->setMinimum(UV_MIN_VAL);
    ui.uvSlider->setMaximum(UV_MAX_VAL);

    ui.vrSlider->setMinimum(VR_MIN_VAL);
    ui.vrSlider->setMaximum(VR_MAX_VAL);

    ui.uvSlider->setEnabled(settings->color);
    ui.vrSlider->setEnabled(settings->color);
    ui.uvLabel->setEnabled(settings->color);
    ui.vrLabel->setEnabled(settings->color);
    ui.wbLabel->setEnabled(settings->color);

    if(settings->videoRects[_cameraIdx].isValid())
    {
        this->setGeometry(settings->videoRects[_cameraIdx]);
    }

    // Setup sliders' positions
    ui.shutterSlider->setValue(settings->videoShutters[_cameraIdx]);
    ui.gainSlider->setValue(settings->videoGains[_cameraIdx]);
    ui.uvSlider->setValue(settings->videoUVs[_cameraIdx]);
    ui.vrSlider->setValue(settings->videoVRs[_cameraIdx]);

    // Write the sliders' values to the camera
    onShutterChanged(settings->videoShutters[_cameraIdx]);
    onGainChanged(settings->videoGains[_cameraIdx]);
    onUVChanged(settings->videoUVs[_cameraIdx]);
    onVRChanged(settings->videoVRs[_cameraIdx]);

    ui.ldsBox->setChecked(settings->videoLimits[_cameraIdx]);

    // Start video running
    videoFileWriter->start();
    videoCompressorThread->start();
    cameraThread->start();
}


VideoDialog::~VideoDialog()
{
    settings->videoRects[cameraIdx] = this->geometry();
    settings->videoShutters[cameraIdx] = ui.shutterSlider->value();
    settings->videoGains[cameraIdx] = ui.gainSlider->value();
    settings->videoUVs[cameraIdx] = ui.uvSlider->value();
    settings->videoVRs[cameraIdx] = ui.vrSlider->value();
    settings->videoLimits[cameraIdx] = ui.ldsBox->isChecked();

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
    dc1394error_t   err;

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
    dc1394error_t   err;

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
    dc1394error_t   err;

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
    dc1394error_t   err;

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

    chunkAttrib = *((ChunkAttrib*)(_jpegBuf-sizeof(ChunkAttrib)));

    if (prevFrameTstamp)
    {
        if(frameCnt == 10)
        {
            fps = 1 / (float(chunkAttrib.timestamp - prevFrameTstamp) / 1000);
            ui.fpsLabel->setText(QString("FPS: %1").arg(fps, 0, 'f', 2));
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

void VideoDialog::onLdsBoxToggled(bool _checked)
{
    ui.videoWidget->limitDisplaySize = _checked;
}

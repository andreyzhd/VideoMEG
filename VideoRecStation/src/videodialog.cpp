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


#include "videodialog.h"
#include "config.h"

using namespace std;

VideoDialog::VideoDialog(Camera* _camera, int _cameraIdx, QWidget *parent)
    : QDialog(parent)
{
    camera = _camera;
    cameraIdx = _cameraIdx;
    prevFrameTstamp = 0;
    frameCnt = 0;

    ui.setupUi(this);
    ui.videoWidget->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    setWindowFlags(Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint| Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint);
    setWindowTitle(QString("Camera %1").arg(cameraIdx + 1));

    // Set up video recording
    cycVideoBufRaw = new CycDataBuffer(CIRC_VIDEO_BUFF_SZ);
    cycVideoBufJpeg = new CycDataBuffer(CIRC_VIDEO_BUFF_SZ);
    camera->setBuffer(cycVideoBufRaw);
    videoFileWriter = new VideoFileWriter(cycVideoBufJpeg, settings.storagePath.toLocal8Bit().data(), cameraIdx + 1);
    videoCompressorThread = new VideoCompressorThread(cycVideoBufRaw, cycVideoBufJpeg, settings.color, settings.jpgQuality);

    QObject::connect(cycVideoBufJpeg, SIGNAL(chunkReady(unsigned char*)), ui.videoWidget, SLOT(onDrawFrame(unsigned char*)));
    QObject::connect(cycVideoBufJpeg, SIGNAL(chunkReady(unsigned char*)), this, SLOT(onNewFrame(unsigned char*)));

    // Setup sliders' limits
    ui.shutterSlider->setMinimum(camera->MIN_VAL);
    ui.shutterSlider->setMaximum(camera->MAX_VAL);

    ui.gainSlider->setMinimum(camera->MIN_VAL);
    ui.gainSlider->setMaximum(camera->MAX_VAL);

    ui.uvSlider->setMinimum(camera->MIN_VAL);
    ui.uvSlider->setMaximum(camera->MAX_VAL);

    ui.vrSlider->setMinimum(camera->MIN_VAL);
    ui.vrSlider->setMaximum(camera->MAX_VAL);

    ui.uvSlider->setEnabled(settings.color);
    ui.vrSlider->setEnabled(settings.color);
    ui.uvLabel->setEnabled(settings.color);
    ui.vrLabel->setEnabled(settings.color);
    ui.wbLabel->setEnabled(settings.color);

    if(settings.videoRects[_cameraIdx].isValid())
    {
        this->setGeometry(settings.videoRects[_cameraIdx]);
    }

    // Setup sliders' positions
    ui.shutterSlider->setValue(settings.videoShutters[_cameraIdx]);
    ui.gainSlider->setValue(settings.videoGains[_cameraIdx]);
    ui.uvSlider->setValue(settings.videoUVs[_cameraIdx]);
    ui.vrSlider->setValue(settings.videoVRs[_cameraIdx]);

    // Write the sliders' values to the camera
    onShutterChanged(settings.videoShutters[_cameraIdx]);
    onGainChanged(settings.videoGains[_cameraIdx]);
    onUVChanged(settings.videoUVs[_cameraIdx]);
    onVRChanged(settings.videoVRs[_cameraIdx]);

    ui.ldsBox->setChecked(settings.videoLimits[_cameraIdx]);

    // Start video running
    videoFileWriter->start();
    videoCompressorThread->start();
    camera->start();
}


VideoDialog::~VideoDialog()
{
    settings.videoRects[cameraIdx] = this->geometry();
    settings.videoShutters[cameraIdx] = ui.shutterSlider->value();
    settings.videoGains[cameraIdx] = ui.gainSlider->value();
    settings.videoUVs[cameraIdx] = ui.uvSlider->value();
    settings.videoVRs[cameraIdx] = ui.vrSlider->value();
    settings.videoLimits[cameraIdx] = ui.ldsBox->isChecked();

    delete cycVideoBufRaw;
    delete cycVideoBufJpeg;
    delete videoFileWriter;
    delete videoCompressorThread;
    delete camera;
}


void VideoDialog::stopThreads()
{
    // The piece of code stopping the threads should execute fast enough,
    // otherwise cycVideoBufRaw or cycVideoBufJpeg buffer might overflow. The
    // order of stopping the threads is important.
    videoFileWriter->stop();
    videoCompressorThread->stop();
    camera->stop();
}


void VideoDialog::onShutterChanged(int _newVal)
{
    camera->setShutter(_newVal);
}


void VideoDialog::onGainChanged(int _newVal)
{
    camera->setGain(_newVal);
}


void VideoDialog::onUVChanged(int _newVal)
{
    camera->setUV(_newVal);
}


void VideoDialog::onVRChanged(int _newVal)
{
    camera->setVR(_newVal);
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

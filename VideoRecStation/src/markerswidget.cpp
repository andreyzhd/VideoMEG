#include <QDateTime>

#include "settings.h"
#include "markerswidget.h"
#include "ui_markerswidget.h"

MarkersWidget::MarkersWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MarkersWidget)
{
    ui->setupUi(this);
    ui->markerTable->setColumnCount(3);
    ui->markerTable->setHorizontalHeaderLabels(QString("time;type;comment").split(";"));

    markersStoragePath = Settings::getSettings().markersStoragePath;

    setEnabled(false);
}

MarkersWidget::~MarkersWidget()
{
    delete ui;
}

void MarkersWidget::onKeyPressed(QString _markerType, uint64_t _timestamp)
{
    QString     timeStr;
    QDateTime   dt;

    dt.setTime_t(_timestamp/1000);
    timeStr = dt.toString("hh:mh:hs ap");
}


void MarkersWidget::setEnabled(bool _enabled)
{
    ui->markerTable->setEnabled(_enabled);
    ui->markerTextEdit->setEnabled(_enabled);
}

void MarkersWidget::clear()
{
}

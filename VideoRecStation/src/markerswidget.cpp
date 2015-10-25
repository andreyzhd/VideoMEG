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
    rowCnt = 0;

    setEnabled(false);

    QObject::connect(&keyMonitor, SIGNAL(keyPressed(QString, quint64)), this, SLOT(onKeyPressed(QString, quint64)));
    keyMonitor.start();
}

MarkersWidget::~MarkersWidget()
{
    keyMonitor.stop();
    delete ui;
}

void MarkersWidget::onKeyPressed(QString _markerType, quint64 _timestamp)
{
    QDateTime   dt;

    if(!isEnabled)
    {
        return;
    }

    ui->markerTable->setRowCount(rowCnt+1);

    dt.setTime_t(_timestamp/1000);
    table[rowCnt][0].setText(dt.toString("hh:mm:ss ap"));
    ui->markerTable->setItem(rowCnt, 0, &(table[rowCnt][0]));


    rowCnt++;
}


void MarkersWidget::setEnabled(bool _enabled)
{
    isEnabled = _enabled;
    ui->markerTable->setEnabled(_enabled);
    ui->markerTextEdit->setEnabled(_enabled);
}

void MarkersWidget::clear()
{

}

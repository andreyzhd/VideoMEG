#include <QDateTime>
#include <QDebug>

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
    ui->markerTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->markerTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    markersStoragePath = Settings::getSettings().markersStoragePath;
    rowCnt = 0;
    curEditIndx = -1;

    setEnabled(false);

    QObject::connect(&keyMonitor, SIGNAL(keyPressed(QString, quint64)), this, SLOT(onKeyPressed(QString, quint64)));
    QObject::connect(ui->markerTable, SIGNAL(itemSelectionChanged()), this, SLOT(onItemSelectionChanged()));
    QObject::connect(ui->markerTextEdit, SIGNAL(textChanged()), this, SLOT(onTextChanged()));

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
    table[rowCnt][0].setText(dt.toString("hh:mm:ss"));
    table[rowCnt][0].setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

    table[rowCnt][1].setText(_markerType);
    table[rowCnt][1].setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

    table[rowCnt][2].setText("Type your comment here ...");
    table[rowCnt][2].setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

    ui->markerTable->setItem(rowCnt, 0, &(table[rowCnt][0]));
    ui->markerTable->setItem(rowCnt, 1, &(table[rowCnt][1]));
    ui->markerTable->setItem(rowCnt, 2, &(table[rowCnt][2]));

    ui->markerTable->scrollToBottom();

    markerFiles[rowCnt].init(_timestamp, _markerType, table[rowCnt][2].text());

    rowCnt++;
}


void MarkersWidget::setEnabled(bool _enabled)
{
    isEnabled = _enabled;
    ui->markerTable->setEnabled(_enabled);

    if(!_enabled)
    {
        // markerTextEdit might need to be disabled even if MarkersWidget is enabled.
        ui->markerTextEdit->setEnabled(_enabled);

        // flush the marker files
        for(int i=0; i<rowCnt; i++)
        {
            markerFiles[i].flush();
        }
    }
}


void MarkersWidget::onItemSelectionChanged()
{
    QList<QTableWidgetItem*> selection;
    QModelIndex indx;

    selection = ui->markerTable->selectedItems();
    indx = ui->markerTable->currentIndex();

    if(selection.length() == 0)
    {
        ui->markerTextEdit->setEnabled(false);
        curEditIndx = -1;
    }
    else
    {
        curEditIndx = indx.row();
        ui->markerTextEdit->setPlainText(table[curEditIndx][2].text());
        ui->markerTextEdit->setEnabled(true);
    }
}


void MarkersWidget::onTextChanged()
{
    if(curEditIndx != -1)
    {
        table[curEditIndx][2].setText(ui->markerTextEdit->toPlainText());
        markerFiles[curEditIndx].setComment(ui->markerTextEdit->toPlainText());
    }
}

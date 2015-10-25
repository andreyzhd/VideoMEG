#ifndef MARKERSWIDGET_H
#define MARKERSWIDGET_H

#include <QWidget>
#include <QTableWidgetItem>

#include "keymonitor.h"

namespace Ui {
class MarkersWidget;
}

class MarkersWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MarkersWidget(QWidget *parent = 0);
    ~MarkersWidget();
    void setEnabled(bool _enabled);
    void clear();

public slots:
    void onKeyPressed(QString _markerType, quint64 _timestamp);

private:
    Ui::MarkersWidget   *ui;

    KeyMonitor          keyMonitor;
    QString             markersStoragePath;
    int                 rowCnt;
    QTableWidgetItem    table[MAX_MARKERS][3];
    bool                isEnabled;
};

#endif // MARKERSWIDGET_H

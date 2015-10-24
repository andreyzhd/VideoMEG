#ifndef MARKERSWIDGET_H
#define MARKERSWIDGET_H

#include <QWidget>

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
    void onKeyPressed(QString _markerType, uint64_t _timestamp);

private:
    Ui::MarkersWidget   *ui;

    KeyMonitor          keyMonitor();
    QString             markersStoragePath;
};

#endif // MARKERSWIDGET_H

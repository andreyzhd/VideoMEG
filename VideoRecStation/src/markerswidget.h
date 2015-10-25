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

public slots:
    void onKeyPressed(QString _markerType, quint64 _timestamp);
    void onItemSelectionChanged();
    void onTextChanged();

private:
    Ui::MarkersWidget   *ui;

    KeyMonitor          keyMonitor;
    QString             markersStoragePath;
    int                 rowCnt;
    QTableWidgetItem    table[MAX_MARKERS][3];
    bool                isEnabled;
    int                 curEditIndx;        // index of the marker currently being edited in the text field
};

#endif // MARKERSWIDGET_H

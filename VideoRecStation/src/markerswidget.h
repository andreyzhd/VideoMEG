#ifndef MARKERSWIDGET_H
#define MARKERSWIDGET_H

#include <QWidget>

namespace Ui {
class MarkersWidget;
}

class MarkersWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MarkersWidget(QWidget *parent = 0);
    ~MarkersWidget();

private:
    Ui::MarkersWidget *ui;
};

#endif // MARKERSWIDGET_H

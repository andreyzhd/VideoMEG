#include "markerswidget.h"
#include "ui_markerswidget.h"

MarkersWidget::MarkersWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MarkersWidget)
{
    ui->setupUi(this);
}

MarkersWidget::~MarkersWidget()
{
    delete ui;
}

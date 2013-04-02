#include "corefeaturewidget.h"
#include <QPainter>

CoreFeatureWidget::CoreFeatureWidget(QWidget *parent) :
    QWidget(parent)
{
}

void CoreFeatureWidget::SetFacePosition(QRect rect)
{
}

void CoreFeatureWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);

    painter.drawLine(0, 0, this->width(), this->height());
}

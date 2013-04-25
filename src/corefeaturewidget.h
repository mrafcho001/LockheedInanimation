#ifndef COREFEATUREWIDGET_H
#define COREFEATUREWIDGET_H

#include <QWidget>

class CoreFeatureWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CoreFeatureWidget(QWidget *parent = 0);
    
public slots:

    void SetFacePosition(QRect rect);

private:
    
};

#endif // COREFEATUREWIDGET_H

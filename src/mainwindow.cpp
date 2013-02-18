#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "facetracker.h"
#include <QPainter>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ft = new FaceTracker(0);
    ft->SetMinFeatureSize(5);

    PositionUpdater *pu = new PositionUpdater(ft);
    connect(pu, SIGNAL(Updated(QImage*)), this, SLOT(UpdatePos(QImage*)));
    pu->start();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::UpdateImage()
{

    QRect rect = ft->GetFacePosition();
}

void MainWindow::UpdatePos(QImage *image)
{
    ui->label->setPixmap(QPixmap::fromImage(*image));
    delete image;
}

PositionUpdater::PositionUpdater() { }
PositionUpdater::PositionUpdater(FaceTracker *ft, QObject *parent) { m_ft = ft; }
void PositionUpdater::run()
{
    while(1)
    {
        QRect rect = m_ft->GetFacePosition();
        QImage *image = m_ft->GetFaceImage();
        QPainter p;
        p.begin(image);
        p.setPen(QPen(Qt::red));
        p.drawRect(rect);
        p.end();
        emit Updated(image);
    }
}

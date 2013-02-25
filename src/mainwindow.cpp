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
    connect(pu, SIGNAL(UpdateFullImage(QImage*)), this, SLOT(UpdateImage(QImage*)));
    connect(pu, SIGNAL(UpdateFace(QImage*)), this, SLOT(UpdateFace(QImage*)));
    pu->start();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::UpdateImage(QImage *image)
{
    ui->label->setPixmap(QPixmap::fromImage(*image));
    delete image;
}

void MainWindow::UpdateFace(QImage *image)
{
    ui->label_3->setPixmap(QPixmap::fromImage(*image));
    delete image;
}

PositionUpdater::PositionUpdater() { }
PositionUpdater::PositionUpdater(FaceTracker *ft, QObject *parent) { m_ft = ft; }
void PositionUpdater::run()
{
    while(1)
    {
        QRect rect = m_ft->GetFacePosition();
        QImage *image = m_ft->GetLastImage();
        QPainter p;
        p.begin(image);
        p.setPen(QPen(Qt::red));
        p.drawRect(rect);
        p.end();

        emit UpdateFace(new QImage(image->copy(rect)));
        emit UpdateFullImage(image);
    }
}

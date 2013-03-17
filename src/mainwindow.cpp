#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "facetracker.h"
#include <QPainter>
#include <QTime>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ft = new FaceTracker(0);
    ft->SetMinFeatureSize(5);
    ft->SetProcessingImageDimensions(640,480);
    ft->SetSearchScaleFactor(1.2f);

    pu = new PositionUpdater(ft);
    connect(pu, SIGNAL(UpdateFullImage(QImage*)), this, SLOT(UpdateImage(QImage*)));
    connect(pu, SIGNAL(UpdateFace(QImage*)), this, SLOT(UpdateFace(QImage*)));
    pu->start();

    ui->tabWidget->setCurrentIndex(1);

    connect(ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(HandleTabChange(int)));
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

void MainWindow::UpdatePosition(QRect rect)
{
    static_cast<FaceInvadersWidget*>(ui->graphicsView)->updatePlayerPosition(rect.center());
}

void MainWindow::HandleTabChange(int index)
{
    if(index == 1)
    {
        pu->EnableImageUpdateSignals(false);
        connect(pu, SIGNAL(UpdatePosition(QRect)), this, SLOT(UpdatePosition(QRect)));
        static_cast<FaceInvadersWidget*>(ui->graphicsView)->beginGame();
    }
    else
    {
        pu->EnableImageUpdateSignals();
        disconnect(pu, SIGNAL(UpdatePosition(QRect)), this, SLOT(UpdatePosition(QRect)));
    }
}

PositionUpdater::PositionUpdater() : images(false) { }
PositionUpdater::PositionUpdater(FaceTracker *ft, QObject *parent): m_ft(ft), stopped(false) { }

void PositionUpdater::PauseThread()
{
    mutex.lock();
    stopped = true;
    mutex.unlock();
}

void PositionUpdater::ResumeThread()
{
    mutex.lock();
    stopped = false;
    condition.wakeAll();
    mutex.unlock();
}

void PositionUpdater::EnableImageUpdateSignals(bool enable)
{
    images = enable;
}
void PositionUpdater::run()
{
    QTime time;
    time.start();
    int counter = 0;
    while(1)
    {
        mutex.lock();
        if(stopped) condition.wait(&mutex);
        mutex.unlock();

        counter++;
        if(counter == 30)
        {
            qDebug() << "Ellapsed: " << time.elapsed() << " ms";
            counter = 0;
            time.restart();
        }

        QRect rect2 = m_ft->GetFacePosition(true);

        if(!images)
        {
            emit UpdatePosition(rect2);
            continue;
        }

        QRect rect = m_ft->GetFacePosition();
        QImage *image = m_ft->GetLastImage();
        QPainter p;
        p.begin(image);
        p.setPen(QPen(Qt::red));
        p.drawRect(rect);
        p.end();

        emit UpdatePosition(rect2);
        emit UpdateFace(new QImage(image->copy(rect)));
        emit UpdateFullImage(image);
    }
}

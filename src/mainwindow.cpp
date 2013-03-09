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

    pu = new PositionUpdater(ft);
    connect(pu, SIGNAL(UpdateFullImage(QImage*)), this, SLOT(UpdateImage(QImage*)));
    connect(pu, SIGNAL(UpdateFace(QImage*)), this, SLOT(UpdateFace(QImage*)));
    pu->start();
    pu->PauseThread();

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

void MainWindow::HandleTabChange(int index)
{
    if(index == 0)
        pu->ResumeThread();
    else
        pu->PauseThread();
}

PositionUpdater::PositionUpdater() { }
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
void PositionUpdater::run()
{
    while(1)
    {
        mutex.lock();
        if(stopped) condition.wait(&mutex);
        mutex.unlock();

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

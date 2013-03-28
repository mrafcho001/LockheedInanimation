#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "facetracker.h"
#include <QPainter>
#include <QTime>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), ft(new FaceTracker(0)), pu(new PositionUpdater(ft))
{
    ui->setupUi(this);

    ft->SetMinFeatureSize(10);
    ft->SetProcessingImageDimensions(320,240);
    ft->SetSearchScaleFactor(1.2f);

    connect(ui->graphicsView, SIGNAL(gameOver(int)), this, SLOT(DisableFacePositionUpdates()));
    connect(ui->graphicsView, SIGNAL(gamePaused()), this, SLOT(DisableFacePositionUpdates()));
    connect(ui->graphicsView, SIGNAL(gameStarted(bool)), this, SLOT(EnableFacePositionUpdates()));
    connect(ui->graphicsView, SIGNAL(ceaseImageUpdates()), this, SLOT(disableFaceImageUpdates()));
    connect(ui->graphicsView, SIGNAL(faceImageUpdatesRequest()), this, SLOT(enableFaceImageUpdates()));

    ui->graphicsView->initScreen();

    connect(pu, SIGNAL(UpdateFullImage(QImage*)), this, SLOT(UpdateImage(QImage*)));
    connect(pu, SIGNAL(UpdateFaceImage(QImage*)), this, SLOT(UpdateFace(QImage*)));
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
    if(image == NULL)
        return;

    ui->label->setPixmap(QPixmap::fromImage(*image));
    delete image;
}

void MainWindow::UpdateFace(QImage *image)
{
    if(image == NULL)
        return;

    ui->label_3->setPixmap(QPixmap::fromImage(*image));
    //delete image;
}

void MainWindow::UpdatePosition(QRect rect)
{
    if(rect.isValid())
        static_cast<FaceInvadersWidget*>(ui->graphicsView)->updatePlayerPosition(rect.center());
}

void MainWindow::DisableFacePositionUpdates()
{
    disconnect(pu, SIGNAL(UpdatePosition(QRect)), this, SLOT(UpdatePosition(QRect)));
}

void MainWindow::EnableFacePositionUpdates()
{
    connect(pu, SIGNAL(UpdatePosition(QRect)), this, SLOT(UpdatePosition(QRect)), Qt::UniqueConnection);
}

void MainWindow::HandleTabChange(int index)
{
    if(index == 1)
    {
        pu->EnableFullImageUpdates(false);
        pu->EnableFaceOnlyUpdates(false);
        pu->EnableFaceHighlighting(false);
        EnableFacePositionUpdates();
        static_cast<FaceInvadersWidget*>(ui->graphicsView)->initScreen();
    }
    else
    {
        pu->EnableFullImageUpdates();
        pu->EnableFaceHighlighting();
        pu->EnableFaceOnlyUpdates();
        DisableFacePositionUpdates();
    }
}

void MainWindow::enableFaceImageUpdates()
{
    qDebug() << "enableFaceImageUpdates()";
    pu->EnableFaceOnlyUpdates();
    FaceInvadersWidget *fiw = static_cast<FaceInvadersWidget*>(ui->graphicsView);
    connect(pu, SIGNAL(UpdateFaceImage(QImage*)), fiw, SLOT(updatePlayerImage(QImage*)));
}

void MainWindow::disableFaceImageUpdates()
{
    qDebug() << "disableFaceImageUpdates()";
    pu->EnableFaceOnlyUpdates(false);
    FaceInvadersWidget *fiw = static_cast<FaceInvadersWidget*>(ui->graphicsView);
    disconnect(pu, SIGNAL(UpdateFaceImage(QImage*)), fiw, SLOT(updatePlayerImage(QImage*)));
}

PositionUpdater::PositionUpdater() : m_updateMask(0x01) { }
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

void PositionUpdater::EnableFullImageUpdates(bool enable)
{
    if(enable)
        m_updateMask |= static_cast<quint8>(1)<<2;
    else
        m_updateMask &= ~(static_cast<quint8>(1)<<2);
}

void PositionUpdater::EnablePositionUpdates(bool enable)
{
    if(enable)
        m_updateMask |= static_cast<quint8>(1)<<0;
    else
        m_updateMask &= ~(static_cast<quint8>(1)<<0);
}

void PositionUpdater::EnableFaceOnlyUpdates(bool enable)
{
    if(enable)
        m_updateMask |= static_cast<quint8>(1)<<1;
    else
        m_updateMask &= ~(static_cast<quint8>(1)<<1);
}

void PositionUpdater::EnableFaceHighlighting(bool enable)
{
    if(enable)
        m_updateMask |= static_cast<quint8>(1)<<3;
    else
        m_updateMask &= ~(static_cast<quint8>(1)<<3);
}

void PositionUpdater::run()
{
#ifdef DEBUG_REPORT_FPS
    QTime time;
    time.start();
    int counter = 0;
#endif
    while(1)
    {
        mutex.lock();
        if(stopped) condition.wait(&mutex);
        mutex.unlock();

#ifdef DEBUG_REPORT_FPS
        counter++;
        if(counter == 30)
        {
            qDebug() << "FPS: " << 1000*30.0f/time.elapsed();
            counter = 0;
            time.restart();
        }
#endif

        QImage *fullImage = NULL;
        QImage *faceImage = NULL;
        QRect facePosition = m_ft->GetFacePosition(true);
        if((m_updateMask & static_cast<quint8>(1U<<1))
                || (m_updateMask & static_cast<quint8>(1U<<2)))
        {
            fullImage = m_ft->GetLastImage();
        }
        if(m_updateMask & static_cast<quint8>(1U<<2))
        {
            faceImage = new QImage(fullImage->copy(m_ft->GetLastPosition()));
        }
        if(m_updateMask & static_cast<quint8>(1U<<3))
        {
            QPainter p;
            p.begin(fullImage);
            p.setPen(QPen(Qt::red));
            p.drawRect(m_ft->GetLastPosition());
            p.end();
        }

        if(m_updateMask & static_cast<quint8>(0x01U))
            emit UpdatePosition(facePosition);
        if(m_updateMask & static_cast<quint8>(0x01U<<1))
            emit UpdateFaceImage(faceImage);
        if(m_updateMask & static_cast<quint8>(0x01U<<2))
            emit UpdateFullImage(fullImage);
    }
}

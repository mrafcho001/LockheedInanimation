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

    connect(ui->graphicsView, SIGNAL(ceaseImageUpdates()), this, SLOT(disableFaceImageUpdates()));
    connect(ui->graphicsView, SIGNAL(faceImageUpdatesRequest()), this, SLOT(enableFaceImageUpdates()));

    connect(pu, SIGNAL(UpdateFullImage(QImageSharedPointer)),
            this, SLOT(UpdateImage(QImageSharedPointer)));

    connect(pu, SIGNAL(UpdateFaceImage(QImageSharedPointer)),
            this, SLOT(UpdateFace(QImageSharedPointer)));

    connect(ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(HandleTabChange(int)));
    ui->tabWidget->setCurrentIndex(1);

    EnableFacePositionUpdates();

    QThread *thread = new QThread(this);
    connect(thread, SIGNAL(started()), pu, SLOT(run()), Qt::QueuedConnection);
    pu->moveToThread(thread);
    thread->start();
    ui->graphicsView->initScreen();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::UpdateImage(QImageSharedPointer image)
{
    if(image.data() == NULL)
        return;

    ui->label->setPixmap(QPixmap::fromImage(*image));
}

void MainWindow::UpdateFace(QImageSharedPointer image)
{
    if(image.data() == NULL)
        return;

    ui->label_3->setPixmap(QPixmap::fromImage(*image));
}

void MainWindow::UpdatePosition(QRect rect)
{
    if(rect.isValid())
        static_cast<FaceInvadersWidget*>(ui->graphicsView)->updatePlayerPosition(rect.center());
}

void MainWindow::DisableFacePositionUpdates()
{
    pu->EnablePositionUpdates(false);
    disconnect(pu, SIGNAL(UpdatePosition(QRect)), this, SLOT(UpdatePosition(QRect)));
}

void MainWindow::EnableFacePositionUpdates()
{
    pu->EnablePositionUpdates();
    connect(pu, SIGNAL(UpdatePosition(QRect)), this, SLOT(UpdatePosition(QRect)), Qt::UniqueConnection);
}

void MainWindow::HandleTabChange(int index)
{
    if(index == 1)
    {
        pu->EnableFullImageUpdates(false);
        pu->EnableFaceOnlyUpdates(false);
        pu->EnableFaceHighlighting(false);
        static_cast<FaceInvadersWidget*>(ui->graphicsView)->initScreen();
        EnableFacePositionUpdates();
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
    pu->EnableFaceOnlyUpdates();

    connect(pu, SIGNAL(UpdateFaceImage(QImageSharedPointer)),
            ui->graphicsView, SLOT(updatePlayerImage(QImageSharedPointer)), Qt::UniqueConnection);
}

void MainWindow::disableFaceImageUpdates()
{
    pu->EnableFaceOnlyUpdates(false);

    disconnect(pu, SIGNAL(UpdateFaceImage(QImageSharedPointer)),
            ui->graphicsView, SLOT(updatePlayerImage(QImageSharedPointer)));
}

PositionUpdater::PositionUpdater() : QObject(), m_updateMask(0x01) { }
PositionUpdater::PositionUpdater(FaceTracker *ft, QObject *parent):
    QObject(parent), m_ft(ft), stopped(false) { }

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
        if(m_updateMask & static_cast<quint8>(1U<<1))
        {
            faceImage = new QImage(fullImage->copy(m_ft->GetLastPosition()));
        }
        if((m_updateMask & static_cast<quint8>(1U<<3)) && fullImage != NULL)
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
        {
            QImageSharedPointer imagePtr(faceImage);
            emit UpdateFaceImage(imagePtr);
        }
        if(m_updateMask & static_cast<quint8>(0x01U<<2))
        {
            QImageSharedPointer imagePtr(fullImage);
            emit UpdateFullImage(imagePtr);
        }
    }
}

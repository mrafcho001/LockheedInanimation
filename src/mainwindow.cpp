#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "facetracker.h"
#include <QPainter>
#include <QTime>
#include <QDebug>
#include <QState>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), ft(new FaceTracker(0)), pu(new PositionUpdater(ft)),
    m_stateMachine(new QStateMachine(this)), m_hardwareManager(new HardwareManager(this))
{
    ui->setupUi(this);

    ft->SetMinFeatureSize(10);
    ft->SetProcessingImageDimensions(320,240);
    ft->SetSearchScaleFactor(1.2f);

    connect(ui->gvFaceInvaders, SIGNAL(ceaseImageUpdates()), this, SLOT(disableFaceImageUpdates()));
    connect(ui->gvFaceInvaders, SIGNAL(faceImageUpdatesRequest()), this, SLOT(enableFaceImageUpdates()));

    connect(pu, SIGNAL(UpdateFullImage(QImageSharedPointer)),
            this, SLOT(UpdateImage(QImageSharedPointer)));

    connect(pu, SIGNAL(UpdateFaceImage(QImageSharedPointer)),
            this, SLOT(UpdateFace(QImageSharedPointer)));

    QThread *thread = new QThread(this);
    connect(thread, SIGNAL(started()), pu, SLOT(run()), Qt::QueuedConnection);
    pu->moveToThread(thread);
    thread->start();

    this->addAction(ui->actionModeSwitch);
    connect(m_hardwareManager, SIGNAL(ModeSwitchTriggered()), this, SIGNAL(ModeSwitchTriggered()));
    connect(ui->actionModeSwitch, SIGNAL(triggered()), this, SIGNAL(ModeSwitchTriggered()));
    connect(this, SIGNAL(ModeSwitchTriggered()), this, SLOT(modeSwitched()));

    //Setup statemachine
    QState *automatic = new QState();
    QState *manual = new QState();
    QState *faceInvaders = new QState();

    automatic->addTransition(this, SIGNAL(ModeSwitchTriggered()), manual);
    connect(automatic, SIGNAL(exited()), this, SLOT(exitAutomaticMode()));
    connect(automatic, SIGNAL(entered()), this, SLOT(enterAutomaticMode()));

    manual->addTransition(this, SIGNAL(ModeSwitchTriggered()), faceInvaders);
    connect(manual, SIGNAL(exited()), this, SLOT(exitManualMode()));
    connect(manual, SIGNAL(entered()), this, SLOT(enterManualMode()));

    faceInvaders->addTransition(this, SIGNAL(ModeSwitchTriggered()), automatic);
    connect(faceInvaders, SIGNAL(exited()), this, SLOT(exitFaceInvadersMode()));
    connect(faceInvaders, SIGNAL(entered()), this, SLOT(enterFaceInvadersMode()));

    m_stateMachine->addState(automatic);
    m_stateMachine->addState(manual);
    m_stateMachine->addState(faceInvaders);
    m_stateMachine->setInitialState(automatic);
    m_stateMachine->start();
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

void MainWindow::enableFaceImageUpdates()
{
    pu->EnableFaceOnlyUpdates();

    connect(pu, SIGNAL(UpdateFaceImage(QImageSharedPointer)),
            ui->gvFaceInvaders, SLOT(updatePlayerImage(QImageSharedPointer)), Qt::UniqueConnection);
}

void MainWindow::disableFaceImageUpdates()
{
    pu->EnableFaceOnlyUpdates(false);

    disconnect(pu, SIGNAL(UpdateFaceImage(QImageSharedPointer)),
               ui->gvFaceInvaders, SLOT(updatePlayerImage(QImageSharedPointer)));
}

void MainWindow::enterAutomaticMode()
{
    pu->ResumeThread();
    pu->EnablePositionUpdates(true);
    pu->EnableFaceHighlighting(false);
    pu->EnableFaceOnlyUpdates(false);
    pu->EnableFullImageUpdates(false);

    connect(pu, SIGNAL(UpdatePosition(QRect)), m_hardwareManager, SLOT(UpdateFacePosition(QRect)));
    ui->tabWidget->setCurrentWidget(ui->tabAutomaticMode);
}

void MainWindow::exitAutomaticMode()
{
    pu->PauseThread();
    disconnect(pu, SIGNAL(UpdatePosition(QRect)), m_hardwareManager, SLOT(UpdateFacePosition(QRect)));
}

void MainWindow::enterManualMode()
{
    pu->PauseThread();
    ui->tabWidget->setCurrentWidget(ui->tabManualMode);
    m_hardwareManager->SetManualMode(true);
}

void MainWindow::exitManualMode()
{
    //Nothing to be done??
    m_hardwareManager->SetManualMode(false);
}

void MainWindow::enterFaceInvadersMode()
{
    pu->ResumeThread();
    pu->EnablePositionUpdates(true);
    connect(pu, SIGNAL(UpdatePosition(QRect)), ui->gvFaceInvaders, SLOT(updatePlayerPosition(QRect)), Qt::UniqueConnection);
    ui->tabWidget->setCurrentWidget(ui->tabFaceInvaders);
    ui->gvFaceInvaders->resetGame();
    ui->gvFaceInvaders->initScreen();
}

void MainWindow::exitFaceInvadersMode()
{
    disconnect(pu, SIGNAL(UpdatePosition(QRect)), ui->gvFaceInvaders, SLOT(updatePlayerPosition(QRect)));
    ui->gvFaceInvaders->endGame();
    ui->gvFaceInvaders->resetGame();
}

void MainWindow::modeSwitched()
{
    qDebug() << "Mode switch triggered...";
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

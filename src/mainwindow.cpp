#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "facetracker.h"
#include <QPainter>
#include <QTime>
#include <QState>
#if defined(DEBUG_MODE_SWITCHING)
#include <QDebug>
#endif

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), ft(new FaceTracker(0)), pu(new PositionUpdater(ft)),
    m_stateMachine(new QStateMachine(this)), m_hardwareManager(new HardwareManager(this)),
    m_ad(NULL), m_isFullScreen(false)
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

    m_puThread = new QThread(this);
    connect(m_puThread, SIGNAL(started()), pu, SLOT(run()), Qt::QueuedConnection);
    pu->moveToThread(m_puThread);
    m_puThread->start();

    this->addAction(ui->actionModeSwitch);
    connect(m_hardwareManager, SIGNAL(ModeSwitchTriggered()), this, SIGNAL(ModeSwitchTriggered()));
    connect(ui->actionModeSwitch, SIGNAL(triggered()), this, SIGNAL(ModeSwitchTriggered()));
#if defined(DEBUG_MODE_SWITCHING)
    connect(this, SIGNAL(ModeSwitchTriggered()), this, SLOT(modeSwitched()));
#endif

    connect(ui->actionFullScreen, SIGNAL(triggered()), this, SLOT(fullScreenToggle()));
    connect(ui->actionAboutDialog, SIGNAL(triggered()), this, SLOT(displayAbout()));

    //debugging signal/slots
    connect(m_hardwareManager, SIGNAL(PositionHUpdate(int)), ui->lblHPos, SLOT(setNum(int)));
    connect(m_hardwareManager, SIGNAL(PositionVUpdate(int)), ui->lblVpos, SLOT(setNum(int)));
    connect(m_hardwareManager, SIGNAL(RequestingHPosition(int)), ui->lblRequestedHPos, SLOT(setNum(int)));
    connect(m_hardwareManager, SIGNAL(RequestingVPosition(int)), ui->lblRequestedVPos, SLOT(setNum(int)));

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
    m_stateMachine->setInitialState(manual);
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
    pu->EnableFaceHighlighting(true);
    pu->EnableFaceOnlyUpdates(false);
    pu->EnableFullImageUpdates(true);

    connect(pu, SIGNAL(UpdatePosition(QRect)), m_hardwareManager, SLOT(UpdateFacePosition(QRect)));
    ui->tabWidget->setCurrentWidget(ui->tabImageTracking);
#ifdef DEBUG_MODE_SWITCHING
    qDebug() << "MainWindow::enterAutomaticMode(): done";
#endif
}

void MainWindow::exitAutomaticMode()
{
    pu->PauseThread();
    disconnect(pu, SIGNAL(UpdatePosition(QRect)), m_hardwareManager, SLOT(UpdateFacePosition(QRect)));
#ifdef DEBUG_MODE_SWITCHING
    qDebug() << "MainWindow::exitAutomaticMode(): done";
#endif
}

void MainWindow::enterManualMode()
{
    pu->PauseThread();
    ui->tabWidget->setCurrentWidget(ui->tabManualMode);
    m_hardwareManager->SetManualMode(true);
#ifdef DEBUG_MODE_SWITCHING
    qDebug() << "MainWindow::enterManualMode(): done";
#endif
}

void MainWindow::exitManualMode()
{
    //Nothing to be done??
    m_hardwareManager->SetManualMode(false);
#ifdef DEBUG_MODE_SWITCHING
    qDebug() << "MainWindow::exitManualMode(): done";
#endif
}

void MainWindow::enterFaceInvadersMode()
{
    pu->EnablePositionUpdates(true);
    pu->EnableFaceHighlighting(false);
    pu->EnableFaceOnlyUpdates(false);
    pu->EnableFullImageUpdates(false);
    pu->ResumeThread();
    connect(pu, SIGNAL(UpdatePosition(QRect)), ui->gvFaceInvaders, SLOT(updatePlayerPosition(QRect)), Qt::UniqueConnection);
    ui->tabWidget->setCurrentWidget(ui->tabFaceInvaders);
    ui->gvFaceInvaders->resetGame();
    ui->gvFaceInvaders->initScreen();

#ifdef DEBUG_MODE_SWITCHING
    qDebug() << "MainWindow::enterFaceInvadersMode(): done";
#endif
}

void MainWindow::exitFaceInvadersMode()
{
    disconnect(pu, SIGNAL(UpdatePosition(QRect)), ui->gvFaceInvaders, SLOT(updatePlayerPosition(QRect)));
    ui->gvFaceInvaders->endGame();
    ui->gvFaceInvaders->resetGame();

#ifdef DEBUG_MODE_SWITCHING
    qDebug() << "MainWindow::exitFaceInvadersMode(): done";
#endif
}

void MainWindow::modeSwitched()
{
    qDebug() << "Mode switch triggered...";
}

void MainWindow::displayAbout()
{
    if(m_ad == NULL)
        m_ad = new AboutDialog(this);

    if(m_ad->isVisible())
        m_ad->hide();
    else
        m_ad->show();
}

void MainWindow::fullScreenToggle()
{
    if(m_isFullScreen)
        this->showFullScreen();
    else
        this->showNormal();

    m_isFullScreen = !m_isFullScreen;
}

void MainWindow::closeEvent(QCloseEvent *)
{
    pu->Quit();
    m_puThread->quit();
    m_puThread->wait();
}


PositionUpdater::PositionUpdater() : QObject(), m_updateMask(0x01) { }
PositionUpdater::PositionUpdater(FaceTracker *ft, QObject *parent):
    QObject(parent), m_ft(ft), stopped(false), m_quitRequested(false) { }

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
#ifdef DEBUG_QTHREADS
    qDebug() << "PositionUpdater::ResumeThread(): done";
#endif
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

void PositionUpdater::Quit()
{
    m_quitRequested = true;
    if(stopped)
        condition.wakeAll();
}

void PositionUpdater::run()
{
#ifdef DEBUG_REPORT_FPS
    QTime time;
    time.start();
    int counter = 0;
#endif
    while(!m_quitRequested)
    {
        mutex.lock();
        if(stopped)
        {
            qDebug() << "Sleeping pu...";
            condition.wait(&mutex);
            if(m_quitRequested)
                return;
            qDebug() << "pu woke up...";
        }
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

        if(m_updateMask & static_cast<quint8>(0x01U) && facePosition.isValid())
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
#ifdef DEBUG_QTHREADS
    qDebug() << "PositionUpdater::run(): done";
#endif
}

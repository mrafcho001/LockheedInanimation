#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "facetracker.h"
#include "faceinvaderswidget.h"
#include "hardwaremanager.h"
#include <QThread>
#include <QWaitCondition>
#include <QMutex>
#include <QStateMachine>

namespace Ui {
class MainWindow;
}

class PositionUpdater;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();


public slots:
    void UpdateImage(QImageSharedPointer image);
    void UpdateFace(QImageSharedPointer image);

    void enableFaceImageUpdates();
    void disableFaceImageUpdates();

    void enterAutomaticMode();
    void exitAutomaticMode();
    void enterManualMode();
    void exitManualMode();
    void enterFaceInvadersMode();
    void exitFaceInvadersMode();

    void modeSwitched();

signals:
    void ModeSwitchTriggered();

protected:
    void closeEvent(QCloseEvent *);

private:
    Ui::MainWindow *ui;

    FaceTracker *ft;
    PositionUpdater *pu;
    QThread *m_puThread;
    QStateMachine *m_stateMachine;
    HardwareManager *m_hardwareManager;
};

class PositionUpdater : public QObject
{
    Q_OBJECT
public:
    PositionUpdater();
    PositionUpdater(FaceTracker *ft, QObject *parent = 0);
    void PauseThread();
    void ResumeThread();
    void EnableFullImageUpdates(bool enable = true);
    void EnablePositionUpdates(bool enable = true);
    void EnableFaceOnlyUpdates(bool enable = true);
    void EnableFaceHighlighting(bool enable = true);
signals:
    void UpdateFullImage(QImageSharedPointer image);
    void UpdateFaceImage(QImageSharedPointer image);
    void UpdatePosition(QRect rect);

public slots:
    void Quit();
    void run();

private:
    FaceTracker *m_ft;

    QMutex mutex;
    QWaitCondition condition;
    bool stopped;
    /*! \brief Indicates update types to emit
      bit 0 - position updates
      bit 1 - face image only updates
      bit 2 - full image update
      bit 3 - highlight face in full image
    */
    quint8 m_updateMask;
    bool m_quitRequested;
};

#endif // MAINWINDOW_H

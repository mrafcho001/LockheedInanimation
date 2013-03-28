#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "facetracker.h"
#include "faceinvaderswidget.h"
#include <QThread>
#include <QWaitCondition>
#include <QMutex>

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
    void UpdateImage(QImage *image);
    void UpdateFace(QImage *image);
    void UpdatePosition(QRect rect);
    void DisableFacePositionUpdates();
    void EnableFacePositionUpdates();

    void HandleTabChange(int index);

    void enableFaceImageUpdates();
    void disableFaceImageUpdates();

    
private:
    Ui::MainWindow *ui;

    FaceTracker *ft;
    PositionUpdater *pu;
};

class PositionUpdater : public QThread
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
    void UpdateFullImage(QImage *image);
    void UpdateFaceImage(QImage *image);
    void UpdatePosition(QRect rect);

private:
    void run();
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
};

#endif // MAINWINDOW_H

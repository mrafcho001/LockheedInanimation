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
    void EnableImageUpdateSignals(bool enable = true);
signals:
    void UpdateFullImage(QImage *image);
    void UpdateFace(QImage *image);
    void UpdatePosition(QRect rect);

private:
    void run();
    FaceTracker *m_ft;

    QMutex mutex;
    QWaitCondition condition;
    bool stopped;
    bool images;
};

#endif // MAINWINDOW_H

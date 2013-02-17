#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "facetracker.h"
#include <QThread>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();


public slots:
    void UpdateImage();
    void UpdatePos(QImage *image);

    
private:
    Ui::MainWindow *ui;

    FaceTracker *ft;
};

class PositionUpdater : public QThread
{
    Q_OBJECT
public:
    PositionUpdater();
    PositionUpdater(FaceTracker *ft, QObject *parent = 0);
signals:
    void Updated(QImage *image);

private:
    void run();
    FaceTracker *m_ft;
};

#endif // MAINWINDOW_H

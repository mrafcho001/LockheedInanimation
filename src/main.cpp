#include "mainwindow.h"
#include <QApplication>
#include <QTime>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qRegisterMetaType<QImageSharedPointer>("QImageSharedPointer");
    MainWindow w;
    w.show();
    
    return a.exec();
}

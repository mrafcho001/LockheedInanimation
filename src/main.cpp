#include "mainwindow.h"
#include <QApplication>
#include <QTime>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCoreApplication::setOrganizationName("Lockheed Martin");
    QCoreApplication::setOrganizationDomain("lockheedmartin.com");
    QCoreApplication::setApplicationName("Inanimation");

    qRegisterMetaType<QImageSharedPointer>("QImageSharedPointer");
    MainWindow w;
    w.show();

    return a.exec();
}

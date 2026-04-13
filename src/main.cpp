#include "mainwindow.h"
#include "simulatedbackend.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("qt_opcua_demo");
    QApplication::setOrganizationName("demo");

    SimulatedBackend backend;
    MainWindow window(&backend);
    window.show();

    return app.exec();
}

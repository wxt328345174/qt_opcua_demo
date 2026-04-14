#include "mainwindow.h"
#include "opcua_client.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    OpcUaClient client;
    MainWindow window(&client);
    window.show();

    return app.exec();
}

#include "mainwindow.h"
#include "opcua_client.h"

#include <QApplication>

// 程序入口：创建 Qt 应用、业务客户端和主窗口。
// 从这里可以看到“界面层依赖客户端层，客户端层负责 OPC UA 通信”的整体关系。
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    OpcUaClient client;
    MainWindow window(&client);
    window.show();

    return app.exec();
}

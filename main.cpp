#include "mainwindow.h"
#include "simulator.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    // QApplication 是 Qt 图形界面程序的核心对象，负责处理窗口、鼠标、键盘等事件。
    QApplication app(argc, argv);

    // Simulator 负责模拟 PLC/OPC UA 数据；MainWindow 只负责显示界面。
    Simulator simulator;
    MainWindow window(&simulator);
    window.show();

    // exec() 会进入 Qt 事件循环。程序运行期间，按钮点击和定时器都会在这里被分发处理。
    return app.exec();
}

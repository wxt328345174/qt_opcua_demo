# Qt + OPC UA + PLC 最小模拟示例

本项目是一个基于 Qt 5 Widgets 的最小示例，用于完成图形界面、变量预留和模拟数据联动。项目保留基本类拆分，只实现必要功能：

- 打开一个 Qt Widgets 窗口。
- 点击“启动”和“停止”按钮。
- 每秒模拟刷新温度、压力、心跳、生产计数。
- 在表格中预留未来要映射到 PLC / OPC UA 的变量。
- 温度或压力超过阈值时显示报警。

当前项目不连接真实 PLC，也不依赖 Qt OPC UA 模块。

## 一、目录内容

```text
qt_opcua_demo/
├── qt_opcua_demo.pro
├── main.cpp
├── mainwindow.h
├── mainwindow.cpp
├── simulator.h
├── simulator.cpp
└── README.md
```

- `qt_opcua_demo.pro`：qmake 项目文件，告诉 Qt Creator 编译哪些源码文件。
- `main.cpp`：程序入口，只负责创建应用、模拟器和主窗口。
- `MainWindow`：界面类，负责按钮、状态栏、变量表格和日志显示。
- `Simulator`：模拟器类，负责启动/停止、定时器刷新、变量值和报警状态。
- `README.md`：当前说明文档。

## 二、在 Windows 上用 Qt Creator 打开项目

1. 打开 Qt Creator。
2. 点击菜单：`File -> Open File or Project`。
3. 选择：

   ```text
   E:\qt_opcua_demo\qt_opcua_demo.pro
   ```

4. Qt Creator 会进入 Kit 选择界面，选择一个 Qt 5 的 Kit。
5. 点击 `Configure Project`。
6. 左侧切换到 `Edit`，依次查看 `main.cpp`、`mainwindow.cpp`、`simulator.cpp`。
7. 点击左下角绿色三角按钮运行。

如果 Windows 上运行时报：

```text
Cannot run compiler 'cl'
```

说明当前 Qt Kit 使用 MSVC 编译器，但这台 Windows 没有配置好 MSVC 命令行环境。
这种情况下，可以先在 Windows 上用 Qt Creator 看代码、改代码，然后把整个
`qt_opcua_demo` 文件夹复制到 openEuler 设备上编译运行。

## 三、在 openEuler 22.03 LTS-SP3 设备上编译运行

进入项目目录：

```bash
cd qt_opcua_demo
```

执行：

```bash
qmake-qt5 qt_opcua_demo.pro
make
./qt_opcua_demo
```

如果设备上的 Qt 5 命令叫 `qmake`，第一行可以改成：

```bash
qmake qt_opcua_demo.pro
```

## 四、代码结构

项目代码按职责拆分如下：

1. `main.cpp`：程序入口。
   - `QApplication app(argc, argv);` 创建 Qt 图形程序对象。
   - `Simulator simulator;` 创建模拟数据对象。
   - `MainWindow window(&simulator);` 创建窗口，并把模拟器传给窗口。
   - `return app.exec();` 进入 Qt 事件循环。
2. `simulator.h`：声明模拟器类对外提供的接口。
   - `variables()` 返回变量表。
   - `start()` / `stop()` 响应启动和停止。
   - `variableChanged(...)` 通知界面变量变了。
   - `logMessage(...)` 通知界面追加日志。
3. `simulator.cpp`：实现模拟数据变化。
   - 构造函数里预留变量。
   - `QTimer` 每 1 秒调用 `updateData()`。
   - 运行时刷新温度、压力、心跳、生产计数。
   - 温度或压力超过阈值时触发报警。
4. `mainwindow.h`：声明窗口类和控件成员。
5. `mainwindow.cpp`：实现界面创建和刷新。
   - `createStatusBox()` 创建顶部状态区。
   - `createControlBox()` 创建启动/停止按钮。
   - `createTableBox()` 创建变量表。
   - `createLogBox()` 创建日志框。
   - `connect(...)` 把按钮、模拟器信号和界面槽函数连接起来。

## 五、变量说明

表格中预留了这些变量：

- `cmdStart`：启动命令，写入型 bool。
- `cmdStop`：停止命令，写入型 bool。
- `runState`：运行状态，读取型 bool。
- `alarmActive`：报警状态，读取型 bool。
- `temperature`：温度，读取型 double，单位 `degC`。
- `pressure`：压力，读取型 double，单位 `MPa`。
- `productionCount`：生产计数，读取型 int。
- `heartbeat`：心跳，读取型 int。

`OPC UA NodeId` 列现在都是占位内容，例如：

```text
TODO:opcua-nodeid/temperature
```

获得真实 PLC / OPC UA 变量映射后，替换这些占位内容。

## 六、可配置项

- 把刷新周期从 1 秒改成 0.5 秒：在 `simulator.cpp` 里把 `m_timer.start(1000);` 改成 `m_timer.start(500);`。
- 调低温度报警阈值：在 `simulator.cpp` 里修改 `m_temperature >= 78.0`。
- 调低压力报警阈值：在 `simulator.cpp` 里修改 `m_pressure >= 0.42`。
- 增加一个新变量：在 `simulator.cpp` 的 `addVariable(...)` 位置照着已有变量再加一行。

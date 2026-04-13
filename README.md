# Qt + OPC UA + PLC 读写变量模拟示例

本项目是一个基于 Qt 5 Widgets 的最小示例，用于完成图形界面、变量预留、模拟读取数据刷新和模拟写入数据生效。当前阶段不连接真实 PLC，也不依赖 Qt OPC UA 模块。

## 一、功能说明

- 使用 Qt Widgets 创建桌面窗口。
- 提供“读取数据”和“写入数据”两组变量表。
- 读取数据表只读显示，定时刷新模拟 PLC 数据。
- 写入数据表允许编辑“当前值”列，并支持“写入选中项”和“写入全部”。
- 覆盖 `int`、`double`、`bool`、`array`、`struct` 五类变量。
- 保留 OPC UA NodeId 占位字段，便于后续替换为实际变量映射。
- 顶部状态区显示运行状态、报警状态、实际温度、设定温度、心跳。
- 日志区记录启动、停止、写入成功、写入失败和报警变化。

## 二、目录内容

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

- `qt_opcua_demo.pro`：qmake 项目文件。
- `main.cpp`：程序入口，创建 Qt 应用、模拟器和主窗口。
- `MainWindow`：界面类，负责状态显示、读写表格、按钮和日志。
- `Simulator`：模拟后端，负责读取数据刷新、写入校验和状态联动。

## 三、变量说明

读取数据：

| ID | 名称 | 类型 | 单位 | 说明 |
| --- | --- | --- | --- | --- |
| `actualTemperature` | 实际温度 | `double` | `degC` | 模拟读取的实际温度 |
| `actualCount` | 实际计数 | `int` | `pcs` | 模拟读取的生产计数 |
| `heartbeat` | 心跳 | `int` |  | 每秒递增 |
| `motorRunning` | 电机运行状态 | `bool` |  | 模拟读取的电机状态 |
| `alarmActive` | 报警状态 | `bool` |  | 温度超过阈值时置为 `true` |
| `zoneTemperatures` | 分区温度 | `array` | `degC` | JSON 数组，例如 `[62.1,63.0,61.8]` |
| `deviceStatus` | 设备状态 | `struct` |  | JSON 对象，例如 `{"mode":"AUTO","errorCode":0,"loadPercent":65.0}` |

写入数据：

| ID | 名称 | 类型 | 单位 | 说明 |
| --- | --- | --- | --- | --- |
| `setTemperature` | 设定温度 | `double` | `degC` | 写入目标温度 |
| `targetCount` | 目标计数 | `int` | `pcs` | 写入目标生产数量 |
| `enableMotor` | 电机使能 | `bool` |  | 写入 `true` 启动电机，`false` 停止电机 |
| `targetSpeeds` | 目标速度 | `array` | `rpm` | JSON 数组，例如 `[100,120,110]` |
| `recipeParams` | 配方参数 | `struct` |  | JSON 对象，例如 `{"recipeId":1,"temperature":60.0,"durationSec":30,"enabled":true}` |

OPC UA NodeId 当前使用占位格式：

```text
TODO:opcua-nodeid/read/actualTemperature
TODO:opcua-nodeid/write/setTemperature
```

接入真实 OPC UA 时，替换这些占位字段，并将 `Simulator` 的模拟读写逻辑替换为实际 OPC UA 读写逻辑。

## 四、写入格式

写入数据表的“当前值”列支持以下格式：

- `int`：十进制整数，例如 `100`
- `double`：小数或整数，例如 `60.5`
- `bool`：`true`、`false`、`1`、`0`
- `array`：JSON 数组，例如 `[100,120,110]`
- `struct`：JSON 对象，例如 `{"recipeId":1,"temperature":60.0,"durationSec":30,"enabled":true}`

如果输入格式错误，程序不会更新变量，并会在运行日志中显示错误信息。

## 五、Windows Qt Creator 打开方式

1. 打开 Qt Creator。
2. 点击菜单：`File -> Open File or Project`。
3. 选择：

   ```text
   E:\qt_opcua_demo\qt_opcua_demo.pro
   ```

4. 选择可用的 Qt 5 Kit。
5. 点击 `Configure Project`。
6. 点击左下角绿色三角按钮构建并运行。

如果 Windows 上运行时报：

```text
Cannot run compiler 'cl'
```

说明当前 Qt Kit 使用 MSVC 编译器，但 Windows 编译环境未配置完整。可以在 Windows 上查看和编辑项目，再将整个 `qt_opcua_demo` 文件夹复制到 openEuler 设备上编译运行。

## 六、openEuler 22.03 LTS-SP3 编译运行

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

## 七、验证点

- 程序启动后显示“读取数据”和“写入数据”两张表。
- 点击“启动”后，电机运行状态变为 `true`，读取数据开始变化。
- 修改 `setTemperature` 后点击“写入选中项”，顶部设定温度同步更新。
- 修改 `enableMotor` 为 `false` 后写入，电机运行状态变为停止。
- `array` 和 `struct` 输入非法 JSON 时，日志显示写入失败。
- 实际温度超过 `78 degC` 后，报警状态变为 `true`。

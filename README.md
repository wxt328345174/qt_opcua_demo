# Qt + OPC UA + PLC 读写变量模拟示例

本项目是一个基于 Qt 5 Widgets 的最小模拟示例，用于演示 PLC / OPC UA 变量的读取、写入和界面联动。

当前阶段不连接真实 PLC，也不依赖 Qt OPC UA 模块，只使用模拟数据。

## 功能

- 显示“读取数据”和“写入数据”两张变量表。
- 读取数据每秒自动刷新。
- 写入数据可以编辑“当前值”，支持写入选中项和写入全部。
- 支持 `int`、`double`、`bool`、`array`、`struct` 类型。
- 保留 OPC UA NodeId 占位字段，后续可替换为真实 NodeId。
- 日志区显示启动、停止、写入成功、写入失败和报警变化。

## 目录

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

## openEuler 22.03 LTS-SP3 编译运行

进入项目目录：

```bash
cd qt_opcua_demo
```

编译并运行：

```bash
qmake-qt5 qt_opcua_demo.pro
make
./qt_opcua_demo
```

如果系统中的 Qt 5 命令叫 `qmake`，则使用：

```bash
qmake qt_opcua_demo.pro
make
./qt_opcua_demo
```

## 变量

读取数据：

| ID | 类型 | 说明 |
| --- | --- | --- |
| `actualTemperature` | `double` | 实际温度 |
| `actualCount` | `int` | 实际计数 |
| `heartbeat` | `int` | 心跳 |
| `motorRunning` | `bool` | 电机运行状态 |
| `alarmActive` | `bool` | 报警状态 |
| `zoneTemperatures` | `array` | 分区温度 JSON 数组 |
| `deviceStatus` | `struct` | 设备状态 JSON 对象 |

写入数据：

| ID | 类型 | 说明 |
| --- | --- | --- |
| `setTemperature` | `double` | 设定温度 |
| `targetCount` | `int` | 目标计数 |
| `enableMotor` | `bool` | 电机使能 |
| `targetSpeeds` | `array` | 目标速度 JSON 数组 |
| `recipeParams` | `struct` | 配方参数 JSON 对象 |

## 写入格式

- `int`：例如 `100`
- `double`：例如 `60.5`
- `bool`：`true`、`false`、`1`、`0`
- `array`：例如 `[100,120,110]`
- `struct`：例如 `{"recipeId":1,"temperature":60.0,"durationSec":30,"enabled":true}`

## OPC UA NodeId

当前 NodeId 是占位内容：

```text
TODO:opcua-nodeid/read/actualTemperature
TODO:opcua-nodeid/write/setTemperature
```

接入真实 OPC UA 时，替换这些占位字段，并将 `Simulator` 中的模拟读写逻辑替换为实际 OPC UA 读写逻辑。

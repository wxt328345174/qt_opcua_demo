# Qt OPC UA PLC 最小示例

本项目是一个基于 Qt 5 Widgets 的最小示例，用于先完成图形界面、PLC
变量预留和模拟数据联动。当前阶段不连接真实 PLC，也不依赖 Qt OPC UA
模块。

## 当前内容

- Qt Widgets 主窗口：运行状态、控制按钮、变量表格、运行日志。
- `IDataBackend`：界面和数据后端之间的接口边界。
- `SimulatedBackend`：使用定时器模拟温度、压力、液位、生产计数、心跳、
  运行状态和报警状态。
- OPC UA NodeId 暂时使用占位字符串，例如
  `TODO:mentor-provide-nodeid/temperature`。

## openEuler 22.03 LTS-SP3 构建方式

先安装 Qt 5 Widgets 相关开发工具，然后使用 qmake 构建：

```bash
qmake qt_opcua_demo.pro
make
./qt_opcua_demo
```

当前阶段只需要 Qt 的 `core`、`gui`、`widgets` 模块。`.pro` 文件中没有加入
`QtOpcUa`，这样目标设备即使暂时没有 OPC UA 模块，也可以先编译并运行界面。

## 变量和后续映射说明

当前已预留以下通用工控变量：

- `cmdStart`：启动命令，写入型 bool。
- `cmdStop`：停止命令，写入型 bool。
- `runState`：运行状态，读取型 bool。
- `alarmActive`：报警状态，读取型 bool。
- `temperature`：温度，读取型 double，单位 `degC`。
- `pressure`：压力，读取型 double，单位 `MPa`。
- `level`：液位，读取型 double，单位 `%`。
- `productionCount`：生产计数，读取型 int。
- `heartbeat`：心跳，读取型 int。

后续 mentor 提供 PLC 变量和 OPC UA NodeId 后，不需要改主窗口界面。可以新增
一个实现 `IDataBackend` 的后端类，例如 `OpcUaBackend`，再把当前占位 NodeId
替换为实际 NodeId。

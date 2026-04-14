# Qt OPC UA 只读接入示例

这个工程使用 Qt Widgets + `open62541` 客户端连接 GCStudio 的 OPC UA Server，直接读取 4 个固定 NodeId：

- `group1_var1` -> `ns=1;s=globals.group1_var1`
- `group1_var2` -> `ns=1;s=globals.group1_var2`
- `group1_var3` -> `ns=1;s=globals.group1_var3`
- `group1_var4` -> `ns=1;s=globals.group1_var4`

目标 endpoint：

```text
opc.tcp://192.168.0.105:4840
```

## 当前行为

- 点击“连接”后，客户端匿名连接到服务器。
- 连接成功后直接配置 4 个固定 NodeId。
- 每 1000ms 轮询读取一次。
- 读取表显示实时值和固定 NodeId。
- 写入区保留，但不向真实设备发送数据。

## 构建

设备端依赖：

- Qt 5 Widgets
- `open62541`

构建命令：

```bash
qmake qt_opcua_demo.pro
make
./qt_opcua_demo
```

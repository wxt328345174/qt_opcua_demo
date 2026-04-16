# Qt OPC UA 客户端项目说明

本项目是一个运行在 openEuler 系统上的 Qt 5 Widgets + open62541 客户端示例，用于验证 OPC UA 固定点位的读取和写入能力。程序通过固定 endpoint 连接 GCStudio 的 OPC UA Server，读取 `group1` 变量并写入 `group2` 变量。

本项目只实现 OPC UA Client，不包含 OPC UA Server。运行前需要准备可访问的服务器和对应点位。

## 功能概览

- 连接指定 OPC UA Server。
- 显示连接状态、endpoint、点位配置数量和最近刷新时间。
- 每 500 ms 轮询读取 `group1` 和 `group2` 点位。
- 支持写入 `INT`、`REAL`、`BOOL`、整型数组和 `ST_Data` 结构体字段。
- 提供运行日志，便于观察连接、读取、写入和错误信息。

## 项目结构

```text
main.cpp                  程序入口，创建 QApplication、OpcUaClient 和 MainWindow
mainwindow.h/.cpp          界面层，负责表格、按钮、状态栏和日志显示
opcua_client.h/.cpp        OPC UA 客户端层，负责连接、断开、轮询、读写调度
opcua_nodes.h/.cpp         固定点位定义层，集中维护 endpoint、轮询周期和变量清单
opcua_value_codec.h/.cpp   类型转换层，处理输入校验、ST_Data 解析和 UA_Variant 转换
variable_row.h             表格行数据模型
docs/demo_guide.md         项目演示说明
```

## 运行环境

当前项目面向 openEuler，使用本地安装的 open62541 静态库：

```text
/usr/local/include/open62541/client.h
/usr/local/lib64/libopen62541.a
```

确认文件存在：

```bash
ls /usr/local/include/open62541/client.h
ls /usr/local/lib64/libopen62541.a
```

安装 Qt 和编译工具：

```bash
sudo dnf install gcc-c++ make qt5-qtbase-devel qt5-qmake
```

## 编译运行

```bash
qmake qt_opcua_demo.pro
make -j$(nproc)
./qt_opcua_demo
```

当前链接的是静态库 `libopen62541.a`，运行时不需要配置 `libopen62541.so` 的动态库搜索路径。

## 连接信息

固定连接地址：

```text
opc.tcp://192.168.0.105:4840
```

运行前请确认：

- openEuler 主机能访问 `192.168.0.105:4840`。
- GCStudio OPC UA Server 已启动。
- 服务器中存在 `globals.group1_*` 和 `globals.group2_*` 点位。
- 防火墙或网络策略没有阻断 4840 端口。

## 现场演示流程

1. 启动 GCStudio OPC UA Server。
2. 编译并启动本程序。
3. 点击“连接”，观察连接状态、NodeId 配置日志和首次读取结果。
4. 修改写入表的“设定值”，点击“写入选中项”或“写入全部”。
5. 观察“读取值”回读结果，确认设备端实际值。
6. 输入错误格式，例如数组写成 `10,20,30`，观察输入校验结果。
7. 如需定位实现，可按 `main.cpp -> MainWindow -> OpcUaClient -> opcua_nodes -> opcua_value_codec` 的顺序查看源码。

## 读取点位

`group1` 用于读取，程序每 500 ms 轮询一次。

| ID | 类型 | NodeId |
| --- | --- | --- |
| `group1_var1` | `INT` | `ns=1;s=globals.group1_var1` |
| `group1_var2` | `REAL` | `ns=1;s=globals.group1_var2` |
| `group1_var3` | `BOOL` | `ns=1;s=globals.group1_var3` |
| `group1_var4` | `INT ARRAY[0..4]` | `ns=1;s=globals.group1_var4` |
| `group1_var5` | `ST_Data` | `ns=1;s=globals.group1_var5` |

`ST_Data` 按字段读取后组合显示：

| 字段 | NodeId |
| --- | --- |
| `group1_var5.A` | `ns=1;s=globals.group1_var5.A` |
| `group1_var5.B` | `ns=1;s=globals.group1_var5.B` |
| `group1_var5.C` | `ns=1;s=globals.group1_var5.C` |

## 写入点位

`group2` 用于写入。界面显示默认设定值，连接成功后可以写入选中行或写入全部。

| ID | 类型 | 默认值 | NodeId |
| --- | --- | --- | --- |
| `group2_var1` | `INT` | `300` | `ns=1;s=globals.group2_var1` |
| `group2_var2` | `REAL` | `5.2` | `ns=1;s=globals.group2_var2` |
| `group2_var3` | `BOOL` | `false` | `ns=1;s=globals.group2_var3` |
| `group2_var4` | `INT ARRAY[0..2]` | `[10,20,30]` | `ns=1;s=globals.group2_var4` |
| `group2_var5` | `ST_Data` | `(A := 10, B := 10.80, C := TRUE)` | `ns=1;s=globals.group2_var5` |

`ST_Data` 按字段写入：

| 字段 | NodeId |
| --- | --- |
| `group2_var5.A` | `ns=1;s=globals.group2_var5.A` |
| `group2_var5.B` | `ns=1;s=globals.group2_var5.B` |
| `group2_var5.C` | `ns=1;s=globals.group2_var5.C` |

## 写入格式

布尔值支持：

```text
true
false
1
0
```

整型数组使用 JSON 数组：

```text
[10,20,30]
```

结构体使用 ST 格式：

```text
(A := 10, B := 10.8, C := TRUE)
```

## 常见问题

### qmake 找不到 open62541

确认静态库和头文件是否存在：

```bash
ls /usr/local/include/open62541/client.h
ls /usr/local/lib64/libopen62541.a
```

如果路径不同，需要同步修改 `qt_opcua_demo.pro` 中的 `INCLUDEPATH` 和 `LIBS`。

### 连接失败

优先检查：

- endpoint 地址是否正确。
- OPC UA Server 是否已启动。
- 客户端和服务器是否在同一网络或可路由网络中。
- 服务器点位命名是否与 README 中的 NodeId 一致。

### 写入失败

优先检查：

- 是否已经连接成功。
- 写入变量是否存在。
- 输入格式是否符合对应类型要求。
- Server 端是否允许客户端写入该节点。

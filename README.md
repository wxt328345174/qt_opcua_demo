# Qt OPC UA 读写接入示例

这个工程使用 Qt Widgets + `open62541` 客户端连接 GCStudio 的 OPC UA Server。

目标 endpoint：

```text
opc.tcp://192.168.0.105:4840
```

## 读取变量

读取组为 `group1`，每 500ms 轮询一次：

| 变量 | 类型 | NodeId |
| --- | --- | --- |
| `group1_var1` | `INT` | `ns=1;s=globals.group1_var1` |
| `group1_var2` | `REAL` | `ns=1;s=globals.group1_var2` |
| `group1_var3` | `BOOL` | `ns=1;s=globals.group1_var3` |
| `group1_var4` | `INT ARRAY[0..4]` | `ns=1;s=globals.group1_var4` |
| `group1_var5` | `ST_Data` | `ns=1;s=globals.group1_var5` |

## 写入变量

写入组为 `group2`：

| 变量 | 类型 | 初始值 | NodeId |
| --- | --- | --- | --- |
| `group2_var1` | `INT` | `300` | `ns=1;s=globals.group2_var1` |
| `group2_var2` | `REAL` | `5.2` | `ns=1;s=globals.group2_var2` |
| `group2_var3` | `BOOL` | `0` | `ns=1;s=globals.group2_var3` |
| `group2_var4` | `INT ARRAY[0..2]` | `[10,20,30]` | `ns=1;s=globals.group2_var4` |
| `group2_var5` | `ST_Data` | `(A := 10, B := 10.8, C := TRUE)` | `ns=1;s=globals.group2_var5` |

写入表包含“读取值”和“设定值”两列：

- “读取值”每 500ms 从设备回读，用于确认是否真的写入成功。
- “设定值”用于输入将要写入设备的值。

数组使用 JSON 格式，例如：

```text
[10,20,30]
```

结构体 `ST_Data` 使用 ST 结构体初始化格式，例如：

```text
(A := 10, B := 10.8, C := TRUE)
```

当前结构体底层按字段节点读写：

- `group1_var5.A/B/C` -> `ns=1;s=globals.group1_var5.A/B/C`
- `group2_var5.A/B/C` -> `ns=1;s=globals.group2_var5.A/B/C`

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

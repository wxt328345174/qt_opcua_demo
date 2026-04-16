# Qt OPC UA Demo

这是一个仅面向 openEuler 的 Qt 5 Widgets 示例程序，使用 `open62541` 连接 GCStudio 的 OPC UA Server，完成固定点位的读取和写入。

固定连接地址：

```text
opc.tcp://192.168.0.105:4840
```

## 依赖

安装 Qt 5、qmake、编译工具和 pkg-config：

```bash
sudo dnf install gcc-c++ make qt5-qtbase-devel qt5-qmake pkgconf-pkg-config
```

当前环境中 `open62541` 安装在 `/usr/local`：

```text
/usr/local/include/open62541/client.h
/usr/local/lib64/libopen62541.a
```

可以用下面的命令确认：

```bash
ls /usr/local/include/open62541/client.h
ls /usr/local/lib64/libopen62541.a
```

项目已经在 `qt_opcua_demo.pro` 中固定使用 `/usr/local/include` 和 `/usr/local/lib64/libopen62541.a`，不依赖 `pkg-config`。

## 编译运行

```bash
qmake qt_opcua_demo.pro
make -j$(nproc)
./qt_opcua_demo
```

当前链接的是静态库 `libopen62541.a`，运行时不需要再配置 `libopen62541.so` 的动态库搜索路径。

## 读取点位

`group1` 用于读取，程序每 500 ms 轮询一次。

| ID | 类型 | NodeId |
| --- | --- | --- |
| `group1_var1` | `INT` | `ns=1;s=globals.group1_var1` |
| `group1_var2` | `REAL` | `ns=1;s=globals.group1_var2` |
| `group1_var3` | `BOOL` | `ns=1;s=globals.group1_var3` |
| `group1_var4` | `INT ARRAY[0..4]` | `ns=1;s=globals.group1_var4` |
| `group1_var5` | `ST_Data` | `ns=1;s=globals.group1_var5` |

结构体 `ST_Data` 会按字段读取并组合显示：

| 字段 | NodeId |
| --- | --- |
| `group1_var5.A` | `ns=1;s=globals.group1_var5.A` |
| `group1_var5.B` | `ns=1;s=globals.group1_var5.B` |
| `group1_var5.C` | `ns=1;s=globals.group1_var5.C` |

## 写入点位

`group2` 用于写入。界面会先显示默认设定值，连接成功后可以写入选中行或写入全部。

| ID | 类型 | 默认值 | NodeId |
| --- | --- | --- | --- |
| `group2_var1` | `INT` | `300` | `ns=1;s=globals.group2_var1` |
| `group2_var2` | `REAL` | `5.2` | `ns=1;s=globals.group2_var2` |
| `group2_var3` | `BOOL` | `false` | `ns=1;s=globals.group2_var3` |
| `group2_var4` | `INT ARRAY[0..2]` | `[10,20,30]` | `ns=1;s=globals.group2_var4` |
| `group2_var5` | `ST_Data` | `(A := 10, B := 10.80, C := TRUE)` | `ns=1;s=globals.group2_var5` |

结构体 `ST_Data` 会按字段写入：

| 字段 | NodeId |
| --- | --- |
| `group2_var5.A` | `ns=1;s=globals.group2_var5.A` |
| `group2_var5.B` | `ns=1;s=globals.group2_var5.B` |
| `group2_var5.C` | `ns=1;s=globals.group2_var5.C` |

## 写入格式

布尔值支持 `true`、`false`、`1`、`0`。

整型数组使用 JSON 数组：

```text
[10,20,30]
```

结构体使用 ST 格式：

```text
(A := 10, B := 10.8, C := TRUE)
```

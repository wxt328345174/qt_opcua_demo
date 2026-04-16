#ifndef OPCUA_NODES_H
#define OPCUA_NODES_H

#include "variable_row.h"

#include <QString>
#include <QVariant>
#include <QVector>

#include <open62541/types.h>

namespace OpcUa {

// 类型标记：把 PLC/OPC UA 的值类型映射到程序可处理的几类。
enum ExpectedType {
    ExpectedInt,
    ExpectedReal,
    ExpectedBool,
    ExpectedIntArray,
    ExpectedStruct
};

struct StructField
{
    StructField();
    StructField(const QString &fieldName,
                const QString &fieldIdentifier,
                ExpectedType fieldType,
                const QVariant &initialValue);

    QString name;
    QString identifier;
    ExpectedType expectedType;
    QVariant value;
    UA_NodeId *nodeId;
};

// 一个 TargetNode 表示一个可读/可写点位及其运行时 NodeId 绑定。
// ST_Data 这类结构体会额外维护字段级节点。
struct TargetNode
{
    TargetNode();

    VariableRow row;
    QString identifier;
    ExpectedType expectedType;
    QVector<StructField> structFields;
    UA_NodeId *nodeId;
};

// 固定点位配置入口：集中管理 endpoint、轮询周期和 group1/group2 变量清单。
const char *endpointUrl();
int pollIntervalMs();
QString nodeIdText(const QString &identifier);
QVector<TargetNode> createReadTargets();
QVector<TargetNode> createWriteTargets();
QVector<VariableRow> createRows(const QVector<TargetNode> &targets);

} // namespace OpcUa

#endif // OPCUA_NODES_H

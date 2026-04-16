#include "opcua_nodes.h"

#include "opcua_value_codec.h"

// 固定点位定义层：把现场演示使用的 endpoint、轮询周期和变量清单集中放在这里。
// 如果需要扩展点位，优先从 createReadTargets/createWriteTargets 调整。
namespace {

const char kEndpointUrl[] = "opc.tcp://192.168.0.105:4840";
const int kPollIntervalMs = 500;

VariableRow makeRow(const QString &id,
                    const QString &name,
                    const QString &type,
                    const QVariant &value,
                    const QString &unit,
                    const QString &access,
                    const QString &nodeId,
                    const QString &note)
{
    VariableRow row;
    row.id = id;
    row.name = name;
    row.type = type;
    row.value = value;
    row.unit = unit;
    row.access = access;
    row.nodeId = nodeId;
    row.note = note;
    return row;
}

OpcUa::TargetNode makeNode(const QString &id,
                           const QString &identifier,
                           OpcUa::ExpectedType expectedType,
                           const QString &type,
                           const QVariant &initialValue,
                           const QString &access,
                           const QString &note)
{
    OpcUa::TargetNode target;
    target.identifier = identifier;
    target.expectedType = expectedType;
    target.row = makeRow(id, id, type, initialValue, QString(), access, OpcUa::nodeIdText(identifier), note);
    return target;
}

} // namespace

namespace OpcUa {

StructField::StructField()
    : expectedType(ExpectedInt)
    , nodeId(nullptr)
{
}

StructField::StructField(const QString &fieldName,
                         const QString &fieldIdentifier,
                         ExpectedType fieldType,
                         const QVariant &initialValue)
    : name(fieldName)
    , identifier(fieldIdentifier)
    , expectedType(fieldType)
    , value(initialValue)
    , nodeId(nullptr)
{
}

TargetNode::TargetNode()
    : expectedType(ExpectedInt)
    , nodeId(nullptr)
{
}

const char *endpointUrl()
{
    return kEndpointUrl;
}

int pollIntervalMs()
{
    return kPollIntervalMs;
}

QString nodeIdText(const QString &identifier)
{
    return QString::fromLatin1("ns=1;s=%1").arg(identifier);
}

QVector<TargetNode> createReadTargets()
{
    QVector<TargetNode> targets;

    targets.append(makeNode(QStringLiteral("group1_var1"),
                            QStringLiteral("globals.group1_var1"),
                            ExpectedInt,
                            QStringLiteral("INT"),
                            0,
                            QStringLiteral("R"),
                            QStringLiteral("固定 NodeId 直接读取")));
    targets.append(makeNode(QStringLiteral("group1_var2"),
                            QStringLiteral("globals.group1_var2"),
                            ExpectedReal,
                            QStringLiteral("REAL"),
                            0.0,
                            QStringLiteral("R"),
                            QStringLiteral("固定 NodeId 直接读取")));
    targets.append(makeNode(QStringLiteral("group1_var3"),
                            QStringLiteral("globals.group1_var3"),
                            ExpectedBool,
                            QStringLiteral("BOOL"),
                            false,
                            QStringLiteral("R"),
                            QStringLiteral("固定 NodeId 直接读取")));
    targets.append(makeNode(QStringLiteral("group1_var4"),
                            QStringLiteral("globals.group1_var4"),
                            ExpectedIntArray,
                            QStringLiteral("INT ARRAY[0..4]"),
                            QStringLiteral("[0,0,0,0,0]"),
                            QStringLiteral("R"),
                            QStringLiteral("固定 NodeId 直接读取")));

    TargetNode structTarget = makeNode(QStringLiteral("group1_var5"),
                                       QStringLiteral("globals.group1_var5"),
                                       ExpectedStruct,
                                       QStringLiteral("ST_Data"),
                                       compactStructValue(10, 10.8, true),
                                       QStringLiteral("R"),
                                       QStringLiteral("结构体字段 A/B/C 组合显示"));
    structTarget.structFields.append(StructField(QStringLiteral("A"), QStringLiteral("globals.group1_var5.A"), ExpectedInt, 10));
    structTarget.structFields.append(StructField(QStringLiteral("B"), QStringLiteral("globals.group1_var5.B"), ExpectedReal, 10.8));
    structTarget.structFields.append(StructField(QStringLiteral("C"), QStringLiteral("globals.group1_var5.C"), ExpectedBool, true));
    targets.append(structTarget);

    return targets;
}

// group2 是写入区。程序写入后也会回读这些点位，用于观察设备端实际值。
QVector<TargetNode> createWriteTargets()
{
    QVector<TargetNode> targets;

    targets.append(makeNode(QStringLiteral("group2_var1"),
                            QStringLiteral("globals.group2_var1"),
                            ExpectedInt,
                            QStringLiteral("INT"),
                            300,
                            QStringLiteral("W"),
                            QStringLiteral("固定 NodeId 写入变量")));
    targets.append(makeNode(QStringLiteral("group2_var2"),
                            QStringLiteral("globals.group2_var2"),
                            ExpectedReal,
                            QStringLiteral("REAL"),
                            5.2,
                            QStringLiteral("W"),
                            QStringLiteral("固定 NodeId 写入变量")));
    targets.append(makeNode(QStringLiteral("group2_var3"),
                            QStringLiteral("globals.group2_var3"),
                            ExpectedBool,
                            QStringLiteral("BOOL"),
                            false,
                            QStringLiteral("W"),
                            QStringLiteral("固定 NodeId 写入变量")));
    targets.append(makeNode(QStringLiteral("group2_var4"),
                            QStringLiteral("globals.group2_var4"),
                            ExpectedIntArray,
                            QStringLiteral("INT ARRAY[0..2]"),
                            QStringLiteral("[10,20,30]"),
                            QStringLiteral("W"),
                            QStringLiteral("固定 NodeId 写入数组")));

    TargetNode structTarget = makeNode(QStringLiteral("group2_var5"),
                                       QStringLiteral("globals.group2_var5"),
                                       ExpectedStruct,
                                       QStringLiteral("ST_Data"),
                                       compactStructValue(10, 10.8, true),
                                       QStringLiteral("W"),
                                       QStringLiteral("结构体字段 A/B/C 组合写入"));
    structTarget.structFields.append(StructField(QStringLiteral("A"), QStringLiteral("globals.group2_var5.A"), ExpectedInt, 10));
    structTarget.structFields.append(StructField(QStringLiteral("B"), QStringLiteral("globals.group2_var5.B"), ExpectedReal, 10.8));
    structTarget.structFields.append(StructField(QStringLiteral("C"), QStringLiteral("globals.group2_var5.C"), ExpectedBool, true));
    targets.append(structTarget);

    return targets;
}

QVector<VariableRow> createRows(const QVector<TargetNode> &targets)
{
    QVector<VariableRow> rows;
    rows.reserve(targets.size());
    for (int i = 0; i < targets.size(); ++i) {
        rows.append(targets.at(i).row);
    }
    return rows;
}

} // namespace OpcUa

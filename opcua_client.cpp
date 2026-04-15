#include "opcua_client.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QStringList>

#ifndef Q_OS_WIN
#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/types_generated.h>
#endif

namespace {

const char kEndpointUrl[] = "opc.tcp://192.168.0.105:4840";
const int kPollIntervalMs = 500;

QString nodeIdText(const QString &identifier)
{
    return QString::fromLatin1("ns=1;s=%1").arg(identifier);
}

QString compactStructValue(int a, double b, bool c)
{
    QJsonObject object;
    object.insert("A", a);
    object.insert("B", b);
    object.insert("C", c);
    return QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact));
}

bool jsonBoolValue(const QJsonValue &value, bool *ok)
{
    *ok = true;
    if (value.isBool()) {
        return value.toBool();
    }
    if (value.isDouble()) {
        return value.toInt() != 0;
    }
    if (value.isString()) {
        const QString lower = value.toString().trimmed().toLower();
        if (lower == "true" || lower == "1") {
            return true;
        }
        if (lower == "false" || lower == "0") {
            return false;
        }
    }

    *ok = false;
    return false;
}

}

OpcUaClient::OpcUaClient(QObject *parent)
    : QObject(parent)
{
    initializeTargets();

    m_pollTimer.setInterval(kPollIntervalMs);
    connect(&m_pollTimer, &QTimer::timeout, this, &OpcUaClient::pollValues);

    setConnectionState(QString::fromUtf8("未连接"));
    updateResolvedCount();
}

OpcUaClient::~OpcUaClient()
{
    disconnectFromServer();
}

QVector<VariableRow> OpcUaClient::readVariables() const
{
    return createRows(m_readTargets);
}

QVector<VariableRow> OpcUaClient::writeVariables() const
{
    return createRows(m_writeTargets);
}

void OpcUaClient::connectToServer()
{
    if (m_connectionState == QString::fromUtf8("连接中")
            || m_connectionState == QString::fromUtf8("已连接")) {
        return;
    }

#ifdef Q_OS_WIN
    setConnectionState(QString::fromUtf8("未连接"));
    emit logMessage(QString::fromUtf8("Windows 端未链接 open62541，请在设备端编译运行。"));
    return;
#else
    disconnectFromServer();

    setConnectionState(QString::fromUtf8("连接中"));
    emit logMessage(QString::fromUtf8("开始连接 OPC UA Server: %1").arg(QString::fromLatin1(kEndpointUrl)));

    m_client = UA_Client_new();
    if (!m_client) {
        setConnectionState(QString::fromUtf8("未连接"));
        emit logMessage(QString::fromUtf8("创建 OPC UA 客户端失败。"));
        return;
    }

    UA_ClientConfig_setDefault(UA_Client_getConfig(m_client));
    const UA_StatusCode status = UA_Client_connect(m_client, kEndpointUrl);
    if (status != UA_STATUSCODE_GOOD) {
        UA_Client_delete(m_client);
        m_client = nullptr;
        setConnectionState(QString::fromUtf8("未连接"));
        emit logMessage(QString::fromUtf8("连接失败: %1")
                        .arg(QString::fromLatin1(UA_StatusCode_name(status))));
        return;
    }

    if (!createNodeBindings()) {
        disconnectFromServer();
        emit logMessage(QString::fromUtf8("固定 NodeId 初始化失败。"));
        return;
    }

    setConnectionState(QString::fromUtf8("已连接"));
    emit logMessage(QString::fromUtf8("OPC UA 连接成功。"));
    emit logMessage(QString::fromUtf8("已配置 %1 个读取变量，%2 个写入变量。")
                    .arg(m_readTargets.size()).arg(m_writeTargets.size()));
    m_pollTimer.start();
    pollValues();
#endif
}

void OpcUaClient::disconnectFromServer()
{
    m_pollTimer.stop();

#ifndef Q_OS_WIN
    clearBindings();
    if (m_client) {
        UA_Client_disconnect(m_client);
        UA_Client_delete(m_client);
        m_client = nullptr;
    }
#endif

    setLastRefresh(QDateTime());
    setConnectionState(QString::fromUtf8("未连接"));
}

bool OpcUaClient::writeValue(const QString &id, const QString &textValue, QString *errorMessage)
{
    TargetNode *target = findWriteTarget(id);
    if (!target) {
        if (errorMessage) {
            *errorMessage = QString::fromUtf8("未知写入变量: %1").arg(id);
        }
        return false;
    }

    QVariant parsedValue;
    if (!parseTextValue(*target, textValue, &parsedValue, errorMessage)) {
        return false;
    }

#ifdef Q_OS_WIN
    if (errorMessage) {
        *errorMessage = QString::fromUtf8("Windows 端未链接 open62541，请在设备端写入。");
    }
    return false;
#else
    if (!m_client || !target->nodeId) {
        if (errorMessage) {
            *errorMessage = QString::fromUtf8("OPC UA 尚未连接，不能写入 %1。").arg(id);
        }
        return false;
    }

    if (!writeNodeValue(target, parsedValue, errorMessage)) {
        return false;
    }

    emit logMessage(QString::fromUtf8("写入成功: %1 = %2").arg(target->row.id, parsedValue.toString()));
    return true;
#endif
}

void OpcUaClient::pollValues()
{
#ifdef Q_OS_WIN
    return;
#else
    if (!m_client) {
        return;
    }

    int successCount = 0;
    for (int i = 0; i < m_readTargets.size(); ++i) {
        if (readNodeValue(&m_readTargets[i])) {
            ++successCount;
        }
    }
    for (int i = 0; i < m_writeTargets.size(); ++i) {
        if (readNodeValue(&m_writeTargets[i])) {
            ++successCount;
        }
    }

    if (successCount > 0) {
        setLastRefresh(QDateTime::currentDateTime());
    }
#endif
}

void OpcUaClient::initializeTargets()
{
    m_readTargets.clear();
    m_writeTargets.clear();

    TargetNode readVar1;
    readVar1.identifier = "globals.group1_var1";
    readVar1.expectedType = ExpectedInt;
    readVar1.row = {"group1_var1", "group1_var1", "INT", 0, "", "R",
                    nodeIdText(readVar1.identifier), QString::fromUtf8("固定 NodeId 直连读取")};
    m_readTargets.append(readVar1);

    TargetNode readVar2;
    readVar2.identifier = "globals.group1_var2";
    readVar2.expectedType = ExpectedReal;
    readVar2.row = {"group1_var2", "group1_var2", "REAL", 0.0, "", "R",
                    nodeIdText(readVar2.identifier), QString::fromUtf8("固定 NodeId 直连读取")};
    m_readTargets.append(readVar2);

    TargetNode readVar3;
    readVar3.identifier = "globals.group1_var3";
    readVar3.expectedType = ExpectedBool;
    readVar3.row = {"group1_var3", "group1_var3", "BOOL", false, "", "R",
                    nodeIdText(readVar3.identifier), QString::fromUtf8("固定 NodeId 直连读取")};
    m_readTargets.append(readVar3);

    TargetNode readVar4;
    readVar4.identifier = "globals.group1_var4";
    readVar4.expectedType = ExpectedIntArray;
    readVar4.row = {"group1_var4", "group1_var4", "INT ARRAY[0..4]", QString::fromLatin1("[0,0,0,0,0]"), "", "R",
                    nodeIdText(readVar4.identifier), QString::fromUtf8("固定 NodeId 直连读取")};
    m_readTargets.append(readVar4);

    TargetNode readVar5;
    readVar5.identifier = "globals.group1_var5";
    readVar5.expectedType = ExpectedStruct;
    readVar5.structFields.append(StructField("A", "globals.group1_var5.A", ExpectedInt, 10));
    readVar5.structFields.append(StructField("B", "globals.group1_var5.B", ExpectedReal, 10.8));
    readVar5.structFields.append(StructField("C", "globals.group1_var5.C", ExpectedBool, true));
    readVar5.row = {"group1_var5", "group1_var5", "ST_Data", compactStructValue(10, 10.8, true), "", "R",
                    nodeIdText(readVar5.identifier), QString::fromUtf8("结构体字段 A/B/C 组合显示")};
    m_readTargets.append(readVar5);

    TargetNode writeVar1;
    writeVar1.identifier = "globals.group_2_var1";
    writeVar1.expectedType = ExpectedInt;
    writeVar1.row = {"group_2_var1", "group_2_var1", "INT", 300, "", "W",
                     nodeIdText(writeVar1.identifier), QString::fromUtf8("固定 NodeId 写入变量")};
    m_writeTargets.append(writeVar1);

    TargetNode writeVar2;
    writeVar2.identifier = "globals.group_2_var2";
    writeVar2.expectedType = ExpectedReal;
    writeVar2.row = {"group_2_var2", "group_2_var2", "REAL", 5.2, "", "W",
                     nodeIdText(writeVar2.identifier), QString::fromUtf8("固定 NodeId 写入变量")};
    m_writeTargets.append(writeVar2);

    TargetNode writeVar3;
    writeVar3.identifier = "globals.group_2_var3";
    writeVar3.expectedType = ExpectedBool;
    writeVar3.row = {"group_2_var3", "group_2_var3", "BOOL", false, "", "W",
                     nodeIdText(writeVar3.identifier), QString::fromUtf8("固定 NodeId 写入变量")};
    m_writeTargets.append(writeVar3);

    TargetNode writeVar4;
    writeVar4.identifier = "globals.group_2_var4";
    writeVar4.expectedType = ExpectedIntArray;
    writeVar4.row = {"group_2_var4", "group_2_var4", "INT ARRAY[0..2]", QString::fromLatin1("[10,20,30]"), "", "W",
                     nodeIdText(writeVar4.identifier), QString::fromUtf8("固定 NodeId 写入数组")};
    m_writeTargets.append(writeVar4);

    TargetNode writeVar5;
    writeVar5.identifier = "globals.group2_var5";
    writeVar5.expectedType = ExpectedStruct;
    writeVar5.structFields.append(StructField("A", "globals.group2_var5.A", ExpectedInt, 10));
    writeVar5.structFields.append(StructField("B", "globals.group2_var5.B", ExpectedReal, 10.8));
    writeVar5.structFields.append(StructField("C", "globals.group2_var5.C", ExpectedBool, true));
    writeVar5.row = {"group_2_var5", "group_2_var5", "ST_Data", compactStructValue(10, 10.8, true), "", "W",
                     nodeIdText(writeVar5.identifier), QString::fromUtf8("结构体字段 A/B/C 组合写入")};
    m_writeTargets.append(writeVar5);
}

QVector<VariableRow> OpcUaClient::createRows(const QVector<TargetNode> &targets) const
{
    QVector<VariableRow> rows;
    rows.reserve(targets.size());
    for (int i = 0; i < targets.size(); ++i) {
        rows.append(targets.at(i).row);
    }
    return rows;
}

void OpcUaClient::setConnectionState(const QString &stateText)
{
    if (m_connectionState == stateText) {
        return;
    }

    m_connectionState = stateText;
    emit connectionStateChanged(stateText);
}

void OpcUaClient::updateResolvedCount()
{
    int resolvedCount = 0;
    for (int i = 0; i < m_readTargets.size(); ++i) {
#ifndef Q_OS_WIN
        if (m_readTargets.at(i).nodeId) {
            ++resolvedCount;
        }
#else
        Q_UNUSED(i);
#endif
    }
    for (int i = 0; i < m_writeTargets.size(); ++i) {
#ifndef Q_OS_WIN
        if (m_writeTargets.at(i).nodeId) {
            ++resolvedCount;
        }
#else
        Q_UNUSED(i);
#endif
    }
    emit resolvedCountChanged(resolvedCount, m_readTargets.size() + m_writeTargets.size());
}

void OpcUaClient::setLastRefresh(const QDateTime &timestamp)
{
    m_lastRefresh = timestamp;
    emit refreshTimestampChanged(timestamp);
}

void OpcUaClient::clearBindings()
{
#ifndef Q_OS_WIN
    for (int i = 0; i < m_readTargets.size(); ++i) {
        clearNodeId(&m_readTargets[i].nodeId);
        for (int fieldIndex = 0; fieldIndex < m_readTargets[i].structFields.size(); ++fieldIndex) {
            clearNodeId(&m_readTargets[i].structFields[fieldIndex].nodeId);
        }
        emit nodeIdResolved(m_readTargets[i].row.id, m_readTargets[i].row.nodeId);
    }
    for (int i = 0; i < m_writeTargets.size(); ++i) {
        clearNodeId(&m_writeTargets[i].nodeId);
        for (int fieldIndex = 0; fieldIndex < m_writeTargets[i].structFields.size(); ++fieldIndex) {
            clearNodeId(&m_writeTargets[i].structFields[fieldIndex].nodeId);
        }
        emit nodeIdResolved(m_writeTargets[i].row.id, m_writeTargets[i].row.nodeId);
    }
#endif
    updateResolvedCount();
}

OpcUaClient::TargetNode *OpcUaClient::findWriteTarget(const QString &id)
{
    for (int i = 0; i < m_writeTargets.size(); ++i) {
        if (m_writeTargets[i].row.id == id) {
            return &m_writeTargets[i];
        }
    }
    return nullptr;
}

bool OpcUaClient::parseTextValue(const TargetNode &target, const QString &textValue, QVariant *parsedValue, QString *errorMessage) const
{
    const QString text = textValue.trimmed();
    bool ok = false;

    if (target.expectedType == ExpectedInt) {
        const int value = text.toInt(&ok);
        if (ok) {
            *parsedValue = value;
            return true;
        }
        if (errorMessage) {
            *errorMessage = QString::fromUtf8("%1 需要输入十进制整数。").arg(target.row.name);
        }
        return false;
    }

    if (target.expectedType == ExpectedReal) {
        const double value = text.toDouble(&ok);
        if (ok) {
            *parsedValue = value;
            return true;
        }
        if (errorMessage) {
            *errorMessage = QString::fromUtf8("%1 需要输入小数或整数。").arg(target.row.name);
        }
        return false;
    }

    if (target.expectedType == ExpectedBool) {
        const QString lower = text.toLower();
        if (lower == "true" || lower == "1") {
            *parsedValue = true;
            return true;
        }
        if (lower == "false" || lower == "0") {
            *parsedValue = false;
            return true;
        }
        if (errorMessage) {
            *errorMessage = QString::fromUtf8("%1 需要输入 true/false 或 1/0。").arg(target.row.name);
        }
        return false;
    }

    if (target.expectedType == ExpectedStruct) {
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            if (errorMessage) {
                *errorMessage = QString::fromUtf8("%1 需要输入 JSON 对象，例如 {\"A\":10,\"B\":10.8,\"C\":true}。").arg(target.row.name);
            }
            return false;
        }

        const QJsonObject object = doc.object();
        if (!object.contains("A") || !object.contains("B") || !object.contains("C")) {
            if (errorMessage) {
                *errorMessage = QString::fromUtf8("%1 必须包含 A、B、C 三个字段。").arg(target.row.name);
            }
            return false;
        }
        if (!object.value("A").isDouble() || !object.value("B").isDouble()) {
            if (errorMessage) {
                *errorMessage = QString::fromUtf8("%1 字段类型应为 A:int, B:real, C:bool。").arg(target.row.name);
            }
            return false;
        }

        const double aNumber = object.value("A").toDouble();
        const int aValue = static_cast<int>(aNumber);
        if (aNumber != aValue) {
            if (errorMessage) {
                *errorMessage = QString::fromUtf8("%1 字段 A 必须是整数。").arg(target.row.name);
            }
            return false;
        }

        bool boolOk = false;
        const bool cValue = jsonBoolValue(object.value("C"), &boolOk);
        if (!boolOk) {
            if (errorMessage) {
                *errorMessage = QString::fromUtf8("%1 字段 C 必须是布尔值。").arg(target.row.name);
            }
            return false;
        }

        QJsonObject normalized;
        normalized.insert("A", aValue);
        normalized.insert("B", object.value("B").toDouble());
        normalized.insert("C", cValue);
        *parsedValue = QString::fromUtf8(QJsonDocument(normalized).toJson(QJsonDocument::Compact));
        return true;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
        if (errorMessage) {
            *errorMessage = QString::fromUtf8("%1 需要输入 JSON 整型数组，例如 [10,20,30]。").arg(target.row.name);
        }
        return false;
    }

    const QJsonArray array = doc.array();
    QStringList parts;
    parts.reserve(array.size());
    for (int i = 0; i < array.size(); ++i) {
        if (!array.at(i).isDouble()) {
            if (errorMessage) {
                *errorMessage = QString::fromUtf8("%1 数组元素必须是整数。").arg(target.row.name);
            }
            return false;
        }
        const double number = array.at(i).toDouble();
        const int intValue = static_cast<int>(number);
        if (number != intValue) {
            if (errorMessage) {
                *errorMessage = QString::fromUtf8("%1 数组元素必须是整数。").arg(target.row.name);
            }
            return false;
        }
        parts << QString::number(intValue);
    }

    *parsedValue = QString::fromLatin1("[%1]").arg(parts.join(","));
    return true;
}

#ifndef Q_OS_WIN
bool OpcUaClient::createNodeBindings()
{
    clearBindings();

    QVector<TargetNode> *groups[] = {&m_readTargets, &m_writeTargets};
    for (int groupIndex = 0; groupIndex < 2; ++groupIndex) {
        QVector<TargetNode> *targets = groups[groupIndex];
        for (int i = 0; i < targets->size(); ++i) {
            TargetNode *target = &(*targets)[i];
            target->nodeId = new UA_NodeId;
            UA_NodeId_init(target->nodeId);

            const QByteArray identifierUtf8 = target->identifier.toUtf8();
            *target->nodeId = UA_NODEID_STRING_ALLOC(1, const_cast<char *>(identifierUtf8.constData()));
            emit nodeIdResolved(target->row.id, target->row.nodeId);
            emit logMessage(QString::fromUtf8("已配置 %1 -> %2").arg(target->row.id, target->row.nodeId));

            for (int fieldIndex = 0; fieldIndex < target->structFields.size(); ++fieldIndex) {
                StructField *field = &target->structFields[fieldIndex];
                field->nodeId = new UA_NodeId;
                UA_NodeId_init(field->nodeId);
                const QByteArray fieldIdentifierUtf8 = field->identifier.toUtf8();
                *field->nodeId = UA_NODEID_STRING_ALLOC(1, const_cast<char *>(fieldIdentifierUtf8.constData()));
                emit logMessage(QString::fromUtf8("已配置 %1.%2 -> %3")
                                .arg(target->row.id, field->name, nodeIdText(field->identifier)));
            }
        }
    }

    updateResolvedCount();
    return true;
}

bool OpcUaClient::readNodeValue(TargetNode *target)
{
    if (!target || !target->nodeId || !m_client) {
        return false;
    }

    if (target->expectedType == ExpectedStruct) {
        return readStructValue(target);
    }

    UA_Variant value;
    UA_Variant_init(&value);
    const UA_StatusCode status = UA_Client_readValueAttribute(m_client, *target->nodeId, &value);
    if (status != UA_STATUSCODE_GOOD) {
        emit logMessage(QString::fromUtf8("读取 %1 失败: %2")
                        .arg(target->row.id, QString::fromLatin1(UA_StatusCode_name(status))));
        UA_Variant_clear(&value);
        return false;
    }

    QString errorMessage;
    bool ok = false;
    const QVariant convertedValue = convertVariant(*target, value, &ok, &errorMessage);
    UA_Variant_clear(&value);

    if (!ok) {
        emit logMessage(QString::fromUtf8("读取 %1 失败: %2").arg(target->row.id, errorMessage));
        return false;
    }

    if (target->row.value != convertedValue) {
        target->row.value = convertedValue;
        emit variableChanged(target->row.id, convertedValue);
    }
    return true;
}

bool OpcUaClient::readStructValue(TargetNode *target)
{
    QJsonObject object;

    for (int i = 0; i < target->structFields.size(); ++i) {
        StructField *field = &target->structFields[i];
        QString errorMessage;
        if (!readStructField(field, &errorMessage)) {
            emit logMessage(QString::fromUtf8("读取 %1.%2 失败: %3")
                            .arg(target->row.id, field->name, errorMessage));
            return false;
        }

        if (field->expectedType == ExpectedBool) {
            object.insert(field->name, field->value.toBool());
        } else if (field->expectedType == ExpectedReal) {
            object.insert(field->name, field->value.toDouble());
        } else {
            object.insert(field->name, field->value.toInt());
        }
    }

    const QVariant convertedValue = QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact));
    if (target->row.value != convertedValue) {
        target->row.value = convertedValue;
        emit variableChanged(target->row.id, convertedValue);
    }
    return true;
}

bool OpcUaClient::readStructField(StructField *field, QString *errorMessage)
{
    if (!field || !field->nodeId || !m_client) {
        if (errorMessage) {
            *errorMessage = QString::fromUtf8("字段 NodeId 未初始化。");
        }
        return false;
    }

    UA_Variant value;
    UA_Variant_init(&value);
    const UA_StatusCode status = UA_Client_readValueAttribute(m_client, *field->nodeId, &value);
    if (status != UA_STATUSCODE_GOOD) {
        if (errorMessage) {
            *errorMessage = QString::fromLatin1(UA_StatusCode_name(status));
        }
        UA_Variant_clear(&value);
        return false;
    }

    bool ok = false;
    const QVariant convertedValue = convertScalarVariant(field->expectedType, field->name, value, &ok, errorMessage);
    UA_Variant_clear(&value);

    if (!ok) {
        return false;
    }

    field->value = convertedValue;
    return true;
}

bool OpcUaClient::writeNodeValue(TargetNode *target, const QVariant &value, QString *errorMessage)
{
    if (target->expectedType == ExpectedStruct) {
        return writeStructValue(target, value, errorMessage);
    }

    return writeScalarValue(*target->nodeId, target->expectedType, value, target->row.name, errorMessage);
}

bool OpcUaClient::writeStructValue(TargetNode *target, const QVariant &value, QString *errorMessage)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(value.toString().toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errorMessage) {
            *errorMessage = QString::fromUtf8("%1 结构体值格式错误。").arg(target->row.name);
        }
        return false;
    }

    const QJsonObject object = doc.object();
    for (int i = 0; i < target->structFields.size(); ++i) {
        StructField *field = &target->structFields[i];
        if (!object.contains(field->name)) {
            if (errorMessage) {
                *errorMessage = QString::fromUtf8("%1 缺少字段 %2。").arg(target->row.name, field->name);
            }
            return false;
        }

        QVariant fieldValue;
        if (field->expectedType == ExpectedBool) {
            bool ok = false;
            fieldValue = jsonBoolValue(object.value(field->name), &ok);
            if (!ok) {
                if (errorMessage) {
                    *errorMessage = QString::fromUtf8("%1.%2 需要布尔值。").arg(target->row.name, field->name);
                }
                return false;
            }
        } else if (field->expectedType == ExpectedReal) {
            fieldValue = object.value(field->name).toDouble();
        } else {
            fieldValue = object.value(field->name).toInt();
        }

        if (!field->nodeId || !writeScalarValue(*field->nodeId, field->expectedType, fieldValue, target->row.name + "." + field->name, errorMessage)) {
            return false;
        }
    }

    return true;
}

bool OpcUaClient::writeScalarValue(const UA_NodeId &nodeId, ExpectedType expectedType, const QVariant &value, const QString &name, QString *errorMessage)
{
    UA_Variant variant;
    UA_Variant_init(&variant);

    UA_Int16 intValue = 0;
    UA_Float realValue = 0.0f;
    UA_Boolean boolValue = false;
    QVector<UA_Int16> arrayValues;

    if (expectedType == ExpectedInt) {
        intValue = static_cast<UA_Int16>(value.toInt());
        UA_Variant_setScalar(&variant, &intValue, &UA_TYPES[UA_TYPES_INT16]);
    } else if (expectedType == ExpectedReal) {
        realValue = static_cast<UA_Float>(value.toDouble());
        UA_Variant_setScalar(&variant, &realValue, &UA_TYPES[UA_TYPES_FLOAT]);
    } else if (expectedType == ExpectedBool) {
        boolValue = value.toBool() ? true : false;
        UA_Variant_setScalar(&variant, &boolValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    } else {
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(value.toString().toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
            if (errorMessage) {
                *errorMessage = QString::fromUtf8("%1 数组值格式错误。").arg(name);
            }
            return false;
        }

        const QJsonArray array = doc.array();
        arrayValues.reserve(array.size());
        for (int i = 0; i < array.size(); ++i) {
            arrayValues.append(static_cast<UA_Int16>(array.at(i).toInt()));
        }
        UA_Variant_setArray(&variant, arrayValues.data(), static_cast<size_t>(arrayValues.size()), &UA_TYPES[UA_TYPES_INT16]);
    }

    const UA_StatusCode status = UA_Client_writeValueAttribute(m_client, nodeId, &variant);
    if (status != UA_STATUSCODE_GOOD) {
        if (errorMessage) {
            *errorMessage = QString::fromUtf8("写入 %1 失败: %2")
                    .arg(name, QString::fromLatin1(UA_StatusCode_name(status)));
        }
        return false;
    }

    return true;
}

QVariant OpcUaClient::convertVariant(const TargetNode &target, const UA_Variant &value, bool *ok, QString *errorMessage) const
{
    if (target.expectedType == ExpectedIntArray) {
        *ok = false;

        if (UA_Variant_isEmpty(&value)) {
            *errorMessage = QString::fromUtf8("节点值为空。");
            return QVariant();
        }
        if (UA_Variant_isScalar(&value)) {
            *errorMessage = QString::fromUtf8("期望 INT 数组，实际返回标量。");
            return QVariant();
        }

        QStringList parts;
        parts.reserve(static_cast<int>(value.arrayLength));

        if (value.type == &UA_TYPES[UA_TYPES_INT16]) {
            const UA_Int16 *data = static_cast<const UA_Int16 *>(value.data);
            for (size_t i = 0; i < value.arrayLength; ++i) {
                parts << QString::number(data[i]);
            }
        } else if (value.type == &UA_TYPES[UA_TYPES_UINT16]) {
            const UA_UInt16 *data = static_cast<const UA_UInt16 *>(value.data);
            for (size_t i = 0; i < value.arrayLength; ++i) {
                parts << QString::number(data[i]);
            }
        } else if (value.type == &UA_TYPES[UA_TYPES_INT32]) {
            const UA_Int32 *data = static_cast<const UA_Int32 *>(value.data);
            for (size_t i = 0; i < value.arrayLength; ++i) {
                parts << QString::number(data[i]);
            }
        } else if (value.type == &UA_TYPES[UA_TYPES_UINT32]) {
            const UA_UInt32 *data = static_cast<const UA_UInt32 *>(value.data);
            for (size_t i = 0; i < value.arrayLength; ++i) {
                parts << QString::number(data[i]);
            }
        } else {
            *errorMessage = QString::fromUtf8("期望 INT 数组，实际类型不匹配。");
            return QVariant();
        }

        *ok = true;
        return QVariant(QString::fromLatin1("[%1]").arg(parts.join(",")));
    }

    return convertScalarVariant(target.expectedType, target.row.name, value, ok, errorMessage);
}

QVariant OpcUaClient::convertScalarVariant(ExpectedType expectedType, const QString &name, const UA_Variant &value, bool *ok, QString *errorMessage) const
{
    *ok = false;

    if (UA_Variant_isEmpty(&value)) {
        *errorMessage = QString::fromUtf8("%1 节点值为空。").arg(name);
        return QVariant();
    }

    if (expectedType == ExpectedBool) {
        if (!UA_Variant_isScalar(&value) || value.type != &UA_TYPES[UA_TYPES_BOOLEAN]) {
            *errorMessage = QString::fromUtf8("%1 期望 BOOL，实际类型不匹配。").arg(name);
            return QVariant();
        }

        *ok = true;
        return QVariant(*static_cast<UA_Boolean *>(value.data) != 0);
    }

    if (expectedType == ExpectedReal) {
        if (!UA_Variant_isScalar(&value)) {
            *errorMessage = QString::fromUtf8("%1 期望标量 REAL，实际返回数组。").arg(name);
            return QVariant();
        }
        if (value.type == &UA_TYPES[UA_TYPES_FLOAT]) {
            *ok = true;
            return QVariant(static_cast<double>(*static_cast<UA_Float *>(value.data)));
        }
        if (value.type == &UA_TYPES[UA_TYPES_DOUBLE]) {
            *ok = true;
            return QVariant(*static_cast<UA_Double *>(value.data));
        }

        *errorMessage = QString::fromUtf8("%1 期望 REAL，实际类型不匹配。").arg(name);
        return QVariant();
    }

    if (expectedType == ExpectedInt) {
        if (!UA_Variant_isScalar(&value)) {
            *errorMessage = QString::fromUtf8("%1 期望标量 INT，实际返回数组。").arg(name);
            return QVariant();
        }
        if (value.type == &UA_TYPES[UA_TYPES_INT16]) {
            *ok = true;
            return QVariant(static_cast<int>(*static_cast<UA_Int16 *>(value.data)));
        }
        if (value.type == &UA_TYPES[UA_TYPES_UINT16]) {
            *ok = true;
            return QVariant(static_cast<int>(*static_cast<UA_UInt16 *>(value.data)));
        }
        if (value.type == &UA_TYPES[UA_TYPES_INT32]) {
            *ok = true;
            return QVariant(static_cast<int>(*static_cast<UA_Int32 *>(value.data)));
        }
        if (value.type == &UA_TYPES[UA_TYPES_UINT32]) {
            *ok = true;
            return QVariant(static_cast<int>(*static_cast<UA_UInt32 *>(value.data)));
        }

        *errorMessage = QString::fromUtf8("%1 期望 INT，实际类型不匹配。").arg(name);
        return QVariant();
    }

    *errorMessage = QString::fromUtf8("%1 不支持的标量类型。").arg(name);
    return QVariant();
}

void OpcUaClient::clearNodeId(UA_NodeId **nodeId)
{
    if (!nodeId || !*nodeId) {
        return;
    }

    UA_NodeId_clear(*nodeId);
    delete *nodeId;
    *nodeId = nullptr;
}
#endif

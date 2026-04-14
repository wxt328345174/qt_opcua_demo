#include "opcua_client.h"

#include <QStringList>

#ifndef Q_OS_WIN
#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/types_generated.h>
#endif

namespace {

const char kEndpointUrl[] = "opc.tcp://192.168.0.105:4840";
const int kPollIntervalMs = 1000;

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
    return createReadRows();
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
    emit logMessage(QString::fromUtf8("已配置 4 个固定读取节点。"));
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

void OpcUaClient::pollValues()
{
#ifdef Q_OS_WIN
    return;
#else
    if (!m_client) {
        return;
    }

    int successCount = 0;
    for (int i = 0; i < m_targets.size(); ++i) {
        if (readNodeValue(&m_targets[i])) {
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
    m_targets.clear();

    TargetNode var1;
    var1.row = {"group1_var1", "group1_var1", "INT", 0, "", "R",
                "ns=1;s=globals.group1_var1", QString::fromUtf8("固定 NodeId 直连读取")};
    var1.identifier = "globals.group1_var1";
    var1.expectedType = ExpectedInt;
    m_targets.append(var1);

    TargetNode var2;
    var2.row = {"group1_var2", "group1_var2", "REAL", 0.0, "", "R",
                "ns=1;s=globals.group1_var2", QString::fromUtf8("固定 NodeId 直连读取")};
    var2.identifier = "globals.group1_var2";
    var2.expectedType = ExpectedReal;
    m_targets.append(var2);

    TargetNode var3;
    var3.row = {"group1_var3", "group1_var3", "BOOL", false, "", "R",
                "ns=1;s=globals.group1_var3", QString::fromUtf8("固定 NodeId 直连读取")};
    var3.identifier = "globals.group1_var3";
    var3.expectedType = ExpectedBool;
    m_targets.append(var3);

    TargetNode var4;
    var4.row = {"group1_var4", "group1_var4", "INT ARRAY[0..4]", QString::fromLatin1("[0,0,0,0,0]"), "", "R",
                "ns=1;s=globals.group1_var4", QString::fromUtf8("固定 NodeId 直连读取")};
    var4.identifier = "globals.group1_var4";
    var4.expectedType = ExpectedIntArray;
    m_targets.append(var4);
}

QVector<VariableRow> OpcUaClient::createReadRows() const
{
    QVector<VariableRow> rows;
    rows.reserve(m_targets.size());
    for (int i = 0; i < m_targets.size(); ++i) {
        rows.append(m_targets.at(i).row);
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
    for (int i = 0; i < m_targets.size(); ++i) {
#ifndef Q_OS_WIN
        if (m_targets.at(i).nodeId) {
            ++resolvedCount;
        }
#else
        Q_UNUSED(i);
#endif
    }
    emit resolvedCountChanged(resolvedCount, m_targets.size());
}

void OpcUaClient::setLastRefresh(const QDateTime &timestamp)
{
    m_lastRefresh = timestamp;
    emit refreshTimestampChanged(timestamp);
}

void OpcUaClient::clearBindings()
{
#ifndef Q_OS_WIN
    for (int i = 0; i < m_targets.size(); ++i) {
        clearNodeId(&m_targets[i].nodeId);
        emit nodeIdResolved(m_targets[i].row.id, m_targets[i].row.nodeId);
    }
#endif
    updateResolvedCount();
}

#ifndef Q_OS_WIN
bool OpcUaClient::createNodeBindings()
{
    clearBindings();

    for (int i = 0; i < m_targets.size(); ++i) {
        TargetNode *target = &m_targets[i];
        target->nodeId = new UA_NodeId;
        UA_NodeId_init(target->nodeId);

        const QByteArray identifierUtf8 = target->identifier.toUtf8();
        *target->nodeId = UA_NODEID_STRING_ALLOC(1, const_cast<char *>(identifierUtf8.constData()));
        emit nodeIdResolved(target->row.id, target->row.nodeId);
        emit logMessage(QString::fromUtf8("已配置 %1 -> %2")
                        .arg(target->row.id, target->row.nodeId));
    }

    updateResolvedCount();
    return true;
}

bool OpcUaClient::readNodeValue(TargetNode *target)
{
    if (!target || !target->nodeId || !m_client) {
        return false;
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

QVariant OpcUaClient::convertVariant(const TargetNode &target, const UA_Variant &value, bool *ok, QString *errorMessage) const
{
    *ok = false;

    if (UA_Variant_isEmpty(&value)) {
        *errorMessage = QString::fromUtf8("节点值为空。");
        return QVariant();
    }

    if (target.expectedType == ExpectedBool) {
        if (!UA_Variant_isScalar(&value) || value.type != &UA_TYPES[UA_TYPES_BOOLEAN]) {
            *errorMessage = QString::fromUtf8("期望 BOOL，实际类型不匹配。");
            return QVariant();
        }

        *ok = true;
        return QVariant(*static_cast<UA_Boolean *>(value.data) != 0);
    }

    if (target.expectedType == ExpectedReal) {
        if (!UA_Variant_isScalar(&value)) {
            *errorMessage = QString::fromUtf8("期望标量 REAL，实际返回数组。");
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

        *errorMessage = QString::fromUtf8("期望 REAL，实际类型不匹配。");
        return QVariant();
    }

    if (target.expectedType == ExpectedInt) {
        if (!UA_Variant_isScalar(&value)) {
            *errorMessage = QString::fromUtf8("期望标量 INT，实际返回数组。");
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

        *errorMessage = QString::fromUtf8("期望 INT，实际类型不匹配。");
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

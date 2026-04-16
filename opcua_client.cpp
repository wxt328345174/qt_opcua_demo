#include "opcua_client.h"

#include "opcua_value_codec.h"

#include <QByteArray>

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>

namespace {

QString disconnectedText()
{
    return QStringLiteral("未连接");
}

QString connectingText()
{
    return QStringLiteral("连接中");
}

QString connectedText()
{
    return QStringLiteral("已连接");
}

} // namespace

OpcUaClient::OpcUaClient(QObject *parent)
    : QObject(parent)
{
    initializeTargets();

    m_pollTimer.setInterval(OpcUa::pollIntervalMs());
    connect(&m_pollTimer, &QTimer::timeout, this, &OpcUaClient::pollValues);

    setConnectionState(disconnectedText());
    updateResolvedCount();
}

OpcUaClient::~OpcUaClient()
{
    disconnectFromServer();
}

QString OpcUaClient::endpointUrl() const
{
    return QString::fromLatin1(OpcUa::endpointUrl());
}

QVector<VariableRow> OpcUaClient::readVariables() const
{
    return OpcUa::createRows(m_readTargets);
}

QVector<VariableRow> OpcUaClient::writeVariables() const
{
    return OpcUa::createRows(m_writeTargets);
}

void OpcUaClient::connectToServer()
{
    if (m_connectionState == connectingText() || m_connectionState == connectedText()) {
        return;
    }

    disconnectFromServer();

    setConnectionState(connectingText());
    emit logMessage(QStringLiteral("开始连接 OPC UA Server: %1").arg(endpointUrl()));

    m_client = UA_Client_new();
    if (!m_client) {
        setConnectionState(disconnectedText());
        emit logMessage(QStringLiteral("创建 OPC UA 客户端失败。"));
        return;
    }

    UA_ClientConfig_setDefault(UA_Client_getConfig(m_client));
    const UA_StatusCode status = UA_Client_connect(m_client, OpcUa::endpointUrl());
    if (status != UA_STATUSCODE_GOOD) {
        UA_Client_delete(m_client);
        m_client = nullptr;
        setConnectionState(disconnectedText());
        emit logMessage(QStringLiteral("连接失败: %1").arg(QString::fromLatin1(UA_StatusCode_name(status))));
        return;
    }

    if (!createNodeBindings()) {
        disconnectFromServer();
        emit logMessage(QStringLiteral("固定 NodeId 初始化失败。"));
        return;
    }

    setConnectionState(connectedText());
    emit logMessage(QStringLiteral("OPC UA 连接成功。"));
    emit logMessage(QStringLiteral("已配置 %1 个读取变量，%2 个写入变量。")
                    .arg(m_readTargets.size())
                    .arg(m_writeTargets.size()));
    m_pollTimer.start();
    pollValues();
}

void OpcUaClient::disconnectFromServer()
{
    m_pollTimer.stop();

    clearBindings();
    if (m_client) {
        UA_Client_disconnect(m_client);
        UA_Client_delete(m_client);
        m_client = nullptr;
    }

    setLastRefresh(QDateTime());
    setConnectionState(disconnectedText());
}

bool OpcUaClient::writeValue(const QString &id, const QString &textValue, QString *errorMessage)
{
    OpcUa::TargetNode *target = findWriteTarget(id);
    if (!target) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("未知写入变量: %1").arg(id);
        }
        return false;
    }

    QVariant parsedValue;
    if (!OpcUa::parseTextValue(*target, textValue, &parsedValue, errorMessage)) {
        return false;
    }

    if (!m_client || !target->nodeId) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("OPC UA 尚未连接，不能写入 %1。").arg(id);
        }
        return false;
    }

    if (!writeNodeValue(target, parsedValue, errorMessage)) {
        return false;
    }

    emit logMessage(QStringLiteral("写入成功: %1 = %2").arg(target->row.id, parsedValue.toString()));
    return true;
}

void OpcUaClient::pollValues()
{
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
}

void OpcUaClient::initializeTargets()
{
    m_readTargets = OpcUa::createReadTargets();
    m_writeTargets = OpcUa::createWriteTargets();
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
        if (m_readTargets.at(i).nodeId) {
            ++resolvedCount;
        }
    }
    for (int i = 0; i < m_writeTargets.size(); ++i) {
        if (m_writeTargets.at(i).nodeId) {
            ++resolvedCount;
        }
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
    updateResolvedCount();
}

OpcUa::TargetNode *OpcUaClient::findWriteTarget(const QString &id)
{
    for (int i = 0; i < m_writeTargets.size(); ++i) {
        if (m_writeTargets[i].row.id == id) {
            return &m_writeTargets[i];
        }
    }
    return nullptr;
}

bool OpcUaClient::createNodeBindings()
{
    clearBindings();

    QVector<OpcUa::TargetNode> *groups[] = {&m_readTargets, &m_writeTargets};
    for (int groupIndex = 0; groupIndex < 2; ++groupIndex) {
        QVector<OpcUa::TargetNode> *targets = groups[groupIndex];
        for (int i = 0; i < targets->size(); ++i) {
            OpcUa::TargetNode *target = &(*targets)[i];
            target->nodeId = new UA_NodeId;
            UA_NodeId_init(target->nodeId);

            const QByteArray identifierUtf8 = target->identifier.toUtf8();
            *target->nodeId = UA_NODEID_STRING_ALLOC(1, const_cast<char *>(identifierUtf8.constData()));
            emit nodeIdResolved(target->row.id, target->row.nodeId);
            emit logMessage(QStringLiteral("已配置 %1 -> %2").arg(target->row.id, target->row.nodeId));

            for (int fieldIndex = 0; fieldIndex < target->structFields.size(); ++fieldIndex) {
                OpcUa::StructField *field = &target->structFields[fieldIndex];
                field->nodeId = new UA_NodeId;
                UA_NodeId_init(field->nodeId);
                const QByteArray fieldIdentifierUtf8 = field->identifier.toUtf8();
                *field->nodeId = UA_NODEID_STRING_ALLOC(1, const_cast<char *>(fieldIdentifierUtf8.constData()));
                emit logMessage(QStringLiteral("已配置 %1.%2 -> %3")
                                .arg(target->row.id, field->name, OpcUa::nodeIdText(field->identifier)));
            }
        }
    }

    updateResolvedCount();
    return true;
}

bool OpcUaClient::readNodeValue(OpcUa::TargetNode *target)
{
    if (!target || !target->nodeId || !m_client) {
        return false;
    }

    if (target->expectedType == OpcUa::ExpectedStruct) {
        return readStructValue(target);
    }

    UA_Variant value;
    UA_Variant_init(&value);
    const UA_StatusCode status = UA_Client_readValueAttribute(m_client, *target->nodeId, &value);
    if (status != UA_STATUSCODE_GOOD) {
        emit logMessage(QStringLiteral("读取 %1 失败: %2")
                        .arg(target->row.id, QString::fromLatin1(UA_StatusCode_name(status))));
        UA_Variant_clear(&value);
        return false;
    }

    QString errorMessage;
    bool ok = false;
    const QVariant convertedValue = OpcUa::convertVariant(*target, value, &ok, &errorMessage);
    UA_Variant_clear(&value);

    if (!ok) {
        emit logMessage(QStringLiteral("读取 %1 失败: %2").arg(target->row.id, errorMessage));
        return false;
    }

    if (target->row.value != convertedValue) {
        target->row.value = convertedValue;
        emit variableChanged(target->row.id, convertedValue);
    }
    return true;
}

bool OpcUaClient::readStructValue(OpcUa::TargetNode *target)
{
    int aValue = 0;
    double bValue = 0.0;
    bool cValue = false;

    for (int i = 0; i < target->structFields.size(); ++i) {
        OpcUa::StructField *field = &target->structFields[i];
        QString errorMessage;
        if (!readStructField(field, &errorMessage)) {
            emit logMessage(QStringLiteral("读取 %1.%2 (%3) 失败: %4")
                            .arg(target->row.id, field->name, OpcUa::nodeIdText(field->identifier), errorMessage));
            return false;
        }

        if (field->name == QStringLiteral("A")) {
            aValue = field->value.toInt();
        } else if (field->name == QStringLiteral("B")) {
            bValue = field->value.toDouble();
        } else if (field->name == QStringLiteral("C")) {
            cValue = field->value.toBool();
        }
    }

    const QVariant convertedValue = OpcUa::compactStructValue(aValue, bValue, cValue);
    if (target->row.value != convertedValue) {
        target->row.value = convertedValue;
        emit variableChanged(target->row.id, convertedValue);
    }
    return true;
}

bool OpcUaClient::readStructField(OpcUa::StructField *field, QString *errorMessage)
{
    if (!field || !field->nodeId || !m_client) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("字段 NodeId 未初始化。");
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
    const QVariant convertedValue = OpcUa::convertScalarVariant(field->expectedType, field->name, value, &ok, errorMessage);
    UA_Variant_clear(&value);

    if (!ok) {
        return false;
    }

    field->value = convertedValue;
    return true;
}

bool OpcUaClient::writeNodeValue(OpcUa::TargetNode *target, const QVariant &value, QString *errorMessage)
{
    if (target->expectedType == OpcUa::ExpectedStruct) {
        return writeStructValue(target, value, errorMessage);
    }

    return OpcUa::writeScalarValue(m_client, *target->nodeId, target->expectedType, value, target->row.name, errorMessage);
}

bool OpcUaClient::writeStructValue(OpcUa::TargetNode *target, const QVariant &value, QString *errorMessage)
{
    int aValue = 0;
    double bValue = 0.0;
    bool cValue = false;
    if (!OpcUa::parseStDataValue(value.toString(), &aValue, &bValue, &cValue)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("%1 结构体值格式错误，应为 (A := 10, B := 10.8, C := TRUE)。").arg(target->row.name);
        }
        return false;
    }

    for (int i = 0; i < target->structFields.size(); ++i) {
        OpcUa::StructField *field = &target->structFields[i];

        QVariant fieldValue;
        if (field->name == QStringLiteral("A")) {
            fieldValue = aValue;
        } else if (field->name == QStringLiteral("B")) {
            fieldValue = bValue;
        } else if (field->name == QStringLiteral("C")) {
            fieldValue = cValue;
        } else {
            continue;
        }

        if (!field->nodeId) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("%1.%2 字段 NodeId 未初始化。").arg(target->row.name, field->name);
            }
            return false;
        }

        if (!OpcUa::writeScalarValue(m_client,
                                     *field->nodeId,
                                     field->expectedType,
                                     fieldValue,
                                     target->row.name + QStringLiteral(".") + field->name,
                                     errorMessage)) {
            return false;
        }
    }

    return true;
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

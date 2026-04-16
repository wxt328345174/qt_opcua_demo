#ifndef OPCUA_CLIENT_H
#define OPCUA_CLIENT_H

#include "opcua_nodes.h"
#include "variable_row.h"

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariant>
#include <QVector>

struct UA_Client;

// OPC UA 客户端层：封装连接、断开、轮询读取、写入和状态通知。
// 模块重点是 Qt 信号槽如何把底层通信结果传回 MainWindow。
class OpcUaClient : public QObject
{
    Q_OBJECT

public:
    explicit OpcUaClient(QObject *parent = nullptr);
    ~OpcUaClient();

    QString endpointUrl() const;
    QVector<VariableRow> readVariables() const;
    QVector<VariableRow> writeVariables() const;
    void connectToServer();
    void disconnectFromServer();
    bool writeValue(const QString &id, const QString &textValue, QString *errorMessage);

signals:
    // 这些信号构成“业务层 -> 界面层”的数据通道。
    void variableChanged(const QString &id, const QVariant &value);
    void nodeIdResolved(const QString &id, const QString &nodeIdText);
    void logMessage(const QString &text);
    void connectionStateChanged(const QString &stateText);
    void refreshTimestampChanged(const QDateTime &timestamp);
    void resolvedCountChanged(int resolvedCount, int totalCount);

private slots:
    void pollValues();

private:
    // 连接生命周期和轮询流程。
    void initializeTargets();
    void setConnectionState(const QString &stateText);
    void updateResolvedCount();
    void setLastRefresh(const QDateTime &timestamp);
    void clearBindings();

    // 具体 OPC UA 节点操作。TargetNode 来自 opcua_nodes.* 的固定点位定义。
    OpcUa::TargetNode *findWriteTarget(const QString &id);
    bool createNodeBindings();
    bool readNodeValue(OpcUa::TargetNode *target);
    bool writeNodeValue(OpcUa::TargetNode *target, const QVariant &value, QString *errorMessage);
    bool readStructValue(OpcUa::TargetNode *target);
    bool readStructField(OpcUa::StructField *field, QString *errorMessage);
    bool writeStructValue(OpcUa::TargetNode *target, const QVariant &value, QString *errorMessage);
    void clearNodeId(UA_NodeId **nodeId);

    QVector<OpcUa::TargetNode> m_readTargets;
    QVector<OpcUa::TargetNode> m_writeTargets;
    QTimer m_pollTimer;
    QString m_connectionState;
    QDateTime m_lastRefresh;
    UA_Client *m_client = nullptr;
};

#endif // OPCUA_CLIENT_H

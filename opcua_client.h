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
    void variableChanged(const QString &id, const QVariant &value);
    void nodeIdResolved(const QString &id, const QString &nodeIdText);
    void logMessage(const QString &text);
    void connectionStateChanged(const QString &stateText);
    void refreshTimestampChanged(const QDateTime &timestamp);
    void resolvedCountChanged(int resolvedCount, int totalCount);

private slots:
    void pollValues();

private:
    void initializeTargets();
    void setConnectionState(const QString &stateText);
    void updateResolvedCount();
    void setLastRefresh(const QDateTime &timestamp);
    void clearBindings();
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

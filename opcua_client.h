#ifndef OPCUA_CLIENT_H
#define OPCUA_CLIENT_H

#include "variable_row.h"

#include <QDateTime>
#include <QObject>
#include <QTimer>
#include <QVector>

#ifndef Q_OS_WIN
#include <open62541/types.h>

struct UA_Client;
#endif

class OpcUaClient : public QObject
{
    Q_OBJECT

public:
    explicit OpcUaClient(QObject *parent = nullptr);
    ~OpcUaClient();

    QVector<VariableRow> readVariables() const;
    void connectToServer();
    void disconnectFromServer();

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
    enum ExpectedType {
        ExpectedInt,
        ExpectedReal,
        ExpectedBool,
        ExpectedIntArray
    };

    struct TargetNode
    {
        VariableRow row;
        QString identifier;
        ExpectedType expectedType = ExpectedInt;

#ifndef Q_OS_WIN
        UA_NodeId *nodeId = nullptr;
#endif
    };

    void initializeTargets();
    QVector<VariableRow> createReadRows() const;
    void setConnectionState(const QString &stateText);
    void updateResolvedCount();
    void setLastRefresh(const QDateTime &timestamp);
    void clearBindings();

#ifndef Q_OS_WIN
    bool createNodeBindings();
    bool readNodeValue(TargetNode *target);
    QVariant convertVariant(const TargetNode &target, const UA_Variant &value, bool *ok, QString *errorMessage) const;
    void clearNodeId(UA_NodeId **nodeId);
#endif

    QVector<TargetNode> m_targets;
    QTimer m_pollTimer;
    QString m_connectionState;
    QDateTime m_lastRefresh;

#ifndef Q_OS_WIN
    UA_Client *m_client = nullptr;
#endif
};

#endif // OPCUA_CLIENT_H

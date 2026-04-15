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
    enum ExpectedType {
        ExpectedInt,
        ExpectedReal,
        ExpectedBool,
        ExpectedIntArray,
        ExpectedStruct
    };

    struct StructField
    {
        StructField() {}
        StructField(const QString &fieldName, const QString &fieldIdentifier, ExpectedType fieldType, const QVariant &initialValue)
            : name(fieldName)
            , identifier(fieldIdentifier)
            , expectedType(fieldType)
            , value(initialValue)
        {
        }

        QString name;
        QString identifier;
        ExpectedType expectedType = ExpectedInt;
        QVariant value;

#ifndef Q_OS_WIN
        UA_NodeId *nodeId = nullptr;
#endif
    };

    struct TargetNode
    {
        VariableRow row;
        QString identifier;
        ExpectedType expectedType = ExpectedInt;
        QVector<StructField> structFields;

#ifndef Q_OS_WIN
        UA_NodeId *nodeId = nullptr;
#endif
    };

    void initializeTargets();
    QVector<VariableRow> createRows(const QVector<TargetNode> &targets) const;
    void setConnectionState(const QString &stateText);
    void updateResolvedCount();
    void setLastRefresh(const QDateTime &timestamp);
    void clearBindings();
    TargetNode *findWriteTarget(const QString &id);
    bool parseTextValue(const TargetNode &target, const QString &textValue, QVariant *parsedValue, QString *errorMessage) const;

#ifndef Q_OS_WIN
    bool createNodeBindings();
    bool readNodeValue(TargetNode *target);
    bool writeNodeValue(TargetNode *target, const QVariant &value, QString *errorMessage);
    bool readStructValue(TargetNode *target);
    bool readStructField(StructField *field, QString *errorMessage);
    bool writeStructValue(TargetNode *target, const QVariant &value, QString *errorMessage);
    bool writeScalarValue(const UA_NodeId &nodeId, ExpectedType expectedType, const QVariant &value, const QString &name, QString *errorMessage);
    QVariant convertVariant(const TargetNode &target, const UA_Variant &value, bool *ok, QString *errorMessage) const;
    QVariant convertScalarVariant(ExpectedType expectedType, const QString &name, const UA_Variant &value, bool *ok, QString *errorMessage) const;
    void clearNodeId(UA_NodeId **nodeId);
#endif

    QVector<TargetNode> m_readTargets;
    QVector<TargetNode> m_writeTargets;
    QTimer m_pollTimer;
    QString m_connectionState;
    QDateTime m_lastRefresh;

#ifndef Q_OS_WIN
    UA_Client *m_client = nullptr;
#endif
};

#endif // OPCUA_CLIENT_H

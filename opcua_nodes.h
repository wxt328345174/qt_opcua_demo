#ifndef OPCUA_NODES_H
#define OPCUA_NODES_H

#include "variable_row.h"

#include <QString>
#include <QVariant>
#include <QVector>

#include <open62541/types.h>

namespace OpcUa {

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

struct TargetNode
{
    TargetNode();

    VariableRow row;
    QString identifier;
    ExpectedType expectedType;
    QVector<StructField> structFields;
    UA_NodeId *nodeId;
};

const char *endpointUrl();
int pollIntervalMs();
QString nodeIdText(const QString &identifier);
QVector<TargetNode> createReadTargets();
QVector<TargetNode> createWriteTargets();
QVector<VariableRow> createRows(const QVector<TargetNode> &targets);

} // namespace OpcUa

#endif // OPCUA_NODES_H

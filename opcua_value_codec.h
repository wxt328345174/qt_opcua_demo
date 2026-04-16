#ifndef OPCUA_VALUE_CODEC_H
#define OPCUA_VALUE_CODEC_H

#include "opcua_nodes.h"

#include <QString>
#include <QVariant>

struct UA_Client;

namespace OpcUa {

QString compactStructValue(int a, double b, bool c);
bool parseStDataValue(const QString &text, int *a, double *b, bool *c);
bool parseTextValue(const TargetNode &target, const QString &textValue, QVariant *parsedValue, QString *errorMessage);
QVariant convertVariant(const TargetNode &target, const UA_Variant &value, bool *ok, QString *errorMessage);
QVariant convertScalarVariant(ExpectedType expectedType, const QString &name, const UA_Variant &value, bool *ok, QString *errorMessage);
bool writeScalarValue(UA_Client *client,
                      const UA_NodeId &nodeId,
                      ExpectedType expectedType,
                      const QVariant &value,
                      const QString &name,
                      QString *errorMessage);

} // namespace OpcUa

#endif // OPCUA_VALUE_CODEC_H

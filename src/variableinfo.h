#ifndef VARIABLEINFO_H
#define VARIABLEINFO_H

#include <QString>
#include <QVariant>

struct VariableInfo
{
    QString id;
    QString displayName;
    QString dataType;
    QVariant value;
    QString unit;
    QString access;
    QString opcUaNodeId;
    QString description;
};

#endif // VARIABLEINFO_H

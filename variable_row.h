#ifndef VARIABLE_ROW_H
#define VARIABLE_ROW_H

#include <QString>
#include <QVariant>

struct VariableRow
{
    QString id;
    QString name;
    QString type;
    QVariant value;
    QString unit;
    QString access;
    QString nodeId;
    QString note;
};

#endif // VARIABLE_ROW_H

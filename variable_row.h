#ifndef VARIABLE_ROW_H
#define VARIABLE_ROW_H

#include <QString>
#include <QVariant>

// 表格行数据模型：界面只关心这些展示字段，不直接接触 open62541 类型。
// 同一个结构同时服务读取表和写入表，体现 UI 与业务数据的分离。
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

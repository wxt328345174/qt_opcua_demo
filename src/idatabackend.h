#ifndef IDATABACKEND_H
#define IDATABACKEND_H

#include "variableinfo.h"

#include <QObject>
#include <QString>
#include <QVariant>
#include <QVector>

class IDataBackend : public QObject
{
    Q_OBJECT

public:
    explicit IDataBackend(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    virtual QVector<VariableInfo> variables() const = 0;
    virtual QVariant value(const QString &id) const = 0;

public slots:
    virtual bool writeVariable(const QString &id, const QVariant &value) = 0;

signals:
    void variableChanged(const QString &id, const QVariant &value);
    void logMessage(const QString &text);
};

#endif // IDATABACKEND_H

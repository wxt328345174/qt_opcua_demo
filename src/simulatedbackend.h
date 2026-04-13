#ifndef SIMULATEDBACKEND_H
#define SIMULATEDBACKEND_H

#include "idatabackend.h"

#include <QHash>
#include <QTimer>

class SimulatedBackend : public IDataBackend
{
    Q_OBJECT

public:
    explicit SimulatedBackend(QObject *parent = nullptr);

    QVector<VariableInfo> variables() const override;
    QVariant value(const QString &id) const override;

public slots:
    bool writeVariable(const QString &id, const QVariant &value) override;

private slots:
    void updateSimulation();

private:
    void addVariable(const VariableInfo &variable);
    bool setValue(const QString &id, const QVariant &value);
    bool boolValue(const QString &id) const;
    double doubleValue(const QString &id) const;
    int intValue(const QString &id) const;
    void setAlarmState(bool active, const QString &reason);

    QVector<VariableInfo> m_variables;
    QHash<QString, int> m_indexById;
    QTimer m_timer;
    int m_tick = 0;
};

#endif // SIMULATEDBACKEND_H

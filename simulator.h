#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariant>
#include <QVector>

struct VariableRow
{
    // 表格中的一行变量信息。后续接真实 OPC UA 时，nodeId 字段替换为导师提供的 NodeId。
    QString id;
    QString name;
    QString type;
    QVariant value;
    QString unit;
    QString access;
    QString nodeId;
    QString note;
};

class Simulator : public QObject
{
    Q_OBJECT

public:
    explicit Simulator(QObject *parent = nullptr);

    QVector<VariableRow> variables() const;

public slots:
    // 槽函数：界面按钮点击后会调用这些函数。
    void start();
    void stop();

signals:
    // 信号：模拟器数据变化后通知界面刷新表格和状态栏。
    void variableChanged(const QString &id, const QVariant &value);
    void logMessage(const QString &text);

private slots:
    // 定时器每秒调用一次，用来产生模拟温度、压力、心跳等数据。
    void updateData();

private:
    void addVariable(const VariableRow &row);
    void setValue(const QString &id, const QVariant &value);
    QVariant value(const QString &id) const;
    void setAlarm(bool active);

    QVector<VariableRow> m_variables;
    QTimer m_timer;
    // 下面这些成员保存当前模拟设备状态。
    bool m_running = false;
    bool m_alarm = false;
    int m_tick = 0;
    int m_heartbeat = 0;
    int m_count = 0;
    double m_temperature = 26.0;
    double m_pressure = 0.20;
};

#endif // SIMULATOR_H

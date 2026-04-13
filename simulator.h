#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariant>
#include <QVector>

struct VariableRow
{
    // 表格中的一行变量信息。接入真实 OPC UA 时，nodeId 字段替换为实际配置的 NodeId。
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

    QVector<VariableRow> readVariables() const;
    QVector<VariableRow> writeVariables() const;
    bool writeValue(const QString &id, const QString &textValue, QString *errorMessage);

public slots:
    // 槽函数：界面按钮点击后会调用这些函数。
    void start();
    void stop();

signals:
    // 信号：模拟器数据变化后通知界面刷新表格和状态栏。
    void variableChanged(const QString &id, const QVariant &value);
    void logMessage(const QString &text);

private slots:
    // 定时器每秒调用一次，用来产生模拟读取数据。
    void updateData();

private:
    void addReadVariable(const VariableRow &row);
    void addWriteVariable(const VariableRow &row);
    bool setReadValue(const QString &id, const QVariant &value);
    bool setWriteValue(const QString &id, const QVariant &value);
    bool setValue(QVector<VariableRow> *variables, const QString &id, const QVariant &value);
    const VariableRow *findWriteVariable(const QString &id) const;
    bool parseTextValue(const VariableRow &row, const QString &textValue, QVariant *parsedValue, QString *errorMessage) const;
    QString compactJsonArray(double first, double second, double third) const;
    QString compactDeviceStatus() const;
    void applyWriteValue(const QString &id, const QVariant &value);
    void setAlarm(bool active);

    QVector<VariableRow> m_readVariables;
    QVector<VariableRow> m_writeVariables;
    QTimer m_timer;

    // 下面这些成员保存当前模拟设备状态。
    bool m_motorRunning = false;
    bool m_alarm = false;
    int m_tick = 0;
    int m_heartbeat = 0;
    int m_actualCount = 0;
    int m_targetCount = 100;
    double m_actualTemperature = 26.0;
    double m_setTemperature = 60.0;
    QString m_targetSpeeds;
    QString m_recipeParams;
};

#endif // SIMULATOR_H

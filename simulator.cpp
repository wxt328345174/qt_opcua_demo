#include "simulator.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QtMath>

Simulator::Simulator(QObject *parent)
    : QObject(parent)
{
    m_targetSpeeds = QString::fromUtf8("[100,120,110]");
    m_recipeParams = QString::fromUtf8("{\"durationSec\":30,\"enabled\":true,\"recipeId\":1,\"temperature\":60}");

    // 读取数据：模拟从 PLC/OPC UA 读取到的实际值。
    addReadVariable({"actualTemperature", QString::fromUtf8("实际温度"), "double", m_actualTemperature, "degC", "R",
                     "TODO:opcua-nodeid/read/actualTemperature", QString::fromUtf8("模拟读取的实际温度")});
    addReadVariable({"actualCount", QString::fromUtf8("实际计数"), "int", m_actualCount, "pcs", "R",
                     "TODO:opcua-nodeid/read/actualCount", QString::fromUtf8("模拟读取的生产计数")});
    addReadVariable({"heartbeat", QString::fromUtf8("心跳"), "int", m_heartbeat, "", "R",
                     "TODO:opcua-nodeid/read/heartbeat", QString::fromUtf8("每秒递增，用于确认刷新")});
    addReadVariable({"motorRunning", QString::fromUtf8("电机运行状态"), "bool", m_motorRunning, "", "R",
                     "TODO:opcua-nodeid/read/motorRunning", QString::fromUtf8("模拟读取的电机状态")});
    addReadVariable({"alarmActive", QString::fromUtf8("报警状态"), "bool", m_alarm, "", "R",
                     "TODO:opcua-nodeid/read/alarmActive", QString::fromUtf8("温度超过阈值时置为 true")});
    addReadVariable({"zoneTemperatures", QString::fromUtf8("分区温度"), "array", compactJsonArray(26.0, 26.0, 26.0), "degC", "R",
                     "TODO:opcua-nodeid/read/zoneTemperatures", QString::fromUtf8("数组示例：三个分区温度")});
    addReadVariable({"deviceStatus", QString::fromUtf8("设备状态"), "struct", compactDeviceStatus(), "", "R",
                     "TODO:opcua-nodeid/read/deviceStatus", QString::fromUtf8("结构体示例：模式、错误码、负载率")});

    // 写入数据：模拟将设定值写入 PLC/OPC UA。
    addWriteVariable({"setTemperature", QString::fromUtf8("设定温度"), "double", m_setTemperature, "degC", "W",
                      "TODO:opcua-nodeid/write/setTemperature", QString::fromUtf8("写入目标温度")});
    addWriteVariable({"targetCount", QString::fromUtf8("目标计数"), "int", m_targetCount, "pcs", "W",
                      "TODO:opcua-nodeid/write/targetCount", QString::fromUtf8("写入目标生产数量")});
    addWriteVariable({"enableMotor", QString::fromUtf8("电机使能"), "bool", m_motorRunning, "", "W",
                      "TODO:opcua-nodeid/write/enableMotor", QString::fromUtf8("写入 true 启动电机，false 停止电机")});
    addWriteVariable({"targetSpeeds", QString::fromUtf8("目标速度"), "array", m_targetSpeeds, "rpm", "W",
                      "TODO:opcua-nodeid/write/targetSpeeds", QString::fromUtf8("数组示例：三个分区目标速度")});
    addWriteVariable({"recipeParams", QString::fromUtf8("配方参数"), "struct", m_recipeParams, "", "W",
                      "TODO:opcua-nodeid/write/recipeParams", QString::fromUtf8("结构体示例：配方号、温度、时长、启用状态")});

    // QTimer 超时后发出 timeout 信号，connect 把这个信号连接到 updateData()。
    connect(&m_timer, &QTimer::timeout, this, &Simulator::updateData);
    m_timer.start(1000);

    emit logMessage(QString::fromUtf8("程序启动完成。当前为模拟模式，OPC UA NodeId 只是占位。"));
}

QVector<VariableRow> Simulator::readVariables() const
{
    return m_readVariables;
}

QVector<VariableRow> Simulator::writeVariables() const
{
    return m_writeVariables;
}

bool Simulator::writeValue(const QString &id, const QString &textValue, QString *errorMessage)
{
    const VariableRow *row = findWriteVariable(id);
    if (!row) {
        if (errorMessage) {
            *errorMessage = QString::fromUtf8("未知写入变量：%1").arg(id);
        }
        emit logMessage(QString::fromUtf8("写入失败：未知写入变量 %1").arg(id));
        return false;
    }

    QVariant parsedValue;
    if (!parseTextValue(*row, textValue, &parsedValue, errorMessage)) {
        emit logMessage(QString::fromUtf8("写入失败：%1，输入值为 %2").arg(row->name, textValue));
        return false;
    }

    setWriteValue(id, parsedValue);
    applyWriteValue(id, parsedValue);
    emit logMessage(QString::fromUtf8("写入成功：%1 = %2").arg(row->name, parsedValue.toString()));
    return true;
}

void Simulator::start()
{
    // 启动按钮等价于写入 enableMotor=true。
    QString error;
    if (writeValue("enableMotor", "true", &error)) {
        emit logMessage(QString::fromUtf8("启动命令执行：电机使能已置为 true。"));
    }
}

void Simulator::stop()
{
    // 停止按钮等价于写入 enableMotor=false。
    QString error;
    if (writeValue("enableMotor", "false", &error)) {
        emit logMessage(QString::fromUtf8("停止命令执行：电机使能已置为 false。"));
    }
}

void Simulator::updateData()
{
    ++m_tick;
    ++m_heartbeat;
    setReadValue("heartbeat", m_heartbeat);

    if (m_motorRunning) {
        // 运行时：实际温度逐步靠近设定温度，并叠加小幅波动，便于观察刷新效果。
        if (m_actualCount < m_targetCount) {
            ++m_actualCount;
        }
        const double wave = 0.8 * qSin(m_tick / 3.0);
        m_actualTemperature += (m_setTemperature - m_actualTemperature) * 0.18 + wave;
    } else {
        // 停止时：温度逐步回落到环境温度。
        m_actualTemperature = qMax(26.0, m_actualTemperature - 1.5);
    }

    const QString zones = compactJsonArray(m_actualTemperature - 0.7,
                                           m_actualTemperature,
                                           m_actualTemperature + 0.5);

    setReadValue("actualTemperature", m_actualTemperature);
    setReadValue("actualCount", m_actualCount);
    setReadValue("zoneTemperatures", zones);
    setReadValue("deviceStatus", compactDeviceStatus());
    setAlarm(m_actualTemperature >= 78.0);
}

void Simulator::addReadVariable(const VariableRow &row)
{
    m_readVariables.append(row);
}

void Simulator::addWriteVariable(const VariableRow &row)
{
    m_writeVariables.append(row);
}

bool Simulator::setReadValue(const QString &id, const QVariant &value)
{
    return setValue(&m_readVariables, id, value);
}

bool Simulator::setWriteValue(const QString &id, const QVariant &value)
{
    return setValue(&m_writeVariables, id, value);
}

bool Simulator::setValue(QVector<VariableRow> *variables, const QString &id, const QVariant &value)
{
    // 统一从这里修改变量值。只有值真的变化时才发信号，避免界面重复刷新。
    for (int i = 0; i < variables->size(); ++i) {
        if ((*variables)[i].id == id) {
            if ((*variables)[i].value == value) {
                return false;
            }
            (*variables)[i].value = value;
            emit variableChanged(id, value);
            return true;
        }
    }
    return false;
}

const VariableRow *Simulator::findWriteVariable(const QString &id) const
{
    for (int i = 0; i < m_writeVariables.size(); ++i) {
        if (m_writeVariables.at(i).id == id) {
            return &m_writeVariables[i];
        }
    }
    return nullptr;
}

bool Simulator::parseTextValue(const VariableRow &row, const QString &textValue, QVariant *parsedValue, QString *errorMessage) const
{
    const QString text = textValue.trimmed();
    bool ok = false;

    if (row.type == "int") {
        const int value = text.toInt(&ok);
        if (ok) {
            *parsedValue = value;
            return true;
        }
        if (errorMessage) {
            *errorMessage = QString::fromUtf8("%1 需要输入十进制整数").arg(row.name);
        }
        return false;
    }

    if (row.type == "double") {
        const double value = text.toDouble(&ok);
        if (ok) {
            *parsedValue = value;
            return true;
        }
        if (errorMessage) {
            *errorMessage = QString::fromUtf8("%1 需要输入小数或整数").arg(row.name);
        }
        return false;
    }

    if (row.type == "bool") {
        const QString lower = text.toLower();
        if (lower == "true" || lower == "1") {
            *parsedValue = true;
            return true;
        }
        if (lower == "false" || lower == "0") {
            *parsedValue = false;
            return true;
        }
        if (errorMessage) {
            *errorMessage = QString::fromUtf8("%1 需要输入 true/false 或 1/0").arg(row.name);
        }
        return false;
    }

    if (row.type == "array" || row.type == "struct") {
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8(), &parseError);
        const bool valid = (row.type == "array" && doc.isArray()) || (row.type == "struct" && doc.isObject());
        if (parseError.error == QJsonParseError::NoError && valid) {
            *parsedValue = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
            return true;
        }

        if (errorMessage) {
            *errorMessage = row.type == "array"
                    ? QString::fromUtf8("%1 需要输入 JSON 数组，例如 [100,120,110]").arg(row.name)
                    : QString::fromUtf8("%1 需要输入 JSON 对象，例如 {\"recipeId\":1,\"temperature\":60.0}").arg(row.name);
        }
        return false;
    }

    *parsedValue = text;
    return true;
}

QString Simulator::compactJsonArray(double first, double second, double third) const
{
    QJsonArray array;
    array.append(first);
    array.append(second);
    array.append(third);
    return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
}

QString Simulator::compactDeviceStatus() const
{
    const double loadPercent = m_targetCount > 0
            ? qMin(100.0, m_actualCount * 100.0 / m_targetCount)
            : 0.0;

    QJsonObject object;
    object.insert("mode", m_motorRunning ? "AUTO" : "STOP");
    object.insert("errorCode", m_alarm ? 101 : 0);
    object.insert("loadPercent", loadPercent);
    return QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact));
}

void Simulator::applyWriteValue(const QString &id, const QVariant &value)
{
    if (id == "setTemperature") {
        m_setTemperature = value.toDouble();
    } else if (id == "targetCount") {
        m_targetCount = value.toInt();
    } else if (id == "enableMotor") {
        m_motorRunning = value.toBool();
        setReadValue("motorRunning", m_motorRunning);
        if (!m_motorRunning) {
            setAlarm(false);
        }
    } else if (id == "targetSpeeds") {
        m_targetSpeeds = value.toString();
    } else if (id == "recipeParams") {
        m_recipeParams = value.toString();
    }

    setReadValue("deviceStatus", compactDeviceStatus());
}

void Simulator::setAlarm(bool active)
{
    // 报警状态不变时直接返回，避免日志每秒重复刷屏。
    if (m_alarm == active) {
        return;
    }

    m_alarm = active;
    setReadValue("alarmActive", active);
    setReadValue("deviceStatus", compactDeviceStatus());

    if (active) {
        emit logMessage(QString::fromUtf8("报警触发：实际温度超过 78 degC。"));
    } else {
        emit logMessage(QString::fromUtf8("报警解除：实际温度恢复到阈值内。"));
    }
}

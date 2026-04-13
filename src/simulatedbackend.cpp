#include "simulatedbackend.h"

#include <QtMath>

namespace {

const char *nodeIdPrefix = "TODO:mentor-provide-nodeid/";

}

SimulatedBackend::SimulatedBackend(QObject *parent)
    : IDataBackend(parent)
{
    addVariable({"cmdStart",
                 QString::fromUtf8("启动命令"),
                 "bool",
                 false,
                 "",
                 "W",
                 nodeIdPrefix + QString("cmdStart"),
                 QString::fromUtf8("写入 true 后触发模拟设备启动")});
    addVariable({"cmdStop",
                 QString::fromUtf8("停止命令"),
                 "bool",
                 false,
                 "",
                 "W",
                 nodeIdPrefix + QString("cmdStop"),
                 QString::fromUtf8("写入 true 后触发模拟设备停止")});
    addVariable({"runState",
                 QString::fromUtf8("运行状态"),
                 "bool",
                 false,
                 "",
                 "R",
                 nodeIdPrefix + QString("runState"),
                 QString::fromUtf8("模拟设备是否处于运行状态")});
    addVariable({"alarmActive",
                 QString::fromUtf8("报警状态"),
                 "bool",
                 false,
                 "",
                 "R",
                 nodeIdPrefix + QString("alarmActive"),
                 QString::fromUtf8("温度或压力超过阈值时置为 true")});
    addVariable({"temperature",
                 QString::fromUtf8("温度"),
                 "double",
                 26.0,
                 "degC",
                 "R",
                 nodeIdPrefix + QString("temperature"),
                 QString::fromUtf8("模拟设备温度")});
    addVariable({"pressure",
                 QString::fromUtf8("压力"),
                 "double",
                 0.20,
                 "MPa",
                 "R",
                 nodeIdPrefix + QString("pressure"),
                 QString::fromUtf8("模拟管路压力")});
    addVariable({"level",
                 QString::fromUtf8("液位"),
                 "double",
                 40.0,
                 "%",
                 "R",
                 nodeIdPrefix + QString("level"),
                 QString::fromUtf8("模拟储罐液位百分比")});
    addVariable({"productionCount",
                 QString::fromUtf8("生产计数"),
                 "int",
                 0,
                 "pcs",
                 "R",
                 nodeIdPrefix + QString("productionCount"),
                 QString::fromUtf8("运行时递增的模拟产量")});
    addVariable({"heartbeat",
                 QString::fromUtf8("心跳"),
                 "int",
                 0,
                 "",
                 "R",
                 nodeIdPrefix + QString("heartbeat"),
                 QString::fromUtf8("运行时递增，用于确认数据刷新")});

    connect(&m_timer, &QTimer::timeout, this, &SimulatedBackend::updateSimulation);
    m_timer.start(1000);

    emit logMessage(QString::fromUtf8("模拟后端已启动，当前未连接真实 PLC。"));
}

QVector<VariableInfo> SimulatedBackend::variables() const
{
    return m_variables;
}

QVariant SimulatedBackend::value(const QString &id) const
{
    const int index = m_indexById.value(id, -1);
    if (index < 0) {
        return QVariant();
    }
    return m_variables.at(index).value;
}

bool SimulatedBackend::writeVariable(const QString &id, const QVariant &value)
{
    if (!m_indexById.contains(id)) {
        emit logMessage(QString::fromUtf8("写入失败：未知变量 %1").arg(id));
        return false;
    }

    if (id == "cmdStart" && value.toBool()) {
        setValue("cmdStart", true);
        setValue("runState", true);
        setValue("cmdStop", false);
        emit logMessage(QString::fromUtf8("收到启动命令，模拟设备进入运行状态。"));
        QTimer::singleShot(300, this, [this]() { setValue("cmdStart", false); });
        return true;
    }

    if (id == "cmdStop" && value.toBool()) {
        setValue("cmdStop", true);
        setValue("runState", false);
        setValue("cmdStart", false);
        setAlarmState(false, QString());
        emit logMessage(QString::fromUtf8("收到停止命令，模拟设备进入停止状态。"));
        QTimer::singleShot(300, this, [this]() { setValue("cmdStop", false); });
        return true;
    }

    emit logMessage(QString::fromUtf8("写入被忽略：%1 当前仅作为预留变量。").arg(id));
    return false;
}

void SimulatedBackend::updateSimulation()
{
    ++m_tick;

    if (boolValue("runState")) {
        const int heartbeat = intValue("heartbeat") + 1;
        const int productionCount = intValue("productionCount") + 1;
        const double temperature = 62.0 + 19.0 * qSin(m_tick / 4.0);
        const double pressure = 0.33 + 0.10 * qSin(m_tick / 5.0);
        const double level = 50.0 + 22.0 * qSin(m_tick / 7.0);

        setValue("heartbeat", heartbeat);
        setValue("productionCount", productionCount);
        setValue("temperature", temperature);
        setValue("pressure", pressure);
        setValue("level", level);

        if (temperature >= 78.0) {
            setAlarmState(true, QString::fromUtf8("温度超过 78 degC"));
        } else if (pressure >= 0.42) {
            setAlarmState(true, QString::fromUtf8("压力超过 0.42 MPa"));
        } else {
            setAlarmState(false, QString());
        }
    } else {
        const double cooledTemperature = qMax(26.0, doubleValue("temperature") - 2.0);
        const double releasedPressure = qMax(0.20, doubleValue("pressure") - 0.02);

        setValue("temperature", cooledTemperature);
        setValue("pressure", releasedPressure);
        setValue("level", 40.0);
    }
}

void SimulatedBackend::addVariable(const VariableInfo &variable)
{
    m_indexById.insert(variable.id, m_variables.size());
    m_variables.append(variable);
}

bool SimulatedBackend::setValue(const QString &id, const QVariant &value)
{
    const int index = m_indexById.value(id, -1);
    if (index < 0) {
        return false;
    }

    if (m_variables[index].value == value) {
        return false;
    }

    m_variables[index].value = value;
    emit variableChanged(id, value);
    return true;
}

bool SimulatedBackend::boolValue(const QString &id) const
{
    return value(id).toBool();
}

double SimulatedBackend::doubleValue(const QString &id) const
{
    return value(id).toDouble();
}

int SimulatedBackend::intValue(const QString &id) const
{
    return value(id).toInt();
}

void SimulatedBackend::setAlarmState(bool active, const QString &reason)
{
    const bool changed = setValue("alarmActive", active);
    if (!changed) {
        return;
    }

    if (active) {
        emit logMessage(QString::fromUtf8("报警触发：%1").arg(reason));
    } else {
        emit logMessage(QString::fromUtf8("报警解除。"));
    }
}

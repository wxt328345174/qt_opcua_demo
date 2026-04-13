#include "simulator.h"

#include <QtMath>

Simulator::Simulator(QObject *parent)
    : QObject(parent)
{
    // 这里集中预留 PLC/OPC UA 变量。当前 nodeId 只是占位，后续再替换为真实 NodeId。
    addVariable({"cmdStart", QString::fromUtf8("启动命令"), "bool", false, "", "W",
                 "TODO:mentor-provide-nodeid/cmdStart", QString::fromUtf8("点击启动按钮时写入 true")});
    addVariable({"cmdStop", QString::fromUtf8("停止命令"), "bool", false, "", "W",
                 "TODO:mentor-provide-nodeid/cmdStop", QString::fromUtf8("点击停止按钮时写入 true")});
    addVariable({"runState", QString::fromUtf8("运行状态"), "bool", false, "", "R",
                 "TODO:mentor-provide-nodeid/runState", QString::fromUtf8("设备是否运行")});
    addVariable({"alarmActive", QString::fromUtf8("报警状态"), "bool", false, "", "R",
                 "TODO:mentor-provide-nodeid/alarmActive", QString::fromUtf8("温度或压力超过阈值")});
    addVariable({"temperature", QString::fromUtf8("温度"), "double", m_temperature, "degC", "R",
                 "TODO:mentor-provide-nodeid/temperature", QString::fromUtf8("模拟温度")});
    addVariable({"pressure", QString::fromUtf8("压力"), "double", m_pressure, "MPa", "R",
                 "TODO:mentor-provide-nodeid/pressure", QString::fromUtf8("模拟压力")});
    addVariable({"productionCount", QString::fromUtf8("生产计数"), "int", m_count, "pcs", "R",
                 "TODO:mentor-provide-nodeid/productionCount", QString::fromUtf8("运行时递增")});
    addVariable({"heartbeat", QString::fromUtf8("心跳"), "int", m_heartbeat, "", "R",
                 "TODO:mentor-provide-nodeid/heartbeat", QString::fromUtf8("每秒递增")});

    // QTimer 超时后发出 timeout 信号，connect 把这个信号连接到 updateData()。
    connect(&m_timer, &QTimer::timeout, this, &Simulator::updateData);
    m_timer.start(1000);

    emit logMessage(QString::fromUtf8("程序启动完成。当前为模拟模式，OPC UA NodeId 只是占位。"));
}

QVector<VariableRow> Simulator::variables() const
{
    return m_variables;
}

void Simulator::start()
{
    // 模拟写入启动命令：命令变量短暂变为 true，运行状态保持为 true。
    m_running = true;
    setValue("cmdStart", true);
    setValue("cmdStop", false);
    setValue("runState", true);
    emit logMessage(QString::fromUtf8("点击启动：模拟设备进入运行状态。"));

    QTimer::singleShot(300, this, [this]() {
        // 命令变量通常是脉冲量，所以 300ms 后自动复位。
        setValue("cmdStart", false);
    });
}

void Simulator::stop()
{
    // 模拟写入停止命令：停止后清除报警，运行状态变为 false。
    m_running = false;
    setAlarm(false);
    setValue("cmdStop", true);
    setValue("cmdStart", false);
    setValue("runState", false);
    emit logMessage(QString::fromUtf8("点击停止：模拟设备进入停止状态。"));

    QTimer::singleShot(300, this, [this]() {
        // 停止命令同样按脉冲处理，短暂置 true 后复位。
        setValue("cmdStop", false);
    });
}

void Simulator::updateData()
{
    ++m_tick;

    if (m_running) {
        // 运行时：心跳和生产计数递增，温度/压力用正弦函数做周期变化，方便观察界面刷新。
        ++m_heartbeat;
        ++m_count;
        m_temperature = 62.0 + 19.0 * qSin(m_tick / 4.0);
        m_pressure = 0.33 + 0.10 * qSin(m_tick / 5.0);

        setValue("heartbeat", m_heartbeat);
        setValue("productionCount", m_count);
        setValue("temperature", m_temperature);
        setValue("pressure", m_pressure);
        // 这里是最简单的报警规则：温度或压力超过阈值就报警。
        setAlarm(m_temperature >= 78.0 || m_pressure >= 0.42);
    } else {
        // 停止时：让温度和压力逐步回落到初始值，界面仍能看到数据变化。
        m_temperature = qMax(26.0, m_temperature - 2.0);
        m_pressure = qMax(0.20, m_pressure - 0.02);

        setValue("temperature", m_temperature);
        setValue("pressure", m_pressure);
    }
}

void Simulator::addVariable(const VariableRow &row)
{
    m_variables.append(row);
}

void Simulator::setValue(const QString &id, const QVariant &value)
{
    // 统一从这里修改变量值。只有值真的变化时才发信号，避免界面重复刷新。
    for (int i = 0; i < m_variables.size(); ++i) {
        if (m_variables[i].id == id) {
            if (m_variables[i].value == value) {
                return;
            }
            m_variables[i].value = value;
            emit variableChanged(id, value);
            return;
        }
    }
}

QVariant Simulator::value(const QString &id) const
{
    for (int i = 0; i < m_variables.size(); ++i) {
        if (m_variables.at(i).id == id) {
            return m_variables.at(i).value;
        }
    }
    return QVariant();
}

void Simulator::setAlarm(bool active)
{
    // 报警状态不变时直接返回，避免日志每秒重复刷屏。
    if (m_alarm == active) {
        return;
    }

    m_alarm = active;
    setValue("alarmActive", active);

    if (active) {
        emit logMessage(QString::fromUtf8("报警触发：温度或压力超过阈值。"));
    } else {
        emit logMessage(QString::fromUtf8("报警解除：温度和压力恢复到阈值内。"));
    }
}

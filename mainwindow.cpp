#include "mainwindow.h"

#include <QDateTime>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>

MainWindow::MainWindow(Simulator *simulator, QWidget *parent)
    : QWidget(parent)
    , m_simulator(simulator)
{
    setWindowTitle(QString::fromUtf8("Qt + OPC UA + PLC 最小模拟示例"));
    resize(1000, 650);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    // 主界面从上到下依次是：状态区、控制区、变量表、日志区。
    mainLayout->addWidget(createStatusBox());
    mainLayout->addWidget(createControlBox());
    mainLayout->addWidget(createTableBox(), 1);
    mainLayout->addWidget(createLogBox());

    loadVariables();

    // 信号槽连接：按钮点击 -> Simulator；Simulator 数据变化 -> MainWindow 刷新界面。
    connect(m_startButton, &QPushButton::clicked, m_simulator, &Simulator::start);
    connect(m_stopButton, &QPushButton::clicked, m_simulator, &Simulator::stop);
    connect(m_simulator, &Simulator::variableChanged, this, &MainWindow::updateVariable);
    connect(m_simulator, &Simulator::logMessage, this, &MainWindow::appendLog);

    appendLog(QString::fromUtf8("界面启动完成。当前只是模拟数据，不连接真实 PLC。"));
}

void MainWindow::updateVariable(const QString &id, const QVariant &value)
{
    // 先更新变量表中的“当前值”列。
    const int row = m_rowById.value(id, -1);
    if (row >= 0 && m_table->item(row, 3)) {
        m_table->item(row, 3)->setText(displayValue(value));
    }

    // 再更新顶部状态栏中的关键值。
    updateStatusLabels(id, value);
}

void MainWindow::appendLog(const QString &text)
{
    const QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    m_logView->append(QString("[%1] %2").arg(time, text));
}

QWidget *MainWindow::createStatusBox()
{
    // 顶部状态区只放最关键的运行信息，便于演示时一眼看到联动效果。
    QGroupBox *box = new QGroupBox(QString::fromUtf8("运行状态"), this);
    QHBoxLayout *layout = new QHBoxLayout(box);

    m_runStateLabel = new QLabel(this);
    m_alarmLabel = new QLabel(this);
    m_temperatureLabel = new QLabel("26.00 degC", this);
    m_pressureLabel = new QLabel("0.20 MPa", this);
    m_heartbeatLabel = new QLabel("0", this);
    m_countLabel = new QLabel("0", this);

    setBadge(m_runStateLabel, QString::fromUtf8("停止"), "#777777");
    setBadge(m_alarmLabel, QString::fromUtf8("正常"), "#1f7a3f");

    layout->addWidget(new QLabel(QString::fromUtf8("设备:"), this));
    layout->addWidget(m_runStateLabel);
    layout->addSpacing(15);
    layout->addWidget(new QLabel(QString::fromUtf8("报警:"), this));
    layout->addWidget(m_alarmLabel);
    layout->addSpacing(15);
    layout->addWidget(new QLabel(QString::fromUtf8("温度:"), this));
    layout->addWidget(m_temperatureLabel);
    layout->addSpacing(15);
    layout->addWidget(new QLabel(QString::fromUtf8("压力:"), this));
    layout->addWidget(m_pressureLabel);
    layout->addSpacing(15);
    layout->addWidget(new QLabel(QString::fromUtf8("心跳:"), this));
    layout->addWidget(m_heartbeatLabel);
    layout->addSpacing(15);
    layout->addWidget(new QLabel(QString::fromUtf8("计数:"), this));
    layout->addWidget(m_countLabel);
    layout->addStretch();

    return box;
}

QWidget *MainWindow::createControlBox()
{
    // 控制区目前只有启动/停止。后续如果增加复位、急停，也可以放在这里。
    QGroupBox *box = new QGroupBox(QString::fromUtf8("控制命令"), this);
    QHBoxLayout *layout = new QHBoxLayout(box);

    m_startButton = new QPushButton(QString::fromUtf8("启动"), this);
    m_stopButton = new QPushButton(QString::fromUtf8("停止"), this);
    m_startButton->setMinimumHeight(34);
    m_stopButton->setMinimumHeight(34);

    layout->addWidget(m_startButton);
    layout->addWidget(m_stopButton);
    layout->addStretch();
    layout->addWidget(new QLabel(QString::fromUtf8("当前只是模拟数据，不连接真实 PLC。"), this));

    return box;
}

QWidget *MainWindow::createTableBox()
{
    // 变量表用于“先把变量预留好”。真实 PLC 接入后，主要替换 OPC UA NodeId。
    QGroupBox *box = new QGroupBox(QString::fromUtf8("变量预留表"), this);
    QVBoxLayout *layout = new QVBoxLayout(box);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(8);
    m_table->setHorizontalHeaderLabels(QStringList()
                                       << "ID"
                                       << QString::fromUtf8("名称")
                                       << QString::fromUtf8("类型")
                                       << QString::fromUtf8("当前值")
                                       << QString::fromUtf8("单位")
                                       << QString::fromUtf8("读写")
                                       << "OPC UA NodeId"
                                       << QString::fromUtf8("说明"));
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(7, QHeaderView::Stretch);

    layout->addWidget(m_table);
    return box;
}

QWidget *MainWindow::createLogBox()
{
    QGroupBox *box = new QGroupBox(QString::fromUtf8("运行日志"), this);
    QVBoxLayout *layout = new QVBoxLayout(box);

    m_logView = new QTextEdit(this);
    m_logView->setReadOnly(true);
    m_logView->setMinimumHeight(120);

    layout->addWidget(m_logView);
    return box;
}

void MainWindow::loadVariables()
{
    // 程序启动时，从 Simulator 读取变量清单，并把每个变量填入表格。
    const QVector<VariableRow> variables = m_simulator->variables();
    m_table->setRowCount(variables.size());

    for (int row = 0; row < variables.size(); ++row) {
        const VariableRow &v = variables.at(row);
        m_rowById.insert(v.id, row);

        QStringList columns;
        columns << v.id
                << v.name
                << v.type
                << displayValue(v.value)
                << v.unit
                << v.access
                << v.nodeId
                << v.note;

        for (int column = 0; column < columns.size(); ++column) {
            m_table->setItem(row, column, new QTableWidgetItem(columns.at(column)));
        }

        updateStatusLabels(v.id, v.value);
    }
}

void MainWindow::updateStatusLabels(const QString &id, const QVariant &value)
{
    // 顶部状态栏只展示少数关键变量，其余变量仍然在表格中展示。
    if (id == "runState") {
        setBadge(m_runStateLabel, value.toBool() ? QString::fromUtf8("运行") : QString::fromUtf8("停止"),
                 value.toBool() ? "#146c94" : "#777777");
    } else if (id == "alarmActive") {
        setBadge(m_alarmLabel, value.toBool() ? QString::fromUtf8("报警") : QString::fromUtf8("正常"),
                 value.toBool() ? "#b00020" : "#1f7a3f");
    } else if (id == "temperature") {
        m_temperatureLabel->setText(QString("%1 degC").arg(value.toDouble(), 0, 'f', 2));
    } else if (id == "pressure") {
        m_pressureLabel->setText(QString("%1 MPa").arg(value.toDouble(), 0, 'f', 2));
    } else if (id == "heartbeat") {
        m_heartbeatLabel->setText(QString::number(value.toInt()));
    } else if (id == "productionCount") {
        m_countLabel->setText(QString::number(value.toInt()));
    }
}

QString MainWindow::displayValue(const QVariant &value) const
{
    // QVariant 可以保存 bool/int/double/QString 等不同类型，这里统一转成表格可显示的文字。
    if (value.type() == QVariant::Bool) {
        return value.toBool() ? "true" : "false";
    }
    if (value.type() == QVariant::Double) {
        return QString::number(value.toDouble(), 'f', 2);
    }
    return value.toString();
}

void MainWindow::setBadge(QLabel *label, const QString &text, const QString &backgroundColor)
{
    // 给运行状态和报警状态做简单的彩色标签，便于观察状态变化。
    label->setText(text);
    label->setMinimumWidth(52);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet(QString("QLabel { color: white; background: %1; border-radius: 4px; padding: 3px 8px; }")
                         .arg(backgroundColor));
}

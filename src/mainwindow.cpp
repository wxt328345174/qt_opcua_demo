#include "mainwindow.h"

#include <QDateTime>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>

MainWindow::MainWindow(IDataBackend *backend, QWidget *parent)
    : QMainWindow(parent)
    , m_backend(backend)
{
    setWindowTitle(QString::fromUtf8("Qt OPC UA PLC 最小示例"));
    resize(1120, 720);

    QWidget *central = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(14, 14, 14, 14);
    mainLayout->setSpacing(12);

    mainLayout->addWidget(createStatusPanel());
    mainLayout->addWidget(createButtonPanel());
    mainLayout->addWidget(createVariableTable(), 1);
    mainLayout->addWidget(createLogPanel());

    setCentralWidget(central);

    loadVariables();

    connect(m_startButton, &QPushButton::clicked, this, &MainWindow::startRequested);
    connect(m_stopButton, &QPushButton::clicked, this, &MainWindow::stopRequested);
    connect(m_backend, &IDataBackend::variableChanged, this, &MainWindow::updateVariable);
    connect(m_backend, &IDataBackend::logMessage, this, &MainWindow::appendLog);

    appendLog(QString::fromUtf8("界面已就绪。OPC UA NodeId 当前为占位字段，等待后续映射 PLC 变量。"));
}

void MainWindow::startRequested()
{
    m_backend->writeVariable("cmdStart", true);
}

void MainWindow::stopRequested()
{
    m_backend->writeVariable("cmdStop", true);
}

void MainWindow::updateVariable(const QString &id, const QVariant &value)
{
    const int row = m_rowById.value(id, -1);
    if (row >= 0) {
        QTableWidgetItem *item = m_table->item(row, 3);
        if (item) {
            item->setText(displayValue(value));
        }
    }

    updateSummary(id, value);
}

void MainWindow::appendLog(const QString &message)
{
    const QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    m_logView->append(QString("[%1] %2").arg(timestamp, message));
}

QWidget *MainWindow::createStatusPanel()
{
    QGroupBox *group = new QGroupBox(QString::fromUtf8("运行状态"), this);
    QHBoxLayout *layout = new QHBoxLayout(group);
    layout->setSpacing(16);

    m_runStateLabel = new QLabel(this);
    m_alarmLabel = new QLabel(this);
    m_heartbeatLabel = new QLabel("0", this);
    m_countLabel = new QLabel("0", this);

    setStateLabel(m_runStateLabel, QString::fromUtf8("停止"), "#777777");
    setStateLabel(m_alarmLabel, QString::fromUtf8("正常"), "#1f7a3f");

    layout->addWidget(new QLabel(QString::fromUtf8("设备状态:"), this));
    layout->addWidget(m_runStateLabel);
    layout->addSpacing(18);
    layout->addWidget(new QLabel(QString::fromUtf8("报警:"), this));
    layout->addWidget(m_alarmLabel);
    layout->addSpacing(18);
    layout->addWidget(new QLabel(QString::fromUtf8("心跳:"), this));
    layout->addWidget(m_heartbeatLabel);
    layout->addSpacing(18);
    layout->addWidget(new QLabel(QString::fromUtf8("生产计数:"), this));
    layout->addWidget(m_countLabel);
    layout->addStretch();

    return group;
}

QWidget *MainWindow::createButtonPanel()
{
    QGroupBox *group = new QGroupBox(QString::fromUtf8("控制命令"), this);
    QHBoxLayout *layout = new QHBoxLayout(group);

    m_startButton = new QPushButton(QString::fromUtf8("启动"), this);
    m_stopButton = new QPushButton(QString::fromUtf8("停止"), this);
    m_startButton->setMinimumHeight(36);
    m_stopButton->setMinimumHeight(36);

    layout->addWidget(m_startButton);
    layout->addWidget(m_stopButton);
    layout->addStretch();
    layout->addWidget(new QLabel(QString::fromUtf8("说明：当前仅写入模拟后端，不连接真实 PLC。"), this));

    return group;
}

QWidget *MainWindow::createVariableTable()
{
    QGroupBox *group = new QGroupBox(QString::fromUtf8("PLC / OPC UA 变量预留表"), this);
    QVBoxLayout *layout = new QVBoxLayout(group);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(8);
    m_table->setHorizontalHeaderLabels({
        "ID",
        QString::fromUtf8("显示名"),
        QString::fromUtf8("类型"),
        QString::fromUtf8("当前值"),
        QString::fromUtf8("单位"),
        QString::fromUtf8("读写"),
        "OPC UA NodeId",
        QString::fromUtf8("说明")
    });
    m_table->setAlternatingRowColors(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(7, QHeaderView::Stretch);

    layout->addWidget(m_table);
    return group;
}

QWidget *MainWindow::createLogPanel()
{
    QGroupBox *group = new QGroupBox(QString::fromUtf8("运行日志"), this);
    QVBoxLayout *layout = new QVBoxLayout(group);

    m_logView = new QTextEdit(this);
    m_logView->setReadOnly(true);
    m_logView->setMinimumHeight(130);
    layout->addWidget(m_logView);

    return group;
}

void MainWindow::loadVariables()
{
    const QVector<VariableInfo> variables = m_backend->variables();
    m_table->setRowCount(variables.size());

    for (int row = 0; row < variables.size(); ++row) {
        const VariableInfo &variable = variables.at(row);
        m_rowById.insert(variable.id, row);

        const QStringList columns = {
            variable.id,
            variable.displayName,
            variable.dataType,
            displayValue(variable.value),
            variable.unit,
            variable.access,
            variable.opcUaNodeId,
            variable.description
        };

        for (int column = 0; column < columns.size(); ++column) {
            QTableWidgetItem *item = new QTableWidgetItem(columns.at(column));
            if (column == 3) {
                item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            }
            m_table->setItem(row, column, item);
        }

        updateSummary(variable.id, variable.value);
    }
}

void MainWindow::updateSummary(const QString &id, const QVariant &value)
{
    if (id == "runState") {
        if (value.toBool()) {
            setStateLabel(m_runStateLabel, QString::fromUtf8("运行"), "#146c94");
        } else {
            setStateLabel(m_runStateLabel, QString::fromUtf8("停止"), "#777777");
        }
    } else if (id == "alarmActive") {
        if (value.toBool()) {
            setStateLabel(m_alarmLabel, QString::fromUtf8("报警"), "#b00020");
        } else {
            setStateLabel(m_alarmLabel, QString::fromUtf8("正常"), "#1f7a3f");
        }
    } else if (id == "heartbeat") {
        m_heartbeatLabel->setText(displayValue(value));
    } else if (id == "productionCount") {
        m_countLabel->setText(displayValue(value));
    }
}

QString MainWindow::displayValue(const QVariant &value) const
{
    switch (value.type()) {
    case QVariant::Bool:
        return value.toBool() ? "true" : "false";
    case QVariant::Double:
        return QString::number(value.toDouble(), 'f', 2);
    case QVariant::Int:
    case QVariant::LongLong:
    case QVariant::UInt:
    case QVariant::ULongLong:
        return QString::number(value.toLongLong());
    default:
        return value.toString();
    }
}

void MainWindow::setStateLabel(QLabel *label, const QString &text, const QString &color)
{
    label->setText(text);
    label->setMinimumWidth(56);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet(QString("QLabel { color: white; background: %1; border-radius: 4px; padding: 4px 10px; }").arg(color));
}

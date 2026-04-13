#include "mainwindow.h"

#include <QAbstractItemView>
#include <QDateTime>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextEdit>
#include <QVBoxLayout>

MainWindow::MainWindow(Simulator *simulator, QWidget *parent)
    : QWidget(parent)
    , m_simulator(simulator)
{
    setWindowTitle(QString::fromUtf8("Qt + OPC UA + PLC 读写变量模拟示例"));
    resize(1180, 780);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    // 主界面从上到下依次是：状态区、控制区、读取表、写入表、日志区。
    mainLayout->addWidget(createStatusBox());
    mainLayout->addWidget(createControlBox());
    mainLayout->addWidget(createReadTableBox(), 1);
    mainLayout->addWidget(createWriteTableBox(), 1);
    mainLayout->addWidget(createLogBox());

    loadVariables();

    // 信号槽连接：按钮点击 -> Simulator；Simulator 数据变化 -> MainWindow 刷新界面。
    connect(m_startButton, &QPushButton::clicked, m_simulator, &Simulator::start);
    connect(m_stopButton, &QPushButton::clicked, m_simulator, &Simulator::stop);
    connect(m_writeSelectedButton, &QPushButton::clicked, this, &MainWindow::writeSelectedValue);
    connect(m_writeAllButton, &QPushButton::clicked, this, &MainWindow::writeAllValues);
    connect(m_simulator, &Simulator::variableChanged, this, &MainWindow::updateVariable);
    connect(m_simulator, &Simulator::logMessage, this, &MainWindow::appendLog);

    appendLog(QString::fromUtf8("界面启动完成。当前只是模拟数据，不连接真实 PLC。"));
}

void MainWindow::updateVariable(const QString &id, const QVariant &value)
{
    // 先更新读取表中的“当前值”列。
    const int readRow = m_readRowById.value(id, -1);
    if (readRow >= 0 && m_readTable->item(readRow, 3)) {
        m_readTable->item(readRow, 3)->setText(displayValue(value));
    }

    // 再更新写入表中的“当前值”列。写入成功后会用规范化后的值覆盖用户输入。
    const int writeRow = m_writeRowById.value(id, -1);
    if (writeRow >= 0 && m_writeTable->item(writeRow, 3)) {
        m_writeTable->item(writeRow, 3)->setText(displayValue(value));
    }

    // 最后更新顶部状态栏中的关键值。
    updateStatusLabels(id, value);
}

void MainWindow::appendLog(const QString &text)
{
    const QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    m_logView->append(QString("[%1] %2").arg(time, text));
}

void MainWindow::writeSelectedValue()
{
    const int row = m_writeTable->currentRow();
    if (row < 0) {
        appendLog(QString::fromUtf8("写入失败：请先在写入数据表中选择一行。"));
        return;
    }

    writeRow(row);
}

void MainWindow::writeAllValues()
{
    int successCount = 0;
    for (int row = 0; row < m_writeTable->rowCount(); ++row) {
        if (writeRow(row)) {
            ++successCount;
        }
    }
    appendLog(QString::fromUtf8("写入全部完成：成功 %1 项，共 %2 项。").arg(successCount).arg(m_writeTable->rowCount()));
}

QWidget *MainWindow::createStatusBox()
{
    // 顶部状态区只放最关键的运行信息，便于演示时一眼看到联动效果。
    QGroupBox *box = new QGroupBox(QString::fromUtf8("运行状态"), this);
    QHBoxLayout *layout = new QHBoxLayout(box);

    m_runStateLabel = new QLabel(this);
    m_alarmLabel = new QLabel(this);
    m_actualTemperatureLabel = new QLabel("26.00 degC", this);
    m_setTemperatureLabel = new QLabel("60.00 degC", this);
    m_heartbeatLabel = new QLabel("0", this);

    setBadge(m_runStateLabel, QString::fromUtf8("停止"), "#777777");
    setBadge(m_alarmLabel, QString::fromUtf8("正常"), "#1f7a3f");

    layout->addWidget(new QLabel(QString::fromUtf8("设备:"), this));
    layout->addWidget(m_runStateLabel);
    layout->addSpacing(15);
    layout->addWidget(new QLabel(QString::fromUtf8("报警:"), this));
    layout->addWidget(m_alarmLabel);
    layout->addSpacing(15);
    layout->addWidget(new QLabel(QString::fromUtf8("实际温度:"), this));
    layout->addWidget(m_actualTemperatureLabel);
    layout->addSpacing(15);
    layout->addWidget(new QLabel(QString::fromUtf8("设定温度:"), this));
    layout->addWidget(m_setTemperatureLabel);
    layout->addSpacing(15);
    layout->addWidget(new QLabel(QString::fromUtf8("心跳:"), this));
    layout->addWidget(m_heartbeatLabel);
    layout->addStretch();

    return box;
}

QWidget *MainWindow::createControlBox()
{
    // 控制区保留启动/停止，并增加写入选中项和写入全部。
    QGroupBox *box = new QGroupBox(QString::fromUtf8("控制命令"), this);
    QHBoxLayout *layout = new QHBoxLayout(box);

    m_startButton = new QPushButton(QString::fromUtf8("启动"), this);
    m_stopButton = new QPushButton(QString::fromUtf8("停止"), this);
    m_writeSelectedButton = new QPushButton(QString::fromUtf8("写入选中项"), this);
    m_writeAllButton = new QPushButton(QString::fromUtf8("写入全部"), this);

    m_startButton->setMinimumHeight(34);
    m_stopButton->setMinimumHeight(34);
    m_writeSelectedButton->setMinimumHeight(34);
    m_writeAllButton->setMinimumHeight(34);

    layout->addWidget(m_startButton);
    layout->addWidget(m_stopButton);
    layout->addSpacing(18);
    layout->addWidget(m_writeSelectedButton);
    layout->addWidget(m_writeAllButton);
    layout->addStretch();
    layout->addWidget(new QLabel(QString::fromUtf8("写入数据只作用于模拟后端，暂不连接真实 PLC。"), this));

    return box;
}

QWidget *MainWindow::createReadTableBox()
{
    // 读取数据表用于展示从 PLC/OPC UA 读取到的变量。
    QGroupBox *box = new QGroupBox(QString::fromUtf8("读取数据"), this);
    QVBoxLayout *layout = new QVBoxLayout(box);

    m_readTable = new QTableWidget(this);
    m_readTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_readTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_readTable->setAlternatingRowColors(true);
    m_readTable->verticalHeader()->setVisible(false);
    layout->addWidget(m_readTable);

    return box;
}

QWidget *MainWindow::createWriteTableBox()
{
    // 写入数据表的“当前值”列允许编辑，点击写入按钮后交给 Simulator 校验。
    QGroupBox *box = new QGroupBox(QString::fromUtf8("写入数据"), this);
    QVBoxLayout *layout = new QVBoxLayout(box);

    m_writeTable = new QTableWidget(this);
    m_writeTable->setEditTriggers(QAbstractItemView::DoubleClicked
                                  | QAbstractItemView::SelectedClicked
                                  | QAbstractItemView::EditKeyPressed);
    m_writeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_writeTable->setAlternatingRowColors(true);
    m_writeTable->verticalHeader()->setVisible(false);
    layout->addWidget(m_writeTable);

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
    // 程序启动时，从 Simulator 读取两组变量清单，并填入对应表格。
    fillTable(m_readTable, m_simulator->readVariables(), &m_readRowById, false);
    fillTable(m_writeTable, m_simulator->writeVariables(), &m_writeRowById, true);
}

void MainWindow::fillTable(QTableWidget *table, const QVector<VariableRow> &variables, QHash<QString, int> *rowMap, bool valueEditable)
{
    table->setColumnCount(8);
    table->setHorizontalHeaderLabels(QStringList()
                                     << "ID"
                                     << QString::fromUtf8("名称")
                                     << QString::fromUtf8("类型")
                                     << QString::fromUtf8("当前值")
                                     << QString::fromUtf8("单位")
                                     << QString::fromUtf8("读写")
                                     << "OPC UA NodeId"
                                     << QString::fromUtf8("说明"));
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(7, QHeaderView::Stretch);
    table->setRowCount(variables.size());
    rowMap->clear();

    for (int row = 0; row < variables.size(); ++row) {
        const VariableRow &v = variables.at(row);
        rowMap->insert(v.id, row);

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
            QTableWidgetItem *item = new QTableWidgetItem(columns.at(column));
            const bool editableCell = valueEditable && column == 3;
            if (!editableCell) {
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            }
            table->setItem(row, column, item);
        }

        updateStatusLabels(v.id, v.value);
    }
}

bool MainWindow::writeRow(int row)
{
    if (row < 0 || row >= m_writeTable->rowCount()) {
        return false;
    }

    QTableWidgetItem *idItem = m_writeTable->item(row, 0);
    QTableWidgetItem *valueItem = m_writeTable->item(row, 3);
    if (!idItem || !valueItem) {
        appendLog(QString::fromUtf8("写入失败：写入数据表第 %1 行内容不完整。").arg(row + 1));
        return false;
    }

    QString errorMessage;
    const bool ok = m_simulator->writeValue(idItem->text(), valueItem->text(), &errorMessage);
    if (!ok && !errorMessage.isEmpty()) {
        appendLog(errorMessage);
    }
    return ok;
}

void MainWindow::updateStatusLabels(const QString &id, const QVariant &value)
{
    // 顶部状态栏只展示少数关键变量，其余变量仍然在表格中展示。
    if (id == "motorRunning") {
        setBadge(m_runStateLabel, value.toBool() ? QString::fromUtf8("运行") : QString::fromUtf8("停止"),
                 value.toBool() ? "#146c94" : "#777777");
    } else if (id == "alarmActive") {
        setBadge(m_alarmLabel, value.toBool() ? QString::fromUtf8("报警") : QString::fromUtf8("正常"),
                 value.toBool() ? "#b00020" : "#1f7a3f");
    } else if (id == "actualTemperature") {
        m_actualTemperatureLabel->setText(QString("%1 degC").arg(value.toDouble(), 0, 'f', 2));
    } else if (id == "setTemperature") {
        m_setTemperatureLabel->setText(QString("%1 degC").arg(value.toDouble(), 0, 'f', 2));
    } else if (id == "heartbeat") {
        m_heartbeatLabel->setText(QString::number(value.toInt()));
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

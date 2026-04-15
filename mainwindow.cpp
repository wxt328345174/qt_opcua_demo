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

namespace {

const char kEndpointUrl[] = "opc.tcp://192.168.0.105:4840";

}

MainWindow::MainWindow(OpcUaClient *client, QWidget *parent)
    : QWidget(parent)
    , m_client(client)
{
    setWindowTitle(QString::fromUtf8("Qt OPC UA 读写接入示例"));
    resize(1180, 780);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(createStatusBox());
    mainLayout->addWidget(createControlBox());
    mainLayout->addWidget(createReadTableBox(), 1);
    mainLayout->addWidget(createWriteTableBox(), 1);
    mainLayout->addWidget(createLogBox());

    loadVariables();

    connect(m_connectButton, &QPushButton::clicked, m_client, &OpcUaClient::connectToServer);
    connect(m_disconnectButton, &QPushButton::clicked, m_client, &OpcUaClient::disconnectFromServer);
    connect(m_writeSelectedButton, &QPushButton::clicked, this, &MainWindow::writeSelectedValue);
    connect(m_writeAllButton, &QPushButton::clicked, this, &MainWindow::writeAllValues);
    connect(m_client, &OpcUaClient::variableChanged, this, &MainWindow::updateVariable);
    connect(m_client, &OpcUaClient::nodeIdResolved, this, &MainWindow::updateNodeId);
    connect(m_client, &OpcUaClient::connectionStateChanged, this, &MainWindow::updateConnectionState);
    connect(m_client, &OpcUaClient::refreshTimestampChanged, this, &MainWindow::updateRefreshTime);
    connect(m_client, &OpcUaClient::resolvedCountChanged, this, &MainWindow::updateResolvedCount);
    connect(m_client, &OpcUaClient::logMessage, this, &MainWindow::appendLog);

    updateConnectionState(QString::fromUtf8("未连接"));
    updateResolvedCount(0, m_client->readVariables().size() + m_client->writeVariables().size());
    updateRefreshTime(QDateTime());
    appendLog(QString::fromUtf8("界面已加载。group1 用于读取，group_2 用于写入。"));
}

void MainWindow::updateVariable(const QString &id, const QVariant &value)
{
    const int readRow = m_readRowById.value(id, -1);
    if (readRow >= 0 && m_readTable->item(readRow, 3)) {
        m_readTable->item(readRow, 3)->setText(displayValue(value));
    }

    const int writeRow = m_writeRowById.value(id, -1);
    if (writeRow >= 0 && m_writeTable->item(writeRow, 3)) {
        m_writeTable->item(writeRow, 3)->setText(displayValue(value));
    }
}

void MainWindow::updateNodeId(const QString &id, const QString &nodeIdText)
{
    const int readRow = m_readRowById.value(id, -1);
    if (readRow >= 0 && m_readTable->item(readRow, 6)) {
        m_readTable->item(readRow, 6)->setText(nodeIdText);
    }

    const int writeRow = m_writeRowById.value(id, -1);
    if (writeRow >= 0 && m_writeTable->item(writeRow, 7)) {
        m_writeTable->item(writeRow, 7)->setText(nodeIdText);
    }
}

void MainWindow::updateConnectionState(const QString &stateText)
{
    if (stateText == QString::fromUtf8("已连接")) {
        setBadge(m_connectionStateLabel, stateText, "#1f7a3f");
        setWriteControlsEnabled(true);
    } else if (stateText == QString::fromUtf8("连接中")) {
        setBadge(m_connectionStateLabel, stateText, "#146c94");
        setWriteControlsEnabled(false);
    } else {
        setBadge(m_connectionStateLabel, stateText, "#666666");
        setWriteControlsEnabled(false);
    }
}

void MainWindow::updateRefreshTime(const QDateTime &timestamp)
{
    if (!timestamp.isValid()) {
        m_lastRefreshLabel->setText(QString::fromUtf8("未刷新"));
        return;
    }

    m_lastRefreshLabel->setText(timestamp.toString("yyyy-MM-dd HH:mm:ss"));
}

void MainWindow::updateResolvedCount(int resolvedCount, int totalCount)
{
    m_bindingCountLabel->setText(QString::fromUtf8("%1 / %2").arg(resolvedCount).arg(totalCount));
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
        appendLog(QString::fromUtf8("写入失败：请先选择写入表中的一行。"));
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
    appendLog(QString::fromUtf8("写入全部完成：成功 %1 项，共 %2 项。")
              .arg(successCount).arg(m_writeTable->rowCount()));
}

QWidget *MainWindow::createStatusBox()
{
    QGroupBox *box = new QGroupBox(QString::fromUtf8("连接状态"), this);
    QHBoxLayout *layout = new QHBoxLayout(box);

    m_connectionStateLabel = new QLabel(this);
    m_endpointLabel = new QLabel(QString::fromLatin1(kEndpointUrl), this);
    m_bindingCountLabel = new QLabel("0 / 10", this);
    m_lastRefreshLabel = new QLabel(QString::fromUtf8("未刷新"), this);

    setBadge(m_connectionStateLabel, QString::fromUtf8("未连接"), "#666666");

    layout->addWidget(new QLabel(QString::fromUtf8("OPC UA:"), this));
    layout->addWidget(m_connectionStateLabel);
    layout->addSpacing(16);
    layout->addWidget(new QLabel(QString::fromUtf8("Endpoint:"), this));
    layout->addWidget(m_endpointLabel, 1);
    layout->addSpacing(16);
    layout->addWidget(new QLabel(QString::fromUtf8("已配置点位:"), this));
    layout->addWidget(m_bindingCountLabel);
    layout->addSpacing(16);
    layout->addWidget(new QLabel(QString::fromUtf8("最近刷新:"), this));
    layout->addWidget(m_lastRefreshLabel);

    return box;
}

QWidget *MainWindow::createControlBox()
{
    QGroupBox *box = new QGroupBox(QString::fromUtf8("控制"), this);
    QHBoxLayout *layout = new QHBoxLayout(box);

    m_connectButton = new QPushButton(QString::fromUtf8("连接"), this);
    m_disconnectButton = new QPushButton(QString::fromUtf8("断开"), this);
    m_writeSelectedButton = new QPushButton(QString::fromUtf8("写入选中项"), this);
    m_writeAllButton = new QPushButton(QString::fromUtf8("写入全部"), this);

    m_connectButton->setMinimumHeight(34);
    m_disconnectButton->setMinimumHeight(34);
    m_writeSelectedButton->setMinimumHeight(34);
    m_writeAllButton->setMinimumHeight(34);

    layout->addWidget(m_connectButton);
    layout->addWidget(m_disconnectButton);
    layout->addSpacing(18);
    layout->addWidget(m_writeSelectedButton);
    layout->addWidget(m_writeAllButton);
    layout->addStretch();
    layout->addWidget(new QLabel(QString::fromUtf8("数组和结构体请输入 JSON，例如 [10,20,30] 或 {\"A\":10,\"B\":10.8,\"C\":true}。"), this));

    return box;
}

QWidget *MainWindow::createReadTableBox()
{
    QGroupBox *box = new QGroupBox(QString::fromUtf8("读取数据：group1"), this);
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
    QGroupBox *box = new QGroupBox(QString::fromUtf8("写入数据：group_2"), this);
    QVBoxLayout *layout = new QVBoxLayout(box);

    QLabel *hintLabel = new QLabel(QString::fromUtf8("“读取值”来自设备回读；只编辑“设定值”列，连接成功后可写入设备。"), this);
    m_writeTable = new QTableWidget(this);
    m_writeTable->setEditTriggers(QAbstractItemView::DoubleClicked
                                  | QAbstractItemView::SelectedClicked
                                  | QAbstractItemView::EditKeyPressed);
    m_writeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_writeTable->setAlternatingRowColors(true);
    m_writeTable->verticalHeader()->setVisible(false);

    layout->addWidget(hintLabel);
    layout->addWidget(m_writeTable);
    return box;
}

QWidget *MainWindow::createLogBox()
{
    QGroupBox *box = new QGroupBox(QString::fromUtf8("运行日志"), this);
    QVBoxLayout *layout = new QVBoxLayout(box);

    m_logView = new QTextEdit(this);
    m_logView->setReadOnly(true);
    m_logView->setMinimumHeight(140);

    layout->addWidget(m_logView);
    return box;
}

void MainWindow::loadVariables()
{
    fillTable(m_readTable, m_client->readVariables(), &m_readRowById, false);
    fillTable(m_writeTable, m_client->writeVariables(), &m_writeRowById, true);
}

void MainWindow::fillTable(QTableWidget *table, const QVector<VariableRow> &variables, QHash<QString, int> *rowMap, bool valueEditable)
{
    if (valueEditable) {
        table->setColumnCount(9);
        table->setHorizontalHeaderLabels(QStringList()
                                         << "ID"
                                         << QString::fromUtf8("名称")
                                         << QString::fromUtf8("类型")
                                         << QString::fromUtf8("读取值")
                                         << QString::fromUtf8("设定值")
                                         << QString::fromUtf8("单位")
                                         << QString::fromUtf8("读写")
                                         << "OPC UA NodeId"
                                         << QString::fromUtf8("说明"));
    } else {
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
    }
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(valueEditable ? 8 : 7, QHeaderView::Stretch);
    table->setRowCount(variables.size());
    rowMap->clear();

    for (int row = 0; row < variables.size(); ++row) {
        const VariableRow &variable = variables.at(row);
        rowMap->insert(variable.id, row);

        QStringList columns;
        if (valueEditable) {
            columns << variable.id
                    << variable.name
                    << variable.type
                    << displayValue(variable.value)
                    << displayValue(variable.value)
                    << variable.unit
                    << variable.access
                    << variable.nodeId
                    << variable.note;
        } else {
            columns << variable.id
                    << variable.name
                    << variable.type
                    << displayValue(variable.value)
                    << variable.unit
                    << variable.access
                    << variable.nodeId
                    << variable.note;
        }

        for (int column = 0; column < columns.size(); ++column) {
            QTableWidgetItem *item = new QTableWidgetItem(columns.at(column));
            if (!(valueEditable && column == 4)) {
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            }
            table->setItem(row, column, item);
        }
    }
}

bool MainWindow::writeRow(int row)
{
    if (row < 0 || row >= m_writeTable->rowCount()) {
        return false;
    }

    QTableWidgetItem *idItem = m_writeTable->item(row, 0);
    QTableWidgetItem *valueItem = m_writeTable->item(row, 4);
    if (!idItem || !valueItem) {
        appendLog(QString::fromUtf8("写入失败：第 %1 行内容不完整。").arg(row + 1));
        return false;
    }

    QString errorMessage;
    if (!m_client->writeValue(idItem->text(), valueItem->text(), &errorMessage)) {
        appendLog(errorMessage);
        return false;
    }

    return true;
}

QString MainWindow::displayValue(const QVariant &value) const
{
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
    label->setText(text);
    label->setMinimumWidth(72);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet(QString::fromLatin1(
                             "QLabel { color: white; background: %1; border-radius: 4px; padding: 3px 10px; }")
                         .arg(backgroundColor));
}

void MainWindow::setWriteControlsEnabled(bool enabled)
{
    m_writeSelectedButton->setEnabled(enabled);
    m_writeAllButton->setEnabled(enabled);
    m_writeTable->setEnabled(enabled);
}

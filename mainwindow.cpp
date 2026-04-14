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
    setWindowTitle(QString::fromUtf8("Qt OPC UA 只读接入示例"));
    resize(1180, 760);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(createStatusBox());
    mainLayout->addWidget(createControlBox());
    mainLayout->addWidget(createReadTableBox(), 1);
    mainLayout->addWidget(createWriteTableBox());
    mainLayout->addWidget(createLogBox());

    loadVariables();

    connect(m_connectButton, &QPushButton::clicked, m_client, &OpcUaClient::connectToServer);
    connect(m_disconnectButton, &QPushButton::clicked, m_client, &OpcUaClient::disconnectFromServer);
    connect(m_client, &OpcUaClient::variableChanged, this, &MainWindow::updateVariable);
    connect(m_client, &OpcUaClient::nodeIdResolved, this, &MainWindow::updateNodeId);
    connect(m_client, &OpcUaClient::connectionStateChanged, this, &MainWindow::updateConnectionState);
    connect(m_client, &OpcUaClient::refreshTimestampChanged, this, &MainWindow::updateRefreshTime);
    connect(m_client, &OpcUaClient::resolvedCountChanged, this, &MainWindow::updateResolvedCount);
    connect(m_client, &OpcUaClient::logMessage, this, &MainWindow::appendLog);

    updateConnectionState(QString::fromUtf8("未连接"));
    updateResolvedCount(0, m_client->readVariables().size());
    updateRefreshTime(QDateTime());
    appendLog(QString::fromUtf8("界面已加载。点击“连接”后按固定 NodeId 读取真实 OPC UA 数据。"));
}

void MainWindow::updateVariable(const QString &id, const QVariant &value)
{
    const int row = m_readRowById.value(id, -1);
    if (row >= 0 && m_readTable->item(row, 3)) {
        m_readTable->item(row, 3)->setText(displayValue(value));
    }
}

void MainWindow::updateNodeId(const QString &id, const QString &nodeIdText)
{
    const int row = m_readRowById.value(id, -1);
    if (row >= 0 && m_readTable->item(row, 6)) {
        m_readTable->item(row, 6)->setText(nodeIdText);
    }
}

void MainWindow::updateConnectionState(const QString &stateText)
{
    if (stateText == QString::fromUtf8("已连接")) {
        setBadge(m_connectionStateLabel, stateText, "#1f7a3f");
    } else if (stateText == QString::fromUtf8("连接中")) {
        setBadge(m_connectionStateLabel, stateText, "#146c94");
    } else {
        setBadge(m_connectionStateLabel, stateText, "#666666");
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

QWidget *MainWindow::createStatusBox()
{
    QGroupBox *box = new QGroupBox(QString::fromUtf8("连接状态"), this);
    QHBoxLayout *layout = new QHBoxLayout(box);

    m_connectionStateLabel = new QLabel(this);
    m_endpointLabel = new QLabel(QString::fromLatin1(kEndpointUrl), this);
    m_bindingCountLabel = new QLabel("0 / 4", this);
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
    m_writeSelectedButton->setEnabled(false);
    m_writeAllButton->setEnabled(false);

    layout->addWidget(m_connectButton);
    layout->addWidget(m_disconnectButton);
    layout->addSpacing(18);
    layout->addWidget(m_writeSelectedButton);
    layout->addWidget(m_writeAllButton);
    layout->addStretch();
    layout->addWidget(new QLabel(QString::fromUtf8("本次仅接入真实读取，写入区保留但不向设备发送数据。"), this));

    return box;
}

QWidget *MainWindow::createReadTableBox()
{
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
    QGroupBox *box = new QGroupBox(QString::fromUtf8("写入数据（未接入）"), this);
    QVBoxLayout *layout = new QVBoxLayout(box);

    QLabel *hintLabel = new QLabel(QString::fromUtf8("真实设备写入点位尚未配置，当前区域仅保留界面占位。"), this);
    m_writeTable = new QTableWidget(this);
    m_writeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_writeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_writeTable->setAlternatingRowColors(true);
    m_writeTable->verticalHeader()->setVisible(false);
    m_writeTable->setEnabled(false);

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

QVector<VariableRow> MainWindow::createWritePlaceholders() const
{
    QVector<VariableRow> rows;
    rows.append({"--", QString::fromUtf8("真实写入未接入"), "-", "-", "", QString::fromUtf8("禁用"),
                 QString::fromUtf8("未配置"), QString::fromUtf8("本次版本只接入 OPC UA 读取，不向设备写入。")});
    return rows;
}

void MainWindow::loadVariables()
{
    fillTable(m_readTable, m_client->readVariables(), &m_readRowById);
    fillTable(m_writeTable, createWritePlaceholders(), &m_writeRowById);
}

void MainWindow::fillTable(QTableWidget *table, const QVector<VariableRow> &variables, QHash<QString, int> *rowMap)
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
        const VariableRow &variable = variables.at(row);
        rowMap->insert(variable.id, row);

        const QStringList columns = QStringList()
                << variable.id
                << variable.name
                << variable.type
                << displayValue(variable.value)
                << variable.unit
                << variable.access
                << variable.nodeId
                << variable.note;

        for (int column = 0; column < columns.size(); ++column) {
            QTableWidgetItem *item = new QTableWidgetItem(columns.at(column));
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            table->setItem(row, column, item);
        }
    }
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

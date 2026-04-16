#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "opcua_client.h"
#include "variable_row.h"

#include <QHash>
#include <QWidget>

class QLabel;
class QPushButton;
class QTableWidget;
class QTextEdit;

// 界面层：负责创建 Qt Widgets、响应按钮操作，并通过信号槽接收客户端数据。
// 它不直接调用 open62541 API，只通过 OpcUaClient 读写变量。
class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(OpcUaClient *client, QWidget *parent = nullptr);

private slots:
    // 客户端信号进入界面后的刷新入口。
    void updateVariable(const QString &id, const QVariant &value);
    void updateNodeId(const QString &id, const QString &nodeIdText);
    void updateConnectionState(const QString &stateText);
    void updateRefreshTime(const QDateTime &timestamp);
    void updateResolvedCount(int resolvedCount, int totalCount);
    void appendLog(const QString &text);
    void writeSelectedValue();
    void writeAllValues();

private:
    // 界面搭建被拆成几个区域，可按状态栏、控制区、读表、写表、日志区理解。
    QWidget *createStatusBox();
    QWidget *createControlBox();
    QWidget *createReadTableBox();
    QWidget *createWriteTableBox();
    QWidget *createLogBox();

    // 表格数据装载与写入动作：将 VariableRow 转成 QTableWidgetItem。
    void loadVariables();
    void fillTable(QTableWidget *table, const QVector<VariableRow> &variables, QHash<QString, int> *rowMap, bool valueEditable);
    bool writeRow(int row);
    QString displayValue(const QVariant &value) const;
    void setBadge(QLabel *label, const QString &text, const QString &backgroundColor);
    void setWriteControlsEnabled(bool enabled);

    OpcUaClient *m_client = nullptr;
    QLabel *m_connectionStateLabel = nullptr;
    QLabel *m_endpointLabel = nullptr;
    QLabel *m_bindingCountLabel = nullptr;
    QLabel *m_lastRefreshLabel = nullptr;
    QPushButton *m_connectButton = nullptr;
    QPushButton *m_disconnectButton = nullptr;
    QPushButton *m_writeSelectedButton = nullptr;
    QPushButton *m_writeAllButton = nullptr;
    QTableWidget *m_readTable = nullptr;
    QTableWidget *m_writeTable = nullptr;
    QTextEdit *m_logView = nullptr;
    QHash<QString, int> m_readRowById;
    QHash<QString, int> m_writeRowById;
};

#endif // MAINWINDOW_H

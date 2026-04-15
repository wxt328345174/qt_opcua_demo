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

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(OpcUaClient *client, QWidget *parent = nullptr);

private slots:
    void updateVariable(const QString &id, const QVariant &value);
    void updateNodeId(const QString &id, const QString &nodeIdText);
    void updateConnectionState(const QString &stateText);
    void updateRefreshTime(const QDateTime &timestamp);
    void updateResolvedCount(int resolvedCount, int totalCount);
    void appendLog(const QString &text);
    void writeSelectedValue();
    void writeAllValues();

private:
    QWidget *createStatusBox();
    QWidget *createControlBox();
    QWidget *createReadTableBox();
    QWidget *createWriteTableBox();
    QWidget *createLogBox();
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

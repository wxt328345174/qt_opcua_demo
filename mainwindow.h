#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "simulator.h"

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
    // MainWindow 不自己产生数据，只接收 Simulator 的信号并刷新界面。
    explicit MainWindow(Simulator *simulator, QWidget *parent = nullptr);

private slots:
    // 槽函数：接收 Simulator 发来的变量变化和日志消息。
    void updateVariable(const QString &id, const QVariant &value);
    void appendLog(const QString &text);

private:
    // 下面几个函数把界面拆成小块，便于阅读和修改。
    QWidget *createStatusBox();
    QWidget *createControlBox();
    QWidget *createTableBox();
    QWidget *createLogBox();
    void loadVariables();
    void updateStatusLabels(const QString &id, const QVariant &value);
    QString displayValue(const QVariant &value) const;
    void setBadge(QLabel *label, const QString &text, const QString &backgroundColor);

    Simulator *m_simulator = nullptr;
    QLabel *m_runStateLabel = nullptr;
    QLabel *m_alarmLabel = nullptr;
    QLabel *m_temperatureLabel = nullptr;
    QLabel *m_pressureLabel = nullptr;
    QLabel *m_heartbeatLabel = nullptr;
    QLabel *m_countLabel = nullptr;
    QPushButton *m_startButton = nullptr;
    QPushButton *m_stopButton = nullptr;
    QTableWidget *m_table = nullptr;
    QTextEdit *m_logView = nullptr;
    // 保存变量 ID 到表格行号的映射。后续变量变化时，可以快速找到要更新哪一行。
    QHash<QString, int> m_rowById;
};

#endif // MAINWINDOW_H

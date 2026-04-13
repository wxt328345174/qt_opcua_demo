#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "idatabackend.h"

#include <QHash>
#include <QMainWindow>

class QLabel;
class QPushButton;
class QTableWidget;
class QTextEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(IDataBackend *backend, QWidget *parent = nullptr);

private slots:
    void startRequested();
    void stopRequested();
    void updateVariable(const QString &id, const QVariant &value);
    void appendLog(const QString &message);

private:
    QWidget *createStatusPanel();
    QWidget *createButtonPanel();
    QWidget *createVariableTable();
    QWidget *createLogPanel();
    void loadVariables();
    void updateSummary(const QString &id, const QVariant &value);
    QString displayValue(const QVariant &value) const;
    void setStateLabel(QLabel *label, const QString &text, const QString &color);

    IDataBackend *m_backend = nullptr;
    QLabel *m_runStateLabel = nullptr;
    QLabel *m_alarmLabel = nullptr;
    QLabel *m_heartbeatLabel = nullptr;
    QLabel *m_countLabel = nullptr;
    QPushButton *m_startButton = nullptr;
    QPushButton *m_stopButton = nullptr;
    QTableWidget *m_table = nullptr;
    QTextEdit *m_logView = nullptr;
    QHash<QString, int> m_rowById;
};

#endif // MAINWINDOW_H

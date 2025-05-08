#pragma once
#include <QDialog>
#include <QProcess>
#include <QTextEdit>
#include <QPushButton>

class FlatpakProcessDialog : public QDialog {
    Q_OBJECT
public:
    FlatpakProcessDialog(const QString& command, const QString& title, QWidget* parent = nullptr);
    ~FlatpakProcessDialog();
    bool wasSuccessful() const;
    void setSuccessString(const QString& str);

signals:
    void processFinished(bool success);

private slots:
    void onReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onCancel();

private:
    QProcess* m_process;
    QTextEdit* m_outputEdit;
    QPushButton* m_cancelButton;
    bool m_success;
    QString m_successString;
}; 
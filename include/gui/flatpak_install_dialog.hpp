#pragma once
#include <QDialog>
#include <QProcess>
#include <QTextEdit>
#include <QProgressBar>
#include <QPushButton>

class FlatpakInstallDialog : public QDialog {
    Q_OBJECT
public:
    FlatpakInstallDialog(const QString& appId, const QString& remote, QWidget* parent = nullptr);
    ~FlatpakInstallDialog();
    bool wasSuccessful() const;

signals:
    void installFinished(bool success);

private slots:
    void onReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onCancel();

private:
    void parseProgress(const QString& output);
    QProcess* m_process;
    QTextEdit* m_outputEdit;
    QProgressBar* m_progressBar;
    QPushButton* m_cancelButton;
    bool m_success;
}; 
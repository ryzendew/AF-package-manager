#pragma once
#include <QDialog>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QProcess>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <qtermwidget6/qtermwidget.h>

namespace pacmangui {
namespace gui {

class InstallProgressDialog : public QDialog {
    Q_OBJECT
public:
    explicit InstallProgressDialog(const QString& title, QWidget* parent = nullptr, bool useTerminal = false);
    void startInstallProcess(const QString& program, const QStringList& arguments);
    void startInteractiveTerminal(const QString& program, const QStringList& arguments);
    void appendOutput(const QString& text);
    bool wasSuccessful() const;
    void setWatchForPasswordPrompt(bool enabled);

signals:
    void passwordPrompted();

public slots:
    void providePassword(const QString& password);

private slots:
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onCancelClicked();
    void onCloseClicked();

private:
    QPlainTextEdit* m_outputEdit;
    QPushButton* m_cancelButton;
    QPushButton* m_closeButton;
    QProcess* m_process;
    bool m_success;
    bool m_watchForPasswordPrompt;
    bool m_passwordSent;
    QTermWidget* m_termWidget;
    bool m_useTerminal;
};

} // namespace gui
} // namespace pacmangui 
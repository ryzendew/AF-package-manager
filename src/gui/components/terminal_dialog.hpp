#pragma once

#include <QDialog>
#include <QTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>
#include <QProcess>

namespace pacmangui {
namespace gui {

/**
 * @brief Dialog showing terminal output with progress
 */
class TerminalDialog : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit TerminalDialog(QWidget* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~TerminalDialog();

    /**
     * @brief Start a command and show its output
     * @param command Command to execute
     * @param workingDir Working directory (optional)
     */
    void executeCommand(const QString& command, const QString& workingDir = QString());

    /**
     * @brief Set the dialog title
     * @param title Title to set
     */
    void setDialogTitle(const QString& title);

    /**
     * @brief Set whether the dialog can be cancelled
     * @param cancellable True if cancellable
     */
    void setCancellable(bool cancellable);

    /**
     * @brief Get the process exit code
     * @return Exit code of the last command
     */
    int getExitCode() const;

    /**
     * @brief Get the complete output
     * @return All output from the command
     */
    QString getOutput() const;

signals:
    /**
     * @brief Signal emitted when command finishes
     * @param exitCode Exit code of the command
     */
    void commandFinished(int exitCode);

    /**
     * @brief Signal emitted when command outputs text
     * @param text The output text
     */
    void outputReceived(const QString& text);

    /**
     * @brief Signal emitted when command is cancelled
     */
    void commandCancelled();

public slots:
    /**
     * @brief Append text to the terminal
     * @param text Text to append
     */
    void appendOutput(const QString& text);

    /**
     * @brief Set progress value
     * @param value Progress value (0-100)
     */
    void setProgress(int value);

    /**
     * @brief Show/hide progress bar
     * @param visible True to show progress bar
     */
    void setProgressVisible(bool visible);

    /**
     * @brief Clear the terminal output
     */
    void clearOutput();

protected:
    /**
     * @brief Handle close event
     * @param event Close event
     */
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onProcessOutput();
    void onProcessError(QProcess::ProcessError error);
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onCancelClicked();

private:
    void setupUi();
    void setupConnections();
    void killProcess();

    QTextEdit* m_terminal;
    QProgressBar* m_progressBar;
    QPushButton* m_cancelButton;
    QVBoxLayout* m_layout;
    QProcess* m_process;
    bool m_cancellable;
    int m_exitCode;
};

} // namespace gui
} // namespace pacmangui 
#include "gui/components/terminal_dialog.hpp"
#include <QCloseEvent>
#include <QScrollBar>
#include <QDebug>

namespace pacmangui {
namespace gui {

TerminalDialog::TerminalDialog(QWidget* parent)
    : QDialog(parent)
    , m_terminal(nullptr)
    , m_progressBar(nullptr)
    , m_cancelButton(nullptr)
    , m_layout(nullptr)
    , m_process(nullptr)
    , m_cancellable(true)
    , m_exitCode(-1)
{
    setupUi();
    setupConnections();
    
    // Create process
    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    
    // Connect process signals
    connect(m_process, &QProcess::readyRead, this, &TerminalDialog::onProcessOutput);
    connect(m_process, &QProcess::errorOccurred, this, &TerminalDialog::onProcessError);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TerminalDialog::onProcessFinished);
}

TerminalDialog::~TerminalDialog()
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        killProcess();
    }
}

void TerminalDialog::setupUi()
{
    // Set window properties
    setWindowTitle(tr("Terminal Output"));
    setMinimumSize(600, 400);
    
    // Create layout
    m_layout = new QVBoxLayout(this);
    
    // Create terminal output
    m_terminal = new QTextEdit(this);
    m_terminal->setReadOnly(true);
    m_terminal->setFont(QFont("Monospace"));
    m_terminal->setStyleSheet(
        "QTextEdit {"
        "    background-color: #1e1e2e;"
        "    color: #ffffff;"
        "    border: 1px solid #3daee9;"
        "    border-radius: 2px;"
        "    padding: 2px;"
        "}"
    );
    m_layout->addWidget(m_terminal);
    
    // Create progress bar
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setVisible(false);
    m_progressBar->setStyleSheet(
        "QProgressBar {"
        "    border: 1px solid #3daee9;"
        "    border-radius: 2px;"
        "    text-align: center;"
        "}"
        "QProgressBar::chunk {"
        "    background-color: #3daee9;"
        "}"
    );
    m_layout->addWidget(m_progressBar);
    
    // Create cancel button
    m_cancelButton = new QPushButton(tr("Cancel"), this);
    m_cancelButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #da4453;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 2px;"
        "    padding: 5px 15px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #ed1515;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #c0392b;"
        "}"
    );
    m_layout->addWidget(m_cancelButton);
}

void TerminalDialog::setupConnections()
{
    connect(m_cancelButton, &QPushButton::clicked, this, &TerminalDialog::onCancelClicked);
}

void TerminalDialog::executeCommand(const QString& command, const QString& workingDir)
{
    // Clear previous output
    clearOutput();
    
    // Set working directory if provided
    if (!workingDir.isEmpty()) {
        m_process->setWorkingDirectory(workingDir);
    }
    
    // Show command being executed
    appendOutput(tr("Executing: %1\n").arg(command));
    
    // Start process
    m_process->start("bash", QStringList() << "-c" << command);
}

void TerminalDialog::setDialogTitle(const QString& title)
{
    setWindowTitle(title);
}

void TerminalDialog::setCancellable(bool cancellable)
{
    m_cancellable = cancellable;
    m_cancelButton->setVisible(cancellable);
}

int TerminalDialog::getExitCode() const
{
    return m_exitCode;
}

QString TerminalDialog::getOutput() const
{
    return m_terminal->toPlainText();
}

void TerminalDialog::appendOutput(const QString& text)
{
    m_terminal->append(text);
    
    // Auto-scroll to bottom
    QScrollBar* scrollbar = m_terminal->verticalScrollBar();
    scrollbar->setValue(scrollbar->maximum());
    
    // Emit signal
    emit outputReceived(text);
}

void TerminalDialog::setProgress(int value)
{
    m_progressBar->setValue(value);
}

void TerminalDialog::setProgressVisible(bool visible)
{
    m_progressBar->setVisible(visible);
}

void TerminalDialog::clearOutput()
{
    m_terminal->clear();
    setProgress(0);
}

void TerminalDialog::closeEvent(QCloseEvent* event)
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        // Ask for confirmation before closing
        if (m_cancellable) {
            killProcess();
            event->accept();
        } else {
            event->ignore();
        }
    } else {
        event->accept();
    }
}

void TerminalDialog::onProcessOutput()
{
    QByteArray output = m_process->readAll();
    appendOutput(QString::fromUtf8(output));
}

void TerminalDialog::onProcessError(QProcess::ProcessError error)
{
    QString errorText;
    switch (error) {
        case QProcess::FailedToStart:
            errorText = tr("Failed to start process");
            break;
        case QProcess::Crashed:
            errorText = tr("Process crashed");
            break;
        case QProcess::Timedout:
            errorText = tr("Process timed out");
            break;
        case QProcess::WriteError:
            errorText = tr("Write error");
            break;
        case QProcess::ReadError:
            errorText = tr("Read error");
            break;
        default:
            errorText = tr("Unknown error");
            break;
    }
    
    appendOutput(tr("\nError: %1\n").arg(errorText));
}

void TerminalDialog::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_exitCode = exitCode;
    
    QString status = (exitStatus == QProcess::NormalExit) ? tr("normally") : tr("with crash");
    appendOutput(tr("\nProcess finished %1 with exit code %2\n").arg(status).arg(exitCode));
    
    emit commandFinished(exitCode);
}

void TerminalDialog::onCancelClicked()
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        killProcess();
    }
    
    emit commandCancelled();
    close();
}

void TerminalDialog::killProcess()
{
    if (!m_process) {
        return;
    }
    
    m_process->terminate();
    if (!m_process->waitForFinished(1000)) {
        m_process->kill();
    }
}

} // namespace gui
} // namespace pacmangui 
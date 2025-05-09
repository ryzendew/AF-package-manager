#include "gui/install_progress_dialog.hpp"
#include "gui/password_prompt_dialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollBar>
#include <QRegularExpression>
#include <qtermwidget6/qtermwidget.h>

namespace pacmangui {
namespace gui {

InstallProgressDialog::InstallProgressDialog(const QString& title, QWidget* parent, bool useTerminal)
    : QDialog(parent), m_outputEdit(new QPlainTextEdit(this)), m_cancelButton(new QPushButton(tr("Cancel"), this)), m_closeButton(new QPushButton(tr("Close"), this)), m_process(new QProcess(this)), m_success(false), m_watchForPasswordPrompt(false), m_passwordSent(false), m_termWidget(nullptr), m_useTerminal(useTerminal)
{
    setWindowTitle(title);
    setModal(true);
    resize(700, 400);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    QLabel* infoLabel = new QLabel(tr("Installation progress:"), this);
    mainLayout->addWidget(infoLabel);

    if (m_useTerminal) {
        m_termWidget = new QTermWidget(0, this);
        mainLayout->addWidget(m_termWidget, 1);
        m_outputEdit->hide();
    } else {
        m_outputEdit->setReadOnly(true);
        m_outputEdit->setStyleSheet("background-color: #181825; color: #a6adc8; font-family: monospace; font-size: 12px;");
        mainLayout->addWidget(m_outputEdit, 1);
    }

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_closeButton);
    mainLayout->addLayout(buttonLayout);

    connect(m_cancelButton, &QPushButton::clicked, this, &InstallProgressDialog::onCancelClicked);
    connect(m_closeButton, &QPushButton::clicked, this, &InstallProgressDialog::onCloseClicked);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &InstallProgressDialog::onReadyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &InstallProgressDialog::onReadyReadStandardError);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &InstallProgressDialog::onProcessFinished);

    m_closeButton->setEnabled(false);
}

void InstallProgressDialog::setWatchForPasswordPrompt(bool enabled) {
    m_watchForPasswordPrompt = enabled;
}

void InstallProgressDialog::startInstallProcess(const QString& program, const QStringList& arguments) {
    if (m_useTerminal && m_termWidget) {
        startInteractiveTerminal(program, arguments);
        return;
    }
    m_outputEdit->clear();
    m_success = false;
    m_passwordSent = false;
    m_process->start(program, arguments);
}

void InstallProgressDialog::startInteractiveTerminal(const QString& program, const QStringList& arguments) {
    if (!m_termWidget) return;
    m_termWidget->setColorScheme("Linux");
    m_termWidget->setScrollBarPosition(QTermWidget::ScrollBarRight);
    m_termWidget->setTerminalFont(QFont("monospace", 11));
    m_termWidget->startShellProgram();

    // Always add -S --noconfirm to the helper
    QString aurCmd = program + " -S --noconfirm";
    if (!arguments.isEmpty()) {
        aurCmd += " " + arguments.join(" ");
    }
    QString scriptCmd = QString("script -q -c '%1' /dev/null").arg(aurCmd);
    m_termWidget->sendText(scriptCmd + "\n");
}

void InstallProgressDialog::appendOutput(const QString& text) {
    m_outputEdit->appendPlainText(text);
    m_outputEdit->verticalScrollBar()->setValue(m_outputEdit->verticalScrollBar()->maximum());
}

void InstallProgressDialog::onReadyReadStandardOutput() {
    QString output = QString::fromLocal8Bit(m_process->readAllStandardOutput());
    appendOutput(output);
    if (m_watchForPasswordPrompt && !m_passwordSent) {
        // Broaden password prompt detection (case-insensitive)
        static const QList<QRegularExpression> promptPatterns = {
            QRegularExpression("\\[sudo\\] password for ", QRegularExpression::CaseInsensitiveOption),
            QRegularExpression(":: sudo password for ", QRegularExpression::CaseInsensitiveOption),
            QRegularExpression("password:", QRegularExpression::CaseInsensitiveOption),
            QRegularExpression("enter your password", QRegularExpression::CaseInsensitiveOption),
            QRegularExpression("please enter your password", QRegularExpression::CaseInsensitiveOption)
        };
        for (const auto& re : promptPatterns) {
            if (output.contains(re)) {
                m_passwordSent = true;
                PasswordPromptDialog pwdDlg(this);
                if (pwdDlg.exec() == QDialog::Accepted) {
                    QString password = pwdDlg.getPassword();
                    providePassword(password);
                } else {
                    appendOutput(tr("\nInstallation canceled (no password provided)."));
                    m_process->kill();
                }
                break;
            }
        }
    }
}

void InstallProgressDialog::onReadyReadStandardError() {
    QString output = QString::fromLocal8Bit(m_process->readAllStandardError());
    appendOutput(output);
    if (m_watchForPasswordPrompt && !m_passwordSent) {
        // Broaden password prompt detection (case-insensitive)
        static const QList<QRegularExpression> promptPatterns = {
            QRegularExpression("\\[sudo\\] password for ", QRegularExpression::CaseInsensitiveOption),
            QRegularExpression(":: sudo password for ", QRegularExpression::CaseInsensitiveOption),
            QRegularExpression("password:", QRegularExpression::CaseInsensitiveOption),
            QRegularExpression("enter your password", QRegularExpression::CaseInsensitiveOption),
            QRegularExpression("please enter your password", QRegularExpression::CaseInsensitiveOption)
        };
        for (const auto& re : promptPatterns) {
            if (output.contains(re)) {
                m_passwordSent = true;
                PasswordPromptDialog pwdDlg(this);
                if (pwdDlg.exec() == QDialog::Accepted) {
                    QString password = pwdDlg.getPassword();
                    providePassword(password);
                } else {
                    appendOutput(tr("\nInstallation canceled (no password provided)."));
                    m_process->kill();
                }
                break;
            }
        }
    }
}

void InstallProgressDialog::providePassword(const QString& password) {
    if (m_process && m_process->state() == QProcess::Running) {
        m_process->write((password + "\n").toLocal8Bit());
        m_process->waitForBytesWritten(1000);
    }
}

void InstallProgressDialog::onProcessFinished(int exitCode, QProcess::ExitStatus status) {
    m_success = (exitCode == 0 && status == QProcess::NormalExit);
    if (m_success) {
        appendOutput(tr("\nInstallation completed successfully."));
    } else {
        appendOutput(tr("\nInstallation failed (exit code %1). Check the output above for details.").arg(exitCode));
    }
    m_cancelButton->setEnabled(false);
    m_closeButton->setEnabled(true);
}

void InstallProgressDialog::onCancelClicked() {
    if (m_process->state() == QProcess::Running) {
        m_process->kill();
        appendOutput(tr("\nInstallation canceled by user."));
        m_cancelButton->setEnabled(false);
    } else {
        accept();
    }
}

void InstallProgressDialog::onCloseClicked() {
    this->accept();
}

bool InstallProgressDialog::wasSuccessful() const {
    return m_success;
}

} // namespace gui
} // namespace pacmangui 
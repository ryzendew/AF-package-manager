#include "gui/flatpak_process_dialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QRegularExpression>

FlatpakProcessDialog::FlatpakProcessDialog(const QString& command, const QString& title, QWidget* parent)
    : QDialog(parent), m_process(new QProcess(this)), m_success(false)
{
    setWindowTitle(title);
    setModal(true);
    resize(600, 400);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    QLabel* label = new QLabel(title, this);
    mainLayout->addWidget(label);

    m_outputEdit = new QTextEdit(this);
    m_outputEdit->setReadOnly(true);
    m_outputEdit->setMinimumHeight(200);
    mainLayout->addWidget(m_outputEdit);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    m_cancelButton = new QPushButton(tr("Cancel"), this);
    buttonLayout->addWidget(m_cancelButton);
    mainLayout->addLayout(buttonLayout);

    connect(m_cancelButton, &QPushButton::clicked, this, &FlatpakProcessDialog::onCancel);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &FlatpakProcessDialog::onReadyRead);
    connect(m_process, &QProcess::readyReadStandardError, this, &FlatpakProcessDialog::onReadyRead);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &FlatpakProcessDialog::onProcessFinished);

    // Start the process
    QStringList args;
    args << "-c" << command;
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    m_process->start("bash", args);
}

FlatpakProcessDialog::~FlatpakProcessDialog() {
    if (m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(1000);
    }
}

void FlatpakProcessDialog::setSuccessString(const QString& str) {
    m_successString = str;
}

void FlatpakProcessDialog::onReadyRead() {
    QString output = m_process->readAll();
    m_outputEdit->moveCursor(QTextCursor::End);
    m_outputEdit->insertPlainText(output);
    m_outputEdit->moveCursor(QTextCursor::End);
    // If the output contains the success string, treat as success
    if (!m_successString.isEmpty() && output.contains(m_successString, Qt::CaseInsensitive)) {
        m_success = true;
    }
}

void FlatpakProcessDialog::onProcessFinished(int exitCode, QProcess::ExitStatus status) {
    // Success if exit code is 0 or the output contained the success string
    m_success = m_success || (exitCode == 0 && status == QProcess::NormalExit);
    m_cancelButton->setText(tr("Close"));
    m_cancelButton->setEnabled(true);
    emit processFinished(m_success);
    if (m_success) {
        m_outputEdit->append(tr("\nOperation completed successfully."));
    } else if (exitCode == 0) {
        // If exit code is 0 but we didn't catch the success string, still consider it a success
        m_success = true;
        m_outputEdit->append(tr("\nOperation completed successfully."));
    } else {
        m_outputEdit->append(tr("\nOperation failed with exit code %1.").arg(exitCode));
    }
}

void FlatpakProcessDialog::onCancel() {
    if (m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_outputEdit->append(tr("\nOperation cancelled by user."));
        m_cancelButton->setEnabled(false);
    } else {
        accept();
    }
}

bool FlatpakProcessDialog::wasSuccessful() const {
    return m_success;
} 
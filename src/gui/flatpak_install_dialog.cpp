#include "gui/flatpak_install_dialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QRegularExpression>

FlatpakInstallDialog::FlatpakInstallDialog(const QString& appId, const QString& remote, QWidget* parent)
    : QDialog(parent), m_process(new QProcess(this)), m_success(false)
{
    setWindowTitle(tr("Installing %1 from %2").arg(appId, remote));
    setModal(true);
    resize(600, 400);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    QLabel* label = new QLabel(tr("Installing <b>%1</b> from <b>%2</b>...").arg(appId, remote), this);
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

    connect(m_cancelButton, &QPushButton::clicked, this, &FlatpakInstallDialog::onCancel);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &FlatpakInstallDialog::onReadyRead);
    connect(m_process, &QProcess::readyReadStandardError, this, &FlatpakInstallDialog::onReadyRead);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &FlatpakInstallDialog::onProcessFinished);

    // Start the flatpak install process
    QStringList args;
    args << "-c" << QString("flatpak install -y %1 %2").arg(remote, appId);
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    m_process->start("bash", args);
}

FlatpakInstallDialog::~FlatpakInstallDialog() {
    if (m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(1000);
    }
}

void FlatpakInstallDialog::onReadyRead() {
    QString output = m_process->readAll();
    m_outputEdit->moveCursor(QTextCursor::End);
    m_outputEdit->insertPlainText(output);
    m_outputEdit->moveCursor(QTextCursor::End);
}

void FlatpakInstallDialog::onProcessFinished(int exitCode, QProcess::ExitStatus status) {
    m_success = (exitCode == 0 && status == QProcess::NormalExit);
    m_cancelButton->setText(tr("Close"));
    m_cancelButton->setEnabled(true);
    emit installFinished(m_success);
    if (m_success) {
        m_outputEdit->append(tr("\nInstallation completed successfully."));
    } else {
        m_outputEdit->append(tr("\nInstallation failed."));
    }
}

void FlatpakInstallDialog::onCancel() {
    if (m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_outputEdit->append(tr("\nInstallation cancelled by user."));
        m_cancelButton->setEnabled(false);
    } else {
        accept();
    }
}

bool FlatpakInstallDialog::wasSuccessful() const {
    return m_success;
} 
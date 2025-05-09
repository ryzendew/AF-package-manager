#include "gui/password_prompt_dialog.hpp"
#include <QLabel>

namespace pacmangui {
namespace gui {

PasswordPromptDialog::PasswordPromptDialog(QWidget* parent)
    : QDialog(parent), m_passwordEdit(new QLineEdit(this)), m_okButton(new QPushButton(tr("OK"), this)), m_cancelButton(new QPushButton(tr("Cancel"), this))
{
    setWindowTitle(tr("Authentication Required"));
    setModal(true);
    resize(350, 120);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    QLabel* label = new QLabel(tr("Please enter your password to proceed:"), this);
    mainLayout->addWidget(label);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setMinimumWidth(200);
    mainLayout->addWidget(m_passwordEdit);
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_okButton);
    buttonLayout->addWidget(m_cancelButton);
    mainLayout->addLayout(buttonLayout);
    connect(m_okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &QDialog::accept);
}

QString PasswordPromptDialog::getPassword() const {
    return m_passwordEdit->text();
}

} // namespace gui
} // namespace pacmangui 
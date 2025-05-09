#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

namespace pacmangui {
namespace gui {

class PasswordPromptDialog : public QDialog {
    Q_OBJECT
public:
    explicit PasswordPromptDialog(QWidget* parent = nullptr);
    QString getPassword() const;

private:
    QLineEdit* m_passwordEdit;
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;
};

} // namespace gui
} // namespace pacmangui 
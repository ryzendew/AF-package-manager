#include "gui/util.hpp"

#include <QDateTime>
#include <QMessageBox>

namespace pacmangui {
namespace gui {

QIcon getPackageStatusIcon(const QString& status) {
    if (status == "installed") {
        return QIcon::fromTheme("package-installed-updated", QIcon(":/icons/package-installed.png"));
    } else if (status == "upgradable") {
        return QIcon::fromTheme("package-upgrade", QIcon(":/icons/package-upgradable.png"));
    } else if (status == "not-installed") {
        return QIcon::fromTheme("package-available", QIcon(":/icons/package-available.png"));
    } else {
        return QIcon::fromTheme("package", QIcon(":/icons/package.png"));
    }
}

QColor getPackageStatusColor(const QString& status) {
    if (status == "installed") {
        return QColor(0, 128, 0);  // Green
    } else if (status == "upgradable") {
        return QColor(255, 140, 0); // Orange
    } else if (status == "not-installed") {
        return QColor(0, 0, 0);     // Black
    } else {
        return QColor(128, 128, 128); // Gray
    }
}

void showErrorDialog(QWidget* parent, const QString& title, const QString& message) {
    QMessageBox::critical(parent, title, message);
}

void showInfoDialog(QWidget* parent, const QString& title, const QString& message) {
    QMessageBox::information(parent, title, message);
}

bool showConfirmDialog(QWidget* parent, const QString& title, const QString& message) {
    return QMessageBox::question(parent, title, message) == QMessageBox::Yes;
}

QString formatPackageSize(qint64 sizeInBytes) {
    constexpr double KB = 1024.0;
    constexpr double MB = 1024.0 * KB;
    constexpr double GB = 1024.0 * MB;

    if (sizeInBytes < 0) {
        return "Unknown";
    } else if (sizeInBytes < KB) {
        return QString("%1 B").arg(sizeInBytes);
    } else if (sizeInBytes < MB) {
        return QString("%1 KB").arg(sizeInBytes / KB, 0, 'f', 2);
    } else if (sizeInBytes < GB) {
        return QString("%1 MB").arg(sizeInBytes / MB, 0, 'f', 2);
    } else {
        return QString("%1 GB").arg(sizeInBytes / GB, 0, 'f', 2);
    }
}

QString formatDateString(qint64 timestamp) {
    if (timestamp <= 0) {
        return "Unknown";
    }

    QDateTime dateTime = QDateTime::fromSecsSinceEpoch(timestamp);
    return dateTime.toString("yyyy-MM-dd hh:mm:ss");
}

} // namespace gui
} // namespace pacmangui 
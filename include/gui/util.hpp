#ifndef PACMANGUI_GUI_UTIL_HPP
#define PACMANGUI_GUI_UTIL_HPP

#include <QString>
#include <QIcon>
#include <QPixmap>
#include <QColor>
#include <QWidget>
#include <QMessageBox>

namespace pacmangui {
namespace gui {

/**
 * @brief Utility functions for the GUI
 */

/**
 * @brief Get an icon for a package based on its status
 * @param status The package status
 * @return The icon
 */
QIcon getPackageStatusIcon(const QString& status);

/**
 * @brief Get a color for a package based on its status
 * @param status The package status
 * @return The color
 */
QColor getPackageStatusColor(const QString& status);

/**
 * @brief Show an error dialog
 * @param parent The parent widget
 * @param title The dialog title
 * @param message The error message
 */
void showErrorDialog(QWidget* parent, const QString& title, const QString& message);

/**
 * @brief Show an information dialog
 * @param parent The parent widget
 * @param title The dialog title
 * @param message The information message
 */
void showInfoDialog(QWidget* parent, const QString& title, const QString& message);

/**
 * @brief Show a confirmation dialog
 * @param parent The parent widget
 * @param title The dialog title
 * @param message The confirmation message
 * @return True if confirmed, false otherwise
 */
bool showConfirmDialog(QWidget* parent, const QString& title, const QString& message);

/**
 * @brief Format a package size in human readable format
 * @param sizeInBytes Size in bytes
 * @return Human readable size
 */
QString formatPackageSize(qint64 sizeInBytes);

/**
 * @brief Format a date string
 * @param timestamp The timestamp to format
 * @return The formatted date string
 */
QString formatDateString(qint64 timestamp);

} // namespace gui
} // namespace pacmangui

#endif // PACMANGUI_GUI_UTIL_HPP 
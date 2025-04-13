#pragma once

#include <QTableView>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <vector>
#include "core/package.hpp"

namespace pacmangui {
namespace gui {

/**
 * @brief Custom model for package data
 */
class PackageModel : public QStandardItemModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * 
     * @param parent Parent object
     */
    explicit PackageModel(QObject *parent = nullptr);
    
    /**
     * @brief Set packages to display
     * 
     * @param packages List of packages
     */
    void setPackages(const std::vector<core::Package>& packages);
    
    /**
     * @brief Get package at index
     * 
     * @param index Model index
     * @return core::Package The package at index
     */
    core::Package getPackage(const QModelIndex& index) const;

private:
    std::vector<core::Package> m_packages;  ///< Stored packages
};

/**
 * @brief Widget for displaying packages
 */
class PackageView : public QTableView
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * 
     * @param parent Parent widget
     */
    explicit PackageView(QWidget *parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~PackageView();
    
    /**
     * @brief Set packages to display
     * 
     * @param packages List of packages
     */
    void setPackages(const std::vector<core::Package>& packages);
    
    /**
     * @brief Get selected package
     * 
     * @return core::Package The selected package
     */
    core::Package getSelectedPackage() const;
    
    /**
     * @brief Set filter pattern
     * 
     * @param pattern Filter pattern
     */
    void setFilterPattern(const QString& pattern);

private:
    PackageModel *m_model;                         ///< Package data model
    QSortFilterProxyModel *m_proxy_model;          ///< Proxy model for sorting/filtering
};

} // namespace gui
} // namespace pacmangui 
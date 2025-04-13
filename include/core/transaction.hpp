#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <alpm.h>
#include "core/package.hpp"

namespace pacmangui {
namespace core {

class Package;

/**
 * @brief Enum representing the type of transaction
 */
enum class TransactionType {
    INSTALL,    ///< Install packages
    REMOVE,     ///< Remove packages
    UPDATE,     ///< Update packages
    SYNC        ///< Sync repositories
};

/**
 * @brief Enum representing the state of a transaction
 */
enum class TransactionState {
    IDLE,       ///< Transaction is idle
    PREPARING,  ///< Transaction is being prepared
    COMMITTING, ///< Transaction is being committed
    COMPLETED,  ///< Transaction has completed successfully
    FAILED      ///< Transaction has failed
};

/**
 * @brief Class representing a package transaction
 */
class Transaction {
public:
    /**
     * @brief Constructor
     * @param type The type of transaction
     */
    explicit Transaction(TransactionType type);

    /**
     * @brief Destructor
     */
    ~Transaction();

    /**
     * @brief Get the type of transaction
     * @return The transaction type
     */
    TransactionType get_type() const;

    /**
     * @brief Get the state of the transaction
     * @return The transaction state
     */
    TransactionState get_state() const;

    /**
     * @brief Set the state of the transaction
     * @param state The transaction state
     */
    void set_state(TransactionState state);

    /**
     * @brief Add a package target to the transaction
     * @param target The package target to add
     */
    void add_target(const std::string& target);

    /**
     * @brief Remove a package target from the transaction
     * @param target The package target to remove
     */
    void remove_target(const std::string& target);

    /**
     * @brief Get the list of package targets
     * @return The list of package targets
     */
    std::vector<std::string> get_targets() const;

    /**
     * @brief Get the list of packages in the transaction
     * @return The list of packages
     */
    std::vector<Package> get_packages() const;

    /**
     * @brief Set the ALPM transaction
     * @param trans The ALPM transaction
     */
    void set_alpm_trans(void* trans);

    /**
     * @brief Get the ALPM transaction
     * @return The ALPM transaction
     */
    void* get_alpm_trans() const;

private:
    TransactionType m_type;                  ///< The type of transaction
    TransactionState m_state;                ///< The state of the transaction
    std::vector<std::string> m_targets;      ///< The list of package targets
    void* m_trans;                           ///< Pointer to the alpm transaction - using void* for compatibility
};

/**
 * @brief Class for managing transactions
 */
class TransactionManager {
public:
    /**
     * @brief Constructor
     * @param handle The ALPM handle
     */
    explicit TransactionManager(alpm_handle_t* handle);

    /**
     * @brief Destructor
     */
    ~TransactionManager();

    /**
     * @brief Initialize a transaction
     * @param type The type of transaction
     * @return The transaction
     */
    std::shared_ptr<Transaction> init_transaction(TransactionType type);

    /**
     * @brief Prepare a transaction for execution
     * @param transaction The transaction to prepare
     * @return True if the transaction was prepared successfully, false otherwise
     */
    bool prepare_transaction(Transaction* transaction);

    /**
     * @brief Commit a transaction
     * @param transaction The transaction to commit
     * @return True if the transaction was committed successfully, false otherwise
     */
    bool commit_transaction(Transaction* transaction);

    /**
     * @brief Release a transaction
     * @param transaction The transaction to release
     */
    void release_transaction(Transaction* transaction);

    /**
     * @brief Resolve dependencies for a transaction
     * @param transaction The transaction to resolve dependencies for
     * @return The list of packages to install
     */
    std::vector<Package> resolve_dependencies(Transaction* transaction);

private:
    /**
     * @brief Initialize an ALPM transaction
     * @param transaction The transaction to initialize
     * @return True if the transaction was initialized successfully, false otherwise
     */
    bool init_alpm_transaction(Transaction* transaction);

    alpm_handle_t* m_handle;                     ///< The ALPM handle
};

} // namespace core
} // namespace pacmangui 
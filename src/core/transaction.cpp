#include "transaction.hpp"
#include "package.hpp"
#include <iostream>
#include <algorithm>

namespace pacmangui {
namespace core {

Transaction::Transaction(TransactionType type)
    : m_type(type)
    , m_state(TransactionState::IDLE)
    , m_trans(nullptr)
{
}

Transaction::~Transaction()
{
    m_trans = nullptr; // We don't own this pointer
}

TransactionType Transaction::get_type() const
{
    return m_type;
}

TransactionState Transaction::get_state() const
{
    return m_state;
}

void Transaction::set_state(TransactionState state)
{
    m_state = state;
}

void Transaction::add_target(const std::string& target)
{
    m_targets.push_back(target);
}

void Transaction::remove_target(const std::string& target)
{
    auto it = std::find(m_targets.begin(), m_targets.end(), target);
    if (it != m_targets.end()) {
        m_targets.erase(it);
    }
}

std::vector<std::string> Transaction::get_targets() const
{
    return m_targets;
}

std::vector<pacmangui::core::Package> Transaction::get_packages() const
{
    std::vector<pacmangui::core::Package> packages;
    
    if (!m_trans) {
        return packages;
    }
    
    // For now, since we don't have proper ALPM functions available, return an empty list
    // alpm_list_t* add_pkgs = alpm_trans_get_add(m_trans);
    
    return packages;
}

void Transaction::set_alpm_trans(void* trans)
{
    m_trans = trans;
}

void* Transaction::get_alpm_trans() const
{
    return m_trans;
}

// Transaction Manager Implementation

TransactionManager::TransactionManager(alpm_handle_t* handle)
    : m_handle(handle)
{
}

TransactionManager::~TransactionManager()
{
}

std::shared_ptr<Transaction> TransactionManager::init_transaction(TransactionType type)
{
    auto transaction = std::make_shared<Transaction>(type);
    return transaction;
}

bool TransactionManager::prepare_transaction(Transaction* transaction)
{
    if (!transaction || !m_handle) {
        return false;
    }
    
    // Initialize the transaction in ALPM
    if (!init_alpm_transaction(transaction)) {
        return false;
    }
    
    // Get transaction targets
    const auto& targets = transaction->get_targets();
    
    // Currently, alpm_add_target is not directly accessible
    // We'll need to create a proper implementation
    
    // For now, just print targets
    for (const auto& target : targets) {
        std::cout << "Would add target: " << target << std::endl;
    }
    
    // Set state to PREPARING
    transaction->set_state(TransactionState::PREPARING);
    
    // For now, just mark it as successful
    transaction->set_state(TransactionState::COMMITTING);
    
    return true;
}

bool TransactionManager::commit_transaction(Transaction* transaction)
{
    if (!transaction || !m_handle) {
        return false;
    }
    
    // Set state to RUNNING
    transaction->set_state(TransactionState::COMMITTING);
    
    // For now, since we don't have full ALPM implementation
    std::cout << "Would commit transaction with " << transaction->get_targets().size() << " targets" << std::endl;
    
    // Set state to COMPLETED
    transaction->set_state(TransactionState::COMPLETED);
    
    // Release the transaction
    release_transaction(transaction);
    
    return true;
}

void TransactionManager::release_transaction(Transaction* transaction)
{
    if (!transaction || !m_handle) {
        return;
    }
    
    std::cout << "Releasing transaction" << std::endl;
    
    // Clear the transaction pointer
    transaction->set_alpm_trans(nullptr);
}

std::vector<pacmangui::core::Package> TransactionManager::resolve_dependencies(Transaction* transaction)
{
    std::vector<pacmangui::core::Package> dependencies;
    
    if (!transaction || !m_handle) {
        return dependencies;
    }
    
    // For now, since we don't have proper ALPM implementation, return empty list
    std::cout << "Would resolve dependencies for transaction" << std::endl;
    
    return dependencies;
}

bool TransactionManager::init_alpm_transaction(Transaction* transaction)
{
    if (!transaction || !m_handle) {
        return false;
    }
    
    int flags = 0;
    
    // Set flags based on transaction type
    switch (transaction->get_type()) {
        case TransactionType::INSTALL:
            std::cout << "Creating install transaction" << std::endl;
            flags = ALPM_TRANS_FLAG_ALLDEPS;
            break;
            
        case TransactionType::REMOVE:
            std::cout << "Creating remove transaction" << std::endl;
            flags = ALPM_TRANS_FLAG_RECURSE;
            break;
            
        case TransactionType::UPDATE:
            std::cout << "Creating update transaction" << std::endl;
            flags = ALPM_TRANS_FLAG_ALLDEPS;
            break;
            
        case TransactionType::SYNC:
            std::cout << "Creating sync transaction" << std::endl;
            flags = ALPM_TRANS_FLAG_ALLDEPS;
            break;
    }
    
    // Initialize the transaction
    alpm_errno_t err;
    int ret = alpm_trans_init(m_handle, flags);
    
    if (ret != 0) {
        err = alpm_errno(m_handle);
        std::cerr << "Failed to initialize transaction: " << alpm_strerror(err) << std::endl;
        return false;
    }
    
    // Store transaction context
    // Since alpm_trans_get_ctx isn't available, we'll temporarily set a dummy value
    transaction->set_alpm_trans(m_handle); // Using handle as a placeholder
    
    return true;
}

} // namespace core
} // namespace pacmangui 
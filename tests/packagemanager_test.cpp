#include <gtest/gtest.h>
#include "core/packagemanager.hpp"

using namespace pacmangui::core;

class PackageManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create package manager
        pkg_manager = new PackageManager();
    }
    
    void TearDown() override {
        delete pkg_manager;
    }

    PackageManager* pkg_manager;
};

// Basic tests that don't require ALPM to be initialized

TEST_F(PackageManagerTest, CanBeInstantiated) {
    // Just ensure the constructor doesn't crash
    EXPECT_NO_THROW({
        PackageManager manager;
    });
}

TEST_F(PackageManagerTest, GetLastErrorReturnsEmptyStringByDefault) {
    EXPECT_EQ(pkg_manager->get_last_error(), "");
}

TEST_F(PackageManagerTest, NonInitializedManagerReturnsEmptyPackageLists) {
    EXPECT_TRUE(pkg_manager->get_installed_packages().empty());
    EXPECT_TRUE(pkg_manager->get_available_packages().empty());
    EXPECT_TRUE(pkg_manager->search_by_name("test").empty());
    EXPECT_TRUE(pkg_manager->get_repositories().empty());
}

TEST_F(PackageManagerTest, NonInitializedManagerReturnsEmptyPackageDetails) {
    Package pkg = pkg_manager->get_package_details("test");
    EXPECT_EQ(pkg.get_name(), "");
}

TEST_F(PackageManagerTest, NonInitializedManagerReturnsFalseForPackageOperations) {
    EXPECT_FALSE(pkg_manager->install_package("test"));
    EXPECT_FALSE(pkg_manager->remove_package("test"));
    EXPECT_FALSE(pkg_manager->update_package("test"));
    EXPECT_FALSE(pkg_manager->sync_all());
    EXPECT_FALSE(pkg_manager->is_package_installed("test"));
}

TEST_F(PackageManagerTest, InitializeWithInvalidPathsFails) {
    // This should fail because these paths likely don't exist in the test environment
    EXPECT_FALSE(pkg_manager->initialize("/invalid/root", "/invalid/db/path"));
    EXPECT_FALSE(pkg_manager->get_last_error().empty()); // Should set an error message
}

// For real functionality tests, we would need to mock ALPM or use a real
// pacman installation in a controlled environment.
/*
class PackageManagerIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create package manager with real paths
        pkg_manager = new PackageManager();
        initialized = pkg_manager->initialize("/", "/var/lib/pacman");
    }
    
    void TearDown() override {
        delete pkg_manager;
    }

    PackageManager* pkg_manager;
    bool initialized;
};

TEST_F(PackageManagerIntegrationTest, CanInitializeWithValidPaths) {
    EXPECT_TRUE(initialized);
}

TEST_F(PackageManagerIntegrationTest, CanGetInstalledPackages) {
    if (!initialized) GTEST_SKIP();
    
    std::vector<Package> installed = pkg_manager->get_installed_packages();
    EXPECT_FALSE(installed.empty());
}
*/ 
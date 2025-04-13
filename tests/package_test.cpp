#include <gtest/gtest.h>
#include "core/package.hpp"

using namespace pacmangui::core;

class PackageTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test packages
        package1 = Package("test-package", "1.0.0");
        package2 = Package("test-package", "1.0.0");
        package3 = Package("other-package", "2.0.0");
    }

    Package package1;
    Package package2;
    Package package3;
};

TEST_F(PackageTest, ConstructorSetsNameAndVersion) {
    EXPECT_EQ(package1.get_name(), "test-package");
    EXPECT_EQ(package1.get_version(), "1.0.0");
}

TEST_F(PackageTest, EqualityOperatorWorks) {
    EXPECT_TRUE(package1 == package2);
    EXPECT_FALSE(package1 == package3);
}

TEST_F(PackageTest, DefaultConstructorCreatesEmptyPackage) {
    Package empty;
    EXPECT_EQ(empty.get_name(), "");
    EXPECT_EQ(empty.get_version(), "");
}

TEST_F(PackageTest, GettersReturnCorrectValues) {
    EXPECT_EQ(package1.get_description(), "");
    EXPECT_EQ(package1.get_install_size(), 0);
    EXPECT_EQ(package1.get_download_size(), 0);
    EXPECT_FALSE(package1.is_installed());
    EXPECT_TRUE(package1.get_dependencies().empty());
}

// This test requires a mock of alpm_pkg_t which is beyond the scope of this example
// In a real implementation, you would use a mocking framework like GoogleMock
/*
TEST_F(PackageTest, CreateFromAlpmCreatesCorrectPackage) {
    // Create a mock alpm_pkg_t
    alpm_pkg_t* mock_pkg = create_mock_alpm_pkg("test-package", "1.0.0", "Test description");
    
    // Create package from alpm
    Package package = Package::create_from_alpm(mock_pkg);
    
    // Check package properties
    EXPECT_EQ(package.get_name(), "test-package");
    EXPECT_EQ(package.get_version(), "1.0.0");
    EXPECT_EQ(package.get_description(), "Test description");
    
    // Clean up mock
    free_mock_alpm_pkg(mock_pkg);
}
*/ 
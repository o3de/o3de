/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Settings/SettingsRegistry.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzTest/AzTest.h>

#include <Configuration/AWSCoreConfiguration.h>
#include <TestFramework/AWSCoreFixture.h>

using namespace AWSCore;

static constexpr const char TEST_VALID_RESOURCE_MAPPING_SETREG[] =
R"({
    "Amazon":
    {
        "AWSCore": {
            "ProfileName": "testprofile",
            "ResourceMappingConfigFileName": "test_aws_resource_mappings.json"
        }
    }
})";

static constexpr const char TEST_INVALID_RESOURCE_MAPPING_SETREG[] =
R"({
    "Amazon":
    {
        "AWSCore": {
        }
    }
})";

class AWSCoreConfigurationTest
    : public AWSCoreFixture
{
public:
    void CreateTestSetRegFile(const AZStd::string& setregContent)
    {
        m_normalizedSetRegFilePath = AZStd::string::format("%s/%s",
            m_normalizedSetRegFolderPath.c_str(), AWSCore::AWSCoreConfiguration::AWSCoreConfigurationFileName);
        AzFramework::StringFunc::Path::Normalize(m_normalizedSetRegFilePath);
        CreateTestFile(m_normalizedSetRegFilePath, setregContent);
    }

    void SetUp() override
    {
        AWSCoreFixture::SetUp();

        m_awsCoreConfiguration = AZStd::make_unique<AWSCore::AWSCoreConfiguration>();

        m_normalizedSourceProjectFolder = AZStd::string::format("%s/%s%s/", AZ::Test::GetCurrentExecutablePath().c_str(),
            "AWSResourceMappingManager", AZ::Uuid::CreateRandom().ToString<AZStd::string>(false, false).c_str());
        AzFramework::StringFunc::Path::Normalize(m_normalizedSourceProjectFolder);
        m_normalizedSetRegFolderPath = AZStd::string::format("%s/%s/",
            m_normalizedSourceProjectFolder.c_str(), AZ::SettingsRegistryInterface::RegistryFolder);
        AzFramework::StringFunc::Path::Normalize(m_normalizedSetRegFolderPath);

        m_localFileIO->SetAlias("@projectroot@", m_normalizedSourceProjectFolder.c_str());

        CreateTestSetRegFile(TEST_VALID_RESOURCE_MAPPING_SETREG);
    }

    void TearDown() override
    {
        RemoveTestFile();
        RemoveTestDirectory();

        m_awsCoreConfiguration.reset();

        AWSCoreFixture::TearDown();
    }

    AZStd::unique_ptr<AWSCore::AWSCoreConfiguration> m_awsCoreConfiguration;
    AZStd::string m_normalizedSetRegFilePath;

private:
    AZStd::string m_normalizedSourceProjectFolder;
    AZStd::string m_normalizedSetRegFolderPath;

    void CreateTestFile(const AZStd::string& filePath, const AZStd::string& fileContent)
    {
        AZ::IO::SystemFile file;
        if (!file.Open(filePath.c_str(),
            AZ::IO::SystemFile::OpenMode::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY))
        {
            AZ_Assert(false, "Failed to open test file");
        }

        if (file.Write(fileContent.c_str(), fileContent.size()) != fileContent.size())
        {
            AZ_Assert(false, "Failed to write test file");
        }
        file.Close();
    }

    void RemoveTestFile()
    {
        if (!m_normalizedSetRegFilePath.empty())
        {
            AZ_Assert(AZ::IO::SystemFile::Delete(m_normalizedSetRegFilePath.c_str()),
                "Failed to delete test settings registry file at %s", m_normalizedSetRegFilePath.c_str());
        }
    }

    void RemoveTestDirectory()
    {
        if (!m_normalizedSetRegFilePath.empty())
        {
            AZ_Assert(AZ::IO::SystemFile::DeleteDir(m_normalizedSetRegFolderPath.c_str()),
                "Failed to delete test settings registry folder at %s", m_normalizedSetRegFolderPath.c_str());
            AZ_Assert(AZ::IO::SystemFile::DeleteDir(m_normalizedSourceProjectFolder.c_str()),
                "Failed to delete test folder at %s", m_normalizedSourceProjectFolder.c_str());
        }
    }
};

TEST_F(AWSCoreConfigurationTest, InitConfig_NoSourceProjectFolderFound_ReturnEmptyConfigFilePath)
{
    m_settingsRegistry->MergeSettingsFile(m_normalizedSetRegFilePath, AZ::SettingsRegistryInterface::Format::JsonMergePatch, {});
    m_localFileIO->ClearAlias("@projectroot@");

    AZ_TEST_START_TRACE_SUPPRESSION;
    m_awsCoreConfiguration->InitConfig();
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // expect the above have thrown an AZ_Error

    auto actualConfigFilePath = m_awsCoreConfiguration->GetResourceMappingConfigFilePath();
    EXPECT_TRUE(actualConfigFilePath.empty());
}

TEST_F(AWSCoreConfigurationTest, InitConfig_SettingsRegistryIsEmpty_ReturnEmptyConfigFilePath)
{
    CreateTestSetRegFile(TEST_INVALID_RESOURCE_MAPPING_SETREG);
    m_settingsRegistry->MergeSettingsFile(m_normalizedSetRegFilePath, AZ::SettingsRegistryInterface::Format::JsonMergePatch, {});
    m_awsCoreConfiguration->InitConfig();

    auto actualConfigFilePath = m_awsCoreConfiguration->GetResourceMappingConfigFilePath();
    EXPECT_TRUE(actualConfigFilePath.empty());
}

TEST_F(AWSCoreConfigurationTest, InitConfig_LoadValidSettingsRegistry_ReturnNonEmptyConfigFilePath)
{
    m_settingsRegistry->MergeSettingsFile(m_normalizedSetRegFilePath, AZ::SettingsRegistryInterface::Format::JsonMergePatch, {});
    m_awsCoreConfiguration->InitConfig();

    auto actualConfigFilePath = m_awsCoreConfiguration->GetResourceMappingConfigFilePath();
    EXPECT_FALSE(actualConfigFilePath.empty());
}

TEST_F(AWSCoreConfigurationTest, ReloadConfiguration_NoSourceProjectFolderFound_ReturnEmptyConfigFilePath)
{
    m_settingsRegistry->MergeSettingsFile(m_normalizedSetRegFilePath, AZ::SettingsRegistryInterface::Format::JsonMergePatch, {});
    m_localFileIO->ClearAlias("@projectroot@");
    m_awsCoreConfiguration->ReloadConfiguration();

    auto actualConfigFilePath = m_awsCoreConfiguration->GetResourceMappingConfigFilePath();
    EXPECT_TRUE(actualConfigFilePath.empty());
}

TEST_F(AWSCoreConfigurationTest, ReloadConfiguration_LoadValidSettingsRegistryAfterInvalidOne_ReturnNonEmptyConfigFilePath)
{
    CreateTestSetRegFile(TEST_INVALID_RESOURCE_MAPPING_SETREG);
    m_settingsRegistry->MergeSettingsFile(m_normalizedSetRegFilePath, AZ::SettingsRegistryInterface::Format::JsonMergePatch, {});
    m_awsCoreConfiguration->InitConfig();

    auto actualConfigFilePath = m_awsCoreConfiguration->GetResourceMappingConfigFilePath();
    auto actualProfileName = m_awsCoreConfiguration->GetProfileName();
    EXPECT_TRUE(actualConfigFilePath.empty());
    EXPECT_TRUE(actualProfileName == AWSCoreConfiguration::AWSCoreDefaultProfileName);

    CreateTestSetRegFile(TEST_VALID_RESOURCE_MAPPING_SETREG);
    m_awsCoreConfiguration->ReloadConfiguration();

    actualConfigFilePath = m_awsCoreConfiguration->GetResourceMappingConfigFilePath();
    actualProfileName = m_awsCoreConfiguration->GetProfileName();
    EXPECT_FALSE(actualConfigFilePath.empty());
    EXPECT_TRUE(actualProfileName != AWSCoreConfiguration::AWSCoreDefaultProfileName);
}

TEST_F(AWSCoreConfigurationTest, ReloadConfiguration_LoadInvalidSettingsRegistryAfterValidOne_ReturnEmptyConfigFilePath)
{
    m_settingsRegistry->MergeSettingsFile(m_normalizedSetRegFilePath, AZ::SettingsRegistryInterface::Format::JsonMergePatch, {});
    m_awsCoreConfiguration->InitConfig();

    auto actualConfigFilePath = m_awsCoreConfiguration->GetResourceMappingConfigFilePath();
    auto actualProfileName = m_awsCoreConfiguration->GetProfileName();
    EXPECT_FALSE(actualConfigFilePath.empty());
    EXPECT_TRUE(actualProfileName != AWSCoreConfiguration::AWSCoreDefaultProfileName);

    CreateTestSetRegFile(TEST_INVALID_RESOURCE_MAPPING_SETREG);
    m_awsCoreConfiguration->ReloadConfiguration();

    actualConfigFilePath = m_awsCoreConfiguration->GetResourceMappingConfigFilePath();
    actualProfileName = m_awsCoreConfiguration->GetProfileName();
    EXPECT_TRUE(actualConfigFilePath.empty());
    EXPECT_TRUE(actualProfileName == AWSCoreConfiguration::AWSCoreDefaultProfileName);
}

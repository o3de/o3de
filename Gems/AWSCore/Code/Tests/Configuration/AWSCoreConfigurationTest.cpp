/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Settings/SettingsRegistry.h>
#include <AzFramework/StringFunc/StringFunc.h>

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
    AWSCoreConfigurationTest()
    {
        m_setRegFilePath = (GetTestTempDirectoryPath() /
                            AZ::SettingsRegistryInterface::RegistryFolder /
                            AWSCore::AWSCoreConfiguration::AWSCoreConfigurationFileName).LexicallyNormal();
    }

    void SetUp() override
    {
        AWSCoreFixture::SetUp();

        m_awsCoreConfiguration = AZStd::make_unique<AWSCore::AWSCoreConfiguration>();

        CreateFile(m_setRegFilePath.Native(), TEST_VALID_RESOURCE_MAPPING_SETREG);
        m_localFileIO->SetAlias("@projectroot@", GetTestTempDirectoryPath().Native().c_str());
    }

    void TearDown() override
    {
        RemoveFile(m_setRegFilePath.Native());

        m_awsCoreConfiguration.reset();

        AWSCoreFixture::TearDown();
    }

    AZStd::unique_ptr<AWSCore::AWSCoreConfiguration> m_awsCoreConfiguration;
    AZ::IO::Path m_setRegFilePath;
};

TEST_F(AWSCoreConfigurationTest, InitConfig_NoSourceProjectFolderFound_ReturnEmptyConfigFilePath)
{
    m_settingsRegistry->MergeSettingsFile(m_setRegFilePath.Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, {});
    m_localFileIO->ClearAlias("@projectroot@");

    AZ_TEST_START_TRACE_SUPPRESSION;
    m_awsCoreConfiguration->InitConfig();
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // expect the above have thrown an AZ_Error

    auto actualConfigFilePath = m_awsCoreConfiguration->GetResourceMappingConfigFilePath();
    EXPECT_TRUE(actualConfigFilePath.empty());
}

TEST_F(AWSCoreConfigurationTest, InitConfig_SettingsRegistryIsEmpty_ReturnEmptyConfigFilePath)
{
    CreateFile(m_setRegFilePath.Native(), TEST_INVALID_RESOURCE_MAPPING_SETREG);
    m_settingsRegistry->MergeSettingsFile(m_setRegFilePath.Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, {});
    m_awsCoreConfiguration->InitConfig();

    auto actualConfigFilePath = m_awsCoreConfiguration->GetResourceMappingConfigFilePath();
    EXPECT_TRUE(actualConfigFilePath.empty());
}

TEST_F(AWSCoreConfigurationTest, InitConfig_LoadValidSettingsRegistry_ReturnNonEmptyConfigFilePath)
{
    m_settingsRegistry->MergeSettingsFile(m_setRegFilePath.Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, {});
    m_awsCoreConfiguration->InitConfig();

    auto actualConfigFilePath = m_awsCoreConfiguration->GetResourceMappingConfigFilePath();
    EXPECT_FALSE(actualConfigFilePath.empty());
}

TEST_F(AWSCoreConfigurationTest, ReloadConfiguration_NoSourceProjectFolderFound_ReturnEmptyConfigFilePath)
{
    m_settingsRegistry->MergeSettingsFile(m_setRegFilePath.Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, {});
    m_localFileIO->ClearAlias("@projectroot@");
    m_awsCoreConfiguration->ReloadConfiguration();

    auto actualConfigFilePath = m_awsCoreConfiguration->GetResourceMappingConfigFilePath();
    EXPECT_TRUE(actualConfigFilePath.empty());
}

TEST_F(AWSCoreConfigurationTest, ReloadConfiguration_LoadValidSettingsRegistryAfterInvalidOne_ReturnNonEmptyConfigFilePath)
{
    CreateFile(m_setRegFilePath.Native(), TEST_INVALID_RESOURCE_MAPPING_SETREG);
    m_settingsRegistry->MergeSettingsFile(m_setRegFilePath.Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, {});
    m_awsCoreConfiguration->InitConfig();

    auto actualConfigFilePath = m_awsCoreConfiguration->GetResourceMappingConfigFilePath();
    auto actualProfileName = m_awsCoreConfiguration->GetProfileName();
    EXPECT_TRUE(actualConfigFilePath.empty());
    EXPECT_TRUE(actualProfileName == AWSCoreConfiguration::AWSCoreDefaultProfileName);

    CreateFile(m_setRegFilePath.Native(), TEST_VALID_RESOURCE_MAPPING_SETREG);
    m_awsCoreConfiguration->ReloadConfiguration();

    actualConfigFilePath = m_awsCoreConfiguration->GetResourceMappingConfigFilePath();
    actualProfileName = m_awsCoreConfiguration->GetProfileName();
    EXPECT_FALSE(actualConfigFilePath.empty());
    EXPECT_TRUE(actualProfileName != AWSCoreConfiguration::AWSCoreDefaultProfileName);
}

TEST_F(AWSCoreConfigurationTest, ReloadConfiguration_LoadInvalidSettingsRegistryAfterValidOne_ReturnEmptyConfigFilePath)
{
    m_settingsRegistry->MergeSettingsFile(m_setRegFilePath.Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, {});
    m_awsCoreConfiguration->InitConfig();

    auto actualConfigFilePath = m_awsCoreConfiguration->GetResourceMappingConfigFilePath();
    auto actualProfileName = m_awsCoreConfiguration->GetProfileName();
    EXPECT_FALSE(actualConfigFilePath.empty());
    EXPECT_TRUE(actualProfileName != AWSCoreConfiguration::AWSCoreDefaultProfileName);

    CreateFile(m_setRegFilePath.Native(), TEST_INVALID_RESOURCE_MAPPING_SETREG);
    m_awsCoreConfiguration->ReloadConfiguration();

    actualConfigFilePath = m_awsCoreConfiguration->GetResourceMappingConfigFilePath();
    actualProfileName = m_awsCoreConfiguration->GetProfileName();
    EXPECT_TRUE(actualConfigFilePath.empty());
    EXPECT_TRUE(actualProfileName == AWSCoreConfiguration::AWSCoreDefaultProfileName);
}

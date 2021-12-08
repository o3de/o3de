/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/Utils.h>

#include <ProjectManagerSettings.h>

namespace O3DE::ProjectManager
{
    class ProjectManagerSettingsTests 
        : public ::UnitTest::ScopedAllocatorSetupFixture
    {
    public:
        ~ProjectManagerSettingsTests() override = default;
        void SetUp() override
        {
            UnitTest::ScopedAllocatorSetupFixture::SetUp();

            m_registry = AZStd::make_unique<AZ::SettingsRegistryImpl>();
            // Store off the old global settings registry to restore after each test
            m_oldSettingsRegistry = AZ::SettingsRegistry::Get();
            if (m_oldSettingsRegistry != nullptr)
            {
                AZ::SettingsRegistry::Unregister(m_oldSettingsRegistry);
            }
            AZ::SettingsRegistry::Register(m_registry.get());

            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
            m_registrationContext = AZStd::make_unique<AZ::JsonRegistrationContext>();

            m_registry->SetContext(m_serializeContext.get());
            m_registry->SetContext(m_registrationContext.get());

            AZ::JsonSystemComponent::Reflect(m_registrationContext.get());

            m_serializeContext->RegisterGenericType<AZStd::set<AZStd::string>>();

            m_projectInfo.m_path = "Z:/ProjectTestPath";
        }

        void TearDown() override
        {
            m_registrationContext->EnableRemoveReflection();
            AZ::JsonSystemComponent::Reflect(m_registrationContext.get());
            m_registrationContext->DisableRemoveReflection();

            m_registrationContext.reset();
            m_serializeContext.reset();

            // Restore the old global settings registry
            AZ::SettingsRegistry::Unregister(m_registry.get());
            if (m_oldSettingsRegistry != nullptr)
            {
                AZ::SettingsRegistry::Register(m_oldSettingsRegistry);
                m_oldSettingsRegistry = nullptr;
            }
            m_registry.reset();

            UnitTest::ScopedAllocatorSetupFixture::TearDown();
        }

    protected:
        const QString m_settingsPath = "/Testing/TestKey";
        const QString m_newSettingsPath = "/Testing/NewTestKey";
        ProjectInfo m_projectInfo;

    private:
        AZ::SettingsRegistryInterface* m_oldSettingsRegistry = nullptr;
        AZStd::unique_ptr<AZ::SettingsRegistryImpl> m_registry;
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::JsonRegistrationContext> m_registrationContext;
    };

    TEST_F(ProjectManagerSettingsTests, PMSettings_GetUnsetPathBool_ReturnsFalse)
    {
        bool settingsResult = false;
        EXPECT_FALSE(PMSettings::GetProjectManagerKey(settingsResult, m_settingsPath));
        EXPECT_FALSE(settingsResult);
    }

    TEST_F(ProjectManagerSettingsTests, PMSettings_SetAndGetValueBool_Success)
    {
        bool settingsResult = false;
        EXPECT_FALSE(PMSettings::GetProjectManagerKey(settingsResult, m_settingsPath));

        // Don't save to disk in test
        EXPECT_TRUE(PMSettings::SetProjectManagerKey(m_settingsPath, true, /*saveToDisk*/ false));

        settingsResult = false;
        EXPECT_TRUE(PMSettings::GetProjectManagerKey(settingsResult, m_settingsPath));
        EXPECT_TRUE(settingsResult);
    }

    TEST_F(ProjectManagerSettingsTests, PMSettings_GetUnsetPathString_ReturnsFalse)
    {
        QString settingsResult;
        EXPECT_FALSE(PMSettings::GetProjectManagerKey(settingsResult, m_settingsPath));
        EXPECT_TRUE(settingsResult.isEmpty());
    }

    TEST_F(ProjectManagerSettingsTests, PMSettings_SetAndGetValueString_Success)
    {
        QString settingsResult;
        EXPECT_FALSE(PMSettings::GetProjectManagerKey(settingsResult, m_settingsPath));

        QString settingsValue = "TestValue";

        // Don't save to disk in test
        EXPECT_TRUE(PMSettings::SetProjectManagerKey(m_settingsPath, settingsValue, /*saveToDisk*/ false));

        EXPECT_TRUE(PMSettings::GetProjectManagerKey(settingsResult, m_settingsPath));
        EXPECT_TRUE(settingsResult == settingsValue);
    }

    TEST_F(ProjectManagerSettingsTests, PMSettings_CopyStringRemoveOriginal_SuccessAndRemovesOriginal)
    {
        QString settingsResult;
        EXPECT_FALSE(PMSettings::GetProjectManagerKey(settingsResult, m_newSettingsPath));

        QString settingsValue = "TestValue";

        // Don't save to disk in test
        EXPECT_TRUE(PMSettings::SetProjectManagerKey(m_settingsPath, settingsValue, /*saveToDisk*/ false));

        EXPECT_TRUE(PMSettings::CopyProjectManagerKeyString(m_settingsPath, m_newSettingsPath, /*removeOrig*/ true, /*saveToDisk*/ false));

        // Check that old path value is removed
        EXPECT_FALSE(PMSettings::GetProjectManagerKey(settingsResult, m_settingsPath));

        EXPECT_TRUE(PMSettings::GetProjectManagerKey(settingsResult, m_newSettingsPath));
        EXPECT_TRUE(settingsResult == settingsValue);
    }

    TEST_F(ProjectManagerSettingsTests, PMSettings_RemoveProjectManagerKey_RemovesKey)
    {
        QString settingsResult;
        EXPECT_FALSE(PMSettings::GetProjectManagerKey(settingsResult, m_settingsPath));

        QString settingsValue = "TestValue";

        // Don't save to disk in test
        EXPECT_TRUE(PMSettings::SetProjectManagerKey(m_settingsPath, settingsValue, /*saveToDisk*/ false));
        EXPECT_TRUE(PMSettings::GetProjectManagerKey(settingsResult, m_settingsPath));

        EXPECT_TRUE(PMSettings::RemoveProjectManagerKey(m_settingsPath, /*saveToDisk*/ false));
        EXPECT_FALSE(PMSettings::GetProjectManagerKey(settingsResult, m_settingsPath));
    }

    TEST_F(ProjectManagerSettingsTests, PMSettings_GetUnsetBuildPath_ReturnsFalse)
    {
        bool buildResult = true;
        EXPECT_FALSE(PMSettings::GetProjectBuiltSuccessfully(buildResult, m_projectInfo));
        EXPECT_FALSE(buildResult);
    }

    TEST_F(ProjectManagerSettingsTests, PMSettings_SetProjectBuiltSuccessfully_ReturnsTrue)
    {
        // Don't save to disk in test
        EXPECT_TRUE(PMSettings::SetProjectBuiltSuccessfully(m_projectInfo, true, /*saveToDisk*/ false));

        bool buildResult = false;
        EXPECT_TRUE(PMSettings::GetProjectBuiltSuccessfully(buildResult, m_projectInfo));
        EXPECT_TRUE(buildResult);
    }

    TEST_F(ProjectManagerSettingsTests, PMSettings_SetProjectBuiltUnsuccessfully_ReturnsFalse)
    {
        // Don't save to disk in test
        EXPECT_TRUE(PMSettings::SetProjectBuiltSuccessfully(m_projectInfo, false, /*saveToDisk*/ false));

        bool buildResult = false;
        EXPECT_TRUE(PMSettings::GetProjectBuiltSuccessfully(buildResult, m_projectInfo));
        EXPECT_FALSE(buildResult);
    }
}

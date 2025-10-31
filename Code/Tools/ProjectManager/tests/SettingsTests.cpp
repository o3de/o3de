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

#include <Settings.h>

namespace O3DE::ProjectManager
{
    class SettingsTests 
        : public ::UnitTest::LeakDetectionFixture
    {
    public:
        ~SettingsTests() override = default;
        void SetUp() override
        {
            UnitTest::LeakDetectionFixture::SetUp();

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

            m_settings = AZStd::make_unique<Settings>(/*saveToDisk*/ false);

            m_projectInfo.m_path = "Z:/ProjectTestPath";
        }

        void TearDown() override
        {
            m_settings.reset();

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

            UnitTest::LeakDetectionFixture::TearDown();
        }

    protected:
        AZStd::unique_ptr<Settings> m_settings;
        const QString m_settingsPath = "/Testing/TestKey";
        const QString m_newSettingsPath = "/Testing/NewTestKey";
        ProjectInfo m_projectInfo;

    private:
        AZ::SettingsRegistryInterface* m_oldSettingsRegistry = nullptr;
        AZStd::unique_ptr<AZ::SettingsRegistryImpl> m_registry;
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::JsonRegistrationContext> m_registrationContext;
    };

    TEST_F(SettingsTests, Settings_GetUnsetPathBool_ReturnsFalse)
    {
        bool settingsResult = false;
        EXPECT_FALSE(m_settings->Get(settingsResult, m_settingsPath));
        EXPECT_FALSE(settingsResult);
    }

    TEST_F(SettingsTests, Settings_SetAndGetValueBool_Success)
    {
        bool settingsResult = false;
        EXPECT_FALSE(m_settings->Get(settingsResult, m_settingsPath));

        EXPECT_TRUE(m_settings->Set(m_settingsPath, true));

        settingsResult = false;
        EXPECT_TRUE(m_settings->Get(settingsResult, m_settingsPath));
        EXPECT_TRUE(settingsResult);
    }

    TEST_F(SettingsTests, Settings_GetUnsetPathString_ReturnsFalse)
    {
        QString settingsResult;
        EXPECT_FALSE(m_settings->Get(settingsResult, m_settingsPath));
        EXPECT_TRUE(settingsResult.isEmpty());
    }

    TEST_F(SettingsTests, Settings_SetAndGetValueString_Success)
    {
        QString settingsResult;
        EXPECT_FALSE(m_settings->Get(settingsResult, m_settingsPath));

        QString settingsValue = "TestValue";

        EXPECT_TRUE(m_settings->Set(m_settingsPath, settingsValue));

        EXPECT_TRUE(m_settings->Get(settingsResult, m_settingsPath));
        EXPECT_TRUE(settingsResult == settingsValue);
    }

    TEST_F(SettingsTests, Settings_CopyStringRemoveOriginal_SuccessAndRemovesOriginal)
    {
        QString settingsResult;
        EXPECT_FALSE(m_settings->Get(settingsResult, m_newSettingsPath));

        QString settingsValue = "TestValue";

        EXPECT_TRUE(m_settings->Set(m_settingsPath, settingsValue));

        EXPECT_TRUE(m_settings->Copy(m_settingsPath, m_newSettingsPath, /*removeOrig*/ true));

        // Check that old path value is removed
        EXPECT_FALSE(m_settings->Get(settingsResult, m_settingsPath));

        EXPECT_TRUE(m_settings->Get(settingsResult, m_newSettingsPath));
        EXPECT_TRUE(settingsResult == settingsValue);
    }

    TEST_F(SettingsTests, Settings_RemoveProjectManagerKey_RemovesKey)
    {
        QString settingsResult;
        EXPECT_FALSE(m_settings->Get(settingsResult, m_settingsPath));

        QString settingsValue = "TestValue";

        EXPECT_TRUE(m_settings->Set(m_settingsPath, settingsValue));
        EXPECT_TRUE(m_settings->Get(settingsResult, m_settingsPath));

        EXPECT_TRUE(m_settings->Remove(m_settingsPath));
        EXPECT_FALSE(m_settings->Get(settingsResult, m_settingsPath));
    }

    TEST_F(SettingsTests, Settings_GetUnsetBuildPath_ReturnsFalse)
    {
        bool buildResult = true;
        EXPECT_FALSE(m_settings->GetProjectBuiltSuccessfully(buildResult, m_projectInfo));
        EXPECT_FALSE(buildResult);
    }

    TEST_F(SettingsTests, Settings_SetProjectBuiltSuccessfully_ReturnsTrue)
    {
        EXPECT_TRUE(m_settings->SetProjectBuiltSuccessfully(m_projectInfo, true));

        bool buildResult = false;
        EXPECT_TRUE(m_settings->GetProjectBuiltSuccessfully(buildResult, m_projectInfo));
        EXPECT_TRUE(buildResult);
    }

    TEST_F(SettingsTests, Settings_SetProjectBuiltUnsuccessfully_ReturnsFalse)
    {
        EXPECT_TRUE(m_settings->SetProjectBuiltSuccessfully(m_projectInfo, false));

        bool buildResult = false;
        EXPECT_TRUE(m_settings->GetProjectBuiltSuccessfully(buildResult, m_projectInfo));
        EXPECT_FALSE(buildResult);
    }
}

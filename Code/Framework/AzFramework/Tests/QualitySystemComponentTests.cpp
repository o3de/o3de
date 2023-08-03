/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Console/Console.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzFramework/Quality/QualitySystemComponent.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace AzFramework;

    class QualitySystemComponentTestFixture : public LeakDetectionFixture
    {
    public:
        QualitySystemComponentTestFixture()
            : LeakDetectionFixture()
        {
        }

    protected:
        void SetUp() override
        {
            m_settingsRegistry = AZStd::make_unique<SettingsRegistryImpl>();
            AZ::SettingsRegistry::Register(m_settingsRegistry.get());

            m_console = AZStd::make_unique<AZ::Console>();
            m_console->LinkDeferredFunctors(AZ::ConsoleFunctorBase::GetDeferredHead());
            AZ::Interface<AZ::IConsole>::Register(m_console.get());

            m_qualitySystemComponent = AZStd::make_unique<QualitySystemComponent>();
        }

        void TearDown() override
        {
            // reset the console first so all the variables are moved to deferred head
            AZ::Interface<AZ::IConsole>::Unregister(m_console.get());
            m_console.reset();

            // next deactivate/reset to unlink from deferred head
            m_qualitySystemComponent->Deactivate();
            m_qualitySystemComponent.reset();


            AZ::SettingsRegistry::Unregister(m_settingsRegistry.get());
            m_settingsRegistry.reset();
        }

        AZStd::unique_ptr<AZ::Console> m_console;
        AZStd::unique_ptr<QualitySystemComponent> m_qualitySystemComponent;
        AZStd::unique_ptr<AZ::SettingsRegistryInterface> m_settingsRegistry;
    };

    TEST_F(QualitySystemComponentTestFixture, QualitySystem_Registers_Group_CVars)
    {
        auto result = m_settingsRegistry->MergeSettings(R"(
            {
                "O3DE": {
                    "Quality": {
                        "Groups": {
                            "q_test": {
                                "Levels": [ "low", "high" ],
                                "Default": 1,
                                "Description": "q_test quality group",
                                "Settings": {
                                    "q_test_sub": [0,1]
                                }
                            },
                            "q_test_sub": {
                                "Levels": [ "low", "high" ],
                                "Default": 0,
                                "Description": "q_test_sub quality group",
                                "Settings": {
                                    "a_cvar": [123,234]
                                }
                            }
                        }
                    }
                }
            }
            )",
            AZ::SettingsRegistryInterface::Format::JsonMergePatch,
            "");
        ASSERT_TRUE(result);

        // when the quality system component registers group cvars
        m_qualitySystemComponent->Activate();

        // expect the cvars are created with their default values
        int value = -1;
        EXPECT_EQ(m_console->GetCvarValue("q_test", value), AZ::GetValueResult::Success);
        EXPECT_EQ(value, 1);

        EXPECT_EQ(m_console->GetCvarValue("q_test_sub", value), AZ::GetValueResult::Success);
        EXPECT_EQ(value, 0);
    }

    AZ_CVAR(int32_t, a_setting, 0, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Example integer setting 1");
    AZ_CVAR(AZ::CVarFixedString, b_setting, "default", nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Example string setting 2");
    AZ_CVAR(int32_t, c_setting, -1, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Example integer setting 3");
    AZ_CVAR(int32_t, d_setting, -2, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Example integer setting 4");

    TEST_F(QualitySystemComponentTestFixture, QualitySystem_Loads_Group_Level)
    {
        auto result = m_settingsRegistry->MergeSettings(R"(
            {
                "O3DE": {
                    "Quality": {
                        "DefaultGroup":"q_test",
                        "Groups": {
                            "q_test": {
                                "Levels": [ "low", "medium","high", "veryhigh"],
                                "Default": 2,
                                "Description": "q_test quality group",
                                "Settings": {
                                    "a_setting": [0,1,2,3],
                                    "b_setting": ["a","b","c","d"],
                                    "q_test_sub": [0,1,1,1]
                                }
                            },
                            "q_test_sub": {
                                "Levels": [ "low", "high" ],
                                "Default": 0,
                                "Description": "q_test_sub quality group",
                                "Settings": {
                                    "c_setting": [123,234],
                                    "d_setting": [42] // test missing high level setting
                                }
                            }
                        }
                    }
                }
            }
            )",
            AZ::SettingsRegistryInterface::Format::JsonMergePatch,
            "");
        ASSERT_TRUE(result);

        // when the cvar values are first created 
        m_qualitySystemComponent->Activate();

        int intValue = -1;
        AZ::CVarFixedString stringValue;

        // expect the value  defaults
        EXPECT_EQ(m_console->GetCvarValue("a_setting", intValue), AZ::GetValueResult::Success);
        EXPECT_EQ(intValue, 0);
        EXPECT_EQ(m_console->GetCvarValue("b_setting", stringValue), AZ::GetValueResult::Success);
        EXPECT_STREQ(stringValue.c_str(), "default");
        EXPECT_EQ(m_console->GetCvarValue("c_setting", intValue), AZ::GetValueResult::Success);
        EXPECT_EQ(intValue, -1);
        EXPECT_EQ(m_console->GetCvarValue("d_setting", intValue), AZ::GetValueResult::Success);
        EXPECT_EQ(intValue, -2);

        // when the default group is loaded
        QualitySystemEvents::Bus::Broadcast(&QualitySystemEvents::LoadDefaultQualityGroup, QualitySystemEvents::LevelFromDeviceRules);

        // expect the values are set based on the default for q_test which is 2 
        EXPECT_EQ(m_console->GetCvarValue("q_test", intValue), AZ::GetValueResult::Success);
        EXPECT_EQ(intValue, 2);

        EXPECT_EQ(m_console->GetCvarValue("a_setting", intValue), AZ::GetValueResult::Success);
        EXPECT_EQ(intValue, 2);

        EXPECT_EQ(m_console->GetCvarValue("b_setting", stringValue), AZ::GetValueResult::Success);
        EXPECT_STREQ(stringValue.c_str(), "c");

        EXPECT_EQ(m_console->GetCvarValue("q_test_sub", intValue), AZ::GetValueResult::Success);
        EXPECT_EQ(intValue, 1);

        EXPECT_EQ(m_console->GetCvarValue("c_setting", intValue), AZ::GetValueResult::Success);
        EXPECT_EQ(intValue, 234);

        EXPECT_EQ(m_console->GetCvarValue("d_setting", intValue), AZ::GetValueResult::Success);
        EXPECT_EQ(intValue, 42);

        // when the group level is loaded 1 : "medium" ( which is "high" for q_test_sub )
        m_console->PerformCommand("q_test", { "1" });

        // expect the values are set based on the group level settings
        EXPECT_EQ(m_console->GetCvarValue("a_setting", intValue), AZ::GetValueResult::Success);
        EXPECT_EQ(intValue, 1);

        EXPECT_EQ(m_console->GetCvarValue("b_setting", stringValue), AZ::GetValueResult::Success);
        EXPECT_STREQ(stringValue.c_str(), "b");

        EXPECT_EQ(m_console->GetCvarValue("q_test_sub", intValue), AZ::GetValueResult::Success);
        EXPECT_EQ(intValue, 1);

        EXPECT_EQ(m_console->GetCvarValue("c_setting", intValue), AZ::GetValueResult::Success);
        EXPECT_EQ(intValue, 234);

        // d_settings doesn't specify a value for "high" so it should use highest available setting
        // which is "low" -> 42
        EXPECT_EQ(m_console->GetCvarValue("d_setting", intValue), AZ::GetValueResult::Success);
        EXPECT_EQ(intValue, 42);
    }
} // namespace UnitTest


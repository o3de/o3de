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
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/string/regex.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzFramework/Application/Application.h>
#include <AzFramework/Quality/QualitySystemComponent.h>
#include <AzFramework/Quality/QualityCVarGroup.h>
#include "DeviceAttribute/TestDeviceAttribute.h" 

namespace UnitTest
{
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
            using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;

            m_settingsRegistry = AZStd::make_unique<AZ::SettingsRegistryImpl>();
            AZ::SettingsRegistry::Register(m_settingsRegistry.get());

            m_application = AZStd::make_unique<AzFramework::Application>();

            // setup the runtime paths for the FileTagComponent
            auto projectPathKey = FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + "/project_path";
            AZ::IO::FixedMaxPath enginePath;
            m_settingsRegistry->Get(enginePath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
            m_settingsRegistry->Set(projectPathKey, (enginePath / "AutomatedTesting").Native());
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(*m_settingsRegistry);

            m_console = AZ::Interface<AZ::IConsole>::Get();
        }

        void TearDown() override
        {
            m_application->Stop();
            m_application.reset();

            AZ::SettingsRegistry::Unregister(m_settingsRegistry.get());
            m_settingsRegistry.reset();
        }

        void StartApplicationWithSettings(AZStd::string_view settings)
        {
            auto result = m_settingsRegistry->MergeSettings(settings,
                AZ::SettingsRegistryInterface::Format::JsonMergePatch,
                "");
            ASSERT_TRUE(result);

            // When the application starts and activates the QualitySystemComponent
            // it will register CVARS based on what is in the registry
            AZ::ComponentApplication::Descriptor desc;
            AZ::ComponentApplication::StartupParameters startupParameters;
            startupParameters.m_loadSettingsRegistry = false;
            startupParameters.m_loadAssetCatalog = false;
            m_application->Start(desc, startupParameters);

            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

            // Create and register a test device attribute for evaluating rules
            AZStd::string name { "TestAttribute" };
            AZStd::string description { "Description" };
            AZStd::string valueString{ "ValidAttribute" };
            AZStd::any value{ valueString };
            TestDeviceAttribute::EvalFunc eval = [valueString](AZStd::string_view rule)
            {
                auto regex = AZStd::regex(rule.data(), rule.size());
                return AZStd::regex_match(valueString, regex);
            };
            m_testDeviceAttribute = AZStd::make_shared<TestDeviceAttribute>(AZStd::move(name), AZStd::move(description), AZStd::move(value), AZStd::move(eval));
            auto registrar = AzFramework::DeviceAttributeRegistrar::Get();
            ASSERT_NE(nullptr, registrar);
            EXPECT_TRUE(registrar->RegisterDeviceAttribute(m_testDeviceAttribute));
        }

        AZ::IConsole* m_console;
        AZStd::unique_ptr<AzFramework::Application> m_application;
        AZStd::unique_ptr<AZ::SettingsRegistryInterface> m_settingsRegistry;
        AZStd::shared_ptr<TestDeviceAttribute> m_testDeviceAttribute;
    };

    TEST_F(QualitySystemComponentTestFixture, QualitySystem_Registers_Group_CVars)
    {
        // when the quality system component registers group cvars
        StartApplicationWithSettings(R"(
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
            )");

        // expect the cvars are created with their default values
        auto value = AzFramework::QualityLevel::LevelFromDeviceRules;
        EXPECT_EQ(m_console->GetCvarValue("q_test", value), AZ::GetValueResult::Success);
        EXPECT_EQ(value, AzFramework::QualityLevel{1});

        EXPECT_EQ(m_console->GetCvarValue("q_test_sub", value), AZ::GetValueResult::Success);
        EXPECT_EQ(value, AzFramework::QualityLevel::DefaultQualityLevel);
    }

    AZ_CVAR(int32_t, a_setting, 0, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Example integer setting 1");
    AZ_CVAR(AZ::CVarFixedString, b_setting, "default", nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Example string setting 2");
    AZ_CVAR(int32_t, c_setting, -1, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Example integer setting 3");
    AZ_CVAR(double, d_setting, -2.0, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Example double setting");

    TEST_F(QualitySystemComponentTestFixture, QualitySystem_Loads_Group_Level)
    {
        // when the quality system component registers group cvars
        StartApplicationWithSettings(R"(
            {
                "O3DE": {
                    "Quality": {
                        "DefaultGroup":"q_test",
                        "Groups": {
                            "q_test": {
                                "Levels": [ "low", "medium", "high", "veryhigh"],
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
                                "Default": "DefaultQualityLevel",
                                "Description": "q_test_sub quality group",
                                "Settings": {
                                    "c_setting": [123,234],
                                    "d_setting": [42.0] // test missing high level setting
                                }
                            }
                        }
                    }
                }
            }
            )");

        auto value = AzFramework::QualityLevel::LevelFromDeviceRules;
        int32_t intValue = -42;
        double doubleValue = -42.0;
        AZ::CVarFixedString stringValue;

        // expect the value defaults
        EXPECT_EQ(m_console->GetCvarValue("a_setting", intValue), AZ::GetValueResult::Success);
        EXPECT_EQ(intValue, 0);
        EXPECT_EQ(m_console->GetCvarValue("b_setting", stringValue), AZ::GetValueResult::Success);
        EXPECT_EQ(stringValue, "default");
        EXPECT_EQ(m_console->GetCvarValue("c_setting", intValue), AZ::GetValueResult::Success);
        EXPECT_EQ(intValue, -1);
        EXPECT_EQ(m_console->GetCvarValue("d_setting", doubleValue), AZ::GetValueResult::Success);
        EXPECT_EQ(doubleValue, -2.0);

        // when the default group is loaded
        AzFramework::QualitySystemEvents::Bus::Broadcast(
            &AzFramework::QualitySystemEvents::LoadDefaultQualityGroup,
            AzFramework::QualityLevel::LevelFromDeviceRules);

        // expect the values are set based on the default for q_test which is 2 
        EXPECT_EQ(m_console->GetCvarValue("q_test", value), AZ::GetValueResult::Success);
        EXPECT_EQ(value, AzFramework::QualityLevel{2});

        EXPECT_EQ(m_console->GetCvarValue("a_setting", intValue), AZ::GetValueResult::Success);
        EXPECT_EQ(intValue, 2);

        EXPECT_EQ(m_console->GetCvarValue("b_setting", stringValue), AZ::GetValueResult::Success);
        EXPECT_EQ(stringValue, "c");

        EXPECT_EQ(m_console->GetCvarValue("q_test_sub", value), AZ::GetValueResult::Success);
        EXPECT_EQ(value, AzFramework::QualityLevel{1});

        EXPECT_EQ(m_console->GetCvarValue("c_setting", intValue), AZ::GetValueResult::Success);
        EXPECT_EQ(intValue, 234);

        EXPECT_EQ(m_console->GetCvarValue("d_setting", doubleValue), AZ::GetValueResult::Success);
        EXPECT_EQ(doubleValue, 42.0);

        // when the group level 1 is loaded ("medium" which is "high" for q_test_sub)
        m_console->PerformCommand("q_test", { "1" });

        // expect the values are set based on the group level settings
        EXPECT_EQ(m_console->GetCvarValue("a_setting", intValue), AZ::GetValueResult::Success);
        EXPECT_EQ(intValue, 1);

        EXPECT_EQ(m_console->GetCvarValue("b_setting", stringValue), AZ::GetValueResult::Success);
        EXPECT_EQ(stringValue, "b");

        EXPECT_EQ(m_console->GetCvarValue("q_test_sub", value), AZ::GetValueResult::Success);
        EXPECT_EQ(value, AzFramework::QualityLevel{1});

        EXPECT_EQ(m_console->GetCvarValue("c_setting", intValue), AZ::GetValueResult::Success);
        EXPECT_EQ(intValue, 234);

        // d_settings doesn't specify a value for "high" so it should use highest available setting
        // which is "low" -> 42
        EXPECT_EQ(m_console->GetCvarValue("d_setting", doubleValue), AZ::GetValueResult::Success);
        EXPECT_EQ(doubleValue, 42.0);

        // when the group level 0 is loaded (low) using mixed-case name 
        m_console->PerformCommand("q_test", { "LoW" });

        // settings at index 0 are correctly loaded
        EXPECT_EQ(m_console->GetCvarValue("a_setting", intValue), AZ::GetValueResult::Success);
        EXPECT_EQ(intValue, 0);

        EXPECT_EQ(m_console->GetCvarValue("b_setting", stringValue), AZ::GetValueResult::Success);
        EXPECT_EQ(stringValue, "a");

        EXPECT_EQ(m_console->GetCvarValue("q_test_sub", value), AZ::GetValueResult::Success);
        EXPECT_EQ(value, AzFramework::QualityLevel{0});

        EXPECT_EQ(m_console->GetCvarValue("c_setting", intValue), AZ::GetValueResult::Success);
        EXPECT_EQ(intValue, 123);

        EXPECT_EQ(m_console->GetCvarValue("d_setting", doubleValue), AZ::GetValueResult::Success);
        EXPECT_EQ(doubleValue, 42.0);
    }

    TEST_F(QualitySystemComponentTestFixture, QualitySystem_Evaluates_Device_Rules)
    {
        // when the quality system component registers group cvars
        StartApplicationWithSettings(R"(
            {
                "O3DE": {
                    "Quality": {
                        "DefaultGroup":"q_test",
                        "Groups": {
                            "q_test": {
                                "Levels": [ "low", "medium", "high", "veryhigh"],
                                "Default": 2,
                                "Description": "q_test quality group",
                                "Settings": {
                                    "d_setting": [0.0,1.0,4.0,8.0],
                                    "q_test_sub": [0,1,1,1]
                                }
                            },
                            "q_test_sub": {
                                "Levels": [ "low", "high" ],
                                "Default": "DefaultQualityLevel",
                                "Description": "q_test_sub quality group",
                                "Settings": {
                                    "c_setting": [123,234],
                                    "d_setting": [42.0]
                                }
                            }
                        },
                        "Devices": {
                            // settings from these rule sections will be applied
                            // based on device rule resolution
                            "Rule Group 1": {
                                "Rules": {
                                    "Test Device": { "TestAttribute": "ValidAttribute" }
                                },
                                "Settings": {
                                    "q_test": 1,
                                    "c_setting": 4,
                                    "d_setting": 1.0
                                }
                            },
                            "Rule Group 2": {
                                "Rules": {
                                    "Test Device": { "TestAttribute": "ValidAttribute" }
                                },
                                "Settings": {
                                    "q_test": 3,
                                    "c_setting": 1,
                                    "d_setting": 2.0
                                }
                            },
                            // this group should not match
                            "Invalid Rule Group": {
                                "Rules": {
                                    "Test Device": { "TestAttribute": "InvalidAttribute" }
                                },
                                "Settings": {
                                    "q_test": 0,
                                    "c_setting": 0,
                                    "d_setting": 0.0
                                }
                            }
                        }
                    }
                }
            }
            )");

        // when the default group is loaded
        AzFramework::QualitySystemEvents::Bus::Broadcast(
            &AzFramework::QualitySystemEvents::LoadDefaultQualityGroup,
            AzFramework::QualityLevel::LevelFromDeviceRules);

        // expect the values are set based on the device rules and the default resolution order of "Last"
        auto cvarValue = AzFramework::QualityLevel::LevelFromDeviceRules;
        EXPECT_EQ(m_console->GetCvarValue("q_test", cvarValue), AZ::GetValueResult::Success);
        EXPECT_EQ(cvarValue, AzFramework::QualityLevel{3});

        int32_t intValue = -42;
        EXPECT_EQ(m_console->GetCvarValue("c_setting", intValue), AZ::GetValueResult::Success);
        EXPECT_EQ(intValue, 1);

        double doubleValue = -2.0;
        EXPECT_EQ(m_console->GetCvarValue("d_setting", doubleValue), AZ::GetValueResult::Success);
        EXPECT_EQ(doubleValue, 2.0);

        // when the rule resolution order is changed to First and default groups are loaded
        m_settingsRegistry.get()->SetObject("/O3DE/Quality/DeviceRulesResolution", AzFramework::DeviceRuleResolution::First);
        AzFramework::QualitySystemEvents::Bus::Broadcast(
            &AzFramework::QualitySystemEvents::LoadDefaultQualityGroup,
            AzFramework::QualityLevel::LevelFromDeviceRules);

        // expect the values are set based on the device rules and the resolution order of "First"
        EXPECT_EQ(m_console->GetCvarValue("q_test", cvarValue), AZ::GetValueResult::Success);
        EXPECT_EQ(cvarValue, AzFramework::QualityLevel{1});

        EXPECT_EQ(m_console->GetCvarValue("c_setting", intValue), AZ::GetValueResult::Success);
        EXPECT_EQ(intValue, 4);

        EXPECT_EQ(m_console->GetCvarValue("d_setting", doubleValue), AZ::GetValueResult::Success);
        EXPECT_EQ(doubleValue, 1.0);

        // when the rule resolution order is changed to Min and default groups are loaded
        m_settingsRegistry.get()->SetObject("/O3DE/Quality/DeviceRulesResolution", AzFramework::DeviceRuleResolution::Min);
        AzFramework::QualitySystemEvents::Bus::Broadcast(
            &AzFramework::QualitySystemEvents::LoadDefaultQualityGroup,
            AzFramework::QualityLevel::LevelFromDeviceRules);

        // expect the values are set based on the device rules and the default resolution rule of "Min"
        EXPECT_EQ(m_console->GetCvarValue("q_test", cvarValue), AZ::GetValueResult::Success);
        EXPECT_EQ(cvarValue, AzFramework::QualityLevel{1});

        EXPECT_EQ(m_console->GetCvarValue("c_setting", intValue), AZ::GetValueResult::Success);
        EXPECT_EQ(intValue, 1);

        EXPECT_EQ(m_console->GetCvarValue("d_setting", doubleValue), AZ::GetValueResult::Success);
        EXPECT_EQ(doubleValue, 1.0);

        // when the rule resolution order is changed to Max and default groups are loaded
        m_settingsRegistry.get()->SetObject("/O3DE/Quality/DeviceRulesResolution", AzFramework::DeviceRuleResolution::Max);
        AzFramework::QualitySystemEvents::Bus::Broadcast(
            &AzFramework::QualitySystemEvents::LoadDefaultQualityGroup,
            AzFramework::QualityLevel::LevelFromDeviceRules);

        // expect the values are set based on the device rules and the default resolution rule of "Max"
        EXPECT_EQ(m_console->GetCvarValue("q_test", cvarValue), AZ::GetValueResult::Success);
        EXPECT_EQ(cvarValue, AzFramework::QualityLevel{3});

        // IMPORTANT the value for c_setting in the second group is lower than the value for
        // c_setting in the first rule group so it is ignored and would have been 4 but
        // the value for q_test is higher in the second group and it is a group cvar which
        // will set c_setting to 234
        EXPECT_EQ(m_console->GetCvarValue("c_setting", intValue), AZ::GetValueResult::Success);
        EXPECT_EQ(intValue, 234);

        // d_setting will be 2.0 because the value in the second group is higher than the
        // value in the first group
        EXPECT_EQ(m_console->GetCvarValue("d_setting", doubleValue), AZ::GetValueResult::Success);
        EXPECT_EQ(doubleValue, 2.0);
    }

} // namespace UnitTest


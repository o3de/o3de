/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gmock/gmock.h>
#include <AzTest/AzTest.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/UnitTest/Mocks/MockFileIOBase.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <Config/Components/SceneProcessingConfigSystemComponent.h>
#include <Config/SettingsObjects/FileSoftNameSetting.h>
#include <Config/SettingsObjects/NodeSoftNameSetting.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>

class SceneProcessingConfigTest
    : public UnitTest::LeakDetectionFixture
    , public AZ::Debug::TraceMessageBus::Handler
{
public:
    void SetUp() override
    {
        m_settingsRegistry = AZStd::make_unique<AZ::SettingsRegistryImpl>();
        AZ::SettingsRegistry::Register(m_settingsRegistry.get());

        m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
        m_registrationContext = AZStd::make_unique<AZ::JsonRegistrationContext>();
        m_settingsRegistry->SetContext(m_serializeContext.get());
        m_settingsRegistry->SetContext(m_registrationContext.get());

        AZ::SettingsRegistryInterface* registry = AZ::SettingsRegistry::Get();
        auto projectPathKey = AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + "/project_path";
        AZ::IO::FixedMaxPath enginePath;
        registry->Get(enginePath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        registry->Set(projectPathKey, (enginePath / "AutomatedTesting").Native());
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(*registry);

        m_fileIOMock = AZStd::make_unique<testing::NiceMock<AZ::IO::MockFileIOBase>>();
        m_prevFileIO = AZ::IO::FileIOBase::GetInstance();
        AZ::IO::FileIOBase::SetInstance(nullptr);
        AZ::IO::FileIOBase::SetInstance(m_fileIOMock.get());
        ASSERT_EQ(m_fileIOMock.get(), AZ::IO::FileIOBase::GetInstance());

        ON_CALL(*m_fileIOMock.get(), ResolvePath(::testing::_, ::testing::_))
            .WillByDefault([](AZ::IO::FixedMaxPath& resolvedPath, const AZ::IO::PathView& path) -> bool
            {
                resolvedPath = AZ::IO::FixedMaxPath("/fake/path") / path.Filename();
                return true;
            }
        );

        ON_CALL(*m_fileIOMock.get(), Exists(::testing::_))
            .WillByDefault([](const char*) -> bool
            {
                return true;
            }
        );

        AZ::Debug::TraceMessageBus::Handler::BusConnect();
    }

    void TearDown() override
    {
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();

        EXPECT_EQ(m_fileIOMock.get(), AZ::IO::FileIOBase::GetInstance());
        AZ::IO::FileIOBase::SetInstance(nullptr);
        AZ::IO::FileIOBase::SetInstance(m_prevFileIO);
        m_fileIOMock.reset();

        AZ::SettingsRegistry::Unregister(m_settingsRegistry.get());

        m_registrationContext.reset();
        m_serializeContext.reset();
        m_settingsRegistry.reset();
    }


    void ReflectTypes()
    {
        AZ::JsonSystemComponent::Reflect(m_registrationContext.get());

        // PatternMatcher is defined in SceneCore. Avoid loading the dynamic-link library in the test by
        // just binding the class for serialization.
        m_serializeContext->Class<AZ::SceneAPI::SceneCore::PatternMatcher>();
        AZ::SceneProcessingConfig::SoftNameSetting::Reflect(m_serializeContext.get());
        AZ::SceneProcessingConfig::NodeSoftNameSetting::Reflect(m_serializeContext.get());
        AZ::SceneProcessingConfig::FileSoftNameSetting::Reflect(m_serializeContext.get());
    }

    void RemoveReflectedTypes()
    {
        m_serializeContext->EnableRemoveReflection();
        AZ::SceneProcessingConfig::FileSoftNameSetting::Reflect(m_serializeContext.get());
        AZ::SceneProcessingConfig::NodeSoftNameSetting::Reflect(m_serializeContext.get());
        AZ::SceneProcessingConfig::SoftNameSetting::Reflect(m_serializeContext.get());
        m_serializeContext->Class<AZ::SceneAPI::SceneCore::PatternMatcher>();
        m_serializeContext->DisableRemoveReflection();

        m_registrationContext->EnableRemoveReflection();
        AZ::JsonSystemComponent::Reflect(m_registrationContext.get());
        m_registrationContext->DisableRemoveReflection();
    }

    // AZ::Debug::TraceMessageBus::Handler overrides
    bool OnWarning(const char* window, const char* message) override
    {
        AZ_UNUSED(window);
        AZ_UNUSED(message);

        ++m_warningsCount;

        return true;
    }

    AZStd::unique_ptr<AZ::SettingsRegistryImpl> m_settingsRegistry;
    AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
    AZStd::unique_ptr<AZ::JsonRegistrationContext> m_registrationContext;
    AZStd::unique_ptr<::testing::NiceMock<AZ::IO::MockFileIOBase>> m_fileIOMock;
    AZ::IO::FileIOBase* m_prevFileIO = nullptr;
    int m_warningsCount = 0;
};

TEST_F(SceneProcessingConfigTest, SceneProcessingConfigSystemComponent_EmptySetReg_ReturnsEmptyGetScriptConfigList)
{
    AZ::SceneProcessingConfig::SceneProcessingConfigSystemComponent sceneProcessingConfigSystemComponent;
    sceneProcessingConfigSystemComponent.Activate();
    AZStd::vector<AZ::SceneAPI::Events::ScriptConfig> scriptConfigList;
    sceneProcessingConfigSystemComponent.GetScriptConfigList(scriptConfigList);
    EXPECT_TRUE(scriptConfigList.empty());
    sceneProcessingConfigSystemComponent.Deactivate();
}

TEST_F(SceneProcessingConfigTest, SceneProcessingConfigSystemComponent_ProperlySetup_ReturnsCompleteList)
{
    const char* settings = R"JSON(
    {
        "O3DE": {
            "AssetProcessor": {
                "SceneBuilder": {
                  "defaultScripts": {
                    "fooPattern": "@projectroot@/test_foo.py",
                    "barPattern": "@projectroot@/test_bar.py",
                    "badValue": 1
                  }
                }
            }
        }
    }
    )JSON";
    m_settingsRegistry->MergeSettings(settings, AZ::SettingsRegistryInterface::Format::JsonMergePatch);
    
    AZ::SceneProcessingConfig::SceneProcessingConfigSystemComponent sceneProcessingConfigSystemComponent;
    sceneProcessingConfigSystemComponent.Activate();
    AZStd::vector<AZ::SceneAPI::Events::ScriptConfig> scriptConfigList;
    sceneProcessingConfigSystemComponent.GetScriptConfigList(scriptConfigList);
    EXPECT_EQ(scriptConfigList.size(), 2);
    sceneProcessingConfigSystemComponent.Deactivate();
}

TEST_F(SceneProcessingConfigTest, SceneProcessingConfigSystemComponent_ScriptConfigEventBus_IsEnabled)
{
    const char* settings = R"JSON(
    {
        "O3DE": {
            "AssetProcessor": {
                "SceneBuilder": {
                  "defaultScripts": {
                    "fooPattern": "@projectroot@/test_foo.py"
                  }
                }
            }
        }
    }
    )JSON";
    m_settingsRegistry->MergeSettings(settings, AZ::SettingsRegistryInterface::Format::JsonMergePatch);

    AZ::SceneProcessingConfig::SceneProcessingConfigSystemComponent sceneProcessingConfigSystemComponent;
    sceneProcessingConfigSystemComponent.Activate();
    AZStd::vector<AZ::SceneAPI::Events::ScriptConfig> scriptConfigList;
    AZ::SceneAPI::Events::ScriptConfigEventBus::Broadcast(
        &AZ::SceneAPI::Events::ScriptConfigEventBus::Events::GetScriptConfigList,
        scriptConfigList);
    EXPECT_EQ(scriptConfigList.size(), 1);
    sceneProcessingConfigSystemComponent.Deactivate();
}

TEST_F(SceneProcessingConfigTest, SceneProcessingConfigSystemComponent_ScriptConfigEventBus_MatchesScriptConfig)
{
    const char* settings = R"JSON(
    {
        "O3DE": {
            "AssetProcessor": {
                "SceneBuilder": {
                  "defaultScripts": {
                    "foo*": "@projectroot@/test_foo.py"
                  }
                }
            }
        }
    }
    )JSON";
    m_settingsRegistry->MergeSettings(settings, AZ::SettingsRegistryInterface::Format::JsonMergePatch);

    AZ::SceneProcessingConfig::SceneProcessingConfigSystemComponent sceneProcessingConfigSystemComponent;
    sceneProcessingConfigSystemComponent.Activate();
    AZStd::optional<AZ::SceneAPI::Events::ScriptConfig> result;
    AZ::SceneAPI::Events::ScriptConfigEventBus::BroadcastResult(
        result,
        &AZ::SceneAPI::Events::ScriptConfigEventBus::Events::MatchesScriptConfig,
        "fake/folder/foo_bar.asset");
        EXPECT_TRUE(result.has_value());
    sceneProcessingConfigSystemComponent.Deactivate();
}

TEST_F(SceneProcessingConfigTest, SceneProcessingConfigSystemComponent_SoftNameSettings_MatchesSettingRegistry)
{
    const char* settings = R"JSON(
    {
        "O3DE": {
            "AssetProcessor": {
                "SceneBuilder": {
                    "NodeSoftNameSettings": [
                        {
                            "pattern": {
                                "pattern": "^.*_[Ll][Oo][Dd]_?1(_optimized)?$",
                                "matcher": 2
                            },
                            "virtualType": "LODMesh1",
                            "includeChildren": true
                        }
                    ],
                    "FileSoftNameSettings": [
                        {
                            "pattern": {
                                "pattern": "_anim",
                                "matcher": 1
                            },
                            "virtualType": "Ignore",
                            "inclusiveList": false,
                            "graphTypes": {
                                "types": [
                                    {
                                        "name": "IAnimationData"
                                    }
                                ]
                            }
                        }
                    ]
                }
            }
        }
    }  
    )JSON";
    m_settingsRegistry->MergeSettings(settings, AZ::SettingsRegistryInterface::Format::JsonMergePatch);

    ReflectTypes();

    AZ::SceneProcessingConfig::SceneProcessingConfigSystemComponent sceneProcessingConfigSystemComponent;
    sceneProcessingConfigSystemComponent.Activate();

    const AZStd::vector<AZStd::unique_ptr<AZ::SceneProcessingConfig::SoftNameSetting>>* result = nullptr;
    AZ::SceneProcessingConfig::SceneProcessingConfigRequestBus::BroadcastResult(
        result,
        &AZ::SceneProcessingConfig::SceneProcessingConfigRequests::GetSoftNames);
    EXPECT_EQ(result->size(), 2);
    EXPECT_EQ(result->at(0)->GetVirtualType(), "LODMesh1");
    EXPECT_EQ(result->at(1)->GetVirtualType(), "Ignore");

    sceneProcessingConfigSystemComponent.Deactivate();

    RemoveReflectedTypes();
}

TEST_F(SceneProcessingConfigTest, SceneProcessingConfigSystemComponent_SoftNameSettings_AddSoftNameSettingsWithDifferentTypeIdAndSameVirtualType)
{
    const char* settings = R"JSON(
    {
        "O3DE": {
            "AssetProcessor": {
                "SceneBuilder": {
                    "NodeSoftNameSettings": [
                        {
                            "pattern": {
                                "pattern": "^.*_[Ll][Oo][Dd]_?1(_optimized)?$",
                                "matcher": 2
                            },
                            "virtualType": "Ignore",
                            "includeChildren": true
                        }
                    ],
                    "FileSoftNameSettings": [
                        {
                            "pattern": {
                                "pattern": "_anim",
                                "matcher": 1
                            },
                            "virtualType": "Ignore",
                            "inclusiveList": false,
                            "graphTypes": {
                                "types": [
                                    {
                                        "name": "IAnimationData"
                                    }
                                ]
                            }
                        }
                    ]
                }
            }
        }
    }  
    )JSON";
    m_settingsRegistry->MergeSettings(settings, AZ::SettingsRegistryInterface::Format::JsonMergePatch);

    ReflectTypes();

    AZ::SceneProcessingConfig::SceneProcessingConfigSystemComponent sceneProcessingConfigSystemComponent;
    sceneProcessingConfigSystemComponent.Activate();

    const AZStd::vector<AZStd::unique_ptr<AZ::SceneProcessingConfig::SoftNameSetting>>* result = nullptr;
    AZ::SceneProcessingConfig::SceneProcessingConfigRequestBus::BroadcastResult(
        result,
        &AZ::SceneProcessingConfig::SceneProcessingConfigRequests::GetSoftNames);
    EXPECT_EQ(result->size(), 2);
    EXPECT_EQ(result->at(0)->GetVirtualType(), "Ignore");
    EXPECT_EQ(result->at(1)->GetVirtualType(), "Ignore");

    sceneProcessingConfigSystemComponent.Deactivate();

    RemoveReflectedTypes();
}

TEST_F(SceneProcessingConfigTest, SceneProcessingConfigSystemComponent_SoftNameSettings_IgnoreSoftNameSettingsWithSameTypeIdAndSameVirtualType)
{
    const char* settings = R"JSON(
    {
        "O3DE": {
            "AssetProcessor": {
                "SceneBuilder": {
                    "NodeSoftNameSettings": [
                        {
                            "pattern": {
                                "pattern": "^.*_[Ll][Oo][Dd]_?1(_optimized)?$",
                                "matcher": 2
                            },
                            "virtualType": "LODMesh1",
                            "includeChildren": true
                        },
                        {
                            "pattern": {
                                "pattern": "^.*_[Ll][Oo][Dd]_?1(_optimized)?$",
                                "matcher": 0
                            },
                            "virtualType": "LODMesh1",
                            "includeChildren": true
                        },
                        {
                            "pattern": {
                                "pattern": "^.*_[Ll][Oo][Dd]_?2(_optimized)?$",
                                "matcher": 2
                            },
                            "virtualType": "LODMesh2",
                            "includeChildren": true
                        }
                    ]
                }
            }
        }
    }  
    )JSON";
    m_settingsRegistry->MergeSettings(settings, AZ::SettingsRegistryInterface::Format::JsonMergePatch);

    ReflectTypes();

    AZ_TEST_START_TRACE_SUPPRESSION;
    // Expect to get one error when adding duplicate soft name settings
    AZ::SceneProcessingConfig::SceneProcessingConfigSystemComponent sceneProcessingConfigSystemComponent;
    AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    sceneProcessingConfigSystemComponent.Activate();

    const AZStd::vector<AZStd::unique_ptr<AZ::SceneProcessingConfig::SoftNameSetting>>* result = nullptr;
    AZ::SceneProcessingConfig::SceneProcessingConfigRequestBus::BroadcastResult(
        result,
        &AZ::SceneProcessingConfig::SceneProcessingConfigRequests::GetSoftNames);

    EXPECT_EQ(result->size(), 2);
    EXPECT_EQ(result->at(0)->GetVirtualType(), "LODMesh1");
    EXPECT_EQ(result->at(1)->GetVirtualType(), "LODMesh2");

    sceneProcessingConfigSystemComponent.Deactivate();

    RemoveReflectedTypes();
}

TEST_F(SceneProcessingConfigTest, SceneProcessingConfigSystemComponent_SoftNameSettings_WarningWithoutSettingsRegistry)
{
    // Expect to get one warning when soft name settings cannot be read from the settings registry
    AZ::SceneProcessingConfig::SceneProcessingConfigSystemComponent sceneProcessingConfigSystemComponent;
    EXPECT_EQ(m_warningsCount, 1);
    sceneProcessingConfigSystemComponent.Activate();

    const AZStd::vector<AZStd::unique_ptr<AZ::SceneProcessingConfig::SoftNameSetting>>* result = nullptr;
    AZ::SceneProcessingConfig::SceneProcessingConfigRequestBus::BroadcastResult(
        result,
        &AZ::SceneProcessingConfig::SceneProcessingConfigRequests::GetSoftNames);

    EXPECT_EQ(result->size(), 0);

    sceneProcessingConfigSystemComponent.Deactivate();
}

TEST_F(SceneProcessingConfigTest, SceneProcessingConfigSystemComponent_SoftNameSettings_NoWarningWithEmptySettingsRegistry)
{
    const char* settings = R"JSON(
    {
        "O3DE": {
            "AssetProcessor": {
                "SceneBuilder": {
                    "NodeSoftNameSettings": [
                    ],
                    "FileSoftNameSettings": [
                    ]
                }
            }
        }
    }  
    )JSON";
    m_settingsRegistry->MergeSettings(settings, AZ::SettingsRegistryInterface::Format::JsonMergePatch);

    ReflectTypes();

    AZ::SceneProcessingConfig::SceneProcessingConfigSystemComponent sceneProcessingConfigSystemComponent;
    EXPECT_EQ(m_warningsCount, 0);
    sceneProcessingConfigSystemComponent.Activate();

    const AZStd::vector<AZStd::unique_ptr<AZ::SceneProcessingConfig::SoftNameSetting>>* result = nullptr;

    AZ::SceneProcessingConfig::SceneProcessingConfigRequestBus::BroadcastResult(
        result,
        &AZ::SceneProcessingConfig::SceneProcessingConfigRequests::GetSoftNames);

    EXPECT_EQ(result->size(), 0);

    sceneProcessingConfigSystemComponent.Deactivate();

    RemoveReflectedTypes();
}

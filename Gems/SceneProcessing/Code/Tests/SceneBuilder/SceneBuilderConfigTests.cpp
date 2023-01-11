/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gmock/gmock.h>
#include <AzTest/AzTest.h>
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
{
public:
    void SetUp() override
    {
        m_settingsRegistry = AZStd::make_unique<AZ::SettingsRegistryImpl>();
        AZ::SettingsRegistry::Register(m_settingsRegistry.get());

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
    }

    void TearDown() override
    {
        EXPECT_EQ(m_fileIOMock.get(), AZ::IO::FileIOBase::GetInstance());
        AZ::IO::FileIOBase::SetInstance(nullptr);
        AZ::IO::FileIOBase::SetInstance(m_prevFileIO);
        m_fileIOMock.reset();

        AZ::SettingsRegistry::Unregister(m_settingsRegistry.get());
    }


    void ReflectTypes(AZ::JsonRegistrationContext* registrationContext, AZ::SerializeContext* serializeContext)
    {
        AZ::JsonSystemComponent::Reflect(registrationContext);

        // PatternMatcher is defined in SceneCore. Avoid loading the dynamic-link library in the test by
        // just binding the class for serialization.
        serializeContext->Class<AZ::SceneAPI::SceneCore::PatternMatcher>();
        AZ::SceneProcessingConfig::SoftNameSetting::Reflect(serializeContext);
        AZ::SceneProcessingConfig::NodeSoftNameSetting::Reflect(serializeContext);
        AZ::SceneProcessingConfig::FileSoftNameSetting::Reflect(serializeContext);
    }

    void RemoveReflectedTypes(AZ::JsonRegistrationContext* registrationContext, AZ::SerializeContext* serializeContext)
    {
        serializeContext->EnableRemoveReflection();
        AZ::SceneProcessingConfig::FileSoftNameSetting::Reflect(serializeContext);
        AZ::SceneProcessingConfig::NodeSoftNameSetting::Reflect(serializeContext);
        AZ::SceneProcessingConfig::SoftNameSetting::Reflect(serializeContext);
        serializeContext->Class<AZ::SceneAPI::SceneCore::PatternMatcher>();
        serializeContext->DisableRemoveReflection();

        registrationContext->EnableRemoveReflection();
        AZ::JsonSystemComponent::Reflect(registrationContext);
        registrationContext->DisableRemoveReflection();
    }

    AZStd::unique_ptr<AZ::SettingsRegistryImpl> m_settingsRegistry;
    AZStd::unique_ptr<::testing::NiceMock<AZ::IO::MockFileIOBase>> m_fileIOMock;
    AZ::IO::FileIOBase* m_prevFileIO = nullptr;
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

    AZStd::unique_ptr<AZ::SerializeContext> serializeContext = AZStd::make_unique<AZ::SerializeContext>();
    AZStd::unique_ptr<AZ::JsonRegistrationContext> registrationContext = AZStd::make_unique<AZ::JsonRegistrationContext>();
    m_settingsRegistry->SetContext(serializeContext.get());
    m_settingsRegistry->SetContext(registrationContext.get());
    ReflectTypes(registrationContext.get(), serializeContext.get());

    AZ::SceneProcessingConfig::SceneProcessingConfigSystemComponent sceneProcessingConfigSystemComponent;
    sceneProcessingConfigSystemComponent.Activate();

    const AZStd::vector<AZ::SceneProcessingConfig::SoftNameSetting*>* result = nullptr;
    AZ::SceneProcessingConfig::SceneProcessingConfigRequestBus::BroadcastResult(
        result,
        &AZ::SceneProcessingConfig::SceneProcessingConfigRequests::GetSoftNames);
    EXPECT_EQ(result->size(), 2);
    EXPECT_EQ(result->at(0)->GetVirtualType(), "LODMesh1");
    EXPECT_EQ(result->at(1)->GetVirtualType(), "Ignore");

    sceneProcessingConfigSystemComponent.Deactivate();

    RemoveReflectedTypes(registrationContext.get(), serializeContext.get());
}

TEST_F(SceneProcessingConfigTest, SceneProcessingConfigSystemComponent_SoftNameSettings_IgnoreDuplicateSoftNameSettings)
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
                                "matcher": 2
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

    AZStd::unique_ptr<AZ::SerializeContext> serializeContext = AZStd::make_unique<AZ::SerializeContext>();
    AZStd::unique_ptr<AZ::JsonRegistrationContext> registrationContext = AZStd::make_unique<AZ::JsonRegistrationContext>();
    m_settingsRegistry->SetContext(serializeContext.get());
    m_settingsRegistry->SetContext(registrationContext.get());
    ReflectTypes(registrationContext.get(), serializeContext.get());

    AZ_TEST_START_TRACE_SUPPRESSION;
    AZ::SceneProcessingConfig::SceneProcessingConfigSystemComponent sceneProcessingConfigSystemComponent;
    AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    sceneProcessingConfigSystemComponent.Activate();

    const AZStd::vector<AZ::SceneProcessingConfig::SoftNameSetting*>* result = nullptr;
    AZ::SceneProcessingConfig::SceneProcessingConfigRequestBus::BroadcastResult(
        result,
        &AZ::SceneProcessingConfig::SceneProcessingConfigRequests::GetSoftNames);
    EXPECT_EQ(result->size(), 2);
    EXPECT_EQ(result->at(0)->GetVirtualType(), "LODMesh1");
    EXPECT_EQ(result->at(1)->GetVirtualType(), "LODMesh2");

    sceneProcessingConfigSystemComponent.Deactivate();

    RemoveReflectedTypes(registrationContext.get(), serializeContext.get());
}

TEST_F(SceneProcessingConfigTest, SceneProcessingConfigSystemComponent_SoftNameSettings_DefaultSoftNameSettings)
{
    AZ::SceneProcessingConfig::SceneProcessingConfigSystemComponent sceneProcessingConfigSystemComponent;
    sceneProcessingConfigSystemComponent.Activate();

    const AZStd::vector<AZ::SceneProcessingConfig::SoftNameSetting*>* result = nullptr;
    AZ::SceneProcessingConfig::SceneProcessingConfigRequestBus::BroadcastResult(
        result,
        &AZ::SceneProcessingConfig::SceneProcessingConfigRequests::GetSoftNames);
    // 8 soft name settings are added by default if there's no setting registry setup
    EXPECT_EQ(result->size(), 8);

    sceneProcessingConfigSystemComponent.Deactivate();
}

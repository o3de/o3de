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
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/UnitTest/Mocks/MockFileIOBase.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <Config/Components/SceneProcessingConfigSystemComponent.h>

class SceneProcessingConfigTest
    : public UnitTest::ScopedAllocatorSetupFixture
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

    AZStd::unique_ptr<AZ::SettingsRegistryInterface> m_settingsRegistry;
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

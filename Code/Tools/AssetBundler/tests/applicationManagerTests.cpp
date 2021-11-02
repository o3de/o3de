/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/Asset/AssetBundler.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>

#include <source/utils/utils.h>
#include <source/utils/applicationManager.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <tests/main.h>

namespace AssetBundler
{
    const char DummyProjectName[] = "DummyProject";

    class MockApplicationManagerTest
        : public AssetBundler::ApplicationManager
    {
    public:
        friend class GTEST_TEST_CLASS_NAME_(ApplicationManagerTest, ValidatePlatformFlags_ReadConfigFiles_OK);
        explicit MockApplicationManagerTest(int* argc, char*** argv)
            : ApplicationManager(argc, argv)
        {
        }

    };

    class BasicApplicationManagerTest
        : public UnitTest::ScopedAllocatorSetupFixture
    {

    };

    class ApplicationManagerTest
        : public UnitTest::ScopedAllocatorSetupFixture
    {
    public:
        
        void SetUp() override
        {
            UnitTest::ScopedAllocatorSetupFixture::SetUp();
            m_data = AZStd::make_unique<StaticData>();

            AZ::SettingsRegistryInterface* registry = nullptr;
            if (!AZ::SettingsRegistry::Get())
            {
                AZ::SettingsRegistry::Register(&m_registry);
                registry = &m_registry;
            }
            else
            {
                registry = AZ::SettingsRegistry::Get();
            }
            auto projectPathKey = AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey)
                + "/project_path";
            registry->Set(projectPathKey, "AutomatedTesting");
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(*registry);


            m_data->m_applicationManager.reset(aznew MockApplicationManagerTest(0, 0));

            AZ::ComponentApplication::StartupParameters startupParameters;
            // The AssetBundler does not need to load gems
            startupParameters.m_loadDynamicModules = false;
            m_data->m_applicationManager->Start(AzFramework::Application::Descriptor(), startupParameters);

            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

            const char* engineRoot = nullptr;
            AzFramework::ApplicationRequests::Bus::BroadcastResult(engineRoot, &AzFramework::ApplicationRequests::GetEngineRoot);
            ASSERT_TRUE(engineRoot) << "Unable to locate engine root.\n";
            AzFramework::StringFunc::Path::Join(engineRoot, RelativeTestFolder, m_data->m_testEngineRoot);

            m_data->m_localFileIO = aznew AZ::IO::LocalFileIO();
            m_data->m_priorFileIO = AZ::IO::FileIOBase::GetInstance();
            // we need to set it to nullptr first because otherwise the 
            // underneath code assumes that we might be leaking the previous instance
            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(m_data->m_localFileIO);

        }
        void TearDown() override
        {
            AZ::IO::FileIOBase::SetInstance(nullptr);
            delete m_data->m_localFileIO;
            AZ::IO::FileIOBase::SetInstance(m_data->m_priorFileIO);

            auto settingsRegistry = AZ::SettingsRegistry::Get();
            if(settingsRegistry == &m_registry)
            {
                AZ::SettingsRegistry::Unregister(settingsRegistry);
            }

            m_data->m_applicationManager->Stop();
            m_data->m_applicationManager.reset();
            m_data.reset();
            UnitTest::ScopedAllocatorSetupFixture::TearDown();
        }

        struct StaticData
        {
            AZStd::unique_ptr<MockApplicationManagerTest> m_applicationManager = {};
            AZ::IO::FileIOBase* m_priorFileIO = nullptr;
            AZ::IO::FileIOBase* m_localFileIO = nullptr;
            AZStd::string m_testEngineRoot;
        };

        AZStd::unique_ptr<StaticData> m_data;
        AZ::SettingsRegistryImpl m_registry;
    };

    TEST_F(ApplicationManagerTest, ValidatePlatformFlags_ReadConfigFiles_OK)
    {
        AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
        ASSERT_NE(nullptr, settingsRegistry);

        AZStd::unordered_set<AZStd::string> gemsNameMap{ "GemA", "GemB", "GemC" };
        for (AZStd::string& gemName : gemsNameMap)
        {
            auto gemSourcePathKey = AZ::SettingsRegistryInterface::FixedValueString::format("%s/Gems/%s/SourcePaths/0",
                AZ::SettingsRegistryMergeUtils::OrganizationRootKey, gemName.c_str());
            auto gemSourcePath = AZ::IO::Path(m_data->m_testEngineRoot) / "Gems" / gemName;
            settingsRegistry->Set(gemSourcePathKey, gemSourcePath.Native());
        }

        AzFramework::GetGemsInfo(m_data->m_applicationManager->m_gemInfoList, *settingsRegistry);
        EXPECT_GE(m_data->m_applicationManager->m_gemInfoList.size(), 3);
        for (const AzFramework::GemInfo& gemInfo : m_data->m_applicationManager->m_gemInfoList)
        {
            gemsNameMap.erase(gemInfo.m_gemName);
        }

        EXPECT_EQ(0, gemsNameMap.size());

        AzFramework::PlatformFlags platformFlags = GetEnabledPlatformFlags(m_data->m_testEngineRoot.c_str(), m_data->m_testEngineRoot.c_str(), DummyProjectName);
        AzFramework::PlatformFlags hostPlatformFlag = AzFramework::PlatformHelper::GetPlatformFlag(AzToolsFramework::AssetSystem::GetHostAssetPlatform());
        AzFramework::PlatformFlags expectedFlags = AzFramework::PlatformFlags::Platform_ANDROID | AzFramework::PlatformFlags::Platform_IOS | AzFramework::PlatformFlags::Platform_PROVO | hostPlatformFlag;
        ASSERT_EQ(platformFlags, expectedFlags);
    }

    TEST_F(BasicApplicationManagerTest, ComputeComparisonTypeFromString_InvalidString_Fails)
    {
        auto invalidResult = AssetBundler::ParseComparisonType("notacomparisontype");
        EXPECT_EQ(invalidResult.IsSuccess(), false);
    }

    TEST_F(BasicApplicationManagerTest, ComputeComparisonTypeFromString_ValidString_Success)
    {
        using namespace AzToolsFramework;

        auto deltaResult = AssetBundler::ParseComparisonType(AssetFileInfoListComparison::ComparisonTypeNames[aznumeric_cast<AZ::u8>(AssetFileInfoListComparison::ComparisonType::Delta)]);
        EXPECT_EQ(deltaResult.IsSuccess(), true);
        EXPECT_EQ(deltaResult.GetValue(), AssetFileInfoListComparison::ComparisonType::Delta);

        auto unionResult = AssetBundler::ParseComparisonType(AssetFileInfoListComparison::ComparisonTypeNames[aznumeric_cast<AZ::u8>(AssetFileInfoListComparison::ComparisonType::Union)]);
        EXPECT_EQ(unionResult.IsSuccess(), true);
        EXPECT_EQ(unionResult.GetValue(), AssetFileInfoListComparison::ComparisonType::Union);

        auto intersectionResult = AssetBundler::ParseComparisonType(AssetFileInfoListComparison::ComparisonTypeNames[aznumeric_cast<AZ::u8>(AssetFileInfoListComparison::ComparisonType::Intersection)]);
        EXPECT_EQ(intersectionResult.IsSuccess(), true);
        EXPECT_EQ(intersectionResult.GetValue(), AssetFileInfoListComparison::ComparisonType::Intersection);

        auto complementResult = AssetBundler::ParseComparisonType(AssetFileInfoListComparison::ComparisonTypeNames[aznumeric_cast<AZ::u8>(AssetFileInfoListComparison::ComparisonType::Complement)]);
        EXPECT_EQ(complementResult.IsSuccess(), true);
        EXPECT_EQ(complementResult.GetValue(), AssetFileInfoListComparison::ComparisonType::Complement);

        auto filePatternResult = AssetBundler::ParseComparisonType(AssetFileInfoListComparison::ComparisonTypeNames[aznumeric_cast<AZ::u8>(AssetFileInfoListComparison::ComparisonType::FilePattern)]);
        EXPECT_EQ(filePatternResult.IsSuccess(), true);
        EXPECT_EQ(filePatternResult.GetValue(), AssetFileInfoListComparison::ComparisonType::FilePattern);
    }

    TEST_F(BasicApplicationManagerTest, ComputeComparisonTypeFromInt_InvalidInt_Fails)
    {
        auto invalidResult = AssetBundler::ParseComparisonType("999");
        EXPECT_EQ(invalidResult.IsSuccess(), false);
    }

    TEST_F(BasicApplicationManagerTest, ComputeComparisonTypeFromInt_ValidInt_Success)
    {
        int unionIndex(aznumeric_cast<int>(AzToolsFramework::AssetFileInfoListComparison::ComparisonType::Union));
        auto unionResult = AssetBundler::ParseComparisonType(AZStd::string::format("%i", unionIndex));
        EXPECT_TRUE(unionResult.IsSuccess());
        EXPECT_EQ(unionResult.GetValue(), AzToolsFramework::AssetFileInfoListComparison::ComparisonType::Union);
    }

    TEST_F(BasicApplicationManagerTest, ComputeFilePatternTypeFromString_InvalidString_Fails)
    {
        auto invalidResult = AssetBundler::ParseFilePatternType("notafilepatterntype");
        EXPECT_EQ(invalidResult.IsSuccess(), false);
    }

    TEST_F(BasicApplicationManagerTest, ComputeFilePatternTypeFromString_ValidString_Success)
    {
        using namespace AzToolsFramework;

        auto wildcardResult = AssetBundler::ParseFilePatternType(AssetFileInfoListComparison::FilePatternTypeNames[aznumeric_cast<AZ::u8>(AssetFileInfoListComparison::FilePatternType::Wildcard)]);
        EXPECT_TRUE(wildcardResult.IsSuccess());
        EXPECT_EQ(wildcardResult.GetValue(), AssetFileInfoListComparison::FilePatternType::Wildcard);

        auto regexResult = AssetBundler::ParseFilePatternType(AssetFileInfoListComparison::FilePatternTypeNames[aznumeric_cast<AZ::u8>(AssetFileInfoListComparison::FilePatternType::Regex)]);
        EXPECT_TRUE(regexResult.IsSuccess());
        EXPECT_EQ(regexResult.GetValue(), AssetFileInfoListComparison::FilePatternType::Regex);
    }

    TEST_F(BasicApplicationManagerTest, ComputeFilePatternTypeFromInt_InvalidInt_Fails)
    {
        auto invalidResult = AssetBundler::ParseFilePatternType("555");
        EXPECT_EQ(invalidResult.IsSuccess(), false);
    }

    TEST_F(BasicApplicationManagerTest, IsTokenFile_Empty_ReturnsFalse)
    {
        EXPECT_FALSE(AzToolsFramework::AssetFileInfoListComparison::IsTokenFile(""));
    }

    TEST_F(BasicApplicationManagerTest, IsTokenFile_NonToken_ReturnsFalse)
    {
        EXPECT_FALSE(AzToolsFramework::AssetFileInfoListComparison::IsTokenFile("Somefile"));
    }

    TEST_F(BasicApplicationManagerTest, IsTokenFile_Token_ReturnsTrue)
    {
        EXPECT_TRUE(AzToolsFramework::AssetFileInfoListComparison::IsTokenFile("$SomeToken"));
    }

    TEST_F(BasicApplicationManagerTest, IsOutputPath_Empty_ReturnsFalse)
    {
        EXPECT_FALSE(AzToolsFramework::AssetFileInfoListComparison::IsOutputPath(""));
    }

    TEST_F(BasicApplicationManagerTest, IsOutputPath_NonToken_ReturnsTrue)
    {
        EXPECT_TRUE(AzToolsFramework::AssetFileInfoListComparison::IsOutputPath("Somefile"));
    }

    TEST_F(BasicApplicationManagerTest, IsOutputPath_Token_ReturnsFalse)
    {
        EXPECT_FALSE(AzToolsFramework::AssetFileInfoListComparison::IsOutputPath("$SomeToken"));
    }

    TEST_F(BasicApplicationManagerTest, ComputeFilePatternTypeFromInt_ValidInt_Success)
    {
        int regexIndex(aznumeric_cast<int>(AzToolsFramework::AssetFileInfoListComparison::FilePatternType::Regex));
        auto regexResult = AssetBundler::ParseFilePatternType(AZStd::string::format("%i", regexIndex));
        EXPECT_TRUE(regexResult.IsSuccess());
        EXPECT_EQ(regexResult.GetValue(), AzToolsFramework::AssetFileInfoListComparison::FilePatternType::Regex);
    }
}

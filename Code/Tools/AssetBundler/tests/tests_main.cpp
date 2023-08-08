/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/FileFunc/FileFunc.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/Asset/AssetBundler.h>
#include <AzFramework/Platform/PlatformDefaults.h>

#include <source/utils/utils.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <tests/main.h>
#include <source/utils/applicationManager.h>

#include <QApplication>

extern char g_cachedEngineRoot[AZ_MAX_PATH_LEN];

namespace AssetBundler
{
    class AssetBundlerBatchUtilsTest
        : public UnitTest::LeakDetectionFixture
    {
    };

    TEST_F(AssetBundlerBatchUtilsTest, SplitFilename_MacFile_OutputBaseNameAndPlatform)
    {
        AZStd::string filePath = "assetInfoFile_mac.xml";
        AZStd::string baseFilename;
        AZStd::string platformIdentifier;

        AzToolsFramework::SplitFilename(filePath, baseFilename, platformIdentifier);

        ASSERT_EQ(baseFilename, "assetInfoFile");
        ASSERT_EQ(platformIdentifier, "mac");
    }

    TEST_F(AssetBundlerBatchUtilsTest, SplitFilename_PcFile_OutputBaseNameAndPlatform)
    {
        AZStd::string filePath = "assetInfoFile_pc.xml";
        AZStd::string baseFilename;
        AZStd::string platformIdentifier;

        AzToolsFramework::SplitFilename(filePath, baseFilename, platformIdentifier);

        ASSERT_EQ(baseFilename, "assetInfoFile");
        ASSERT_EQ(platformIdentifier, "pc");
    }

    TEST_F(AssetBundlerBatchUtilsTest, SplitFilename_MacFileWithUnderScoreInFileName_OutputBaseNameAndPlatform)
    {
        AZStd::string filePath = "assetInfoFile_test_mac.xml";
        AZStd::string baseFilename;
        AZStd::string platformIdentifier;

        AzToolsFramework::SplitFilename(filePath, baseFilename, platformIdentifier);

        ASSERT_EQ(baseFilename, "assetInfoFile_test");
        ASSERT_EQ(platformIdentifier, "mac");
    }

    TEST_F(AssetBundlerBatchUtilsTest, SplitFilename_PcFileWithUnderScoreInFileName_OutputBaseNameAndPlatform)
    {
        AZStd::string filePath = "assetInfoFile_test_pc.xml";
        AZStd::string baseFilename;
        AZStd::string platformIdentifier;

        AzToolsFramework::SplitFilename(filePath, baseFilename, platformIdentifier);

        ASSERT_EQ(baseFilename, "assetInfoFile_test");
        ASSERT_EQ(platformIdentifier, "pc");
    }

    const char RelativeTestFolder[] = "AssetBundler.Tests.dir";
    const char GemsFolder[] = "Gems";
    constexpr auto EngineFolder = AZ::IO::FixedMaxPath("Assets") / "Engine";
    const char PlatformsFolder[] = "Platforms";
    const char DummyProjectFolder[] = "DummyProject";

    class AssetBundlerGemsUtilTest
        : public UnitTest::LeakDetectionFixture
    {
    public:
        void SetUp() override
        {
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
            AZ::IO::FixedMaxPath enginePath = AZ::SettingsRegistryMergeUtils::FindEngineRoot(*registry);
            registry->Set(projectPathKey, (enginePath / "AutomatedTesting").Native());
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(*registry);

            AZ::IO::FixedMaxPath executableDirectory = AZ::Utils::GetExecutableDirectory();
            if (executableDirectory.empty())
            {
                GTEST_FATAL_FAILURE_(AZStd::string::format("Unable to locate executable.\n").c_str());
            }

            m_data = AZStd::make_unique<StaticData>();
            m_data->m_application.reset(aznew AzToolsFramework::ToolsApplication());
            m_data->m_application.get()->Start(AzFramework::Application::Descriptor());

            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

            // Set the test engine root to be the executable directory
            m_data->m_testEngineRoot = (executableDirectory / RelativeTestFolder).LexicallyNormal().String();

            m_data->m_localFileIO = aznew AZ::IO::LocalFileIO();
            m_data->m_priorFileIO = AZ::IO::FileIOBase::GetInstance();
            // we need to set it to nullptr first because otherwise the
            // underneath code assumes that we might be leaking the previous instance
            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(m_data->m_localFileIO);

            AddGemData(m_data->m_testEngineRoot.c_str(), "GemA");
            AddGemData(m_data->m_testEngineRoot.c_str(), "GemB");

            auto absoluteEngineSeedFilePath = m_data->m_testEngineRoot / EngineFolder / "SeedAssetList";
            absoluteEngineSeedFilePath.ReplaceExtension(AzToolsFramework::AssetSeedManager::GetSeedFileExtension());
            m_data->m_gemSeedFilePairList.push_back({ absoluteEngineSeedFilePath.Native(), true});

            AddGemData(m_data->m_testEngineRoot.c_str(), "GemC", false);


            auto absoluteProjectJsonSeedFilePath = AZ::IO::Path(m_data->m_testEngineRoot) / DummyProjectFolder / "SeedAssetList";
            absoluteProjectJsonSeedFilePath.ReplaceExtension(AzToolsFramework::AssetSeedManager::GetSeedFileExtension());
            m_data->m_gemSeedFilePairList.push_back({ absoluteProjectJsonSeedFilePath.Native(), true });

            // Add an explicit ObjectStream format XML seed file to validate that it loads successfully
            auto absoluteProjectObjectStreamSeedFilePath = AZ::IO::Path(m_data->m_testEngineRoot) / DummyProjectFolder / "SeedAssetListObjectStreamXML";
            absoluteProjectObjectStreamSeedFilePath.ReplaceExtension(AzToolsFramework::AssetSeedManager::GetSeedFileExtension());
            m_data->m_gemSeedFilePairList.push_back({ absoluteProjectObjectStreamSeedFilePath.Native(), true });
        }
        void TearDown() override
        {
            if (m_data)
            {
                AZ::IO::FileIOBase::SetInstance(nullptr);
                delete m_data->m_localFileIO;
                AZ::IO::FileIOBase::SetInstance(m_data->m_priorFileIO);

                m_data->m_gemInfoList.set_capacity(0);
                m_data->m_gemSeedFilePairList.set_capacity(0);
                m_data->m_application.get()->Stop();
                m_data->m_application.reset();
            }

            if(auto settingsRegistry = AZ::SettingsRegistry::Get();
                settingsRegistry == &m_registry)
            {
                AZ::SettingsRegistry::Unregister(settingsRegistry);
            }

        }

        void AddGemData(const char* engineRoot, const char* gemName, bool seedFileExists = true)
        {
            AZ::IO::Path relativeGemPath{ GemsFolder };
            relativeGemPath /= gemName;

            AZ::IO::Path absoluteGemPath{ engineRoot };
            absoluteGemPath /= relativeGemPath;

            AZ::IO::Path absoluteGemSeedFilePath = absoluteGemPath;
            absoluteGemSeedFilePath /= "Assets/seedList";
            absoluteGemSeedFilePath.ReplaceExtension(AZ::IO::PathView{ AzToolsFramework::AssetSeedManager::GetSeedFileExtension() });
            absoluteGemSeedFilePath = absoluteGemSeedFilePath.LexicallyNormal();

            m_data->m_gemSeedFilePairList.emplace_back(absoluteGemSeedFilePath, seedFileExists);

            m_data->m_gemInfoList.emplace_back(gemName);
            m_data->m_gemInfoList.back().m_absoluteSourcePaths.push_back(absoluteGemPath.Native());

            AZ::IO::Path platformsDirectory = absoluteGemPath / "Assets" / PlatformsFolder;
            if (m_data->m_localFileIO->Exists(platformsDirectory.c_str()))
            {
                m_data->m_localFileIO->FindFiles(platformsDirectory.c_str(),
                    AZStd::string::format("*.%s", AzToolsFramework::AssetSeedManager::GetSeedFileExtension()).c_str(),
                    [&](const char* fileName)
                {
                    m_data->m_gemSeedFilePairList.push_back({ AZ::IO::PathView(fileName).LexicallyNormal().String(), seedFileExists });
                    return true;
                });
            }

            AZ::IO::Path iosDirectory = platformsDirectory / AzFramework::PlatformIOS;
            if (m_data->m_localFileIO->Exists(iosDirectory.c_str()))
            {
                bool recurse = true;
                AZ::Outcome<AZStd::list<AZStd::string>, AZStd::string> result = AzFramework::FileFunc::FindFileList(iosDirectory.Native(),
                    AZStd::string::format("*.%s", AzToolsFramework::AssetSeedManager::GetSeedFileExtension()).c_str(), recurse);

                if (result.IsSuccess())
                {
                    AZStd::list<AZStd::string> seedFiles = result.TakeValue();
                    for (AZStd::string& seedFile : seedFiles)
                    {
                        m_data->m_gemSeedFilePairList.push_back({ AZ::IO::PathView(seedFile).LexicallyNormal().String(), seedFileExists });
                    }
                }
            }
        }

        struct StaticData
        {
            AZStd::vector<AzFramework::GemInfo> m_gemInfoList;
            AZStd::vector<AZStd::pair<AZStd::string, bool>> m_gemSeedFilePairList;
            AZStd::unique_ptr<AzToolsFramework::ToolsApplication> m_application = {};
            AZ::IO::FileIOBase* m_priorFileIO = nullptr;
            AZ::IO::FileIOBase* m_localFileIO = nullptr;
            AZ::IO::Path m_testEngineRoot;
        };

        const int GemAIndex = 0;
        const int GemBIndex = 1;
        const int GemBSharedFileIndex = 2;
        const int GemBIosFileIndex = 3;
        const int EngineIndex = 4;
        const int GemCIndex = 5;
        const int ProjectIndex = 6;

        AZStd::unique_ptr<StaticData> m_data;
        AZ::SettingsRegistryImpl m_registry;
    };

    TEST_F(AssetBundlerGemsUtilTest, GetDefaultSeedFiles_AllSeedFiles_Found)
    {
        // DummyProject and fake Engine/Gem structure lives at dev/Code/Tools/AssetBundler/tests/
        auto dummyProjectPath = AZ::IO::Path(m_data->m_testEngineRoot) / DummyProjectFolder;
        auto defaultSeedList = AssetBundler::GetDefaultSeedListFiles(m_data->m_testEngineRoot.Native(), dummyProjectPath.Native(), m_data->m_gemInfoList, AzFramework::PlatformFlags::Platform_PC);
        ASSERT_EQ(defaultSeedList.size(), 5); //adding one for the engine seed file and one for the project file

        // Validate whether both GemA and GemB seed file are present
        EXPECT_NE(defaultSeedList.find(m_data->m_gemSeedFilePairList[GemAIndex].first), defaultSeedList.end());
        EXPECT_NE(defaultSeedList.find(m_data->m_gemSeedFilePairList[GemBIndex].first), defaultSeedList.end());
        EXPECT_NE(defaultSeedList.find(m_data->m_gemSeedFilePairList[GemBSharedFileIndex].first), defaultSeedList.end());

        // Validate that the engine seed file is present
        EXPECT_NE(defaultSeedList.find(m_data->m_gemSeedFilePairList[EngineIndex].first), defaultSeedList.end());
        EXPECT_NE(defaultSeedList.find(m_data->m_gemSeedFilePairList[ProjectIndex].first), defaultSeedList.end());
    }

    TEST_F(AssetBundlerGemsUtilTest, GetDefaultSeedFilesForMultiplePlatforms_AllSeedFiles_Found)
    {
        // DummyProject and fake Engine/Gem structure lives at dev/Code/Tools/AssetBundler/tests/
        auto dummyProjectPath = AZ::IO::Path(m_data->m_testEngineRoot) / DummyProjectFolder;
        auto defaultSeedList = AssetBundler::GetDefaultSeedListFiles(m_data->m_testEngineRoot.Native(), dummyProjectPath.Native(), m_data->m_gemInfoList, AzFramework::PlatformFlags::Platform_PC | AzFramework::PlatformFlags::Platform_IOS);
        ASSERT_EQ(defaultSeedList.size(), 6); //adding one for the engine seed file and one for the project file


        // Validate whether both GemA and GemB seed file are present
        EXPECT_NE(defaultSeedList.find(m_data->m_gemSeedFilePairList[GemAIndex].first), defaultSeedList.end());
        EXPECT_NE(defaultSeedList.find(m_data->m_gemSeedFilePairList[GemBIndex].first), defaultSeedList.end());
        EXPECT_NE(defaultSeedList.find(m_data->m_gemSeedFilePairList[GemBSharedFileIndex].first), defaultSeedList.end());
        EXPECT_NE(defaultSeedList.find(m_data->m_gemSeedFilePairList[GemBIosFileIndex].first), defaultSeedList.end());

        // Validate that the engine seed file is present
        EXPECT_NE(defaultSeedList.find(m_data->m_gemSeedFilePairList[EngineIndex].first), defaultSeedList.end());
        EXPECT_NE(defaultSeedList.find(m_data->m_gemSeedFilePairList[ProjectIndex].first), defaultSeedList.end());
    }


    TEST_F(AssetBundlerGemsUtilTest, IsSeedFileValid_Ok)
    {
        for (const auto& pair : m_data->m_gemSeedFilePairList)
        {
            bool result = IsGemSeedFilePathValid(m_data->m_testEngineRoot.c_str(), pair.first, m_data->m_gemInfoList, AzFramework::PlatformFlags::Platform_PC | AzFramework::PlatformFlags::Platform_IOS);
            EXPECT_EQ(result, pair.second);
        }
    }
}

int main(int argc, char* argv[])
{
    AZ::Debug::Trace::HandleExceptions(true);
    QApplication app(argc, argv);
    AZ::Test::ApplyGlobalParameters(&argc, argv);

    INVOKE_AZ_UNIT_TEST_MAIN();

    int runSuccess = 0;
    {
        AssetBundler::ApplicationManager applicationManger(&argc, &argv);
        applicationManger.Init();
        runSuccess = applicationManger.Run() ? 0 : 1;
    }
    return runSuccess;
}

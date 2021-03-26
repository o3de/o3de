/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/FileFunc/FileFunc.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>
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


extern char g_cachedEngineRoot[AZ_MAX_PATH_LEN];

namespace AssetBundler
{
    class AssetBundlerBatchUtilsTest
        : public UnitTest::ScopedAllocatorSetupFixture
    {
    };

    TEST_F(AssetBundlerBatchUtilsTest, SplitFilename_MacFile_OutputBaseNameAndPlatform)
    {
        AZStd::string filePath = "assetInfoFile_osx_gl.xml";
        AZStd::string baseFilename;
        AZStd::string platformIdentifier;

        AzToolsFramework::SplitFilename(filePath, baseFilename, platformIdentifier);

        ASSERT_EQ(baseFilename, "assetInfoFile");
        ASSERT_EQ(platformIdentifier, "osx_gl");
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
        AZStd::string filePath = "assetInfoFile_test_osx_gl.xml";
        AZStd::string baseFilename;
        AZStd::string platformIdentifier;

        AzToolsFramework::SplitFilename(filePath, baseFilename, platformIdentifier);

        ASSERT_EQ(baseFilename, "assetInfoFile_test");
        ASSERT_EQ(platformIdentifier, "osx_gl");
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

    const char RelativeTestFolder[] = "Code/Tools/AssetBundler/tests";
    const char GemsFolder[] = "Gems";
    const char EngineFolder[] = "Engine";
    const char PlatformsFolder[] = "Platforms";
    const char DummyProjectFolder[] = "DummyProject";

    class AssetBundlerGemsUtilTest
        : public UnitTest::ScopedAllocatorSetupFixture
    {
    public:
        void SetUp() override
        {
            m_data = AZStd::make_unique<StaticData>();
            m_data->m_application.reset(aznew AzToolsFramework::ToolsApplication());
            m_data->m_application.get()->Start(AzFramework::Application::Descriptor());

            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

            if (!AZ::SettingsRegistry::Get())
            {
                AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_Bootstrap(m_registry);
                AZ::SettingsRegistry::Register(&m_registry);
            }

            const char* engineRoot = nullptr;
            AzFramework::ApplicationRequests::Bus::BroadcastResult(engineRoot, &AzFramework::ApplicationRequests::GetEngineRoot);
            if (!engineRoot)
            {
                GTEST_FATAL_FAILURE_(AZStd::string::format("Unable to locate engine root.\n").c_str());
            }

            AzFramework::StringFunc::Path::Join(engineRoot, RelativeTestFolder, m_data->m_testEngineRoot);

            m_data->m_localFileIO = aznew AZ::IO::LocalFileIO();
            m_data->m_priorFileIO = AZ::IO::FileIOBase::GetInstance();
            // we need to set it to nullptr first because otherwise the 
            // underneath code assumes that we might be leaking the previous instance
            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(m_data->m_localFileIO);

            AddGemData(m_data->m_testEngineRoot.c_str(), "GemA");
            AddGemData(m_data->m_testEngineRoot.c_str(), "GemB");

            AZStd::string absoluteEngineSeedFilePath;
            AzFramework::StringFunc::Path::ConstructFull(m_data->m_testEngineRoot.c_str(), EngineFolder, "SeedAssetList", AzToolsFramework::AssetSeedManager::GetSeedFileExtension(), absoluteEngineSeedFilePath, true);
            m_data->m_gemSeedFilePairList.emplace_back(AZStd::make_pair(absoluteEngineSeedFilePath, true));

            AddGemData(m_data->m_testEngineRoot.c_str(), "GemC", false);

            AZStd::string absoluteProjectSeedFilePath;
            AzFramework::StringFunc::Path::ConstructFull(m_data->m_testEngineRoot.c_str(), DummyProjectFolder, "SeedAssetList", AzToolsFramework::AssetSeedManager::GetSeedFileExtension(), absoluteProjectSeedFilePath, true);
            m_data->m_gemSeedFilePairList.emplace_back(AZStd::make_pair(absoluteProjectSeedFilePath, true));
        }
        void TearDown() override
        {
            AZ::IO::FileIOBase::SetInstance(nullptr);
            delete m_data->m_localFileIO;
            AZ::IO::FileIOBase::SetInstance(m_data->m_priorFileIO);

            m_data->m_gemInfoList.set_capacity(0);
            m_data->m_gemSeedFilePairList.set_capacity(0);
            m_data->m_application.get()->Stop();
            m_data->m_application.reset();
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
                    AZStd::string normalizedFilePath = fileName;
                    AzFramework::StringFunc::Path::Normalize(normalizedFilePath);
                    m_data->m_gemSeedFilePairList.emplace_back(AZStd::make_pair(normalizedFilePath, seedFileExists));
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
                        AZStd::string normalizedFilePath = seedFile;
                        AzFramework::StringFunc::Path::Normalize(normalizedFilePath);
                        m_data->m_gemSeedFilePairList.emplace_back(AZStd::make_pair(normalizedFilePath, seedFileExists));
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
            AZStd::string m_testEngineRoot;
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
        auto defaultSeedList = AssetBundler::GetDefaultSeedListFiles(m_data->m_testEngineRoot.c_str(), dummyProjectPath.Native(), m_data->m_gemInfoList, AzFramework::PlatformFlags::Platform_PC);
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
        auto defaultSeedList = AssetBundler::GetDefaultSeedListFiles(m_data->m_testEngineRoot.c_str(), dummyProjectPath.Native(), m_data->m_gemInfoList, AzFramework::PlatformFlags::Platform_PC | AzFramework::PlatformFlags::Platform_IOS);
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
    INVOKE_AZ_UNIT_TEST_MAIN();

    AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
    int runSuccess = 0;
    {
        AssetBundler::ApplicationManager applicationManger(&argc, &argv);
        applicationManger.Init();
        runSuccess = applicationManger.Run() ? 0 : 1;
    }
    AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
    return runSuccess;
}

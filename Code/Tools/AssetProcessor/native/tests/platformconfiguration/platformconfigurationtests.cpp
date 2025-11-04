/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include "native/tests/platformconfiguration/platformconfigurationtests.h"

#include <AzTest/AzTest.h>
#include <gmock/gmock.h>

const char TestAppRoot[] = "@exefolder@/testdata";
const char EmptyDummyProjectName[] = "EmptyDummyProject";

// make the internal calls public for the purposes of the unit test!
class UnitTestPlatformConfiguration : public AssetProcessor::PlatformConfiguration
{
    friend class GTEST_TEST_CLASS_NAME_(PlatformConfigurationUnitTests, Test_GemHandling);
    friend class GTEST_TEST_CLASS_NAME_(PlatformConfigurationUnitTests, Test_MetaFileTypes);
    friend class GTEST_TEST_CLASS_NAME_(PlatformConfigurationUnitTests, Test_MetaFileTypes_AssetImporterExtensions);
protected:
};

PlatformConfigurationUnitTests::PlatformConfigurationUnitTests()
    : m_argc(0)
    , m_argv(0)
{
}

void PlatformConfigurationUnitTests::SetUp()
{
    using namespace AssetProcessor;
    m_qApp = new QCoreApplication(m_argc, m_argv);
    AssetProcessorTest::SetUp();
    AssetUtilities::ResetAssetRoot();
}

void PlatformConfigurationUnitTests::TearDown()
{
    AssetUtilities::ResetAssetRoot();
    delete m_qApp;
    AssetProcessor::AssetProcessorTest::TearDown();
}

TEST_F(PlatformConfigurationUnitTests, TestFailReadConfigFile_BadPlatform)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    const auto testExeFolder = AZ::IO::FileIOBase::GetInstance()->ResolvePath(TestAppRoot);
    const AZ::IO::FixedMaxPath projectPath = (*testExeFolder) / EmptyDummyProjectName;
    auto configRoot = AZ::IO::FileIOBase::GetInstance()->ResolvePath("@exefolder@/testdata/config_broken_badplatform");
    ASSERT_TRUE(configRoot);
    UnitTestPlatformConfiguration config;
    m_errorAbsorber->Clear();
    ASSERT_FALSE(config.InitializeFromConfigFiles(configRoot->c_str(), testExeFolder->c_str(), projectPath.c_str(), false, false));
    ASSERT_GT(m_errorAbsorber->m_numErrorsAbsorbed, 0);
}


TEST_F(PlatformConfigurationUnitTests, TestFailReadConfigFile_NoPlatform)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    const auto testExeFolder = AZ::IO::FileIOBase::GetInstance()->ResolvePath(TestAppRoot);
    const AZ::IO::FixedMaxPath projectPath = (*testExeFolder) / EmptyDummyProjectName;
    auto configRoot = AZ::IO::FileIOBase::GetInstance()->ResolvePath("@exefolder@/testdata/config_broken_noplatform");
    ASSERT_TRUE(configRoot);
    UnitTestPlatformConfiguration config;
    m_errorAbsorber->Clear();
    ASSERT_FALSE(config.InitializeFromConfigFiles(configRoot->c_str(), testExeFolder->c_str(), projectPath.c_str(), false, false));
    ASSERT_GT(m_errorAbsorber->m_numErrorsAbsorbed, 0);
}

TEST_F(PlatformConfigurationUnitTests, TestFailReadConfigFile_NoScanFolders)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    const auto testExeFolder = AZ::IO::FileIOBase::GetInstance()->ResolvePath(TestAppRoot);
    const AZ::IO::FixedMaxPath projectPath = (*testExeFolder) / EmptyDummyProjectName;
    auto configRoot = AZ::IO::FileIOBase::GetInstance()->ResolvePath("@exefolder@/testdata/config_broken_noscans");
    ASSERT_TRUE(configRoot);
    UnitTestPlatformConfiguration config;
    m_errorAbsorber->Clear();
    ASSERT_FALSE(config.InitializeFromConfigFiles(configRoot->c_str(), testExeFolder->c_str(), projectPath.c_str(), false, false));
    ASSERT_GT(m_errorAbsorber->m_numErrorsAbsorbed, 0);
}

TEST_F(PlatformConfigurationUnitTests, TestFailReadConfigFile_BrokenRecognizers)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    const auto testExeFolder = AZ::IO::FileIOBase::GetInstance()->ResolvePath(TestAppRoot);
    const AZ::IO::FixedMaxPath projectPath = (*testExeFolder) / EmptyDummyProjectName;
    auto configRoot = AZ::IO::FileIOBase::GetInstance()->ResolvePath("@exefolder@/testdata/config_broken_recognizers");
    ASSERT_TRUE(configRoot);
    UnitTestPlatformConfiguration config;
    m_errorAbsorber->Clear();
    ASSERT_FALSE(config.InitializeFromConfigFiles(configRoot->c_str(), testExeFolder->c_str(), projectPath.c_str(), false, false));
    ASSERT_GT(m_errorAbsorber->m_numErrorsAbsorbed, 0);
}

TEST_F(PlatformConfigurationUnitTests, TestFailReadConfigFile_Regular_Platforms)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    const auto testExeFolder = AZ::IO::FileIOBase::GetInstance()->ResolvePath(TestAppRoot);
    const AZ::IO::FixedMaxPath projectPath = (*testExeFolder) / EmptyDummyProjectName;
    auto configRoot = AZ::IO::FileIOBase::GetInstance()->ResolvePath("@exefolder@/testdata/config_regular");
    ASSERT_TRUE(configRoot);
    UnitTestPlatformConfiguration config;
    m_errorAbsorber->Clear();
    ASSERT_TRUE(config.InitializeFromConfigFiles(configRoot->c_str(), testExeFolder->c_str(), projectPath.c_str(), false, false));
    ASSERT_EQ(m_errorAbsorber->m_numErrorsAbsorbed, 0);

    // verify the data.
    ASSERT_NE(config.GetPlatformByIdentifier(AzToolsFramework::AssetSystem::GetHostAssetPlatform()), nullptr);
    ASSERT_NE(config.GetPlatformByIdentifier("android"), nullptr);
    ASSERT_NE(config.GetPlatformByIdentifier("server"), nullptr);

    ASSERT_TRUE(config.GetPlatformByIdentifier("android")->HasTag("mobile"));
    ASSERT_TRUE(config.GetPlatformByIdentifier("android")->HasTag("renderer"));
    ASSERT_TRUE(config.GetPlatformByIdentifier("android")->HasTag("android"));
    ASSERT_TRUE(config.GetPlatformByIdentifier("server")->HasTag("server"));
    ASSERT_FALSE(config.GetPlatformByIdentifier("android")->HasTag("server"));
    ASSERT_FALSE(config.GetPlatformByIdentifier("server")->HasTag("renderer"));
}

TEST_F(PlatformConfigurationUnitTests, TestReadScanFolderRoot_FromSettingsRegistry_Succeeds)
{
    auto settingsRegistry = AZ::SettingsRegistry::Get();
    ASSERT_NE(nullptr, settingsRegistry);

    QTemporaryDir tempEngineRoot;
    QDir tempPath(tempEngineRoot.path());
    QString testScanFolderSetregPath = tempPath.absoluteFilePath("test.setreg");
    UnitTestUtils::CreateDummyFile(testScanFolderSetregPath, QString(R"({ "Amazon":
    {
        "AssetProcessor":
        {
            "Settings":
             {
                "ScanFolder SettingsRegistryTest":
                {
                    "watch": "_TestPath",
                    "recursive": false,
                    "order": 20000
                }
            }
        }
    }
    }\n)"));

    EXPECT_TRUE(AssetProcessor::PlatformConfiguration::MergeConfigFileToSettingsRegistry(*settingsRegistry, testScanFolderSetregPath.toUtf8().data()));

    AZStd::string watchPath;
    bool recurseScanFolder{ true };
    AZ::s64 scanOrder{};

    EXPECT_TRUE(settingsRegistry->Get(watchPath, AZ::SettingsRegistryInterface::FixedValueString(AssetProcessor::AssetProcessorSettingsKey)
        + "/ScanFolder SettingsRegistryTest/watch"));
    EXPECT_TRUE(settingsRegistry->Get(recurseScanFolder, AZ::SettingsRegistryInterface::FixedValueString(AssetProcessor::AssetProcessorSettingsKey)
        + "/ScanFolder SettingsRegistryTest/recursive"));
    EXPECT_TRUE(settingsRegistry->Get(scanOrder, AZ::SettingsRegistryInterface::FixedValueString(AssetProcessor::AssetProcessorSettingsKey)
        + "/ScanFolder SettingsRegistryTest/order"));

    // These test values come from the <dev_root>/Registry/AssetProcessorPlatformConfig.setreg file
    EXPECT_STREQ("_TestPath", watchPath.c_str());
    EXPECT_FALSE(recurseScanFolder);
    EXPECT_EQ(20000, scanOrder);
}


// a reusable fixture that sets up one host as a pc with a temp path and such.
class PlatformConfigurationUnitTests_OnePCHostFixture : public PlatformConfigurationUnitTests
{
public:
    void SetUp() override
    {
        PlatformConfigurationUnitTests::SetUp();

        m_tempEngineRoot.reset(new QTemporaryDir());
        m_tempPath = QDir(m_tempEngineRoot->path());

        m_config.reset(new UnitTestPlatformConfiguration());
        m_config->EnablePlatform({ "pc",{ "desktop", "host" } }, true);
        m_config->PopulatePlatformsForScanFolder(m_platforms);
    }

    void TearDown() override
    {
        m_platforms.set_capacity(0);
        m_tempEngineRoot.reset();
        m_config.reset();
        PlatformConfigurationUnitTests::TearDown();
    }

    AZStd::vector<AssetBuilderSDK::PlatformInfo> m_platforms;
    AZStd::unique_ptr<UnitTestPlatformConfiguration> m_config;
    AZStd::unique_ptr<QTemporaryDir> m_tempEngineRoot = nullptr; // this actually creates the folder in its constructor, so hold off until setup..
    QDir m_tempPath;
};

// ensures that when a file in the root (non recursive) folder is searched for, the root is found.
TEST_F(PlatformConfigurationUnitTests_OnePCHostFixture, GetScanFolderForFile_RootFolderFile_IsFound)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    m_config->AddScanFolder(ScanFolderInfo(m_tempPath.filePath("scanfolder1"), "ScanFolder1", "sf1", true, false, m_platforms), true); // a root folder that has watched subfolders, not recursive
    m_config->AddScanFolder(ScanFolderInfo(m_tempPath.filePath("scanfolder1/Editor"), "Editor", "sf2", false, true, m_platforms), true); // a child folder that exists within that scan folder.

    const ScanFolderInfo* info = m_config->GetScanFolderForFile(m_tempPath.filePath("scanfolder1/something.txt"));

    ASSERT_TRUE(info);
    EXPECT_STREQ(info->ScanPath().toUtf8().constData(), m_tempPath.filePath("scanfolder1").toUtf8().constData());
    EXPECT_STREQ(info->GetDisplayName().toUtf8().constData(), "ScanFolder1");
}

// ensures that when a file in a subfolder (recursive) is searched for, the subfolder is found despite it being inside the root, technically.
TEST_F(PlatformConfigurationUnitTests_OnePCHostFixture, GetScanFolderForFile_SubFolderFile_IsFound)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    m_config->AddScanFolder(ScanFolderInfo(m_tempPath.filePath("scanfolder1"), "ScanFolder1", "sf1", true, false, m_platforms), true); // a root folder that has watched subfolders, not recursive
    m_config->AddScanFolder(ScanFolderInfo(m_tempPath.filePath("scanfolder1/Editor"), "Editor ScanFolder", "sf2", false, true, m_platforms), true); // a child folder that exists within that scan folder.

    const ScanFolderInfo* info = m_config->GetScanFolderForFile(m_tempPath.filePath("scanfolder1/Editor/something.txt"));

    ASSERT_TRUE(info);
    EXPECT_STREQ(info->ScanPath().toUtf8().constData(), m_tempPath.filePath("scanfolder1/Editor").toUtf8().constData());
    EXPECT_STREQ(info->GetDisplayName().toUtf8().constData(), "Editor ScanFolder");
}

TEST_F(PlatformConfigurationUnitTests_OnePCHostFixture, GetOverridingFile_ExistsButNotOverridden_ReturnsEmpty)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    // create two scan folders, since its order dependent, the ScanFolder1 is the "winner" in tie breakers.
    QString scanfolder1Path = m_tempPath.filePath("scanfolder1");
    QString scanfolder2Path = m_tempPath.filePath("scanfolder2");
    QString caseSensitiveDummyFileName = m_tempPath.absoluteFilePath("scanfolder1/TestCase.tXt");
    QString differentCaseDummyFileName = m_tempPath.absoluteFilePath("scanfolder2/testcase.txt");
    UnitTestUtils::CreateDummyFile(caseSensitiveDummyFileName, QString("testcase1\n"));
    UnitTestUtils::CreateDummyFile(differentCaseDummyFileName, QString("testcase2\n"));
    m_config->AddScanFolder(ScanFolderInfo(scanfolder1Path, "ScanFolder1", "sf1", false, true, m_platforms), true);
    m_config->AddScanFolder(ScanFolderInfo(scanfolder2Path, "ScanFolder2", "sf2", false, true, m_platforms), true);

    // Perform the test by asking it whether the existing real winning file is being overridden by anyone.
    QString overrider = m_config->GetOverridingFile("TestCase.tXt", scanfolder1Path);

    // note that this should return the emptystring, because there is nothing that OVERRIDES it (ie, its already the winner).
    EXPECT_TRUE(overrider.isEmpty());
}

TEST_F(PlatformConfigurationUnitTests_OnePCHostFixture, GetOverridingFile_DoesNotExist_ReturnsEmptyString)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;
    // create two scan folders, since its order dependent, the ScanFolder1 is the "winner" in tie breakers.

    QString scanfolder1Path = m_tempPath.filePath("scanfolder1");
    QString scanfolder2Path = m_tempPath.filePath("scanfolder2");

    m_config->AddScanFolder(ScanFolderInfo(scanfolder1Path, "ScanFolder1", "sf1", false, true, m_platforms), true);
    m_config->AddScanFolder(ScanFolderInfo(scanfolder2Path, "ScanFolder2", "sf2", false, true, m_platforms), true);

    // Perform the test by asking it whether anyone overrides "testcase" (lowercase) in scanfolder 2.
    QString overrider = m_config->GetOverridingFile("doesntExist.txt", scanfolder2Path);

    EXPECT_TRUE(overrider.isEmpty());
}

TEST_F(PlatformConfigurationUnitTests_OnePCHostFixture, FindFirstMatchingFile_DoesNotExist_ReturnsEmptyString)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    // create two scan folders, since its order dependent, the ScanFolder1 is the "winner" in tie breakers.
    QString scanfolder1Path = m_tempPath.filePath("scanfolder1");
    QString scanfolder2Path = m_tempPath.filePath("scanfolder2");
    m_config->AddScanFolder(ScanFolderInfo(scanfolder1Path, "ScanFolder1", "sf1", false, true, m_platforms), true);
    m_config->AddScanFolder(ScanFolderInfo(scanfolder2Path, "ScanFolder2", "sf2", false, true, m_platforms), true);

    // Perform the test by asking it whether anyone overrides "testcase" (lowercase) in scanfolder 2.
    QString foundFile = m_config->FindFirstMatchingFile("doesntExist.txt");

    EXPECT_TRUE(foundFile.isEmpty());
}

// note that we do not guarantee that FindFirstMatchingFile always returns the correct case, as it is a super hot path
// function, and the only time case could be incorrect is in the situation where a file with different case overrides
// an underlying file, ie,
// Engine/EngineAssets/Textures/StartScreen.tif
// MyGame/EngineAssets/textures/startscreen.tif <-- would override the above because game has higher / more important priority.

// ensures that exact matches take priority over subfolder matches
TEST_F(PlatformConfigurationUnitTests_OnePCHostFixture, GetScanFolderForFile_SubFolder_ExactMatch_IsFound)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    m_config->AddScanFolder(ScanFolderInfo(m_tempPath.filePath("scanfolder1"), "ScanFolder1", "sf1", true, false, m_platforms), true); // a root folder that has watched subfolders, not recursive
    m_config->AddScanFolder(ScanFolderInfo(m_tempPath.filePath("scanfolder1/Editor"), "Editor ScanFolder", "sf2", false, true, m_platforms), true); // a child folder that exists within that scan folder.

    const ScanFolderInfo* info = m_config->GetScanFolderForFile(m_tempPath.filePath("scanfolder1/Editor"));
    ASSERT_TRUE(info);

    EXPECT_STREQ(info->ScanPath().toUtf8().constData(), m_tempPath.filePath("scanfolder1/Editor").toUtf8().constData());
    EXPECT_STREQ(info->GetDisplayName().toUtf8().constData(), "Editor ScanFolder");
}


TEST_F(PlatformConfigurationUnitTests, TestFailReadConfigFile_RegularScanfolder)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    const auto testExeFolder = AZ::IO::FileIOBase::GetInstance()->ResolvePath(TestAppRoot);
    const AZ::IO::FixedMaxPath projectPath = (*testExeFolder) / EmptyDummyProjectName;
    auto configRoot = AZ::IO::FileIOBase::GetInstance()->ResolvePath("@exefolder@/testdata/config_regular");
    ASSERT_TRUE(configRoot);
    UnitTestPlatformConfiguration config;
    m_errorAbsorber->Clear();
    AssetUtilities::ComputeProjectName(EmptyDummyProjectName, true);
    ASSERT_TRUE(config.InitializeFromConfigFiles(configRoot->c_str(), testExeFolder->c_str(), projectPath.c_str(), false, false));
    ASSERT_EQ(m_errorAbsorber->m_numErrorsAbsorbed, 0);

    ASSERT_EQ(config.GetScanFolderCount(), 4); // the two, and then the one that has the same data as prior but different identifier, plus hardcoded intermediates scanfolder
    QString scanName = AssetUtilities::ComputeProjectPath(true) + " Scan Folder";

    // Scanfolder 0 is the intermediate assets scanfolder, we don't need to check that folder, so start checking at 1

    ASSERT_EQ(config.GetScanFolderAt(1).GetDisplayName(), scanName);
    ASSERT_EQ(config.GetScanFolderAt(1).RecurseSubFolders(), true);
    ASSERT_EQ(config.GetScanFolderAt(1).GetOrder(), 0);
    ASSERT_EQ(config.GetScanFolderAt(1).GetPortableKey(), QString("Game"));

    ASSERT_EQ(config.GetScanFolderAt(2).GetDisplayName(), QString("FeatureTests"));
    ASSERT_EQ(config.GetScanFolderAt(2).RecurseSubFolders(), false);
    ASSERT_EQ(config.GetScanFolderAt(2).GetOrder(), 5000);
    // this proves that the featuretests name is used instead of the output prefix
    ASSERT_EQ(config.GetScanFolderAt(2).GetPortableKey(), QString("FeatureTests"));

    ASSERT_EQ(config.GetScanFolderAt(3).GetDisplayName(), QString("FeatureTests2"));
    ASSERT_EQ(config.GetScanFolderAt(3).RecurseSubFolders(), false);
    ASSERT_EQ(config.GetScanFolderAt(3).GetOrder(), 6000);
    // this proves that the featuretests name is used instead of the output prefix
    ASSERT_EQ(config.GetScanFolderAt(3).GetPortableKey(), QString("FeatureTests2"));
}

TEST_F(PlatformConfigurationUnitTests, TestFailReadConfigFile_RegularScanfolderPlatformSpecific)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    const auto testExeFolder = AZ::IO::FileIOBase::GetInstance()->ResolvePath(TestAppRoot);
    const AZ::IO::FixedMaxPath projectPath = (*testExeFolder) / EmptyDummyProjectName;
    auto configRoot = AZ::IO::FileIOBase::GetInstance()->ResolvePath("@exefolder@/testdata/config_regular_platform_scanfolder");
    ASSERT_TRUE(configRoot);
    UnitTestPlatformConfiguration config;
    m_errorAbsorber->Clear();
    ASSERT_TRUE(config.InitializeFromConfigFiles(configRoot->c_str(), testExeFolder->c_str(), projectPath.c_str(), false, false));
    ASSERT_EQ(m_errorAbsorber->m_numErrorsAbsorbed, 0);

    ASSERT_EQ(config.GetScanFolderCount(), 6); // +1 for hardcoded intermediates scanfolder
    // Scanfolder 0 is the intermediate assets folder, so start at 1
    ASSERT_EQ(config.GetScanFolderAt(1).GetDisplayName(), QString("gameoutput"));
    AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms = config.GetScanFolderAt(1).GetPlatforms();
    ASSERT_EQ(platforms.size(), 4);
    ASSERT_TRUE(AZStd::find(platforms.begin(), platforms.end(), AssetBuilderSDK::PlatformInfo(AzToolsFramework::AssetSystem::GetHostAssetPlatform(), AZStd::unordered_set<AZStd::string>{})) != platforms.end());
    ASSERT_TRUE(AZStd::find(platforms.begin(), platforms.end(), AssetBuilderSDK::PlatformInfo("android", AZStd::unordered_set<AZStd::string>{})) != platforms.end());
    ASSERT_TRUE(AZStd::find(platforms.begin(), platforms.end(), AssetBuilderSDK::PlatformInfo("ios", AZStd::unordered_set<AZStd::string>{})) != platforms.end());
    ASSERT_TRUE(AZStd::find(platforms.begin(), platforms.end(), AssetBuilderSDK::PlatformInfo("server", AZStd::unordered_set<AZStd::string>{})) != platforms.end());

    ASSERT_EQ(config.GetScanFolderAt(2).GetDisplayName(), QString("editoroutput"));
    platforms = config.GetScanFolderAt(2).GetPlatforms();
    ASSERT_EQ(platforms.size(), 2);
    ASSERT_TRUE(AZStd::find(platforms.begin(), platforms.end(), AssetBuilderSDK::PlatformInfo(AzToolsFramework::AssetSystem::GetHostAssetPlatform(), AZStd::unordered_set<AZStd::string>{})) != platforms.end());
    ASSERT_TRUE(AZStd::find(platforms.begin(), platforms.end(), AssetBuilderSDK::PlatformInfo("android", AZStd::unordered_set<AZStd::string>{})) != platforms.end());

    ASSERT_EQ(config.GetScanFolderAt(3).GetDisplayName(), QString("folder1output"));
    platforms = config.GetScanFolderAt(3).GetPlatforms();
    ASSERT_EQ(platforms.size(), 1);
    ASSERT_TRUE(AZStd::find(platforms.begin(), platforms.end(), AssetBuilderSDK::PlatformInfo("android", AZStd::unordered_set<AZStd::string>{})) != platforms.end());

    ASSERT_EQ(config.GetScanFolderAt(4).GetDisplayName(), QString("folder2output"));
    platforms = config.GetScanFolderAt(4).GetPlatforms();
    ASSERT_EQ(platforms.size(), 3);
    ASSERT_TRUE(AZStd::find(platforms.begin(), platforms.end(), AssetBuilderSDK::PlatformInfo(AzToolsFramework::AssetSystem::GetHostAssetPlatform(), AZStd::unordered_set<AZStd::string>{})) != platforms.end());
    ASSERT_TRUE(AZStd::find(platforms.begin(), platforms.end(), AssetBuilderSDK::PlatformInfo("ios", AZStd::unordered_set<AZStd::string>{})) != platforms.end());
    ASSERT_TRUE(AZStd::find(platforms.begin(), platforms.end(), AssetBuilderSDK::PlatformInfo("server", AZStd::unordered_set<AZStd::string>{})) != platforms.end());

    ASSERT_EQ(config.GetScanFolderAt(5).GetDisplayName(), QString("folder3output"));
    platforms = config.GetScanFolderAt(5).GetPlatforms();
    ASSERT_EQ(platforms.size(), 0);
}


TEST_F(PlatformConfigurationUnitTests, TestFailReadConfigFile_RegularExcludes)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    const auto testExeFolder = AZ::IO::FileIOBase::GetInstance()->ResolvePath(TestAppRoot);
    const AZ::IO::FixedMaxPath projectPath = (*testExeFolder) / EmptyDummyProjectName;
    auto configRoot = AZ::IO::FileIOBase::GetInstance()->ResolvePath("@exefolder@/testdata/config_regular");
    ASSERT_TRUE(configRoot);
    UnitTestPlatformConfiguration config;

    config.AddScanFolder(ScanFolderInfo("blahblah", "Blah ScanFolder", "sf2", true, true), true);
    m_errorAbsorber->Clear();
    ASSERT_TRUE(config.InitializeFromConfigFiles(configRoot->c_str(), testExeFolder->c_str(), projectPath.c_str(), false, false));
    ASSERT_EQ(m_errorAbsorber->m_numErrorsAbsorbed, 0);

    ASSERT_TRUE(config.IsFileExcluded("blahblah/$tmp_01.test"));
    ASSERT_FALSE(config.IsFileExcluded("blahblah/tmp_01.test"));

    ASSERT_TRUE(config.IsFileExcluded("blahblah/Levels/blahblah_hold/whatever.test"));
    ASSERT_FALSE(config.IsFileExcluded("blahblah/Levels/blahblahhold/whatever.test"));
}

TEST_F(PlatformConfigurationUnitTests, Test_GemHandling)
{
    UnitTestPlatformConfiguration config;
    QTemporaryDir tempEngineRoot;
    QDir tempPath(tempEngineRoot.path());
    AssetUtilities::ResetAssetRoot();
    AssetUtilities::ComputeProjectName("AutomatedTesting", true);

    QDir computedEngineRoot;
    ASSERT_TRUE(AssetUtilities::ComputeAssetRoot(computedEngineRoot, &tempPath));
    ASSERT_TRUE(!computedEngineRoot.absolutePath().isEmpty());
    ASSERT_TRUE(tempPath.absolutePath() == computedEngineRoot.absolutePath());

    // create ONE of the two files - they are optional, but the paths to them should always be checked and generated.
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("Gems/LyShine/AssetProcessorGemConfig.ini"), ";nothing to see here"));

    // note that it is expected that the gems system gives us absolute paths.
    AZStd::vector<AzFramework::GemInfo> fakeGems;
    fakeGems.emplace_back("LyShine");// true = pretend this is a game gem.
    fakeGems.back().m_absoluteSourcePaths.push_back(tempPath.absoluteFilePath("Gems/LyShine").toUtf8().constData());
    fakeGems.emplace_back("LmbrCentral");
    fakeGems.back().m_absoluteSourcePaths.push_back(tempPath.absoluteFilePath("Gems/LmbrCentral/v2").toUtf8().constData());

    // reading gems via the Gems System is already to be tested in the actual Gems API tests.
    // to avoid trying to load those DLLs we avoid calling the actual ReadGems function
    config.AddGemScanFolders(fakeGems);

    QString expectedScanFolder = tempPath.absoluteFilePath("Gems/LyShine/Assets");

    AssetUtilities::ResetAssetRoot();

    ASSERT_EQ(4, config.GetScanFolderCount());
    EXPECT_FALSE(config.GetScanFolderAt(0).IsRoot());
    EXPECT_TRUE(config.GetScanFolderAt(0).RecurseSubFolders());
    // the first one is a game gem, so its order should be above 1 but below 100.
    EXPECT_GE(config.GetScanFolderAt(0).GetOrder(), 100);
    EXPECT_EQ(0, config.GetScanFolderAt(0).ScanPath().compare(expectedScanFolder, Qt::CaseInsensitive));

    // for each gem, there are currently 2 scan folders:
    // The Gem's 'Assets' folder
    // The Gem's 'Registry' folder

    expectedScanFolder = tempPath.absoluteFilePath("Gems/LmbrCentral/v2/Assets");
    EXPECT_FALSE(config.GetScanFolderAt(2).IsRoot() );
    EXPECT_TRUE(config.GetScanFolderAt(2).RecurseSubFolders());
    EXPECT_GT(config.GetScanFolderAt(2).GetOrder(), config.GetScanFolderAt(0).GetOrder());
    EXPECT_EQ(0, config.GetScanFolderAt(2).ScanPath().compare(expectedScanFolder, Qt::CaseInsensitive));
}

TEST_F(PlatformConfigurationUnitTests, Test_MetaFileTypes)
{
    UnitTestPlatformConfiguration config;

    config.AddMetaDataType("xxxx", "");
    config.AddMetaDataType("yyyy", "zzzz");
    ASSERT_TRUE(config.MetaDataFileTypesCount() == 2);
    ASSERT_TRUE(QString::compare(config.GetMetaDataFileTypeAt(1).first, "yyyy", Qt::CaseInsensitive) == 0);
    ASSERT_TRUE(QString::compare(config.GetMetaDataFileTypeAt(1).second, "zzzz", Qt::CaseInsensitive) == 0);
}

TEST_F(PlatformConfigurationUnitTests, ReadCheckServer_FromConfig_Valid)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    const auto testExeFolder = AZ::IO::FileIOBase::GetInstance()->ResolvePath(TestAppRoot);
    const AZ::IO::FixedMaxPath projectPath = (*testExeFolder) / EmptyDummyProjectName;
    auto configRoot = AZ::IO::FileIOBase::GetInstance()->ResolvePath("@exefolder@/testdata/config_regular");
    ASSERT_TRUE(configRoot);
    UnitTestPlatformConfiguration config;
    m_errorAbsorber->Clear();
    ASSERT_TRUE(config.InitializeFromConfigFiles(configRoot->c_str(), testExeFolder->c_str(), projectPath.c_str(), false, false));
    ASSERT_EQ(m_errorAbsorber->m_numErrorsAbsorbed, 0);

    const AssetProcessor::RecognizerContainer& recogs = config.GetAssetRecognizerContainer();

    // verify that check server flag is set to true for i_caf
    ASSERT_TRUE(recogs.contains("i_caf"));
    ASSERT_TRUE(recogs.at("i_caf").m_checkServer);
}

TEST_F(PlatformConfigurationUnitTests, PlatformConfigFile_IsPresent_Found)
{
    UnitTestPlatformConfiguration config;
    QTemporaryDir tempEngineRoot;
    QDir tempPath(tempEngineRoot.path());
    AssetUtilities::ResetAssetRoot();
    AssetUtilities::ComputeProjectName("AutomatedTesting", true);

    auto settingsRegistry = AZ::SettingsRegistry::Get();
    ASSERT_NE(nullptr, settingsRegistry);
    settingsRegistry->Set(AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder, tempPath.absolutePath().toUtf8().constData());

    QDir computedEngineRoot;
    ASSERT_TRUE(AssetUtilities::ComputeAssetRoot(computedEngineRoot, &tempPath));
    ASSERT_TRUE(AssetUtilities::ComputeEngineRoot(computedEngineRoot, &tempPath));
    ASSERT_TRUE(!computedEngineRoot.absolutePath().isEmpty());
    ASSERT_TRUE(tempPath.absolutePath() == computedEngineRoot.absolutePath());

    // create ONE of the two files - they are optional, but the paths to them should always be checked and generated.
    QString platformConfigPath{ AssetProcessor::AssetConfigPlatformDir };
    platformConfigPath.append("TestPlatform/");
    platformConfigPath.append(AssetProcessor::AssetProcessorPlatformConfigFileName);

    AZStd::vector<AZ::IO::Path> platformConfigList;
    ASSERT_FALSE(config.AddPlatformConfigFilePaths(platformConfigList));
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath(platformConfigPath), ";nothing to see here"));
    ASSERT_TRUE(config.AddPlatformConfigFilePaths(platformConfigList));
    ASSERT_EQ(platformConfigList.size(), 1);
}

TEST_F(PlatformConfigurationUnitTests, Test_MetaFileTypes_AssetImporterExtensions)
{
    using namespace AssetProcessor;

    const auto testExeFolder = AZ::IO::FileIOBase::GetInstance()->ResolvePath(TestAppRoot);
    const AZ::IO::FixedMaxPath projectPath = (*testExeFolder) / EmptyDummyProjectName;
    auto configRoot = AZ::IO::FileIOBase::GetInstance()->ResolvePath("@exefolder@/testdata/config_metadata");
    ASSERT_TRUE(configRoot);
    UnitTestPlatformConfiguration config;
    m_errorAbsorber->Clear();
    ASSERT_FALSE(config.InitializeFromConfigFiles(configRoot->c_str(), testExeFolder->c_str(), projectPath.c_str(), false, false));
    ASSERT_GT(m_errorAbsorber->m_numErrorsAbsorbed, 0);
    ASSERT_TRUE(config.MetaDataFileTypesCount() == 3);

    QStringList entriesToTest{ "aaa", "bbb" };
    for (int idx = 0; idx < entriesToTest.size(); idx++)
    {
        // + 1 skip .meta files
        ASSERT_EQ(config.GetMetaDataFileTypeAt(idx + 1).first, QString("%1.assetinfo").arg(entriesToTest[idx]));
        ASSERT_EQ(config.GetMetaDataFileTypeAt(idx + 1).second, QString("%1").arg(entriesToTest[idx]));
    }
}

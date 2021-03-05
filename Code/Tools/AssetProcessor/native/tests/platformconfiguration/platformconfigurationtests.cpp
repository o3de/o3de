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

#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include "native/tests/platformconfiguration/platformconfigurationtests.h"

const char TestAppRoot[] = ":/testdata";
const char EmptyDummyProjectName[] = "EmptyDummyProject";
const char DummyProjectName[] = "DummyProject";

// make the internal calls public for the purposes of the unit test!
class UnitTestPlatformConfiguration : public AssetProcessor::PlatformConfiguration
{
    friend class GTEST_TEST_CLASS_NAME_(PlatformConfigurationUnitTests, Test_GemHandling);
    friend class GTEST_TEST_CLASS_NAME_(PlatformConfigurationUnitTests, Test_MetaFileTypes);
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

    const char* configRoot = ":/testdata/config_broken_badplatform";
    UnitTestPlatformConfiguration config;
    m_absorber.Clear();
    ASSERT_FALSE(config.InitializeFromConfigFiles(configRoot, TestAppRoot, EmptyDummyProjectName, false, false));
    ASSERT_GT(m_absorber.m_numErrorsAbsorbed, 0);
}


TEST_F(PlatformConfigurationUnitTests, TestFailReadConfigFile_NoPlatform)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    const char* configRoot = ":/testdata/config_broken_noplatform";
    UnitTestPlatformConfiguration config;
    m_absorber.Clear();
    ASSERT_FALSE(config.InitializeFromConfigFiles(configRoot, TestAppRoot, EmptyDummyProjectName, false, false));
    ASSERT_GT(m_absorber.m_numErrorsAbsorbed, 0);
}

TEST_F(PlatformConfigurationUnitTests, TestFailReadConfigFile_NoScanFolders)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    const char* configRoot = ":/testdata/config_broken_noscans";
    UnitTestPlatformConfiguration config;
    m_absorber.Clear();
    ASSERT_FALSE(config.InitializeFromConfigFiles(configRoot, TestAppRoot, EmptyDummyProjectName, false, false));
    ASSERT_GT(m_absorber.m_numErrorsAbsorbed, 0);
}

TEST_F(PlatformConfigurationUnitTests, TestFailReadConfigFile_BrokenRecognizers)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    const char* configRoot = ":/testdata/config_broken_recognizers";
    UnitTestPlatformConfiguration config;
    m_absorber.Clear();
    ASSERT_FALSE(config.InitializeFromConfigFiles(configRoot, TestAppRoot, EmptyDummyProjectName, false, false));
    ASSERT_GT(m_absorber.m_numErrorsAbsorbed, 0);
}

TEST_F(PlatformConfigurationUnitTests, TestFailReadConfigFile_Regular_Platforms)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    const char* configRoot = ":/testdata/config_regular";
    UnitTestPlatformConfiguration config;
    m_absorber.Clear();
    ASSERT_TRUE(config.InitializeFromConfigFiles(configRoot, TestAppRoot, EmptyDummyProjectName, false, false));
    ASSERT_EQ(m_absorber.m_numErrorsAbsorbed, 0);

    // verify the data.
    ASSERT_NE(config.GetPlatformByIdentifier(AzToolsFramework::AssetSystem::GetHostAssetPlatform()), nullptr);
    ASSERT_NE(config.GetPlatformByIdentifier("es3"), nullptr);
    ASSERT_NE(config.GetPlatformByIdentifier("server"), nullptr);
    ASSERT_EQ(config.GetPlatformByIdentifier("xenia"), nullptr);

    ASSERT_TRUE(config.GetPlatformByIdentifier("es3")->HasTag("mobile"));
    ASSERT_TRUE(config.GetPlatformByIdentifier("es3")->HasTag("renderer"));
    ASSERT_TRUE(config.GetPlatformByIdentifier("es3")->HasTag("android"));
    ASSERT_TRUE(config.GetPlatformByIdentifier("server")->HasTag("server"));
    ASSERT_FALSE(config.GetPlatformByIdentifier("es3")->HasTag("server"));
    ASSERT_FALSE(config.GetPlatformByIdentifier("server")->HasTag("renderer"));
}

TEST_F(PlatformConfigurationUnitTests, TestReadScanFolderRoot_FromSettingsRegistry_Succeeds)
{
    auto settingsRegistry = AZ::SettingsRegistry::Get();
    ASSERT_NE(nullptr, settingsRegistry);

    AZ::SettingsRegistryInterface::Specializations apSpecializations;
    AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_EngineRegistry(*settingsRegistry, AZ_TRAIT_OS_PLATFORM_CODENAME, apSpecializations);

    struct ScanFolderVisitor
        : AZ::SettingsRegistryInterface::Visitor
    {
        void Visit([[maybe_unused]] AZStd::string_view path, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, AZ::s64 value)
        {
            if (valueName == "recursive")
            {
                m_isRecursive = value != 0;
            }
            else if (valueName == "order")
            {
                m_scanOrder = value;
            }
        }
        void Visit([[maybe_unused]] AZStd::string_view path, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, AZStd::string_view value)
        {
            if (valueName == "watch")
            {
                m_watchPath = value;
            }
        }
        AZ::SettingsRegistryInterface::FixedValueString m_watchPath;
        bool m_isRecursive{};
        int m_scanOrder{};
    };

    ScanFolderVisitor scanFolderVisitor;
    EXPECT_TRUE(settingsRegistry->Visit(scanFolderVisitor, "/Amazon/AssetProcessor/Settings/ScanFolder Root"));

    // These test values come from the <dev_root>/Engine/Registry/AssetProcessorPlatformConfig.setreg file
    EXPECT_STREQ("@ROOT@", scanFolderVisitor.m_watchPath.c_str());
    EXPECT_FALSE(scanFolderVisitor.m_isRecursive);
    EXPECT_EQ(10000, scanFolderVisitor.m_scanOrder);
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

    m_config->AddScanFolder(ScanFolderInfo(m_tempPath.filePath("scanfolder1"), "ScanFolder1", "sf1", "", true, false, m_platforms), true); // a root folder that has watched subfolders, not recursive
    m_config->AddScanFolder(ScanFolderInfo(m_tempPath.filePath("scanfolder1/Editor"), "Editor", "sf2", "", false, true, m_platforms), true); // a child folder that exists within that scan folder.

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

    m_config->AddScanFolder(ScanFolderInfo(m_tempPath.filePath("scanfolder1"), "ScanFolder1", "sf1", "", true, false, m_platforms), true); // a root folder that has watched subfolders, not recursive
    m_config->AddScanFolder(ScanFolderInfo(m_tempPath.filePath("scanfolder1/Editor"), "Editor ScanFolder", "sf2", "", false, true, m_platforms), true); // a child folder that exists within that scan folder.

    const ScanFolderInfo* info = m_config->GetScanFolderForFile(m_tempPath.filePath("scanfolder1/Editor/something.txt"));

    ASSERT_TRUE(info);
    EXPECT_STREQ(info->ScanPath().toUtf8().constData(), m_tempPath.filePath("scanfolder1/Editor").toUtf8().constData());
    EXPECT_STREQ(info->GetDisplayName().toUtf8().constData(), "Editor ScanFolder");
}

// note that in the case of GetOverridingFile, this SHOULD return the correct case if an override is found
// because its possible to override a file with another file with different case in a different scan folder
// such a situation is supposed to be very rare, so the cost of correcting the case is mitigated.
TEST_F(PlatformConfigurationUnitTests_OnePCHostFixture, GetOverridingFile_Exists_ReturnsCorrectCase)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    // create two scan folders, since its order dependent, the ScanFolder1 is the "winner" in tie breakers (when they both contain same file relpath)
    QString scanfolder1Path = m_tempPath.filePath("scanfolder1");
    QString scanfolder2Path = m_tempPath.filePath("scanfolder2");
    QString caseSensitiveDummyFileName = m_tempPath.absoluteFilePath("scanfolder1/TestCase.tXt");
    QString differentCaseDummyFileName = m_tempPath.absoluteFilePath("scanfolder2/testcase.txt");
    UnitTestUtils::CreateDummyFile(caseSensitiveDummyFileName, QString("testcase1\n"));
    UnitTestUtils::CreateDummyFile(differentCaseDummyFileName, QString("testcase2\n"));
    m_config->AddScanFolder(ScanFolderInfo(scanfolder1Path, "ScanFolder1", "sf1", "", false, true, m_platforms), true);
    m_config->AddScanFolder(ScanFolderInfo(scanfolder2Path, "ScanFolder2", "sf2", "", false, true, m_platforms), true);

    // Perform the test by asking it whether anyone overrides "testcase" (lowercase) in scanfolder 2.
    QString overrider = m_config->GetOverridingFile("testcase.txt", scanfolder2Path);

    ASSERT_FALSE(overrider.isEmpty());
    // the result should be the real actual case of the file in scanfolder 1:
    EXPECT_STREQ(overrider.toUtf8().constData(), caseSensitiveDummyFileName.toUtf8().constData());
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
    m_config->AddScanFolder(ScanFolderInfo(scanfolder1Path, "ScanFolder1", "sf1", "", false, true, m_platforms), true);
    m_config->AddScanFolder(ScanFolderInfo(scanfolder2Path, "ScanFolder2", "sf2", "", false, true, m_platforms), true);
    
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

    m_config->AddScanFolder(ScanFolderInfo(scanfolder1Path, "ScanFolder1", "sf1", "", false, true, m_platforms), true);
    m_config->AddScanFolder(ScanFolderInfo(scanfolder2Path, "ScanFolder2", "sf2", "", false, true, m_platforms), true);

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
    m_config->AddScanFolder(ScanFolderInfo(scanfolder1Path, "ScanFolder1", "sf1", "", false, true, m_platforms), true);
    m_config->AddScanFolder(ScanFolderInfo(scanfolder2Path, "ScanFolder2", "sf2", "", false, true, m_platforms), true);

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

    m_config->AddScanFolder(ScanFolderInfo(m_tempPath.filePath("scanfolder1"), "ScanFolder1", "sf1", "", true, false, m_platforms), true); // a root folder that has watched subfolders, not recursive
    m_config->AddScanFolder(ScanFolderInfo(m_tempPath.filePath("scanfolder1/Editor"), "Editor ScanFolder", "sf2", "", false, true, m_platforms), true); // a child folder that exists within that scan folder.

    const ScanFolderInfo* info = m_config->GetScanFolderForFile(m_tempPath.filePath("scanfolder1/Editor"));
    ASSERT_TRUE(info);

    EXPECT_STREQ(info->ScanPath().toUtf8().constData(), m_tempPath.filePath("scanfolder1/Editor").toUtf8().constData());
    EXPECT_STREQ(info->GetDisplayName().toUtf8().constData(), "Editor ScanFolder");
}


TEST_F(PlatformConfigurationUnitTests, TestFailReadConfigFile_RegularScanfolder)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    const char* configRoot = ":/testdata/config_regular";
    UnitTestPlatformConfiguration config;
    m_absorber.Clear();
    ASSERT_TRUE(config.InitializeFromConfigFiles(configRoot, TestAppRoot, EmptyDummyProjectName, false, false));
    ASSERT_EQ(m_absorber.m_numErrorsAbsorbed, 0);

    ASSERT_EQ(config.GetScanFolderCount(), 3); // the two, and then the one that has the same data as prior but different identifier.
    QString scanName = AssetUtilities::ComputeGameName() + " Scan Folder";
    ASSERT_EQ(config.GetScanFolderAt(0).GetDisplayName(), scanName);
    ASSERT_EQ(config.GetScanFolderAt(0).GetOutputPrefix(), QString());
    ASSERT_EQ(config.GetScanFolderAt(0).RecurseSubFolders(), true);
    ASSERT_EQ(config.GetScanFolderAt(0).GetOrder(), 0);
    // its important that this does NOT change and this makes sure the old way of doing it (case-sensitive name) persists
    ASSERT_EQ(config.GetScanFolderAt(0).GetPortableKey(), QString("from-ini-file-Game"));

    ASSERT_EQ(config.GetScanFolderAt(1).GetDisplayName(), QString("FeatureTests"));
    ASSERT_EQ(config.GetScanFolderAt(1).GetOutputPrefix(), QString("featuretestsoutputfolder")); // to prove its not related to display name
    ASSERT_EQ(config.GetScanFolderAt(1).RecurseSubFolders(), false);
    ASSERT_EQ(config.GetScanFolderAt(1).GetOrder(), 5000);
    // this proves that the featuretests name is used instead of the output prefix
    ASSERT_EQ(config.GetScanFolderAt(1).GetPortableKey(), QString("from-ini-file-FeatureTests"));

    ASSERT_EQ(config.GetScanFolderAt(2).GetDisplayName(), QString("FeatureTests2"));
    ASSERT_EQ(config.GetScanFolderAt(2).GetOutputPrefix(), QString("featuretestsoutputfolder")); // to prove its not related to display name
    ASSERT_EQ(config.GetScanFolderAt(2).RecurseSubFolders(), false);
    ASSERT_EQ(config.GetScanFolderAt(2).GetOrder(), 6000);
    // this proves that the featuretests name is used instead of the output prefix
    ASSERT_EQ(config.GetScanFolderAt(2).GetPortableKey(), QString("from-ini-file-FeatureTests2"));
}

TEST_F(PlatformConfigurationUnitTests, TestFailReadConfigFile_RegularScanfolderPlatformSpecific)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    const char* configRoot = ":/testdata/config_regular_platform_scanfolder";
    UnitTestPlatformConfiguration config;
    m_absorber.Clear();
    ASSERT_TRUE(config.InitializeFromConfigFiles(configRoot, TestAppRoot, EmptyDummyProjectName, false, false));
    ASSERT_EQ(m_absorber.m_numErrorsAbsorbed, 0);

    ASSERT_EQ(config.GetScanFolderCount(), 5);
    ASSERT_EQ(config.GetScanFolderAt(0).GetDisplayName(), QString("gameoutput"));
    AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms = config.GetScanFolderAt(0).GetPlatforms();
    ASSERT_EQ(platforms.size(), 4);
    ASSERT_TRUE(AZStd::find(platforms.begin(), platforms.end(), AssetBuilderSDK::PlatformInfo(AzToolsFramework::AssetSystem::GetHostAssetPlatform(), AZStd::unordered_set<AZStd::string>{})) != platforms.end());
    ASSERT_TRUE(AZStd::find(platforms.begin(), platforms.end(), AssetBuilderSDK::PlatformInfo("es3", AZStd::unordered_set<AZStd::string>{})) != platforms.end());
    ASSERT_TRUE(AZStd::find(platforms.begin(), platforms.end(), AssetBuilderSDK::PlatformInfo("ios", AZStd::unordered_set<AZStd::string>{})) != platforms.end());
    ASSERT_TRUE(AZStd::find(platforms.begin(), platforms.end(), AssetBuilderSDK::PlatformInfo("server", AZStd::unordered_set<AZStd::string>{})) != platforms.end());

    ASSERT_EQ(config.GetScanFolderAt(1).GetDisplayName(), QString("editoroutput"));
    platforms = config.GetScanFolderAt(1).GetPlatforms();
    ASSERT_EQ(platforms.size(), 2);
    ASSERT_TRUE(AZStd::find(platforms.begin(), platforms.end(), AssetBuilderSDK::PlatformInfo(AzToolsFramework::AssetSystem::GetHostAssetPlatform(), AZStd::unordered_set<AZStd::string>{})) != platforms.end());
    ASSERT_TRUE(AZStd::find(platforms.begin(), platforms.end(), AssetBuilderSDK::PlatformInfo("es3", AZStd::unordered_set<AZStd::string>{})) != platforms.end());

    ASSERT_EQ(config.GetScanFolderAt(2).GetDisplayName(), QString("folder1output"));
    platforms = config.GetScanFolderAt(2).GetPlatforms();
    ASSERT_EQ(platforms.size(), 1);
    ASSERT_TRUE(AZStd::find(platforms.begin(), platforms.end(), AssetBuilderSDK::PlatformInfo("es3", AZStd::unordered_set<AZStd::string>{})) != platforms.end());

    ASSERT_EQ(config.GetScanFolderAt(3).GetDisplayName(), QString("folder2output"));
    platforms = config.GetScanFolderAt(3).GetPlatforms();
    ASSERT_EQ(platforms.size(), 3);
    ASSERT_TRUE(AZStd::find(platforms.begin(), platforms.end(), AssetBuilderSDK::PlatformInfo(AzToolsFramework::AssetSystem::GetHostAssetPlatform(), AZStd::unordered_set<AZStd::string>{})) != platforms.end());
    ASSERT_TRUE(AZStd::find(platforms.begin(), platforms.end(), AssetBuilderSDK::PlatformInfo("ios", AZStd::unordered_set<AZStd::string>{})) != platforms.end());
    ASSERT_TRUE(AZStd::find(platforms.begin(), platforms.end(), AssetBuilderSDK::PlatformInfo("server", AZStd::unordered_set<AZStd::string>{})) != platforms.end());

    ASSERT_EQ(config.GetScanFolderAt(4).GetDisplayName(), QString("folder3output"));
    platforms = config.GetScanFolderAt(4).GetPlatforms();
    ASSERT_EQ(platforms.size(), 0);
}


TEST_F(PlatformConfigurationUnitTests, TestFailReadConfigFile_RegularExcludes)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    const char* configRoot = ":/testdata/config_regular";
    UnitTestPlatformConfiguration config;
    m_absorber.Clear();
    ASSERT_TRUE(config.InitializeFromConfigFiles(configRoot, TestAppRoot, EmptyDummyProjectName, false, false));
    ASSERT_EQ(m_absorber.m_numErrorsAbsorbed, 0);

    ASSERT_TRUE(config.IsFileExcluded("blahblah/$tmp_01.test"));
    ASSERT_FALSE(config.IsFileExcluded("blahblah/tmp_01.test"));

    ASSERT_TRUE(config.IsFileExcluded("blahblah/Levels/blahblah_hold/whatever.test"));
    ASSERT_FALSE(config.IsFileExcluded("blahblah/Levels/blahblahhold/whatever.test"));
}

TEST_F(PlatformConfigurationUnitTests, TestFailReadConfigFile_Recognizers)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;
#if defined(AZ_PLATFORM_WINDOWS)
    const char* platformWhichIsNotCurrentPlatform = "osx_gl";
#else
    const char* platformWhichIsNotCurrentPlatform = "pc";
#endif

    const char* configRoot = ":/testdata/config_regular";
    UnitTestPlatformConfiguration config;
    m_absorber.Clear();
    ASSERT_TRUE(config.InitializeFromConfigFiles(configRoot, TestAppRoot, EmptyDummyProjectName, false, false));
    ASSERT_EQ(m_absorber.m_numErrorsAbsorbed, 0);

    const AssetProcessor::RecognizerContainer& recogs = config.GetAssetRecognizerContainer();

    ASSERT_EQ(recogs.size(), 6);

    ASSERT_TRUE(recogs.contains("i_caf"));
    ASSERT_EQ(recogs["i_caf"].m_patternMatcher.GetBuilderPattern().m_pattern, "*.i_caf");
    ASSERT_EQ(recogs["i_caf"].m_patternMatcher.GetBuilderPattern().m_type, AssetBuilderSDK::AssetBuilderPattern::Wildcard);
    ASSERT_EQ(recogs["i_caf"].m_platformSpecs.size(), 2);
    ASSERT_TRUE(recogs["i_caf"].m_platformSpecs.contains("es3"));
    ASSERT_TRUE(recogs["i_caf"].m_platformSpecs.contains(AzToolsFramework::AssetSystem::GetHostAssetPlatform()));
    ASSERT_FALSE(recogs["i_caf"].m_platformSpecs.contains("server")); // server has been set to skip.
    ASSERT_EQ(recogs["i_caf"].m_platformSpecs["es3"].m_extraRCParams, "mobile");
    ASSERT_EQ(recogs["i_caf"].m_platformSpecs[AzToolsFramework::AssetSystem::GetHostAssetPlatform()].m_extraRCParams, "defaultparams");

    ASSERT_TRUE(recogs.contains("caf"));
    ASSERT_TRUE(recogs["caf"].m_platformSpecs.contains("es3"));
    ASSERT_TRUE(recogs["caf"].m_platformSpecs.contains("server"));
    ASSERT_TRUE(recogs["caf"].m_platformSpecs.contains(AzToolsFramework::AssetSystem::GetHostAssetPlatform()));
    ASSERT_EQ(recogs["caf"].m_platformSpecs.size(), 3);
    ASSERT_EQ(recogs["caf"].m_platformSpecs["es3"].m_extraRCParams, "rendererparams");
    ASSERT_EQ(recogs["caf"].m_platformSpecs[AzToolsFramework::AssetSystem::GetHostAssetPlatform()].m_extraRCParams, "rendererparams");
    ASSERT_EQ(recogs["caf"].m_platformSpecs["server"].m_extraRCParams, "copy");

    ASSERT_TRUE(recogs.contains("mov"));
    ASSERT_TRUE(recogs["mov"].m_platformSpecs.contains("es3"));
    ASSERT_TRUE(recogs["mov"].m_platformSpecs.contains("server"));
    ASSERT_TRUE(recogs["mov"].m_platformSpecs.contains(AzToolsFramework::AssetSystem::GetHostAssetPlatform()));
    ASSERT_EQ(recogs["mov"].m_platformSpecs.size(), 3);
    ASSERT_EQ(recogs["mov"].m_platformSpecs["es3"].m_extraRCParams, "platformspecificoverride");
    ASSERT_EQ(recogs["mov"].m_platformSpecs[AzToolsFramework::AssetSystem::GetHostAssetPlatform()].m_extraRCParams, "rendererparams");
    ASSERT_EQ(recogs["mov"].m_platformSpecs["server"].m_extraRCParams, "copy");

    // the "rend" test makes sure that even if you dont specify 'params' its still there by default for all enabled platforms.
    // (but platforms can override it)
    ASSERT_TRUE(recogs.contains("rend"));
    ASSERT_TRUE(recogs["rend"].m_platformSpecs.contains(AzToolsFramework::AssetSystem::GetHostAssetPlatform()));
    ASSERT_TRUE(recogs["rend"].m_platformSpecs.contains("es3"));
    ASSERT_TRUE(recogs["rend"].m_platformSpecs.contains("server"));
    ASSERT_FALSE(recogs["rend"].m_platformSpecs.contains(platformWhichIsNotCurrentPlatform)); // this is not an enabled platform and should not be there.
    ASSERT_EQ(recogs["rend"].m_platformSpecs.size(), 3);
    ASSERT_EQ(recogs["rend"].m_platformSpecs[AzToolsFramework::AssetSystem::GetHostAssetPlatform()].m_extraRCParams, "rendererparams");
    ASSERT_EQ(recogs["rend"].m_platformSpecs["es3"].m_extraRCParams, "rendererparams");
    ASSERT_EQ(recogs["rend"].m_platformSpecs["server"].m_extraRCParams, ""); // default if not specified is empty string

    ASSERT_TRUE(recogs.contains("alldefault"));
    ASSERT_TRUE(recogs["alldefault"].m_platformSpecs.contains(AzToolsFramework::AssetSystem::GetHostAssetPlatform()));
    ASSERT_TRUE(recogs["alldefault"].m_platformSpecs.contains("es3"));
    ASSERT_TRUE(recogs["alldefault"].m_platformSpecs.contains("server"));
    ASSERT_FALSE(recogs["alldefault"].m_platformSpecs.contains(platformWhichIsNotCurrentPlatform)); // this is not an enabled platform and should not be there.
    ASSERT_EQ(recogs["alldefault"].m_platformSpecs.size(), 3);
    ASSERT_EQ(recogs["alldefault"].m_platformSpecs[AzToolsFramework::AssetSystem::GetHostAssetPlatform()].m_extraRCParams, "");
    ASSERT_EQ(recogs["alldefault"].m_platformSpecs["es3"].m_extraRCParams, "");
    ASSERT_EQ(recogs["alldefault"].m_platformSpecs["server"].m_extraRCParams, "");

    ASSERT_TRUE(recogs.contains("skipallbutone"));
    ASSERT_FALSE(recogs["skipallbutone"].m_platformSpecs.contains(AzToolsFramework::AssetSystem::GetHostAssetPlatform()));
    ASSERT_FALSE(recogs["skipallbutone"].m_platformSpecs.contains("es3"));
    ASSERT_TRUE(recogs["skipallbutone"].m_platformSpecs.contains("server")); // server is only one enabled (set to copy)
    ASSERT_EQ(recogs["skipallbutone"].m_platformSpecs.size(), 1);
    ASSERT_EQ(recogs["skipallbutone"].m_platformSpecs["server"].m_extraRCParams, "copy");
}


TEST_F(PlatformConfigurationUnitTests, TestFailReadConfigFile_Overrides)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;
    const char* configRoot = ":/testdata/config_regular";
    UnitTestPlatformConfiguration config;
    m_absorber.Clear();
        
    ASSERT_TRUE(config.InitializeFromConfigFiles(configRoot, TestAppRoot, DummyProjectName, false, false));
    ASSERT_EQ(m_absorber.m_numErrorsAbsorbed, 0);

    const AssetProcessor::RecognizerContainer& recogs = config.GetAssetRecognizerContainer();

    // note that the override config DISABLES the server platform - and this in turn disables the "server only" compile rule called skipallbutone

    // verify the data.
    ASSERT_NE(config.GetPlatformByIdentifier(AzToolsFramework::AssetSystem::GetHostAssetPlatform()), nullptr);
    ASSERT_NE(config.GetPlatformByIdentifier("es3"), nullptr);
    ASSERT_NE(config.GetPlatformByIdentifier("provo"), nullptr);
    // this override swaps server with provo in that it turns ON provo, turns off server
    ASSERT_EQ(config.GetPlatformByIdentifier("xenia"), nullptr);
    ASSERT_EQ(config.GetPlatformByIdentifier("server"), nullptr); // this should be off due to overrides

    // there is a rule which only output on server, so that rule should be omitted
    ASSERT_FALSE(recogs.contains("skipallbutone")); // this is the rule that had only a server.

    // this exists in config_regular.ini but is removed by config_overrides.ini
    ASSERT_FALSE(recogs.contains("mov"));

    ASSERT_EQ(recogs.size(), 4); // so there's 4 instead of 6 because of the above omissions

    ASSERT_TRUE(recogs.contains("i_caf"));
    ASSERT_EQ(recogs["i_caf"].m_patternMatcher.GetBuilderPattern().m_pattern, "*.i_caf");
    ASSERT_EQ(recogs["i_caf"].m_patternMatcher.GetBuilderPattern().m_type, AssetBuilderSDK::AssetBuilderPattern::Wildcard);
    ASSERT_EQ(recogs["i_caf"].m_platformSpecs.size(), 3);
    ASSERT_TRUE(recogs["i_caf"].m_platformSpecs.contains("es3"));
    ASSERT_TRUE(recogs["i_caf"].m_platformSpecs.contains("provo"));
    ASSERT_TRUE(recogs["i_caf"].m_platformSpecs.contains(AzToolsFramework::AssetSystem::GetHostAssetPlatform()));
    ASSERT_FALSE(recogs["i_caf"].m_platformSpecs.contains("server")); // server has been set to skip.
    ASSERT_EQ(recogs["i_caf"].m_platformSpecs["es3"].m_extraRCParams, "mobile");
    ASSERT_EQ(recogs["i_caf"].m_platformSpecs[AzToolsFramework::AssetSystem::GetHostAssetPlatform()].m_extraRCParams, "defaultparams");
    ASSERT_EQ(recogs["i_caf"].m_platformSpecs["provo"].m_extraRCParams, "copy");

}

TEST_F(PlatformConfigurationUnitTests, Test_GemHandling)
{
    UnitTestPlatformConfiguration config;
    QTemporaryDir tempEngineRoot;
    QDir tempPath(tempEngineRoot.path());
    AssetUtilities::ResetAssetRoot();
    AssetUtilities::ComputeGameName("SamplesProject", true);

    QDir computedEngineRoot;
    ASSERT_TRUE(AssetUtilities::ComputeAssetRoot(computedEngineRoot, &tempPath));
    ASSERT_TRUE(!computedEngineRoot.absolutePath().isEmpty());
    ASSERT_TRUE(tempPath.absolutePath() == computedEngineRoot.absolutePath());

    // create ONE of the two files - they are optional, but the paths to them should always be checked and generated.
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("Gems/LyShine/AssetProcessorGemConfig.ini"), ";nothing to see here"));

    // note that it is expected that the gems system gives us absolute paths.
    AZStd::vector<AzToolsFramework::AssetUtils::GemInfo> fakeGems;
    fakeGems.push_back({ "LyShine", "Gems/LyShine", tempPath.absoluteFilePath("Gems/LyShine").toUtf8().constData(), "0fefab3f13364722b2eab3b96ce2bf20", true, false });// true = pretend this is a game gem.
    fakeGems.push_back({ "LmbrCentral", "Gems/LmbrCentral/v2", tempPath.absoluteFilePath("Gems/LmbrCentral/v2").toUtf8().constData(), "ff06785f7145416b9d46fde39098cb0c", false, false });

    // reading gems via the Gems System is already to be tested in the actual Gems API tests.
    // to avoid trying to load those DLLs we avoid calling the actual ReadGems function
    config.AddGemScanFolders(fakeGems);

    QString expectedScanFolder = tempPath.absoluteFilePath("Gems/LyShine/Assets");

    AssetUtilities::ResetAssetRoot();

    ASSERT_EQ(2, config.GetScanFolderCount());
    EXPECT_FALSE(config.GetScanFolderAt(0).IsRoot());
    EXPECT_TRUE(config.GetScanFolderAt(0).GetOutputPrefix().isEmpty());
    EXPECT_TRUE(config.GetScanFolderAt(0).RecurseSubFolders());
    // the first one is a game gem, so its order should be above 1 but below 100.
    EXPECT_GE(config.GetScanFolderAt(0).GetOrder(), 1);
    EXPECT_LE(config.GetScanFolderAt(0).GetOrder(), 100);
    EXPECT_EQ(0, config.GetScanFolderAt(0).ScanPath().compare(expectedScanFolder, Qt::CaseInsensitive));

    // for each gem, there are currently 1 scan folder, the gem assets folder, with no output prefix

    expectedScanFolder = tempPath.absoluteFilePath("Gems/LmbrCentral/v2/Assets");
    EXPECT_FALSE(config.GetScanFolderAt(1).IsRoot() );
    EXPECT_TRUE(config.GetScanFolderAt(1).GetOutputPrefix().isEmpty());
    EXPECT_TRUE(config.GetScanFolderAt(1).RecurseSubFolders());
    EXPECT_GT(config.GetScanFolderAt(1).GetOrder(), config.GetScanFolderAt(0).GetOrder());
    EXPECT_EQ(0, config.GetScanFolderAt(1).ScanPath().compare(expectedScanFolder, Qt::CaseInsensitive));
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

TEST_F(PlatformConfigurationUnitTests, ReadCheckSever_FromConfig_Valid)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    const char* configRoot = ":/testdata/config_regular";
    UnitTestPlatformConfiguration config;
    m_absorber.Clear();
    ASSERT_TRUE(config.InitializeFromConfigFiles(configRoot, TestAppRoot, EmptyDummyProjectName, false, false));
    ASSERT_EQ(m_absorber.m_numErrorsAbsorbed, 0);

    const AssetProcessor::RecognizerContainer& recogs = config.GetAssetRecognizerContainer();

    // verify that check server flag is set to true for i_caf
    ASSERT_TRUE(recogs.contains("i_caf"));
    ASSERT_TRUE(recogs["i_caf"].m_checkServer);
}

TEST_F(PlatformConfigurationUnitTests, PlatformConfigFile_IsPresent_Found)
{
    UnitTestPlatformConfiguration config;
    QTemporaryDir tempEngineRoot;
    QDir tempPath(tempEngineRoot.path());
    AssetUtilities::ResetAssetRoot();
    AssetUtilities::ComputeGameName("SamplesProject", true);

    QDir computedEngineRoot;
    ASSERT_TRUE(AssetUtilities::ComputeAssetRoot(computedEngineRoot, &tempPath));
    ASSERT_TRUE(AssetUtilities::ComputeEngineRoot(computedEngineRoot, &tempPath));
    ASSERT_TRUE(!computedEngineRoot.absolutePath().isEmpty());
    ASSERT_TRUE(tempPath.absolutePath() == computedEngineRoot.absolutePath());

    // create ONE of the two files - they are optional, but the paths to them should always be checked and generated.
    QString platformConfigPath{ AssetProcessor::AssetConfigPlatformDir };
    platformConfigPath.append("TestPlatform/");
    platformConfigPath.append(AssetProcessor::AssetProcessorPlatformConfigFileName);

    QStringList platformConfigList;
    ASSERT_FALSE(config.AddPlatformConfigFilePaths(platformConfigList));
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath(platformConfigPath), ";nothing to see here"));
    ASSERT_TRUE(config.AddPlatformConfigFilePaths(platformConfigList));
    ASSERT_EQ(platformConfigList.size(), 1);
}

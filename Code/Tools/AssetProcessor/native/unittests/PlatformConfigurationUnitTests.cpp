/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/utilities/PlatformConfiguration.h>
#include <native/unittests/UnitTestUtils.h>
#include <native/unittests/AssetProcessorUnitTests.h>
#include <native/tests/MockAssetDatabaseRequestsHandler.h>

using namespace UnitTestUtils;
using namespace AssetProcessor;

class PlatformConfigurationTests
    : public UnitTest::AssetProcessorUnitTestBase
{
public:
    void SetUp() override
    {
        UnitTest::AssetProcessorUnitTestBase::SetUp();

        m_assetRootPath = QDir(m_assetDatabaseRequestsHandler->GetAssetRootDir().c_str());

        for (const AssetBuilderSDK::PlatformInfo& enabledPlatform : m_enabledPlatforms)
        {
            m_config.EnablePlatform(enabledPlatform, true);
        }
        m_config.EnablePlatform({ "fandago",{ "console" } }, false);

        AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms;
        m_config.PopulatePlatformsForScanFolder(platforms);

        //                                                       PATH             DisplayName PortKey root recurse platforms
        m_scanFolders.emplace_back(ScanFolderInfo(m_assetRootPath.filePath("subfolder3"), "", "sf3", false, false, platforms)); // subfolder 3 is expected to override subfolder2
        m_scanFolders.emplace_back(ScanFolderInfo(m_assetRootPath.filePath("subfolder2"),   "", "sf4",  false, true,    platforms)); // subfolder 2 is expected to override subfolder1
        m_scanFolders.emplace_back(ScanFolderInfo(m_assetRootPath.filePath("subfolder1"),   "", "sf1",  false, true,    platforms)); // subfolder1 is expected to override root
        m_scanFolders.emplace_back(ScanFolderInfo(m_assetRootPath.filePath("subfolder4"),   "", "sf4",  false, true,    platforms)); // subfolder4 is expected to override subfolder5
        m_scanFolders.emplace_back(ScanFolderInfo(m_assetRootPath.filePath("subfolder5"),   "", "sf5",  false, true,    platforms)); // subfolder5 is expected to override subfolder6
        m_scanFolders.emplace_back(ScanFolderInfo(m_assetRootPath.filePath("subfolder6"), "", "sf6", false, true,    platforms)); // subfolder6 is expected to override subfolder7
        m_scanFolders.emplace_back(ScanFolderInfo(m_assetRootPath.filePath("subfolder7"), "", "sf7", false, true,    platforms)); // subfolder7 is expected to override subfolder8
        m_scanFolders.emplace_back(ScanFolderInfo(m_assetRootPath.filePath("subfolder8/x"), "", "sf8", false, true,    platforms)); // subfolder8 is expected to override root
        m_scanFolders.emplace_back(ScanFolderInfo(m_assetRootPath.absolutePath(),       "temp", "temp", true, false,    platforms)); // add the root

        m_scanFolders.emplace_back(ScanFolderInfo(m_assetRootPath.filePath("GameName"),  "gn",    "",   false, true, platforms));
        m_scanFolders.emplace_back(ScanFolderInfo(m_assetRootPath.filePath("GameNameButWithExtra"), "gnbwe", "", false, true, platforms));

        for (const ScanFolderInfo& scanFolder : m_scanFolders)
        {
            m_config.AddScanFolder(scanFolder, true);
        }

        AssetRecognizer txtRecognizer;
        txtRecognizer.m_name = "txt files";
        txtRecognizer.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
        txtRecognizer.m_platformSpecs.insert({"pc", AssetInternalSpec::Copy});
        txtRecognizer.m_platformSpecs.insert({"android", AssetInternalSpec::Copy});
        txtRecognizer.m_platformSpecs.insert({"fandago", AssetInternalSpec::Copy});
        m_txtRecognizerContainer[txtRecognizer.m_name] = txtRecognizer;

        // test dual-recognisers - two recognisers for the same pattern.
        txtRecognizer.m_name = "txt files 2";
        m_txtRecognizerContainer[txtRecognizer.m_name] = txtRecognizer;

        for (const auto& recognizerKeyPair : m_txtRecognizerContainer)
        {
            m_config.AddRecognizer(recognizerKeyPair.second);
        }

        m_formatRecognizer.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher(".*\\/test\\/.*\\.format", AssetBuilderSDK::AssetBuilderPattern::Regex);
        m_formatRecognizer.m_name = "format files that live in a folder called test";
        m_config.AddRecognizer(m_formatRecognizer);
    }

protected:
    void CreateTestFiles()
    {
        QSet<QString> expectedFiles;
        // set up some interesting files:
        expectedFiles << m_assetRootPath.absoluteFilePath("rootfile2.txt");
        expectedFiles << m_assetRootPath.absoluteFilePath("subfolder1/rootfile1.txt"); // note:  Must override the actual root file
        expectedFiles << m_assetRootPath.absoluteFilePath("subfolder1/basefile.txt");
        expectedFiles << m_assetRootPath.absoluteFilePath("subfolder2/basefile.txt");
        expectedFiles << m_assetRootPath.absoluteFilePath("subfolder2/aaa/basefile.txt");
        expectedFiles << m_assetRootPath.absoluteFilePath("subfolder2/aaa/bbb/basefile.txt");
        expectedFiles << m_assetRootPath.absoluteFilePath("subfolder2/aaa/bbb/ccc/basefile.txt");
        expectedFiles << m_assetRootPath.absoluteFilePath("subfolder2/aaa/bbb/ccc/ddd/basefile.txt");
        expectedFiles << m_assetRootPath.absoluteFilePath("subfolder2/subfolder1/override.txt");
        expectedFiles << m_assetRootPath.absoluteFilePath("subfolder3/basefile.txt");
        expectedFiles << m_assetRootPath.absoluteFilePath("subfolder4/a/testfile.txt");
        expectedFiles << m_assetRootPath.absoluteFilePath("subfolder5/a/testfile.txt");
        expectedFiles << m_assetRootPath.absoluteFilePath("subfolder6/a/testfile.txt");
        expectedFiles << m_assetRootPath.absoluteFilePath("subfolder7/a/testfile.txt");
        expectedFiles << m_assetRootPath.absoluteFilePath("subfolder8/x/a/testfile.txt");


        // subfolder3 is not recursive so none of these should show up in any scan or override check
        expectedFiles << m_assetRootPath.absoluteFilePath("subfolder3/aaa/basefile.txt");
        expectedFiles << m_assetRootPath.absoluteFilePath("subfolder3/aaa/bbb/basefile.txt");
        expectedFiles << m_assetRootPath.absoluteFilePath("subfolder3/aaa/bbb/ccc/basefile.txt");

        expectedFiles << m_assetRootPath.absoluteFilePath("subfolder3/rootfile3.txt"); // must override rootfile3 in root
        expectedFiles << m_assetRootPath.absoluteFilePath("rootfile1.txt");
        expectedFiles << m_assetRootPath.absoluteFilePath("rootfile3.txt");
        expectedFiles << m_assetRootPath.absoluteFilePath("unrecognised.file"); // a file that should not be recognised
        expectedFiles << m_assetRootPath.absoluteFilePath("unrecognised2.file"); // a file that should not be recognised
        expectedFiles << m_assetRootPath.absoluteFilePath("subfolder1/test/test.format"); // a file that should be recognised
        expectedFiles << m_assetRootPath.absoluteFilePath("test.format"); // a file that should NOT be recognised

        expectedFiles << m_assetRootPath.absoluteFilePath("GameNameButWithExtra/somefile.meo"); // a file that lives in a folder that must not be mistaken for the wrong scan folder
        expectedFiles << m_assetRootPath.absoluteFilePath("GameName/otherfile.meo"); // a file that lives in a folder that must not be mistaken for the wrong scan folder

        for (const QString& expect : expectedFiles)
        {
            EXPECT_TRUE(CreateDummyFile(expect));
        }
    }

    FileStatePassthrough m_fileStateCache;
    QDir m_assetRootPath;
    PlatformConfiguration m_config;
    AZStd::vector<ScanFolderInfo> m_scanFolders;
    AZStd::vector<AssetBuilderSDK::PlatformInfo> m_enabledPlatforms = {
        { "pc",{ "desktop", "host" }},
        { "android",{ "mobile", "android" }}
    };
    RecognizerContainer m_txtRecognizerContainer;
    AssetRecognizer m_formatRecognizer;
};

TEST_F(PlatformConfigurationTests, TestPlatformsAndScanFolders_FeedPlatformConfiguration_Succeeds)
{
    EXPECT_EQ(m_config.GetEnabledPlatforms().size(), m_enabledPlatforms.size());
    for (int index = 0; index < m_enabledPlatforms.size(); ++index)
    {
        EXPECT_EQ(m_config.GetEnabledPlatforms()[index].m_identifier, m_enabledPlatforms[index].m_identifier);
    }

    EXPECT_EQ(m_config.GetScanFolderCount(), m_scanFolders.size());
    for (int index = 0; index < m_scanFolders.size(); ++index)
    {
        EXPECT_EQ(m_config.GetScanFolderAt(index).IsRoot(), m_scanFolders[index].IsRoot());
        EXPECT_EQ(m_config.GetScanFolderAt(index).RecurseSubFolders(), m_scanFolders[index].RecurseSubFolders());
    }
}

TEST_F(PlatformConfigurationTests, TestRecogonizer_FeedPlatformConfiguration_Succeeds)
{
    CreateTestFiles();

    RecognizerPointerContainer results;
    EXPECT_TRUE(m_config.GetMatchingRecognizers(m_assetRootPath.absoluteFilePath("subfolder1/rootfile1.txt"), results));
    EXPECT_EQ(results.size(), m_txtRecognizerContainer.size());
    for (int index = 0; index < results.size(); ++index)
    {
        EXPECT_TRUE(results[index]);
        EXPECT_NE(m_txtRecognizerContainer.find(results[index]->m_name), m_txtRecognizerContainer.end());
    }

    results.clear();
    EXPECT_FALSE(m_config.GetMatchingRecognizers(m_assetRootPath.absoluteFilePath("test.format"), results));
    EXPECT_TRUE(m_config.GetMatchingRecognizers(m_assetRootPath.absoluteFilePath("subfolder1/test/test.format"), results));
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0]->m_name, m_formatRecognizer.m_name);

    // double call:
    EXPECT_FALSE(m_config.GetMatchingRecognizers(m_assetRootPath.absoluteFilePath("unrecognised.file"), results));
    EXPECT_FALSE(m_config.GetMatchingRecognizers(m_assetRootPath.absoluteFilePath("unrecognised.file"), results));

    // files which do and dont exist:
    EXPECT_FALSE(m_config.GetMatchingRecognizers(m_assetRootPath.absoluteFilePath("unrecognised2.file"), results));
    EXPECT_FALSE(m_config.GetMatchingRecognizers(m_assetRootPath.absoluteFilePath("unrecognised3.file"), results));
}

TEST_F(PlatformConfigurationTests, GetOverridingFile_FeedPlatformConfiguration_Succeeds)
{
    CreateTestFiles();

    EXPECT_EQ(m_config.GetOverridingFile("rootfile3.txt", m_assetRootPath.filePath("subfolder3")), QString());
    EXPECT_EQ(m_config.GetOverridingFile("rootfile3.txt", m_assetRootPath.absolutePath()), m_assetRootPath.absoluteFilePath("subfolder3/rootfile3.txt"));
    EXPECT_EQ(m_config.GetOverridingFile("subfolder1/whatever.txt", m_assetRootPath.filePath("subfolder1")), QString());
    EXPECT_EQ(AssetUtilities::NormalizeFilePath(m_config.GetOverridingFile("subfolder1/override.txt", m_assetRootPath.filePath("subfolder1"))), AssetUtilities::NormalizeFilePath(m_assetRootPath.absoluteFilePath("subfolder2/subfolder1/override.txt")));
    EXPECT_EQ(AssetUtilities::NormalizeFilePath(m_config.GetOverridingFile("a/testfile.txt", m_assetRootPath.filePath("subfolder6"))), AssetUtilities::NormalizeFilePath(m_assetRootPath.absoluteFilePath("subfolder4/a/testfile.txt")));
    EXPECT_EQ(AssetUtilities::NormalizeFilePath(m_config.GetOverridingFile("a/testfile.txt", m_assetRootPath.filePath("subfolder7"))), AssetUtilities::NormalizeFilePath(m_assetRootPath.absoluteFilePath("subfolder4/a/testfile.txt")));
    EXPECT_EQ(AssetUtilities::NormalizeFilePath(m_config.GetOverridingFile("a/testfile.txt", m_assetRootPath.filePath("subfolder8/x"))), AssetUtilities::NormalizeFilePath(m_assetRootPath.absoluteFilePath("subfolder4/a/testfile.txt")));

    // files which dont exist:
    EXPECT_EQ(m_config.GetOverridingFile("rootfile3", m_assetRootPath.filePath("subfolder3")), QString());

    // watch folders which dont exist should still return the best match:
    EXPECT_NE(m_config.GetOverridingFile("rootfile3.txt", m_assetRootPath.filePath("nonesuch")), QString());

    // subfolder 3 is first, but non-recursive, so it should NOT resolve this:
    EXPECT_EQ(m_config.GetOverridingFile("aaa/bbb/basefile.txt", m_assetRootPath.filePath("subfolder2")), QString());
}

TEST_F(PlatformConfigurationTests, FindFirstMatchingFile_FeedPlatformConfiguration_Succeeds)
{
    CreateTestFiles();

    // sanity
    EXPECT_TRUE(m_config.FindFirstMatchingFile("").isEmpty());  // empty should return empty.

    // must not find the one in subfolder3 because its not a recursive watch:
    EXPECT_EQ(m_config.FindFirstMatchingFile("aaa/bbb/basefile.txt"), m_assetRootPath.filePath("subfolder2/aaa/bbb/basefile.txt"));

    // however, stuff at the root is overridden:
    EXPECT_EQ(m_config.FindFirstMatchingFile("rootfile3.txt"), m_assetRootPath.filePath("subfolder3/rootfile3.txt"));

    // not allowed to find files which do not exist:
    EXPECT_EQ(m_config.FindFirstMatchingFile("asdasdsa.txt"), QString());

    // find things in the root folder, too
    EXPECT_EQ(m_config.FindFirstMatchingFile("rootfile2.txt"), m_assetRootPath.filePath("rootfile2.txt"));

    // different regex rule should not interfere
    EXPECT_EQ(m_config.FindFirstMatchingFile("test/test.format"), m_assetRootPath.filePath("subfolder1/test/test.format"));

    EXPECT_EQ(m_config.FindFirstMatchingFile("a/testfile.txt"), m_assetRootPath.filePath("subfolder4/a/testfile.txt"));
}

TEST_F(PlatformConfigurationTests, GetScanFolderForFile_FeedPlatformConfiguration_Succeeds)
{
    CreateTestFiles();
    // other functions depend on this one, test it first:
    EXPECT_TRUE(m_config.GetScanFolderForFile(m_assetRootPath.filePath("rootfile3.txt")));
    EXPECT_EQ(m_config.GetScanFolderForFile(m_assetRootPath.filePath("subfolder3/rootfile3.txt"))->ScanPath(), m_assetRootPath.filePath("subfolder3"));

    // this file exists and is in subfolder3, but subfolder3 is non-recursive, so it must not find it:
    EXPECT_FALSE(m_config.GetScanFolderForFile(m_assetRootPath.filePath("subfolder3/aaa/bbb/basefile.txt")));

    // test of root files in actual root folder:
    EXPECT_TRUE(m_config.GetScanFolderForFile(m_assetRootPath.filePath("rootfile2.txt")));
    EXPECT_EQ(m_config.GetScanFolderForFile(m_assetRootPath.filePath("rootfile2.txt"))->ScanPath(), m_assetRootPath.absolutePath());
}

TEST_F(PlatformConfigurationTests, ConvertToRelativePath_FeedPlatformConfiguration_Succeeds)
{
    CreateTestFiles();

    QString fileName;
    QString scanFolderPath;

    // scan folders themselves should still convert to relative paths.
    EXPECT_TRUE(m_config.ConvertToRelativePath(m_assetRootPath.absolutePath(), fileName, scanFolderPath));
    EXPECT_EQ(fileName, "");
    EXPECT_EQ(scanFolderPath, m_assetRootPath.absolutePath());

    // a root file that actually exists in a root folder:
    EXPECT_TRUE(m_config.ConvertToRelativePath(m_assetRootPath.absoluteFilePath("rootfile2.txt"), fileName, scanFolderPath));
    EXPECT_EQ(fileName, "rootfile2.txt");
    EXPECT_EQ(scanFolderPath, m_assetRootPath.absolutePath());

    // find overridden file from root that is overridden in a higher priority folder:
    EXPECT_TRUE(m_config.ConvertToRelativePath(m_assetRootPath.absoluteFilePath("subfolder3/rootfile3.txt"), fileName, scanFolderPath));
    EXPECT_EQ(fileName, "rootfile3.txt");
    EXPECT_EQ(scanFolderPath, m_assetRootPath.filePath("subfolder3"));

    // must not find this, since its in a non-recursive folder:
    EXPECT_FALSE(m_config.ConvertToRelativePath(m_assetRootPath.absoluteFilePath("subfolder3/aaa/basefile.txt"), fileName, scanFolderPath));

    // must not find this since its not even in any folder we care about:
    EXPECT_FALSE(m_config.ConvertToRelativePath(m_assetRootPath.absoluteFilePath("subfolder8/aaa/basefile.txt"), fileName, scanFolderPath));

    // deep folder:
    EXPECT_TRUE(m_config.ConvertToRelativePath(m_assetRootPath.absoluteFilePath("subfolder2/aaa/bbb/ccc/ddd/basefile.txt"), fileName, scanFolderPath));
    EXPECT_EQ(fileName, "aaa/bbb/ccc/ddd/basefile.txt");
    EXPECT_EQ(scanFolderPath, m_assetRootPath.filePath("subfolder2"));

    // verify that output relative paths
    EXPECT_TRUE(m_config.ConvertToRelativePath(m_assetRootPath.absoluteFilePath("subfolder1/whatever.txt"), fileName, scanFolderPath));
    EXPECT_EQ(fileName, "whatever.txt");
    EXPECT_EQ(scanFolderPath, m_assetRootPath.filePath("subfolder1"));
}

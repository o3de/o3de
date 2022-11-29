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

        m_config.EnablePlatform({ "pc",{ "desktop", "host" } }, true);
        m_config.EnablePlatform({ "android",{ "mobile", "android" } }, true);
        m_config.EnablePlatform({ "fandago",{ "console" } }, false);
        AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms;
        m_config.PopulatePlatformsForScanFolder(platforms);

        //                                         PATH               DisplayName  PortKey root   recurse  platforms,      isunittesting
        m_config.AddScanFolder(ScanFolderInfo(m_assetRootPath.filePath("subfolder3"),   "", "sf3",  false, false,   platforms), true); // subfolder 3 overrides subfolder2
        m_config.AddScanFolder(ScanFolderInfo(m_assetRootPath.filePath("subfolder2"),   "", "sf4",  false, true,    platforms), true); // subfolder 2 overrides subfolder1
        m_config.AddScanFolder(ScanFolderInfo(m_assetRootPath.filePath("subfolder1"),   "", "sf1",  false, true,    platforms), true); // subfolder1 overrides root
        m_config.AddScanFolder(ScanFolderInfo(m_assetRootPath.filePath("subfolder4"),   "", "sf4",  false, true,    platforms), true); // subfolder4 overrides subfolder5
        m_config.AddScanFolder(ScanFolderInfo(m_assetRootPath.filePath("subfolder5"),   "", "sf5",  false, true,    platforms), true); // subfolder5 overrides subfolder6
        m_config.AddScanFolder(ScanFolderInfo(m_assetRootPath.filePath("subfolder6"), "", "sf6", false, true,    platforms), true); // subfolder6 overrides subfolder7
        m_config.AddScanFolder(ScanFolderInfo(m_assetRootPath.filePath("subfolder7"), "", "sf7", false, true,    platforms), true); // subfolder7 overrides subfolder8
        m_config.AddScanFolder(ScanFolderInfo(m_assetRootPath.filePath("subfolder8/x"), "", "sf8", false, true,    platforms), true); // subfolder8 overrides root
        m_config.AddScanFolder(ScanFolderInfo(m_assetRootPath.absolutePath(),       "temp", "temp", true, false,    platforms), true); // add the root

                                                                                                                                     // these are checked for later.
        m_config.AddScanFolder(ScanFolderInfo(m_assetRootPath.filePath("GameName"),             "gn",    "", false, true, platforms), true);
        m_config.AddScanFolder(ScanFolderInfo(m_assetRootPath.filePath("GameNameButWithExtra"), "gnbwe", "", false, true, platforms), true);

        AssetRecognizer rec;

        rec.m_name = "txt files";
        rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
        rec.m_platformSpecs.insert({"pc", AssetInternalSpec::Copy});
        rec.m_platformSpecs.insert({"android", AssetInternalSpec::Copy});
        rec.m_platformSpecs.insert({"fandago", AssetInternalSpec::Copy});
        m_config.AddRecognizer(rec);

        // test dual-recognisers - two recognisers for the same pattern.
        rec.m_name = "txt files 2";
        m_config.AddRecognizer(rec);
        rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher(".*\\/test\\/.*\\.format", AssetBuilderSDK::AssetBuilderPattern::Regex);
        rec.m_name = "format files that live in a folder called test";
        m_config.AddRecognizer(rec);
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
};

TEST_F(PlatformConfigurationTests, TestPlatformsAndScanFolders_FeedPlatformConfiguration_Succeeds)
{
    EXPECT_EQ(m_config.GetEnabledPlatforms().size(), 2);
    EXPECT_EQ(m_config.GetEnabledPlatforms()[0].m_identifier, "pc");
    EXPECT_EQ(m_config.GetEnabledPlatforms()[1].m_identifier, "android");

    EXPECT_EQ(m_config.GetScanFolderCount(), 11);
    EXPECT_FALSE(m_config.GetScanFolderAt(0).IsRoot());
    EXPECT_TRUE(m_config.GetScanFolderAt(8).IsRoot());
    EXPECT_FALSE(m_config.GetScanFolderAt(0).RecurseSubFolders());
    EXPECT_TRUE(m_config.GetScanFolderAt(1).RecurseSubFolders());
    EXPECT_TRUE(m_config.GetScanFolderAt(2).RecurseSubFolders());
    EXPECT_FALSE(m_config.GetScanFolderAt(8).RecurseSubFolders());
}

TEST_F(PlatformConfigurationTests, TestRecogonizer_FeedPlatformConfiguration_Succeeds)
{
    CreateTestFiles();

    RecognizerPointerContainer results;
    EXPECT_TRUE(m_config.GetMatchingRecognizers(m_assetRootPath.absoluteFilePath("subfolder1/rootfile1.txt"), results));
    EXPECT_EQ(results.size(), 2);
    EXPECT_TRUE(results[0]);
    EXPECT_TRUE(results[1]);
    EXPECT_NE(results[0]->m_name, results[1]->m_name);
    EXPECT_TRUE(results[0]->m_name == "txt files" || results[1]->m_name == "txt files");
    EXPECT_TRUE(results[0]->m_name == "txt files 2" || results[1]->m_name == "txt files 2");

    results.clear();
    EXPECT_FALSE(m_config.GetMatchingRecognizers(m_assetRootPath.absoluteFilePath("test.format"), results));
    EXPECT_TRUE(m_config.GetMatchingRecognizers(m_assetRootPath.absoluteFilePath("subfolder1/test/test.format"), results));
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0]->m_name, "format files that live in a folder called test");

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

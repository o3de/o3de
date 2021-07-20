/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "PlatformConfigurationUnitTests.h"
#include "native/utilities/PlatformConfiguration.h"

using namespace UnitTestUtils;
using namespace AssetProcessor;

void PlatformConfigurationTests::StartTest()
{
    {
        // put everything in a scope so that it dies.
        QTemporaryDir tempEngineRoot;
        QDir tempPath(tempEngineRoot.path());

        QSet<QString> expectedFiles;
        // set up some interesting files:
        expectedFiles << tempPath.absoluteFilePath("rootfile2.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder1/rootfile1.txt"); // note:  Must override the actual root file
        expectedFiles << tempPath.absoluteFilePath("subfolder1/basefile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder2/basefile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder2/aaa/basefile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder2/aaa/bbb/basefile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder2/aaa/bbb/ccc/basefile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder2/aaa/bbb/ccc/ddd/basefile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder2/subfolder1/override.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder3/basefile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder4/a/testfile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder5/a/testfile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder6/a/testfile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder7/a/testfile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder8/x/a/testfile.txt");


        // subfolder3 is not recursive so none of these should show up in any scan or override check
        expectedFiles << tempPath.absoluteFilePath("subfolder3/aaa/basefile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder3/aaa/bbb/basefile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder3/aaa/bbb/ccc/basefile.txt");

        expectedFiles << tempPath.absoluteFilePath("subfolder3/rootfile3.txt"); // must override rootfile3 in root
        expectedFiles << tempPath.absoluteFilePath("rootfile1.txt");
        expectedFiles << tempPath.absoluteFilePath("rootfile3.txt");
        expectedFiles << tempPath.absoluteFilePath("unrecognised.file"); // a file that should not be recognised
        expectedFiles << tempPath.absoluteFilePath("unrecognised2.file"); // a file that should not be recognised
        expectedFiles << tempPath.absoluteFilePath("subfolder1/test/test.format"); // a file that should be recognised
        expectedFiles << tempPath.absoluteFilePath("test.format"); // a file that should NOT be recognised

        expectedFiles << tempPath.absoluteFilePath("GameNameButWithExtra/somefile.meo"); // a file that lives in a folder that must not be mistaken for the wrong scan folder
        expectedFiles << tempPath.absoluteFilePath("GameName/otherfile.meo"); // a file that lives in a folder that must not be mistaken for the wrong scan folder

        for (const QString& expect : expectedFiles)
        {
            UNIT_TEST_EXPECT_TRUE(CreateDummyFile(expect));
        }

        PlatformConfiguration config;
        config.EnablePlatform({ "pc",{ "desktop", "host" } }, true);
        config.EnablePlatform({ "android",{ "mobile", "android" } }, true);
        config.EnablePlatform({ "fandago",{ "console" } }, false);
        AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms;
        config.PopulatePlatformsForScanFolder(platforms);

        //                                         PATH               DisplayName  PortKey root   recurse  platforms,      isunittesting
        config.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder3"),   "", "sf3",  false, false,   platforms), true); // subfolder 3 overrides subfolder2
        config.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder2"),   "", "sf4",  false, true,    platforms), true); // subfolder 2 overrides subfolder1
        config.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder1"),   "", "sf1",  false, true,    platforms), true); // subfolder1 overrides root
        config.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder4"),   "", "sf4",  false, true,    platforms), true); // subfolder4 overrides subfolder5
        config.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder5"),   "", "sf5",  false, true,    platforms), true); // subfolder5 overrides subfolder6
        config.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder6"), "", "sf6", false, true,    platforms), true); // subfolder6 overrides subfolder7
        config.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder7"), "", "sf7", false, true,    platforms), true); // subfolder7 overrides subfolder8
        config.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder8/x"), "", "sf8", false, true,    platforms), true); // subfolder8 overrides root
        config.AddScanFolder(ScanFolderInfo(tempPath.absolutePath(),       "temp", "temp", true, false,    platforms), true); // add the root

        // these are checked for later.
        config.AddScanFolder(ScanFolderInfo(tempPath.filePath("GameName"),             "gn",    "", false, true, platforms), true);
        config.AddScanFolder(ScanFolderInfo(tempPath.filePath("GameNameButWithExtra"), "gnbwe", "", false, true, platforms), true);



        AssetRecognizer rec;
        AssetPlatformSpec specpc;
        AssetPlatformSpec specandroid;
        AssetPlatformSpec specfandago; 
        specpc.m_extraRCParams = ""; // blank must work
        specandroid.m_extraRCParams = "testextraparams";

        rec.m_name = "txt files";
        rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
        rec.m_platformSpecs.insert("pc", specpc);
        rec.m_platformSpecs.insert("android", specandroid);
        rec.m_platformSpecs.insert("fandago", specfandago); 
        config.AddRecognizer(rec);

        // test dual-recognisers - two recognisers for the same pattern.
        rec.m_name = "txt files 2";
        config.AddRecognizer(rec);
        rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher(".*\\/test\\/.*\\.format", AssetBuilderSDK::AssetBuilderPattern::Regex);
        rec.m_name = "format files that live in a folder called test";
        config.AddRecognizer(rec);

        // --------------------------- SCAN FOLDER TEST ------------------------

        UNIT_TEST_EXPECT_TRUE(config.GetEnabledPlatforms().size() == 2);
        UNIT_TEST_EXPECT_TRUE(config.GetEnabledPlatforms()[0].m_identifier == "pc");
        UNIT_TEST_EXPECT_TRUE(config.GetEnabledPlatforms()[1].m_identifier == "android");

        UNIT_TEST_EXPECT_TRUE(config.GetScanFolderCount() == 11);
        UNIT_TEST_EXPECT_FALSE(config.GetScanFolderAt(0).IsRoot());
        UNIT_TEST_EXPECT_TRUE(config.GetScanFolderAt(8).IsRoot());
        UNIT_TEST_EXPECT_FALSE(config.GetScanFolderAt(0).RecurseSubFolders());
        UNIT_TEST_EXPECT_TRUE(config.GetScanFolderAt(1).RecurseSubFolders());
        UNIT_TEST_EXPECT_TRUE(config.GetScanFolderAt(2).RecurseSubFolders());
        UNIT_TEST_EXPECT_FALSE(config.GetScanFolderAt(8).RecurseSubFolders());

        // --------------------------- RECOGNIZER TEST ------------------------

        RecognizerPointerContainer results;
        UNIT_TEST_EXPECT_TRUE(config.GetMatchingRecognizers(tempPath.absoluteFilePath("subfolder1/rootfile1.txt"), results));
        UNIT_TEST_EXPECT_TRUE(results.size() == 2);
        UNIT_TEST_EXPECT_TRUE(results[0]);
        UNIT_TEST_EXPECT_TRUE(results[1]);
        UNIT_TEST_EXPECT_TRUE(results[0]->m_name != results[1]->m_name);
        UNIT_TEST_EXPECT_TRUE(results[0]->m_name == "txt files" || results[1]->m_name == "txt files");
        UNIT_TEST_EXPECT_TRUE(results[0]->m_name == "txt files 2" || results[1]->m_name == "txt files 2");

        results.clear();
        UNIT_TEST_EXPECT_FALSE(config.GetMatchingRecognizers(tempPath.absoluteFilePath("test.format"), results));
        UNIT_TEST_EXPECT_TRUE(config.GetMatchingRecognizers(tempPath.absoluteFilePath("subfolder1/test/test.format"), results));
        UNIT_TEST_EXPECT_TRUE(results.size() == 1);
        UNIT_TEST_EXPECT_TRUE(results[0]->m_name == "format files that live in a folder called test");

        // double call:
        UNIT_TEST_EXPECT_FALSE(config.GetMatchingRecognizers(tempPath.absoluteFilePath("unrecognised.file"), results));
        UNIT_TEST_EXPECT_FALSE(config.GetMatchingRecognizers(tempPath.absoluteFilePath("unrecognised.file"), results));

        // files which do and dont exist:
        UNIT_TEST_EXPECT_FALSE(config.GetMatchingRecognizers(tempPath.absoluteFilePath("unrecognised2.file"), results));
        UNIT_TEST_EXPECT_FALSE(config.GetMatchingRecognizers(tempPath.absoluteFilePath("unrecognised3.file"), results));

        // --------------------- GetOverridingFile --------------------------
        UNIT_TEST_EXPECT_TRUE(config.GetOverridingFile("rootfile3.txt", tempPath.filePath("subfolder3")) == QString());
        UNIT_TEST_EXPECT_TRUE(config.GetOverridingFile("rootfile3.txt", tempPath.absolutePath()) == tempPath.absoluteFilePath("subfolder3/rootfile3.txt"));
        UNIT_TEST_EXPECT_TRUE(config.GetOverridingFile("subfolder1/whatever.txt", tempPath.filePath("subfolder1")) == QString());
        UNIT_TEST_EXPECT_TRUE(AssetUtilities::NormalizeFilePath(config.GetOverridingFile("subfolder1/override.txt", tempPath.filePath("subfolder1"))) == AssetUtilities::NormalizeFilePath(tempPath.absoluteFilePath("subfolder2/subfolder1/override.txt")));
        UNIT_TEST_EXPECT_TRUE(AssetUtilities::NormalizeFilePath(config.GetOverridingFile("a/testfile.txt", tempPath.filePath("subfolder6"))) == AssetUtilities::NormalizeFilePath(tempPath.absoluteFilePath("subfolder4/a/testfile.txt")));
        UNIT_TEST_EXPECT_TRUE(AssetUtilities::NormalizeFilePath(config.GetOverridingFile("a/testfile.txt", tempPath.filePath("subfolder7"))) == AssetUtilities::NormalizeFilePath(tempPath.absoluteFilePath("subfolder4/a/testfile.txt")));
        UNIT_TEST_EXPECT_TRUE(AssetUtilities::NormalizeFilePath(config.GetOverridingFile("a/testfile.txt", tempPath.filePath("subfolder8/x"))) == AssetUtilities::NormalizeFilePath(tempPath.absoluteFilePath("subfolder4/a/testfile.txt")));

        // files which dont exist:
        UNIT_TEST_EXPECT_TRUE(config.GetOverridingFile("rootfile3", tempPath.filePath("subfolder3")) == QString());

        // watch folders which dont exist should still return the best match:
        UNIT_TEST_EXPECT_TRUE(config.GetOverridingFile("rootfile3.txt", tempPath.filePath("nonesuch")) != QString());

        // subfolder 3 is first, but non-recursive, so it should NOT resolve this:
        UNIT_TEST_EXPECT_TRUE(config.GetOverridingFile("aaa/bbb/basefile.txt", tempPath.filePath("subfolder2")) == QString());

        // ------------------------ FindFirstMatchingFile --------------------------
        // sanity
        UNIT_TEST_EXPECT_TRUE(config.FindFirstMatchingFile("").isEmpty());  // empty should return empty.

        // must not find the one in subfolder3 because its not a recursive watch:
        UNIT_TEST_EXPECT_TRUE(config.FindFirstMatchingFile("aaa/bbb/basefile.txt") == tempPath.filePath("subfolder2/aaa/bbb/basefile.txt"));

        // however, stuff at the root is overridden:
        UNIT_TEST_EXPECT_TRUE(config.FindFirstMatchingFile("rootfile3.txt") == tempPath.filePath("subfolder3/rootfile3.txt"));

        // not allowed to find files which do not exist:
        UNIT_TEST_EXPECT_TRUE(config.FindFirstMatchingFile("asdasdsa.txt") == QString());

        // find things in the root folder, too
        UNIT_TEST_EXPECT_TRUE(config.FindFirstMatchingFile("rootfile2.txt") == tempPath.filePath("rootfile2.txt"));

        // different regex rule should not interfere
        UNIT_TEST_EXPECT_TRUE(config.FindFirstMatchingFile("test/test.format") == tempPath.filePath("subfolder1/test/test.format"));

        UNIT_TEST_EXPECT_TRUE(config.FindFirstMatchingFile("a/testfile.txt") == tempPath.filePath("subfolder4/a/testfile.txt"));

        // ---------------------------- GetScanFolderForFile -----------------
        // other functions depend on this one, test it first:
        UNIT_TEST_EXPECT_TRUE(config.GetScanFolderForFile(tempPath.filePath("rootfile3.txt")));
        UNIT_TEST_EXPECT_TRUE(config.GetScanFolderForFile(tempPath.filePath("subfolder3/rootfile3.txt"))->ScanPath() == tempPath.filePath("subfolder3"));

        // this file exists and is in subfolder3, but subfolder3 is non-recursive, so it must not find it:
        UNIT_TEST_EXPECT_FALSE(config.GetScanFolderForFile(tempPath.filePath("subfolder3/aaa/bbb/basefile.txt")));

        // test of root files in actual root folder:
        UNIT_TEST_EXPECT_TRUE(config.GetScanFolderForFile(tempPath.filePath("rootfile2.txt")));
        UNIT_TEST_EXPECT_TRUE(config.GetScanFolderForFile(tempPath.filePath("rootfile2.txt"))->ScanPath() == tempPath.absolutePath());

        // -------------------------- ConvertToRelativePath  ------------------------------

        QString fileName;
        QString scanFolderPath;

        // scan folders themselves should still convert to relative paths.
        UNIT_TEST_EXPECT_TRUE(config.ConvertToRelativePath(tempPath.absolutePath(), fileName, scanFolderPath));
        UNIT_TEST_EXPECT_TRUE(fileName == "");
        UNIT_TEST_EXPECT_TRUE(scanFolderPath == tempPath.absolutePath());

        // a root file that actually exists in a root folder:
        UNIT_TEST_EXPECT_TRUE(config.ConvertToRelativePath(tempPath.absoluteFilePath("rootfile2.txt"), fileName, scanFolderPath));
        UNIT_TEST_EXPECT_TRUE(fileName == "rootfile2.txt");
        UNIT_TEST_EXPECT_TRUE(scanFolderPath == tempPath.absolutePath());

        // find overridden file from root that is overridden in a higher priority folder:
        UNIT_TEST_EXPECT_TRUE(config.ConvertToRelativePath(tempPath.absoluteFilePath("subfolder3/rootfile3.txt"), fileName, scanFolderPath));
        UNIT_TEST_EXPECT_TRUE(fileName == "rootfile3.txt");
        UNIT_TEST_EXPECT_TRUE(scanFolderPath == tempPath.filePath("subfolder3"));

        // must not find this, since its in a non-recursive folder:
        UNIT_TEST_EXPECT_FALSE(config.ConvertToRelativePath(tempPath.absoluteFilePath("subfolder3/aaa/basefile.txt"), fileName, scanFolderPath));

        // must not find this since its not even in any folder we care about:
        UNIT_TEST_EXPECT_FALSE(config.ConvertToRelativePath(tempPath.absoluteFilePath("subfolder8/aaa/basefile.txt"), fileName, scanFolderPath));

        // deep folder:
        UNIT_TEST_EXPECT_TRUE(config.ConvertToRelativePath(tempPath.absoluteFilePath("subfolder2/aaa/bbb/ccc/ddd/basefile.txt"), fileName, scanFolderPath));
        UNIT_TEST_EXPECT_TRUE(fileName == "aaa/bbb/ccc/ddd/basefile.txt");
        UNIT_TEST_EXPECT_TRUE(scanFolderPath == tempPath.filePath("subfolder2"));

        // verify that output relative paths
        UNIT_TEST_EXPECT_TRUE(config.ConvertToRelativePath(tempPath.absoluteFilePath("subfolder1/whatever.txt"), fileName, scanFolderPath));
        UNIT_TEST_EXPECT_TRUE(fileName == "whatever.txt");
        UNIT_TEST_EXPECT_TRUE(scanFolderPath == tempPath.filePath("subfolder1"));
    }

    Q_EMIT UnitTestPassed();
}

REGISTER_UNIT_TEST(PlatformConfigurationTests)


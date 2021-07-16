/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AssetScannerUnitTests.h"
#include "native/AssetManager/assetScanner.h"
#include "native/utilities/PlatformConfiguration.h"
#include <QCoreApplication>
#include <QElapsedTimer>


using namespace UnitTestUtils;
using namespace AssetProcessor;

void AssetScannerUnitTest::StartTest()
{
    QTemporaryDir tempEngineRoot;
    QDir tempPath(tempEngineRoot.path());

    QSet<QString> expectedFiles;
    // set up some interesting files:
    expectedFiles << tempPath.absoluteFilePath("rootfile2.txt");
    expectedFiles << tempPath.absoluteFilePath("subfolder1/basefile.txt");
    expectedFiles << tempPath.absoluteFilePath("subfolder2/basefile.txt");
    expectedFiles << tempPath.absoluteFilePath("subfolder2/aaa/basefile.txt");
    expectedFiles << tempPath.absoluteFilePath("subfolder2/aaa/bbb/basefile.txt");
    expectedFiles << tempPath.absoluteFilePath("subfolder2/aaa/bbb/ccc/basefile.txt");
    expectedFiles << tempPath.absoluteFilePath("subfolder2/aaa/bbb/ccc/ddd/basefile.txt");
    expectedFiles << tempPath.absoluteFilePath("subfolder2/aaa/bbb/ccc/ddd/eee.fff.ggg/basefile.txt");//adding a folder name containing dots
    expectedFiles << tempPath.absoluteFilePath("subfolder2/aaa/bbb/ccc/ddd/eee.fff.ggg/basefile1.txt");
    expectedFiles << tempPath.absoluteFilePath("subfolder3/basefile.txt");
    expectedFiles << tempPath.absoluteFilePath("subfolder3/aaa/basefile.txt");
    expectedFiles << tempPath.absoluteFilePath("subfolder3/aaa/bbb/basefile.txt");
    expectedFiles << tempPath.absoluteFilePath("subfolder3/aaa/bbb/ccc/basefile.txt");
    expectedFiles << tempPath.absoluteFilePath("rootfile1.txt");

    for (const QString& expect : expectedFiles)
    {
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(expect));
    }

    // but we're going to not watch subfolder3 recursively, so... remove these files (even though they're on disk)
    // if this causes a failure it means that its ignoring the "do not recurse" flag:
    expectedFiles.remove(tempPath.absoluteFilePath("subfolder3/aaa/basefile.txt"));
    expectedFiles.remove(tempPath.absoluteFilePath("subfolder3/aaa/bbb/basefile.txt"));
    expectedFiles.remove(tempPath.absoluteFilePath("subfolder3/aaa/bbb/ccc/basefile.txt"));

    QSet<QString> expectedFolders;
    expectedFolders << tempPath.absoluteFilePath("subfolder2/aaa");
    expectedFolders << tempPath.absoluteFilePath("subfolder2/aaa/bbb");
    expectedFolders << tempPath.absoluteFilePath("subfolder2/aaa/bbb/ccc");
    expectedFolders << tempPath.absoluteFilePath("subfolder2/aaa/bbb/ccc/ddd");
    expectedFolders << tempPath.absoluteFilePath("subfolder2/aaa/bbb/ccc/ddd/eee.fff.ggg");


    PlatformConfiguration config;
    AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms;
    config.PopulatePlatformsForScanFolder(platforms);
    //                                               PATH               DisplayName  PortKey outputfolder  root recurse  platforms
    config.AddScanFolder(ScanFolderInfo(tempPath.absolutePath(),         "temp",       "ap1",    true,  false, platforms));  // note:  "Recurse" set to false.
    config.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder1"), "",           "ap2",   false,  true,  platforms));
    config.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder2"), "",           "ap3",   false,  true,  platforms));
    config.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder3"), "",           "ap4",   false,  false, platforms)); // note:  "Recurse" set to false.
    AssetScanner scanner(&config);

    QList<AssetFileInfo> filesFound;
    QList<AssetFileInfo> foldersFound;

    bool doneScan = false;

    connect(&scanner, &AssetScanner::FilesFound, this, [&filesFound](QSet<AssetFileInfo> fileList)
        {
            for (AssetFileInfo foundFile : fileList)
            {
                filesFound.push_back(foundFile);
            }
        }
        );

    connect(&scanner, &AssetScanner::FoldersFound, this, [&foldersFound](QSet<AssetFileInfo> folderList)
    {
        for (AssetFileInfo foundFolder : folderList)
        {
            foldersFound.push_back(foundFolder);
        }
    }
    );

    connect(&scanner, &AssetScanner::AssetScanningStatusChanged, this, [&doneScan](AssetProcessor::AssetScanningStatus status)
        {
            if ((status == AssetProcessor::AssetScanningStatus::Completed) || (status == AssetProcessor::AssetScanningStatus::Stopped))
            {
                doneScan = true;
            }
        }
        );

    // this test makes sure that no files that should be missed were actually missed.
    // it makes sure that if a folder is added recursively, child files and folders are found
    // it makes sure that if a folder is added NON-recursively, child folder files are not found.

    scanner.StartScan();
    QElapsedTimer nowTime;
    nowTime.start();
    while (!doneScan)
    {
        QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents, 100);

        if (nowTime.elapsed() > 10000)
        {
            break;
        }
    }

    UNIT_TEST_EXPECT_TRUE(doneScan);
    UNIT_TEST_EXPECT_TRUE(filesFound.count() == expectedFiles.count());

    for (const AssetFileInfo& file : filesFound)
    {
        UNIT_TEST_EXPECT_TRUE(expectedFiles.find(file.m_filePath) != expectedFiles.end());
    }

    for (const AssetFileInfo& folder : foldersFound)
    {
        UNIT_TEST_EXPECT_TRUE(expectedFolders.find(folder.m_filePath) != expectedFolders.end());
    }

    Q_EMIT UnitTestPassed();
}


REGISTER_UNIT_TEST(AssetScannerUnitTest)

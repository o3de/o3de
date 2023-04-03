/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AssetScannerUnitTests.h"
#include <AzCore/std/chrono/chrono.h>
#include <AzTest/Utils.h>
#include <native/AssetManager/assetScanner.h>
#include <native/utilities/PlatformConfiguration.h>
#include <native/unittests/UnitTestUtils.h> // for CreateDummyFile
#include <QApplication>
#include <QElapsedTimer>
#include <QTemporaryDir>

namespace UnitTest
{
    TEST_F(AssetScannerUnitTest, AssetScanner_ScanMultipleFolders_ExpectedFilesAndFoldersFound)
    {
        using namespace AssetProcessor;
        AZStd::unique_ptr<QCoreApplication> m_qApp;

        // Qt documentation claims QCoreApplication's constructor requires a greater than zero argC
        // and a valid argV, however the version of Qt in use works fine with 0 and nullptr.
        int argC = 0;
        m_qApp.reset(new QApplication(argC, nullptr));

        qRegisterMetaType<AssetProcessor::AssetScanningStatus>("AssetScanningStatus");
        qRegisterMetaType<QSet<AssetProcessor::AssetFileInfo>>("QSet<AssetFileInfo>");

        AZ::Test::ScopedAutoTempDirectory tempEngineRoot;

        AZStd::set<AZ::IO::Path> expectedFiles;
        // set up some interesting files:
        expectedFiles.insert(tempEngineRoot.Resolve("rootfile2.txt"));
        expectedFiles.insert(tempEngineRoot.Resolve("subfolder1/basefile.txt"));
        expectedFiles.insert(tempEngineRoot.Resolve("subfolder2/basefile.txt"));
        expectedFiles.insert(tempEngineRoot.Resolve("subfolder2/aaa/basefile.txt"));
        expectedFiles.insert(tempEngineRoot.Resolve("subfolder2/aaa/bbb/basefile.txt"));
        expectedFiles.insert(tempEngineRoot.Resolve("subfolder2/aaa/bbb/ccc/basefile.txt"));
        expectedFiles.insert(tempEngineRoot.Resolve("subfolder2/aaa/bbb/ccc/ddd/basefile.txt"));
        expectedFiles.insert(tempEngineRoot.Resolve(
            "subfolder2/aaa/bbb/ccc/ddd/eee.fff.ggg/basefile.txt")); // adding a folder name containing dots
        expectedFiles.insert(tempEngineRoot.Resolve("subfolder2/aaa/bbb/ccc/ddd/eee.fff.ggg/basefile1.txt"));
        expectedFiles.insert(tempEngineRoot.Resolve("subfolder3/basefile.txt"));
        expectedFiles.insert(tempEngineRoot.Resolve("subfolder3/aaa/basefile.txt"));
        expectedFiles.insert(tempEngineRoot.Resolve("subfolder3/aaa/bbb/basefile.txt"));
        expectedFiles.insert(tempEngineRoot.Resolve("subfolder3/aaa/bbb/ccc/basefile.txt"));
        expectedFiles.insert(tempEngineRoot.Resolve("rootfile1.txt"));

        for (const AZ::IO::Path& expect : expectedFiles)
        {
            EXPECT_TRUE(UnitTestUtils::CreateDummyFile(expect.c_str()));
        }

        // but we're going to not watch subfolder3 recursively, so... remove these files (even though they're on disk)
        // if this causes a failure it means that its ignoring the "do not recurse" flag:
        expectedFiles.erase(tempEngineRoot.Resolve("subfolder3/aaa/basefile.txt"));
        expectedFiles.erase(tempEngineRoot.Resolve("subfolder3/aaa/bbb/basefile.txt"));
        expectedFiles.erase(tempEngineRoot.Resolve("subfolder3/aaa/bbb/ccc/basefile.txt"));

        AZStd::set<AZ::IO::Path> expectedFolders;
        expectedFolders.insert(tempEngineRoot.Resolve("subfolder2/aaa"));
        expectedFolders.insert(tempEngineRoot.Resolve("subfolder2/aaa/bbb"));
        expectedFolders.insert(tempEngineRoot.Resolve("subfolder2/aaa/bbb/ccc"));
        expectedFolders.insert(tempEngineRoot.Resolve("subfolder2/aaa/bbb/ccc/ddd"));
        expectedFolders.insert(tempEngineRoot.Resolve("subfolder2/aaa/bbb/ccc/ddd/eee.fff.ggg"));

        PlatformConfiguration config;
        AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms;
        config.PopulatePlatformsForScanFolder(platforms);
        // PATH, DisplayName, PortKey, outputfolder, root, recurse, platforms
        config.AddScanFolder(
            ScanFolderInfo(tempEngineRoot.GetDirectory(), "temp", "ap1", true, false, platforms)); // note:  "Recurse" set to false.
        config.AddScanFolder(ScanFolderInfo(tempEngineRoot.Resolve("subfolder1").c_str(), "", "ap2", false, true, platforms));
        config.AddScanFolder(ScanFolderInfo(tempEngineRoot.Resolve("subfolder2").c_str(), "", "ap3", false, true, platforms));
        config.AddScanFolder(
            ScanFolderInfo(tempEngineRoot.Resolve("subfolder3").c_str(), "", "ap4", false, false, platforms)); // note:  "Recurse" set to false.
        AssetScanner scanner(&config);

        AZStd::list<AssetFileInfo> filesFound;
        AZStd::list<AssetFileInfo> foldersFound;

        bool doneScan = false;

        connect(
            &scanner, &AssetScanner::FilesFound, this,
            [&filesFound](QSet<AssetFileInfo> fileList)
            {
                for (AssetFileInfo foundFile : fileList)
                {
                    filesFound.push_back(foundFile);
                }
            });

        connect(
            &scanner, &AssetScanner::FoldersFound, this,
            [&foldersFound](QSet<AssetFileInfo> folderList)
            {
                for (AssetFileInfo foundFolder : folderList)
                {
                    foldersFound.push_back(foundFolder);
                }
            });

        connect(
            &scanner, &AssetScanner::AssetScanningStatusChanged, this,
            [&doneScan](AssetProcessor::AssetScanningStatus status)
            {
                if ((status == AssetProcessor::AssetScanningStatus::Completed) || (status == AssetProcessor::AssetScanningStatus::Stopped))
                {
                    doneScan = true;
                }
            });

        // this test makes sure that no files that should be missed were actually missed.
        // it makes sure that if a folder is added recursively, child files and folders are found
        // it makes sure that if a folder is added NON-recursively, child folder files are not found.

        scanner.StartScan();
        auto startTime = AZStd::chrono::steady_clock::now();
        while (!doneScan)
        {
            QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents, 100);

            auto millisecondsSpentScanning =
                AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(AZStd::chrono::steady_clock::now() - startTime);
            if (millisecondsSpentScanning > AZStd::chrono::milliseconds(10000))
            {
                break;
            }
        }

        EXPECT_TRUE(doneScan);
        EXPECT_EQ(filesFound.size(), expectedFiles.size());

        for (const AssetFileInfo& file : filesFound)
        {
            EXPECT_NE(expectedFiles.find(file.m_filePath.toUtf8().constData()), expectedFiles.end());
        }

        for (const AssetFileInfo& folder : foldersFound)
        {
            EXPECT_NE(expectedFolders.find(folder.m_filePath.toUtf8().constData()), expectedFolders.end());
        }

    }
}


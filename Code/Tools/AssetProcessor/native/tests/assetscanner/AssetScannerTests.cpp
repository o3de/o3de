/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/tests/assetscanner/AssetScannerTests.h>
#include <native/AssetManager/assetScanner.h>

namespace AssetProcessor
{

    class AssetScanner_Test
        : public AssetScanner
    {
    public:
        AssetScanner_Test(PlatformConfiguration* config, QObject* parent = nullptr)
            :AssetScanner(config, parent)
        {
        }
        friend class GTEST_TEST_CLASS_NAME_(AssetScannerTest, AssetScannerExcludeFileTest);
        friend class GTEST_TEST_CLASS_NAME_(AssetScannerTest, AssetScannerExcludeFolderTest);
    };


    AssetScannerTest::AssetScannerTest()
        :m_argc(0)
        ,m_argv(0)
    {
        m_qApp.reset(new QCoreApplication(m_argc,m_argv));
        qRegisterMetaType<QSet<QString> >("QSet<QString>");
        qRegisterMetaType<AssetProcessor::AssetScanningStatus>("AssetScanningStatus");
        qRegisterMetaType<QSet<AssetFileInfo>>("QSet<AssetFileInfo>");
    }

    bool AssetScannerTest::BlockUntilScanComplete(int millisecondsMax)
    {
        QElapsedTimer limit;
        limit.start();
        while ((!m_scanComplete) && (limit.elapsed() < millisecondsMax))
        {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        }

        // and then once more, so that any queued events as a result of the above finish.
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);

        return m_scanComplete;
    }

    void AssetScannerTest::SetUp()
    {
        using namespace UnitTestUtils;
        AssetProcessorTest::SetUp();
        QDir tempPath(m_tempDir.path());


        QSet<QString> expectedFiles;
        expectedFiles << tempPath.absoluteFilePath("rootfile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder1/basefile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder2/basefile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder2/aaa/basefile.txt");

        for (const QString& expect : expectedFiles)
        {
            EXPECT_TRUE(CreateDummyFile(expect));
        }
        m_platformConfig.reset(new AssetProcessor::PlatformConfiguration());
        AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms;
        m_platformConfig.get()->PopulatePlatformsForScanFolder(platforms);
        //                                                      PATH                     DisplayName  PortKey root recurse  platforms
        m_platformConfig.get()->AddScanFolder(ScanFolderInfo(tempPath.absolutePath(), "", "ap1", true, false, platforms));
        m_platformConfig.get()->AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder1"), "", "ap2", false, true, platforms));
        m_platformConfig.get()->AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder2"), "", "ap3", false, true, platforms));

        m_assetScanner.reset(new AssetScanner_Test(m_platformConfig.get()));

        QObject::connect(m_assetScanner.get(), &AssetScanner::FilesFound, [this](QSet<AssetProcessor::AssetFileInfo> fileList)
        {
            for (AssetProcessor::AssetFileInfo foundFile : fileList)
            {
                m_files.insert(foundFile.m_filePath);
            }
        }
        );

        QObject::connect(m_assetScanner.get(), &AssetScanner::AssetScanningStatusChanged, [this](AssetProcessor::AssetScanningStatus status)
        {
            if ((status == AssetProcessor::AssetScanningStatus::Completed) || (status == AssetProcessor::AssetScanningStatus::Stopped))
            {
                m_scanComplete = true;
            }
        }
        );

        QObject::connect(m_assetScanner.get(), &AssetScanner::FoldersFound, [this](QSet<AssetProcessor::AssetFileInfo> folderList)
        {
            for (AssetProcessor::AssetFileInfo foundFolder : folderList)
            {
                m_folders.insert(foundFolder.m_filePath);
            }
        }
        );

    }

    void AssetScannerTest::TearDown()
    {
        m_assetScanner.reset();
        m_platformConfig.reset();
        QDir tempDir(m_tempDir.path());
        tempDir.removeRecursively();
        m_qApp.reset();
        AssetProcessor::AssetProcessorTest::TearDown();
    }

    TEST_F(AssetScannerTest, AssetScannerExcludeFileTest)
    {
        QDir tempDir(m_tempDir.path());

        ExcludeAssetRecognizer excludeRecogniser;
        excludeRecogniser.m_name = "backup";
        // we are excluding all the files in the folder but not the folder itself
        excludeRecogniser.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("(^|[^/]+/)aaa/.*", AssetBuilderSDK::AssetBuilderPattern::Regex);
        m_platformConfig.get()->AddExcludeRecognizer(excludeRecogniser);
        m_assetScanner.get()->StartScan();

        BlockUntilScanComplete(5000);

        EXPECT_EQ(m_files.size(), 3);
        EXPECT_FALSE(m_files.contains(tempDir.filePath("subfolder2/aaa/basefile.txt")));

        EXPECT_EQ(m_folders.size(), 1);
        EXPECT_TRUE(m_folders.contains(tempDir.filePath("subfolder2/aaa")));
    }


    TEST_F(AssetScannerTest, AssetScannerExcludeFolderTest)
    {
        QDir tempDir(m_tempDir.path());

        ExcludeAssetRecognizer excludeRecogniser;
        excludeRecogniser.m_name = "backup";
        // we are excluding the complete folder here
        excludeRecogniser.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("(^|[^/]+/)aaa", AssetBuilderSDK::AssetBuilderPattern::Regex);
        m_platformConfig.get()->AddExcludeRecognizer(excludeRecogniser);
        m_assetScanner.get()->StartScan();

        BlockUntilScanComplete(5000);

        EXPECT_EQ(m_files.size(), 3);
        EXPECT_FALSE(m_files.contains(tempDir.filePath("subfolder2/aaa/basefile.txt")));
        EXPECT_EQ(m_folders.size(), 0);
    }
}

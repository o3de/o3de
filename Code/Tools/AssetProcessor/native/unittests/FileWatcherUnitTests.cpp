/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AssetBuilderSDK/AssetBuilderSDK.h>

#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/IO/FileOperations.h>

#include <AzTest/AzTest.h>
#include <AzTest/Utils.h>

#include <native/FileWatcher/FileWatcher.h>
#include <native/unittests/UnitTestUtils.h>

#include <QSet>
#include <QDir>
#include <QString>
#include <QFileInfo>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QMetaObject>
#include <QList>

namespace FileWatcherTests
{
    // the maximum amount of time to wait for file changes to appear
    // note that a busy system can have significant time delay before changes bubble through, and these tests
    // will exit the instant they get what they're looking for, so this can be set very high and will only impact
    // failure cases rather than the usual (pass) case.
    const int c_MaxWaitForFileChangesMS = 30000;

    // the number of iterations to run tests that interact with asynchronous threads.
    // tests in this category should be added to SUITE_periodic, because they are open ended
    // and may wear SSD
    const unsigned long c_FilesInFloodTest = 1000;

    class FileWatcherUnitTest : public ::testing::Test
    {
    public:
        void SetUp() override;
        void TearDown() override;

        //! Watch until we know there will be no more events forthcoming.
        //! Test will fail if the number of events expected is not exactly what was received.
        //! See the caveat below about modified notifications.
        void WatchUntilNoMoreEvents(int expectedAddFiles, int expectedModifyFiles, int expectedRemoveFiles);
        virtual void SetupWatches();
        void Flush();
        virtual QString GetFenceFolder(); // some tests may need to override this

    protected:
        AZStd::unique_ptr<FileWatcher> m_fileWatcher;
        QString m_assetRootPath;
        QMetaObject::Connection m_addFileConnection;
        QMetaObject::Connection m_removeFileConnection;
        QMetaObject::Connection m_modifyFileConnection;

        QList<QString> m_filesAdded;
        QList<QString> m_filesRemoved;

        // Modified is tricky becuase different operating systems emit differing numbers of modifies for a file.
        // For example, one OS will send a modify for every create (in ADDITION to the create) as well as potentially
        // multiple modifies for each change to the file if you write to the file (it may consider size changing, date changing,
        // and content changing as potentially different modifies and notify for the same file, and in many cases, the API
        // does not have a differentiator to indicate the difference between those modify events and must forward them to
        // the application.

        // as such, we store it as a set to ignore duplicates instead of as a list.
        QSet<QString> m_filesModified;

        AZStd::unique_ptr<QCoreApplication> m_app;
        AZStd::unique_ptr<AZ::IO::FileIOBase> m_baseFileIO;
        AZStd::unique_ptr<AZ::Test::ScopedAutoTempDirectory> m_tempDir;

        // to appear in the notify queue.  Once that happens we know that all prior events have already been handled and nothing
        // is still forthcoming from the OS. 
        // Known as a 'fence' file since its similar to issuing 'fence' CPU instructions.
        QString m_currentFenceFilePath; 
        bool m_fenceFileFound = false;
        void CreateFenceFile();
    };

    void FileWatcherUnitTest::SetUp()
    {
        m_fenceFileFound = false;
        int m_dummyArgC = 0;
        char** m_dummyArgV = nullptr;
        m_app = AZStd::make_unique<QCoreApplication>(m_dummyArgC, m_dummyArgV);

        m_baseFileIO = AZStd::make_unique<AZ::IO::LocalFileIO>();
        AZ::IO::FileIOBase::SetInstance(m_baseFileIO.get());

        m_tempDir = AZStd::make_unique<AZ::Test::ScopedAutoTempDirectory>();
        // remove any symlinking.  
        // This is necessary because on some operating systems, the temp dir
        // may be a symlinked folder, but the file watching API generally implemented
        // at a lower level than things like symlinks and will tend to emit
        // real paths, which then won't match up with the expected values.
        m_assetRootPath = QFileInfo(m_tempDir->GetDirectory()).canonicalFilePath();
        m_fileWatcher = AZStd::make_unique<FileWatcher>();

        SetupWatches();

        m_fileWatcher->StartWatching();

        m_addFileConnection = QObject::connect(m_fileWatcher.get(), &FileWatcher::fileAdded, [&](QString filename)
            {
                if (filename == m_currentFenceFilePath)
                {
                    m_fenceFileFound = true;
                }
                else
                {
                    m_filesAdded.push_back(filename);
                }
            });
        
        m_removeFileConnection = QObject::connect(m_fileWatcher.get(), &FileWatcher::fileRemoved, [&](QString filename)
            {
                m_filesRemoved.push_back(filename);
            });

        m_modifyFileConnection = QObject::connect(m_fileWatcher.get(), &FileWatcher::fileModified, [&](QString filename)
            {
                // its possible to get 'file modified' on a fence file, which we ignore.
                if (filename.contains("__fence__"))
                {
                    return;
                }
                m_filesModified.insert(filename);
            });

        Flush();
    }

    // default setup watches the asset root and ignores all files with the word ignored in them.
    void FileWatcherUnitTest::SetupWatches()
    {
        m_fileWatcher->AddExclusion(AssetBuilderSDK::FilePatternMatcher("*ignored*", AssetBuilderSDK::AssetBuilderPattern::Wildcard));
        m_fileWatcher->AddFolderWatch(m_assetRootPath);
    }

    QString FileWatcherUnitTest::GetFenceFolder()
    {
        return m_assetRootPath;
    }

    void FileWatcherUnitTest::CreateFenceFile()
    {
        AZ::IO::Path fencePath = AZ::IO::Path(GetFenceFolder().toUtf8().constData()) / "__fence__";
        AZStd::string fenceString; 
        ASSERT_TRUE(AZ::IO::CreateTempFileName(fencePath.String().c_str(), fenceString));
        m_currentFenceFilePath = QString::fromUtf8(fenceString.c_str());
        QFile fenceFile(m_currentFenceFilePath);
        ASSERT_TRUE(fenceFile.open(QFile::WriteOnly));
        fenceFile.close();
    }
    
    void FileWatcherUnitTest::Flush()
    {
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        m_fenceFileFound = false;
        CreateFenceFile();
        QElapsedTimer timer;
        timer.start();

        // Wait for fence file to appear.
        while ((!m_fenceFileFound) && (timer.elapsed() < c_MaxWaitForFileChangesMS))
        {
            QCoreApplication::processEvents(QEventLoop::AllEvents);
        }
        m_filesAdded.clear();
        m_filesRemoved.clear();
        m_filesModified.clear();
    }

    void FileWatcherUnitTest::WatchUntilNoMoreEvents(int expectedAddFiles, int expectedModifyFiles, int expectedRemoveFiles)
    {
        m_fenceFileFound = false;
        CreateFenceFile();
        QElapsedTimer timer;
        timer.start();

        // Wait for timeout, or the fence file to appear to indicate nothing more is forthcoming
        // since OS events are issued in the order they occur:
        while ((timer.elapsed() < c_MaxWaitForFileChangesMS) && (!m_fenceFileFound))
        {
            QCoreApplication::processEvents(QEventLoop::AllEvents);
        }

        // if we didn't find the fence file everything else is not valid:
        ASSERT_TRUE(m_fenceFileFound);

        EXPECT_EQ(m_filesAdded.count(), expectedAddFiles);

        // note that modify is different in that on some OS we receive a modify on file create and some we don't.
        // on some we receive multiple modifies for every change.  So we can only make sure we have at least the expected
        // amount, not exactly the expected amount.
        EXPECT_GE(m_filesModified.count(), expectedModifyFiles);
        EXPECT_EQ(m_filesRemoved.count(), expectedRemoveFiles);
    }

    void FileWatcherUnitTest::TearDown()
    {
        QObject::disconnect(m_addFileConnection);
        QObject::disconnect(m_removeFileConnection);
        QObject::disconnect(m_modifyFileConnection);

        m_fileWatcher->StopWatching();
        m_fileWatcher.reset();
        AZ::IO::FileIOBase::SetInstance(nullptr);
        m_baseFileIO.reset();
        m_app.reset();
        m_tempDir.reset();
    }

    TEST_F(FileWatcherUnitTest, WatchFileCreation_CreateSingleFile_FileChangeFound)
    {
        QString testFileName = QDir::toNativeSeparators(QDir(m_assetRootPath).absoluteFilePath("test.tif"));
        QFile testTif(testFileName);

        bool open = testTif.open(QFile::WriteOnly); // this should trigger an 'add'
        ASSERT_TRUE(open);
        testTif.write("0"); // this should trigger a modify.
        testTif.close();

        // expect exactly 1 add, 1 modify.
        WatchUntilNoMoreEvents(1, 1, 0);

        EXPECT_TRUE(m_filesAdded.contains(testFileName));
        EXPECT_TRUE(m_filesModified.contains(testFileName));
    }
    
    TEST_F(FileWatcherUnitTest, WatchFileDeletion_RemoveTestAsset_FileChangeFound)
    {
        QString fileName = QDir::toNativeSeparators(QDir(m_assetRootPath).absoluteFilePath("test.tif"));
        EXPECT_TRUE(UnitTestUtils::CreateDummyFile(fileName, "hello world"));

        EXPECT_TRUE(QFile::remove(fileName));

        // everything happened to this file, so there should be 1 create, 1 modify, 1 delete event.
        WatchUntilNoMoreEvents(1, 1, 1);

        EXPECT_TRUE(m_filesAdded.contains(fileName));
        EXPECT_TRUE(m_filesModified.contains(fileName));
        EXPECT_TRUE(m_filesRemoved.contains(fileName));
    }

    // make sure that if multiple events including deletion happen to 1 file, we don't miss it.
    TEST_F(FileWatcherUnitTest, WatchFileCreateModifyDeletion_AllFileChangesFound)
    {
        QString fileName = QDir::toNativeSeparators(QDir(m_assetRootPath).absoluteFilePath("test.tif"));
        EXPECT_TRUE(UnitTestUtils::CreateDummyFile(fileName));

        // expect exactly 1 add.   We may recieve a modify on some operating systems.
        WatchUntilNoMoreEvents(1, 0, 0);
        Flush(); 

        EXPECT_TRUE(QFile::remove(fileName));

        // expect exactly 1 remove, 0 of anything else.
        WatchUntilNoMoreEvents(0, 0, 1);

        EXPECT_TRUE(m_filesRemoved.contains(fileName));
    }

    TEST_F(FileWatcherUnitTest, WatchFileCreation_MultipleFiles_FileChangesFound_ChangesAreInOrder_SUITE_periodic)
    {
        for (unsigned long fileIndex = 0; fileIndex < c_FilesInFloodTest; ++fileIndex)
        {
            QString filename = QDir::toNativeSeparators(QDir(m_assetRootPath).absoluteFilePath(QString("test%1.tif").arg(fileIndex)));
            filename = QDir::toNativeSeparators(filename);
            QFile testTif(filename);
            bool open = testTif.open(QFile::WriteOnly);

            EXPECT_TRUE(open);

            testTif.write("0");
            testTif.close();

            QFile::remove(filename);
        }

        WatchUntilNoMoreEvents(c_FilesInFloodTest, c_FilesInFloodTest, c_FilesInFloodTest); 
        
        ASSERT_EQ(m_filesAdded.count(), c_FilesInFloodTest);
        ASSERT_EQ(m_filesRemoved.count(), c_FilesInFloodTest);

        // for modifies, since this is a set (no duplicates) we can expect that we have exactly as many as expected
        // since we are supposed to get at LEAST one modify for each file.
        EXPECT_EQ(m_filesModified.count(), c_FilesInFloodTest);

        for (unsigned long fileIndex = 0; fileIndex < c_FilesInFloodTest; ++fileIndex)
        {
            QString filename = QDir(m_assetRootPath).absoluteFilePath(QString("test%1.tif").arg(fileIndex));
            filename = QDir::toNativeSeparators(filename);
            EXPECT_STREQ(m_filesAdded[static_cast<unsigned int>(fileIndex)].toUtf8().constData(), filename.toUtf8().constData());
            // there may be more modifications than expected but we should at least see each one, once.
            EXPECT_TRUE(m_filesModified.contains(filename));
            EXPECT_STREQ(m_filesRemoved[static_cast<unsigned int>(fileIndex)].toUtf8().constData(), filename.toUtf8().constData());
        }
    }

    TEST_F(FileWatcherUnitTest, WatchFileCreation_MultipleFiles_IgnoresAreIgnored_SUITE_periodic)
    {
        // similar to previous test but interlace ignored patterns:
        QList<QString> nonIgnoredFiles;
        bool lastFileWasIgnored = false;

        for (unsigned long fileIndex = 0; fileIndex < c_FilesInFloodTest; ++fileIndex)
        {
            QString filename;

            if ((fileIndex % 4) != 0)
            {
                filename = QDir::toNativeSeparators(QDir(m_assetRootPath).absoluteFilePath(QString("test%1.tif").arg(fileIndex)));
                nonIgnoredFiles.push_back(filename);
                lastFileWasIgnored = false;
            }
            else
            {
                filename = QDir::toNativeSeparators(QDir(m_assetRootPath).absoluteFilePath(QString("test%1ignored.tif").arg(fileIndex)));
                lastFileWasIgnored = true;
            }

            QFile testTif(filename);
            bool open = testTif.open(QFile::WriteOnly);

            EXPECT_TRUE(open);

            testTif.write("0");
            testTif.close();

            QFile::remove(filename);
        }

        // this is just a sanity check for the test itself.  Because all operating systems notify
        // file events in the order they occur, making sure that the last file was not in the ignore list
        // means we can assume that all prior events (including ignored events) have already been processed and that the test
        // is done, without sleeping.

        ASSERT_FALSE(lastFileWasIgnored); 

        int totalNonIgnored = nonIgnoredFiles.count();

        WatchUntilNoMoreEvents(totalNonIgnored, totalNonIgnored, totalNonIgnored);
        
        // we are about to access these by index so there should be at least as many as indexed
        // note that in actuality it should be actually exactly the same, but if there's an error
        // its useful to loop and show whats doubled up...
        ASSERT_GE(m_filesAdded.count(), totalNonIgnored);
        ASSERT_GE(m_filesModified.count(), totalNonIgnored);
        ASSERT_GE(m_filesRemoved.count(), totalNonIgnored);

        for (int fileIndex = 0; fileIndex < totalNonIgnored; ++fileIndex)
        {
            EXPECT_STREQ(m_filesAdded[fileIndex].toUtf8().constData(), nonIgnoredFiles[fileIndex].toUtf8().constData());
            EXPECT_TRUE(m_filesModified.contains(nonIgnoredFiles[fileIndex]));
            EXPECT_STREQ(m_filesRemoved[fileIndex].toUtf8().constData(), nonIgnoredFiles[fileIndex].toUtf8().constData());
        }
    }

    TEST_F(FileWatcherUnitTest, DirectoryAdditions_ShowUp)
    {
        QDir tempDirPath(m_assetRootPath);
        tempDirPath.mkpath("dir1");
        tempDirPath.mkpath("dir2");
        tempDirPath.mkpath("dir3");

        WatchUntilNoMoreEvents(3, 0, 0); // should have gotten 3 directory adds for the above 3 dirs.

        EXPECT_TRUE(m_filesAdded.contains(QDir::toNativeSeparators(tempDirPath.absoluteFilePath("dir1"))));
        EXPECT_TRUE(m_filesAdded.contains(QDir::toNativeSeparators(tempDirPath.absoluteFilePath("dir2"))));
        EXPECT_TRUE(m_filesAdded.contains(QDir::toNativeSeparators(tempDirPath.absoluteFilePath("dir3"))));
    }

    TEST_F(FileWatcherUnitTest, DirectoryAdditions_IgnoredFilesDoNotShowUp)
    {
        QDir tempDirPath(m_assetRootPath);
        tempDirPath.mkpath("dir1");
        tempDirPath.mkpath("dir_ignored_2");
        tempDirPath.mkpath("dir3");

        WatchUntilNoMoreEvents(2, 0, 0); // should have gotten 2 directory adds for the above 3 dirs due to ignores.

        EXPECT_TRUE(m_filesAdded.contains(QDir::toNativeSeparators(tempDirPath.absoluteFilePath("dir1"))));
        EXPECT_FALSE(m_filesAdded.contains(QDir::toNativeSeparators(tempDirPath.absoluteFilePath("dir_ignored_2"))));
        EXPECT_TRUE(m_filesAdded.contains(QDir::toNativeSeparators(tempDirPath.absoluteFilePath("dir3"))));
    }

    TEST_F(FileWatcherUnitTest, DirectoryAdditions_NonIgnoredFiles_InIgnoredDirectories_DoNotShowUp)
    {
        QDir tempDirPath(m_assetRootPath);
        tempDirPath.mkpath("dir1");
        tempDirPath.mkpath("dir_ignored_2");
        tempDirPath.mkpath("dir3");

        // normal file name, ignored directory name
        QString normalFileShouldBeIgnored = QDir::toNativeSeparators(tempDirPath.absoluteFilePath("dir_ignored_2/myfile.tif"));

        // normal file name, normal directory name
        QString normalFileShouldNotBeIgnored = QDir::toNativeSeparators(tempDirPath.absoluteFilePath("dir1/myfile.tif"));

        EXPECT_TRUE(UnitTestUtils::CreateDummyFile(normalFileShouldBeIgnored));
        EXPECT_TRUE(UnitTestUtils::CreateDummyFile(normalFileShouldNotBeIgnored));

        WatchUntilNoMoreEvents(3, 0, 0); 
        // should have gotten just the one file add and 2 adds.

        EXPECT_TRUE(m_filesAdded.contains(QDir::toNativeSeparators(tempDirPath.absoluteFilePath("dir1"))));
        EXPECT_TRUE(m_filesAdded.contains(QDir::toNativeSeparators(tempDirPath.absoluteFilePath("dir3"))));
        EXPECT_TRUE(m_filesAdded.contains(normalFileShouldNotBeIgnored));
    }

     TEST_F(FileWatcherUnitTest, DirectoryAdditions_NonIgnoredDirectories_InIgnoredDirectories_DoNotShowUp)
    {
        QDir tempDirPath(m_assetRootPath);
        tempDirPath.mkpath("dir1");
        tempDirPath.mkpath("dir_ignored_2/normaldir");
        tempDirPath.mkpath("dir3");

        WatchUntilNoMoreEvents(2, 0, 0); // only 2 adds, even though 4 objects created.
        
        // if m_filesAdded is 2 elements long and contains the 2 expected entries, we don't have to check that it does not contain unexpected elements.
        EXPECT_TRUE(m_filesAdded.contains(QDir::toNativeSeparators(tempDirPath.absoluteFilePath("dir1"))));
        EXPECT_TRUE(m_filesAdded.contains(QDir::toNativeSeparators(tempDirPath.absoluteFilePath("dir3"))));
    }

    class FileWatcherUnitTest_FloodTests : public ::testing::WithParamInterface<bool>, public FileWatcherUnitTest
    {
    };

    TEST_P(FileWatcherUnitTest_FloodTests, FileAddedAfterDirectoryAdded_IsNotMissed_SUITE_periodic)
    {
        bool interleaveFileCreationWithDirectoryCreation = GetParam();

        // makes sure there is no race condition for the case where you immediately add a file after adding a directory
        // Its unclear how many trials to try here, and we don't want to introduce flaky tests here,
        // but this one (when it is bad) tends to trigger in just 3 trials or faster since you're either watching for this edge case
        // or are not. 
        QDir tempDirPath(m_assetRootPath);

        // pre-create these to make the inner loop as tight as is possible:
        QList<QString> expectedFileAdds;
        QList<QString> subDirPaths;
        QList<QString> filePaths;

        for (int trial = 0; trial < c_FilesInFloodTest; ++trial)
        {
            QDir newDirPath(tempDirPath.absoluteFilePath(QString("dir_%1").arg(trial)));
            QDir subDir(newDirPath.absoluteFilePath(QString("subdir_%1").arg(trial)));
            
            QString filePathName = QDir::toNativeSeparators(subDir.absoluteFilePath(QString("file_%1.txt").arg(trial)));

            expectedFileAdds.push_back(QDir::toNativeSeparators(newDirPath.absolutePath()));
            expectedFileAdds.push_back(QDir::toNativeSeparators(subDir.absolutePath()));
            
            // if we're not interleaving, we expect all dirs to happen before any files:
            if (interleaveFileCreationWithDirectoryCreation)
            {
                expectedFileAdds.push_back(filePathName);
            }

            subDirPaths.push_back(QDir::toNativeSeparators(subDir.absolutePath()));
            filePaths.push_back(filePathName);
        }

        if (!interleaveFileCreationWithDirectoryCreation)
        {
            // all files at the end.
            for (int trial = 0; trial < c_FilesInFloodTest; ++trial)
            {
                expectedFileAdds.push_back(filePaths[trial]);
            }
        }

        // now that this is all precomputed, this loop can be very tight, creating the files and dirs extremely rapidly:

        // in the one parameterized version, we create all dirs first then all files.
        // in the other version we create files and dirs interleaved:

        if (!interleaveFileCreationWithDirectoryCreation)
        {
            // create dirs first then files
            for (int trial = 0; trial < c_FilesInFloodTest; ++trial)
            {
                tempDirPath.mkpath(subDirPaths[trial]);
            }

            for (int trial = 0; trial < c_FilesInFloodTest; ++trial)
            {
                UnitTestUtils::CreateDummyFile(filePaths[trial]);
            }
        }
        else
        {
            // create files first, then dirs.
            for (int trial = 0; trial < c_FilesInFloodTest; ++trial)
            {
                tempDirPath.mkpath(subDirPaths[trial]);
                UnitTestUtils::CreateDummyFile(filePaths[trial]);
            }
        }
        
        
        WatchUntilNoMoreEvents(expectedFileAdds.count(), 0, 0); // dir%1, subdir%1 and the file are created so 3 per.
        ASSERT_EQ(expectedFileAdds.count(), m_filesAdded.count()); // we are about to use operator[] on these, so we must assert.

        // order is not necessarily consistent in this case - to be more specific, its locally consistent - you will always
        // see the parent folder(s) before the file, but you won't necessarily get the files after all the dirs or interleaved in the same order
        // since the file monitor runs asynchronously and can have a backlog - meaning, unless we add a giant sleep between creating dirs and creating files,
        // it might discover the files in the dirs during dir traversal (files interleaved but after their parent) or after it (some files interleaved, some after)
        // but you will always get the parents before the children. 
        // This loop is thus just making sure we don' get any double adds:
        for (int expectedFileIndex = 0; expectedFileIndex < expectedFileAdds.count(); ++expectedFileIndex)
        {
            EXPECT_TRUE(m_filesAdded.contains(expectedFileAdds[expectedFileIndex]));
        }
    }

    INSTANTIATE_TEST_CASE_P(FileWatcherUnitTest, FileWatcherUnitTest_FloodTests, ::testing::Bool());
    
    TEST_F(FileWatcherUnitTest, DirectoryRemoves_ShowUp)
    {
        QDir tempDirPath(m_assetRootPath);
        tempDirPath.mkpath("dir1");
        tempDirPath.mkpath("dir2");
        tempDirPath.mkpath("dir3");

        WatchUntilNoMoreEvents(3, 0, 0); // should have gotten 3 directory adds for the above 3 dirs.
        Flush();

        QDir(tempDirPath.absoluteFilePath("dir1")).removeRecursively();
        QDir(tempDirPath.absoluteFilePath("dir2")).removeRecursively();
        QDir(tempDirPath.absoluteFilePath("dir3")).removeRecursively();

        WatchUntilNoMoreEvents(0, 0, 3); 

        EXPECT_TRUE(m_filesRemoved.contains(QDir::toNativeSeparators(tempDirPath.absoluteFilePath("dir1"))));
        EXPECT_TRUE(m_filesRemoved.contains(QDir::toNativeSeparators(tempDirPath.absoluteFilePath("dir2"))));
        EXPECT_TRUE(m_filesRemoved.contains(QDir::toNativeSeparators(tempDirPath.absoluteFilePath("dir3"))));
    }

    TEST_F(FileWatcherUnitTest, DirectoryRemoves_IgnoredDoNotShowUp)
    {
        QDir tempDirPath(m_assetRootPath);
        tempDirPath.mkpath("dir1");
        tempDirPath.mkpath("dir2_ignored");
        tempDirPath.mkpath("dir3");

        WatchUntilNoMoreEvents(2, 0, 0); // should have gotten 2 directory adds for the above 3 dirs due to ignores
        Flush();

        QDir(tempDirPath.absoluteFilePath("dir1")).removeRecursively();
        QDir(tempDirPath.absoluteFilePath("dir2_ignoreds")).removeRecursively();
        QDir(tempDirPath.absoluteFilePath("dir3")).removeRecursively();

        WatchUntilNoMoreEvents(0, 0, 2); 

        EXPECT_TRUE(m_filesRemoved.contains(QDir::toNativeSeparators(tempDirPath.absoluteFilePath("dir1"))));
        EXPECT_FALSE(m_filesRemoved.contains(QDir::toNativeSeparators(tempDirPath.absoluteFilePath("dir2"))));
        EXPECT_TRUE(m_filesRemoved.contains(QDir::toNativeSeparators(tempDirPath.absoluteFilePath("dir3"))));
    }

    TEST_F(FileWatcherUnitTest, WatchFileRelocation_RenameTestAsset_FileChangeFound)
    {
        QDir tempDirPath(m_assetRootPath);
        tempDirPath.mkpath("dir1");
        tempDirPath.mkpath("dir2");
        tempDirPath.mkpath("dir3");

        QString originalName = QDir::toNativeSeparators(tempDirPath.absoluteFilePath("dir1/test.tif"));
        QString newName1 = QDir::toNativeSeparators(tempDirPath.absoluteFilePath("dir1/test2.tif")); // change name only
        QString newName2 = QDir::toNativeSeparators(tempDirPath.absoluteFilePath("dir2/test2.tif")); // change dir only
        QString newName3 = QDir::toNativeSeparators(tempDirPath.absoluteFilePath("dir3/test3.tif")); // change name and dir.

        EXPECT_TRUE(UnitTestUtils::CreateDummyFile(originalName));

        WatchUntilNoMoreEvents(4, 0, 0); // should have gotten 3 directory adds for the above 3 dirs and 1 file add.

        EXPECT_TRUE(m_filesAdded.contains(QDir::toNativeSeparators(tempDirPath.absoluteFilePath("dir1"))));
        EXPECT_TRUE(m_filesAdded.contains(QDir::toNativeSeparators(tempDirPath.absoluteFilePath("dir2"))));
        EXPECT_TRUE(m_filesAdded.contains(QDir::toNativeSeparators(tempDirPath.absoluteFilePath("dir3"))));
        EXPECT_TRUE(m_filesAdded.contains(originalName)); // we should have received the 'added' for the file
        
        Flush();
        EXPECT_TRUE(QFile::rename(originalName, newName1));
        WatchUntilNoMoreEvents(1, 0, 1);
        EXPECT_TRUE(m_filesRemoved.contains(originalName));
        EXPECT_TRUE(m_filesAdded.contains(newName1));

        // okay, now rename it to the second folder.
        Flush();
        EXPECT_TRUE(QFile::rename(newName1, newName2));
        WatchUntilNoMoreEvents(1, 0, 1);
        EXPECT_TRUE(m_filesRemoved.contains(newName1));
        EXPECT_TRUE(m_filesAdded.contains(newName2));

        // okay, now rename it to the 3rd folder.
        Flush();
        EXPECT_TRUE(QFile::rename(newName2, newName3));
        WatchUntilNoMoreEvents(1, 0, 1);
        EXPECT_TRUE(m_filesRemoved.contains(newName2));
        EXPECT_TRUE(m_filesAdded.contains(newName3));
        
        Flush();
        // now rename an entire actual folder:
        QDir renamer;
        EXPECT_TRUE(renamer.rename(tempDirPath.absoluteFilePath("dir3"), tempDirPath.absoluteFilePath("dir4")));
        // surprise:  You should also see the new file get added that was moved along with the folder:

        WatchUntilNoMoreEvents(1, 0, 1);
        EXPECT_TRUE(m_filesRemoved.contains(QDir::toNativeSeparators(tempDirPath.absoluteFilePath("dir3"))));
        EXPECT_TRUE(m_filesAdded.contains(QDir::toNativeSeparators(tempDirPath.absoluteFilePath("dir4"))));
    }

    TEST_F(FileWatcherUnitTest, WatchFolder_ValidFoldersWatched)
    {
        // reset watched folders
        m_fileWatcher->StopWatching();
        m_fileWatcher->ClearFolderWatches();

        QDir tempDirPath(m_assetRootPath);

        // when a folder named "dir" is added
        auto folder1 = tempDirPath.absoluteFilePath("dir1");
        m_fileWatcher->AddFolderWatch(folder1);
        // the folder is watched
        EXPECT_TRUE(m_fileWatcher->HasWatchFolder(folder1));

        // when a folder with a similar name is added
        auto folder2 = tempDirPath.absoluteFilePath("dir11");
        m_fileWatcher->AddFolderWatch(folder2);
        // the folder is watched
        EXPECT_TRUE(m_fileWatcher->HasWatchFolder(folder2));

        // when a folder that is a subdirectory of an existing added folder
        auto folder3 = tempDirPath.absoluteFilePath("dir1/subdir");
        m_fileWatcher->AddFolderWatch(folder3);
        // the folder is NOT added becuase the parent is already watched
        EXPECT_FALSE(m_fileWatcher->HasWatchFolder(folder3));
    }

    // This test makes sure that there are no platform-specific gotchas with the default exclusion lists.
    // For example slash direction, weird edge cases with regex or filters, casing, or special internal implementation
    // of how it approaches these excludes.
    // Keep this test up to date with FileWatcher::InstallDefaultExclusionRules!
    
    class FileWatcherUnitTest_DefaultExclusions
        : public ::testing::WithParamInterface<bool>
        , public FileWatcherUnitTest
    {
    public:
        QDir m_projectFolder;
        QDir m_cacheLocation;

        QString GetFenceFolder() override
        {
            return m_cacheLocation.absoluteFilePath("fence");
        }

        void SetupWatches() override
        {
            bool cacheIsInsideProject = GetParam();

            QDir tempDirPath(m_assetRootPath);
            m_projectFolder = QDir(tempDirPath.absoluteFilePath("ProjectRoot"));

            if (cacheIsInsideProject)
            {
                m_cacheLocation = QDir(m_projectFolder.absoluteFilePath("Cache"));
            }
            else
            {
                m_cacheLocation = QDir(tempDirPath.absoluteFilePath("Cache"));
            }

            // you cannot watch a non existent folder as the root.  We must make these up front and the fence has to be
            // there for the tests to work.
            m_cacheLocation.mkpath(".");
            m_projectFolder.mkpath(".");
            m_cacheLocation.mkpath("fence");

            m_fileWatcher->AddFolderWatch(QDir::toNativeSeparators(m_projectFolder.absolutePath()));
            m_fileWatcher->AddFolderWatch(QDir::toNativeSeparators(m_cacheLocation.absolutePath()));

            m_fileWatcher->InstallDefaultExclusionRules(m_cacheLocation.absolutePath(), m_projectFolder.absolutePath());
        }
    };

    TEST_P(FileWatcherUnitTest_DefaultExclusions, ProjectRootHasCache_FiltersAsExpected)
    {
        // Items marked with * are expected to be ignored, the rest should be visible!
        // Note that there are 2 variations on this test, one where cache is a child of ProjectRoot, one where it is not.
        // tempdir
        //     ProjectRoot
        //         User
        //            someuserfile.txt *
        //      Assets
        //          Cache
        //             some_file.txt
        //          User
        //             some_file.txt
        //      projectrootfile.txt
        //      Cache < --could also be rooted in tempdir if cacheIsInsideProject is false.
        //          cacherootfile.txt *
        //          fence
        //             somefence.fence
        //          Intermediate Assets
        //                some_intermediate.txt
        //          pc
        //            some_random_cache_file.txt *

        // the order of creation here matters here for consistincy, so this has to be enforced.
        QList<QString> regularFiles;
        QList<QString> ignoredFiles;
        QList<QString> regularFldrs; // name chosen to make the following section easier to read
        QList<QString> ignoredFldrs;
        
        regularFldrs.push_back(m_projectFolder.absoluteFilePath("User"));
        ignoredFiles.push_back(m_projectFolder.absoluteFilePath("User/someuserfile.txt"));
        regularFldrs.push_back(m_projectFolder.absoluteFilePath("Assets"));
        regularFldrs.push_back(m_projectFolder.absoluteFilePath("Assets/Cache"));
        regularFiles.push_back(m_projectFolder.absoluteFilePath("Assets/Cache/some_file.txt"));
        regularFldrs.push_back(m_projectFolder.absoluteFilePath("Assets/User"));
        regularFiles.push_back(m_projectFolder.absoluteFilePath("Assets/User/some_file.txt"));
        regularFiles.push_back(m_projectFolder.absoluteFilePath("projectrootfile.txt"));

        ignoredFiles.push_back(m_cacheLocation.absoluteFilePath("cacherootfile.txt"));
        regularFiles.push_back(m_cacheLocation.absoluteFilePath("fence/somefence.fence"));
        regularFldrs.push_back(m_cacheLocation.absoluteFilePath("Intermediate Assets"));
        regularFiles.push_back(m_cacheLocation.absoluteFilePath("Intermediate Assets/some_intermediate.txt"));
        ignoredFldrs.push_back(m_cacheLocation.absoluteFilePath("pc"));
        ignoredFiles.push_back(m_cacheLocation.absoluteFilePath("pc/some_random_cache_file.txt"));
        
        int expectedCreates = 0;
        for (const auto& folderName : regularFldrs)
        {
            QDir(folderName).mkpath(".");
            ++expectedCreates; // we expect to see each folder in regularFldrs appear.
        }

        for (const auto& folderName : ignoredFldrs)
        {
            QDir(folderName).mkpath(".");
        }

        for (const auto& fileName : regularFiles)
        {
            EXPECT_TRUE(UnitTestUtils::CreateDummyFile(fileName));
            ++expectedCreates; // we expect to see each file in regularFiles appear.
        }

        for (const auto& fileName : ignoredFiles)
        {
            EXPECT_TRUE(UnitTestUtils::CreateDummyFile(fileName));
        }

        WatchUntilNoMoreEvents(expectedCreates, 0, 0);

        for (const auto& fileName : regularFiles)
        {
            QString nativeFormat = QDir::toNativeSeparators(fileName);
            EXPECT_TRUE(m_filesAdded.contains(nativeFormat)) << "Missing file watch:" << nativeFormat.toUtf8().constData();
        }

        for (const auto& folderName : regularFldrs)
        {
            QString nativeFormat = QDir::toNativeSeparators(folderName);
            EXPECT_TRUE(m_filesAdded.contains(nativeFormat)) << "Missing file watch:" << nativeFormat.toUtf8().constData();
        }

        for (const auto& fileName : ignoredFiles)
        {
            QString nativeFormat = QDir::toNativeSeparators(fileName);
            EXPECT_FALSE(m_filesAdded.contains(nativeFormat)) << "Unexpected file watch:" << nativeFormat.toUtf8().constData();
        }
        for (const auto& folderName : ignoredFldrs)
        {
            QString nativeFormat = QDir::toNativeSeparators(folderName);
            EXPECT_FALSE(m_filesAdded.contains(nativeFormat)) << "Unexpected file watch:" << nativeFormat.toUtf8().constData();
        }
    }

    INSTANTIATE_TEST_CASE_P(FileWatcherUnitTest, FileWatcherUnitTest_DefaultExclusions, ::testing::Bool());

} // namespace File Watcher Tests.

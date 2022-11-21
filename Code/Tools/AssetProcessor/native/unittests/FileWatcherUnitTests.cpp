/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/tests/MockAssetDatabaseRequestsHandler.h>
#include <native/unittests/FileWatcherUnitTests.h>
#include <native/unittests/UnitTestUtils.h>

#include <QCoreApplication>

using namespace AssetProcessor;

void FileWatcherUnitTest::SetUp()
{
    UnitTest::AssetProcessorUnitTestBase::SetUp();

    m_assetRootPath = QString(m_assetDatabaseRequestsHandler->GetAssetRootDir().c_str());
    m_fileWatcher = AZStd::make_unique<FileWatcher>();
    m_fileWatcher->AddFolderWatch(m_assetRootPath);
    m_fileWatcher->StartWatching();
}

void FileWatcherUnitTest::TearDown()
{
    m_fileWatcher.reset();

    UnitTest::AssetProcessorUnitTestBase::TearDown();
}

// File operations on Linux behave differently on Linux than Windows.
// The system under test doesn't yet handle Linux's file operations, which surfaced as this test arbitrarily passing and failing.
// This test is disabled on Linux until the system under test can handle Linux file system behavior correctly.
#if !AZ_TRAIT_DISABLE_FAILED_ASSET_PROCESSOR_TESTS && !defined(AZ_PLATFORM_LINUX)

TEST_F(FileWatcherUnitTest, WatchFileCreation_CreateSingleFile_FileChangeFound)
{
    // test a single file create/write
    bool foundFile = false;
    auto connection = QObject::connect(m_fileWatcher.get(), &FileWatcher::fileAdded, this, [&](QString filename)
        {
            AZ_TracePrintf(AssetProcessor::DebugChannel, "Single file test Found asset: %s.\n", filename.toUtf8().data());
            foundFile = true;
        });

    // give the file watcher thread a moment to get started
    QThread::msleep(50);

    QFile testTif(m_assetRootPath + "/test.tif");
    bool open = testTif.open(QFile::WriteOnly);

    EXPECT_TRUE(open);

    testTif.write("0");
    testTif.close();

    // Wait for file change to be found, timeout, and be delivered
    unsigned int tries = 0;
    while (!foundFile && tries++ < 10)
    {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        QThread::msleep(10);
    }

    EXPECT_TRUE(foundFile);

    QObject::disconnect(connection);
}

TEST_F(FileWatcherUnitTest, WatchFileCreation_CreateMultipleFiles_FileChangesFound)
{
    // test a whole bunch of files created/written
    // send enough files such that main thread is likely to be blocked:
    const unsigned long maxFiles = 1000;
    QSet<QString> outstandingFiles;

    auto connection = QObject::connect(m_fileWatcher.get(), &FileWatcher::fileAdded, this, [&](QString filename)
        {
            outstandingFiles.remove(filename);
        });
    AZ_TracePrintf(AssetProcessor::DebugChannel, "Performing multi-file test...\n");

    // give the file watcher thread a moment to get started
    QThread::msleep(50);

    for (unsigned long fileIndex = 0; fileIndex < maxFiles; ++fileIndex)
    {
        if (fileIndex % 100 == 0)
        {
            AZ_TracePrintf(AssetProcessor::DebugChannel, "Performing multi-file test... creating file  %d / %d\n", fileIndex, maxFiles);
        }
        QString filename = QString(m_assetRootPath + "/test%1.tif").arg(fileIndex);
        filename = QDir::toNativeSeparators(filename);
        outstandingFiles.insert(filename);
        QFile testTif(filename);
        bool open = testTif.open(QFile::WriteOnly);

        EXPECT_TRUE(open);

        testTif.write("0");
        testTif.close();
    }

    // Wait for file change to be found, timeout, and be delivered
    unsigned int tries = 0;
    while (outstandingFiles.count() > 0 && tries++ < 50) // 5 secs is more than enough for all remaining deliveries.
    {
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QThread::msleep(100);
        AZ_TracePrintf(AssetProcessor::DebugChannel, "Waiting for remaining notifications: %d \n", outstandingFiles.count());
    }

    bool filesMissed = outstandingFiles.count() > 0;
    if (filesMissed)
    {
#if defined(AZ_ENABLE_TRACING)
        AZ_TracePrintf(AssetProcessor::DebugChannel, "Timed out waiting for file changes: %d / %d  missed\n", outstandingFiles.count(), maxFiles);
        for (const QString& pending : outstandingFiles)
        {
            AZ_TracePrintf(AssetProcessor::DebugChannel, "Missed file: %s", pending.toUtf8().data());
        }
#endif
        ASSERT_FALSE(filesMissed) << "Missed files waiting for file changes";
    }

    QObject::disconnect(connection);
}

TEST_F(FileWatcherUnitTest, WatchFileDeletion_RemoveTestAsset_FileChangeFound)
{
    bool foundFile = false;
    auto connection = QObject::connect(m_fileWatcher.get(), &FileWatcher::fileRemoved, this, [&](QString filename)
        {
            AZ_TracePrintf(AssetProcessor::DebugChannel, "Deleted asset: %s...\n", filename.toUtf8().data());
            foundFile = true;
        });

    // give the file watcher thread a moment to get started
    QThread::msleep(50);

    QString fileName = m_assetRootPath + "/test.tif";
    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(fileName));

    bool removed = QFile::remove(fileName);

    EXPECT_TRUE(removed);

    // Wait for file change to be found, timeout, and be delivered
    unsigned int tries = 0;
    while (!foundFile && tries++ < 100)
    {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        QThread::msleep(10);
        AZ_TracePrintf(AssetProcessor::DebugChannel, "Deletion test ... waiting for deletion notification \n");
    }

    EXPECT_TRUE(foundFile);

    QObject::disconnect(connection);
}

TEST_F(FileWatcherUnitTest, WatchFileRelocation_RenameTestAsset_FileChangeFound)
{
    AZ_TracePrintf(AssetProcessor::DebugChannel, "rename/move test ...\n");

    {
        bool fileAddCalled = false;
        QString fileAddName;
        auto connectionAdd = QObject::connect(m_fileWatcher.get(), &FileWatcher::fileAdded, this, [&](QString filename)
        {
            fileAddCalled = true;
            fileAddName = filename;
        });

        bool fileRemoveCalled = false;
        QString fileRemoveName;
        auto connectionRemove = QObject::connect(m_fileWatcher.get(), &FileWatcher::fileRemoved, this, [&](QString filename)
        {
            fileRemoveCalled = true;
            fileRemoveName = filename;
        });

        QStringList fileModifiedNames;
        bool fileModifiedCalled = false;
        auto connectionModified = QObject::connect(m_fileWatcher.get(), &FileWatcher::fileModified, this, [&](QString filename)
        {
            fileModifiedCalled = true;
            fileModifiedNames.append(filename);
        });

        // give the file watcher thread a moment to get started
        QThread::msleep(50);

        QDir tempDirPath(m_assetRootPath);
        tempDirPath.mkpath("dir1");
        tempDirPath.mkpath("dir2");
        tempDirPath.mkpath("dir3");

        QString originalName = tempDirPath.absoluteFilePath("dir1/test.tif");
        QString newName1 = tempDirPath.absoluteFilePath("dir1/test2.tif"); // change name only
        QString newName2 = tempDirPath.absoluteFilePath("dir2/test2.tif"); // change dir only
        QString newName3 = tempDirPath.absoluteFilePath("dir3/test3.tif"); // change name and dir.


        EXPECT_TRUE(UnitTestUtils::CreateDummyFile(originalName));

        int tries = 0;
        while (!(fileAddCalled && fileModifiedCalled) && tries++ < 100)
        {
            QThread::msleep(10);
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        }


        EXPECT_TRUE(fileAddCalled);
        EXPECT_FALSE(fileRemoveCalled);
        fileAddCalled = false;
        fileRemoveCalled = false;
        fileModifiedCalled = false;

        // okay, now rename it:
        EXPECT_TRUE(QFile::rename(originalName, newName1));

        tries = 0;
        while (!(fileAddCalled || fileRemoveCalled || fileModifiedCalled) && tries++ < 100)
        {
            QThread::msleep(10);
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        }

        // make sure both callbacks fired and that
        // the original was "removed" and the new was "added"
        EXPECT_TRUE(fileAddCalled);
        EXPECT_TRUE(fileRemoveCalled);

#if defined(AZ_PLATFORM_WINDOWS)
        EXPECT_TRUE(fileModifiedCalled); // modified should be called on the folder that the file lives in (Only on Windows)
#endif // AZ_PLATFORM_WINDOWS

        EXPECT_EQ(QDir::toNativeSeparators(fileRemoveName).toLower(), QDir::toNativeSeparators(originalName).toLower());
        EXPECT_EQ(QDir::toNativeSeparators(fileAddName).toLower(), QDir::toNativeSeparators(newName1).toLower());

        fileAddCalled = false;
        fileRemoveCalled = false;
        fileModifiedCalled = false;

        // okay, now rename it to the second folder.
        EXPECT_TRUE(QFile::rename(newName1, newName2));

        tries = 0;
        while (!(fileAddCalled || fileRemoveCalled || fileModifiedCalled) && tries++ < 100)
        {
            QThread::msleep(10);
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        }

        // make sure both callbacks fired and that
        // the new1 was "removed" and the new2 was "added"
        EXPECT_TRUE(fileAddCalled);
        EXPECT_TRUE(fileRemoveCalled);
#if defined(AZ_PLATFORM_WINDOWS)
        EXPECT_TRUE(fileModifiedCalled); // modified should be called on the folder that the file lives in (Only on Windows)
#endif // AZ_PLATFORM_WINDOWS

        EXPECT_EQ(QDir::toNativeSeparators(fileRemoveName).toLower(), QDir::toNativeSeparators(newName1).toLower());
        EXPECT_EQ(QDir::toNativeSeparators(fileAddName).toLower(), QDir::toNativeSeparators(newName2).toLower());

        // okay, now rename it to the 3rd folder.
        EXPECT_TRUE(QFile::rename(newName2, newName3));

        fileAddCalled = false;
        fileRemoveCalled = false;
        fileModifiedCalled = false;

        tries = 0;
        while (!(fileAddCalled || fileRemoveCalled || fileModifiedCalled) && tries++ < 100)
        {
            QThread::msleep(10);
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        }
        // make sure both callbacks fired and that
        // the new1 was "removed" and the new2 was "added"
        EXPECT_TRUE(fileAddCalled);
        EXPECT_TRUE(fileRemoveCalled);
#if defined(AZ_PLATFORM_WINDOWS)
        EXPECT_TRUE(fileModifiedCalled); // modified should be called on the folder that the file lives in (Only on Windows)
#endif // AZ_PLATFORM_WINDOWS
        EXPECT_EQ(QDir::toNativeSeparators(fileRemoveName).toLower(), QDir::toNativeSeparators(newName2).toLower());
        EXPECT_EQ(QDir::toNativeSeparators(fileAddName).toLower(), QDir::toNativeSeparators(newName3).toLower());

#if !defined(AZ_PLATFORM_LINUX)
        // final test... make sure that renaming a DIRECTORY works too.
        // Note that linux does not get any callbacks if just the directory is renamed (from inotify)
        QDir renamer;
        fileAddCalled = false;
        fileRemoveCalled = false;
        fileModifiedCalled = false;

        EXPECT_TRUE(renamer.rename(tempDirPath.absoluteFilePath("dir3"), tempDirPath.absoluteFilePath("dir4")));

        tries = 0;
        while (!(fileAddCalled && fileRemoveCalled) && tries++ < 100)
        {
            QThread::msleep(10);
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        }

        EXPECT_TRUE(fileAddCalled);
        EXPECT_TRUE(fileRemoveCalled);

        // note that when you rename a directory ONLY, its os-specific as to whether you get a modify callback.  Windows does not
        // but we don't specify or require or deny it in our API.

        EXPECT_EQ(QDir::toNativeSeparators(fileRemoveName).toLower(), QDir::toNativeSeparators(tempDirPath.absoluteFilePath("dir3")).toLower());
        EXPECT_EQ(QDir::toNativeSeparators(fileAddName).toLower(), QDir::toNativeSeparators(tempDirPath.absoluteFilePath("dir4")).toLower());
#endif // AZ_PLATFORM_LINUX

        QObject::disconnect(connectionRemove);
        QObject::disconnect(connectionAdd);
        QObject::disconnect(connectionModified);
    }
}
#endif // !AZ_TRAIT_DISABLE_FAILED_ASSET_PROCESSOR_TESTS

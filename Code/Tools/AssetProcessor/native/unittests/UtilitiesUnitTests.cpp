/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Jobs/Job.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>

#include <native/utilities/assetUtils.h>
#include <native/utilities/ByteArrayStream.h>
#include <native/tests/MockAssetDatabaseRequestsHandler.h>
#include <native/unittests/UnitTestUtils.h>
#include <native/unittests/AssetProcessorUnitTests.h>

#include <QThread>

#if defined(AZ_PLATFORM_LINUX)
#include <sys/stat.h>
#include <fcntl.h>
#endif

using namespace UnitTestUtils;
using namespace AssetUtilities;
using namespace AssetProcessor;

namespace AssetProcessor
{
    // simple utility class to make sure threads join and don't cause asserts
    // if the unit test exits early.
    class AutoThreadJoiner final
    {
        public:
        explicit AutoThreadJoiner(AZStd::thread* ownershipTransferThread)
        {
            m_threadToOwn = ownershipTransferThread;
        }

        ~AutoThreadJoiner()
        {
            if (m_threadToOwn)
            {
                m_threadToOwn->join();
                delete m_threadToOwn;
            }
        }

        AZStd::thread* m_threadToOwn;
    };
}

class UtilitiesUnitTests
    : public UnitTest::AssetProcessorUnitTestBase
{
};

TEST_F(UtilitiesUnitTests, NormalizeFilePath_FeedFilePathInDifferentFormats_Succeeds)
{
    using namespace AzToolsFramework::AssetDatabase;

    // do not change case
    // do not chop extension
    // do not make full path
    EXPECT_EQ(NormalizeFilePath("a/b\\c\\d/E.txt"), "a/b/c/d/E.txt");

    // do not erase full path
#if defined(AZ_PLATFORM_WINDOWS)
    EXPECT_EQ(NormalizeFilePath("c:\\a/b\\c\\d/E.txt"), "C:/a/b/c/d/E.txt");
#else
    EXPECT_EQ(NormalizeFilePath("c:\\a/b\\c\\d/E.txt"), "c:/a/b/c/d/E.txt");
#endif // defined(AZ_PLATFORM_WINDOWS)

    // same tests but for directories:
#if defined(AZ_PLATFORM_WINDOWS)
    EXPECT_EQ(NormalizeDirectoryPath("c:\\a/b\\c\\d"), "C:/a/b/c/d");
#else
    EXPECT_EQ(NormalizeDirectoryPath("c:\\a/b\\c\\d"), "c:/a/b/c/d");
#endif // defined(AZ_PLATFORM_WINDOWS)

    EXPECT_EQ(NormalizeDirectoryPath("a/b\\c\\d"), "a/b/c/d");

    // directories automatically chop slashes:
#if defined(AZ_PLATFORM_WINDOWS)
    EXPECT_EQ(NormalizeDirectoryPath("c:\\a/b\\c\\d\\"), "C:/a/b/c/d");
    EXPECT_EQ(NormalizeDirectoryPath("c:\\a/b\\c\\d//"), "C:/a/b/c/d");
#else
    EXPECT_EQ(NormalizeDirectoryPath("c:\\a/b\\c\\d\\"), "c:/a/b/c/d");
    EXPECT_EQ(NormalizeDirectoryPath("c:\\a/b\\c\\d//"), "c:/a/b/c/d");
#endif // defined(AZ_PLATFORM_WINDOWS)
}

TEST_F(UtilitiesUnitTests, ChangeFileAttributes_MakeFileReadOnlyOrWritable_Succeeds)
{
    QDir dir(m_assetDatabaseRequestsHandler->GetAssetRootDir().c_str());
    QString fileName(dir.filePath("test.txt"));
    CreateDummyFile(fileName);
#if defined WIN32
    DWORD fileAttributes = GetFileAttributesA(fileName.toUtf8());

    if (!(fileAttributes & FILE_ATTRIBUTE_READONLY))
    {
        //make the file Readonly
        if (!SetFileAttributesA(fileName.toUtf8(), fileAttributes | (FILE_ATTRIBUTE_READONLY)))
        {
            AZ_TracePrintf(AssetProcessor::DebugChannel, "Unable to change file attributes for the file: %s.\n", fileName.toUtf8().data());
        }
    }
#else
    QFileInfo fileInfo(fileName);
    if (fileInfo.permission(QFile::WriteUser))
    {
        //remove write user flag
        QFile::Permissions permissions = QFile::permissions(fileName) & ~(QFile::WriteUser);
        if (!QFile::setPermissions(fileName, permissions))
        {
            AZ_TracePrintf(AssetProcessor::DebugChannel, "Unable to change file attributes for the file: %s.\n", fileName.toUtf8().data());
        }
    }
#endif
    EXPECT_TRUE(AssetUtilities::MakeFileWritable(fileName));
}

TEST_F(UtilitiesUnitTests, NormalizeAndRemoveAlias_FeedFilePathWithDoubleSlackesAndAlias_Succeeds)
{
    // ------------- Test NormalizeAndRemoveAlias --------------

    EXPECT_EQ(AssetUtilities::NormalizeAndRemoveAlias("@test@\\my\\file.txt"), QString("my/file.txt"));
    EXPECT_EQ(AssetUtilities::NormalizeAndRemoveAlias("@test@my\\file.txt"), QString("my/file.txt"));
    EXPECT_EQ(AssetUtilities::NormalizeAndRemoveAlias("@TeSt@my\\file.txt"), QString("my/file.txt")); // case sensitivity test!
}

TEST_F(UtilitiesUnitTests, CopyFileWithTimeout_FeedFilesInDifferentStates_Succeeds)
{
    QDir dir(m_assetDatabaseRequestsHandler->GetAssetRootDir().c_str());
    QString fileName(dir.filePath("test.txt"));
    QString outputFileName(dir.filePath("test1.txt"));
    
    QFile inputFile(fileName);
    inputFile.open(QFile::WriteOnly);
    QFile outputFile(outputFileName);
    outputFile.open(QFile::WriteOnly);

#if defined(AZ_PLATFORM_WINDOWS)
    // this test is intentionally disabled on other platforms
    // because in general on other platforms its actually possible to delete and move
    // files out of the way even if they are currently opened for writing by a different
    // handle.
    //Trying to copy when the output file is open for reading should fail.
    {
        UnitTestUtils::AssertAbsorber absorb;
        EXPECT_FALSE(CopyFileWithTimeout(fileName, outputFileName, 1));
        EXPECT_EQ(absorb.m_numWarningsAbsorbed, 2); // 2 for each fail
        //Trying to move when the output file is open for reading
        EXPECT_FALSE(MoveFileWithTimeout(fileName, outputFileName, 1));
        EXPECT_EQ(absorb.m_numWarningsAbsorbed, 4);
    }
#endif // AZ_PLATFORM_WINDOWS ONLY

    inputFile.close();
    outputFile.close();

    //Trying to copy when the output file is not open
    EXPECT_TRUE(CopyFileWithTimeout(fileName, outputFileName, 1));
    EXPECT_TRUE(CopyFileWithTimeout(fileName, outputFileName, aznumeric_caster(-1)));//invalid timeout time
    // Trying to move when the output file is not open
    EXPECT_TRUE(MoveFileWithTimeout(fileName, outputFileName, 1));
    EXPECT_TRUE(MoveFileWithTimeout(outputFileName, fileName, 1));

    // Open the file and then close it in the near future
    AZStd::atomic_bool setupDone{ false };
    AssetProcessor::AutoThreadJoiner joiner(new AZStd::thread(
        [&]()
        {
            //opening file
            outputFile.open(QFile::WriteOnly);
            setupDone = true;
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(500));
            //closing file
            outputFile.close();
        }));

    while (!setupDone.load())
    {
        QThread::msleep(1);
    }

    EXPECT_TRUE(outputFile.isOpen());

    //Trying to copy when the output file is open,but will close before the timeout inputted
    {
        UnitTestUtils::AssertAbsorber absorb;
        EXPECT_TRUE(CopyFileWithTimeout(fileName, outputFileName, 1));
#if defined(AZ_PLATFORM_WINDOWS)
        // only windows has an issue with moving files out that are in use.
        // other platforms do so without issue.
        EXPECT_GT(absorb.m_numWarningsAbsorbed, 0);
#endif // windows platform.
    }
}

TEST_F(UtilitiesUnitTests, CheckCanLock_FeedFileToLock_GetsLockStatus)
{
    QDir lockTestDir(m_assetDatabaseRequestsHandler->GetAssetRootDir().c_str());
    QString lockTestFileName(lockTestDir.filePath("lockTest.txt"));

    EXPECT_FALSE(AssetUtilities::CheckCanLock(lockTestFileName));

    EXPECT_TRUE(CreateDummyFile(lockTestFileName));
    EXPECT_TRUE(AssetUtilities::CheckCanLock(lockTestFileName));

#if defined(AZ_PLATFORM_WINDOWS)
    // on windows, opening a file for reading locks it
    // but on other platforms, this is not the case.
    QFile lockTestFile(lockTestFileName);
    lockTestFile.open(QFile::ReadOnly);
#elif defined(AZ_PLATFORM_LINUX)
    int handle = open(lockTestFileName.toUtf8().constData(), O_RDONLY | O_EXCL | O_NONBLOCK);
#else
    int handle = open(lockTestFileName.toUtf8().constData(), O_RDONLY | O_EXLOCK | O_NONBLOCK);       
#endif // AZ_PLATFORM_WINDOWS

#if defined(AZ_PLATFORM_WINDOWS)
    EXPECT_FALSE(AssetUtilities::CheckCanLock(lockTestFileName));
    lockTestFile.close();
#else
    if (handle != -1)
    {
        close(handle);
    }
#endif // windows/other platforms ifdef 
}

TEST_F(UtilitiesUnitTests, TestByteArrayStream_WriteToStream_Succeeds)
{
    AssetProcessor::ByteArrayStream stream;
    EXPECT_TRUE(stream.CanSeek());
    EXPECT_TRUE(stream.IsOpen());
    EXPECT_EQ(stream.GetLength(), 0);
    EXPECT_EQ(stream.GetCurPos(), 0);
    char tempReadBuffer[24];
    azstrcpy(tempReadBuffer, 24, "This is a Test String");
    EXPECT_EQ(stream.Read(100, tempReadBuffer), 0);

    // reserving does not alter the length.
    stream.Reserve(128);
    EXPECT_EQ(stream.GetLength(), 0);
    EXPECT_EQ(stream.Write(7, tempReadBuffer), 7);
    EXPECT_EQ(stream.GetCurPos(), 7);
    EXPECT_EQ(stream.GetLength(), 7);
    EXPECT_EQ(memcmp(stream.GetArray().constData(), tempReadBuffer, 7), 0);
    EXPECT_EQ(stream.Write(7, tempReadBuffer), 7);
    EXPECT_EQ(stream.GetLength(), 14);
    EXPECT_EQ(stream.GetCurPos(), 14);
    EXPECT_EQ(memcmp(stream.GetArray().constData(), "This isThis is", 14), 0);


    stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN); // write at begin, without overrunning
    EXPECT_EQ(stream.GetCurPos(), 0);
    EXPECT_EQ(stream.Write(4, "that"), 4);
    EXPECT_EQ(stream.GetLength(), 14);
    EXPECT_EQ(stream.GetCurPos(), 4);
    EXPECT_EQ(memcmp(stream.GetArray().constData(), "that isThis is", 14), 0);

    stream.Seek(2, AZ::IO::GenericStream::ST_SEEK_CUR); // write in middle, without overrunning
    EXPECT_EQ(stream.GetCurPos(), 6);
    EXPECT_EQ(stream.Write(4, "1234"), 4);
    EXPECT_EQ(stream.GetLength(), 14);
    EXPECT_EQ(stream.GetCurPos(), 10);
    EXPECT_EQ(memcmp(stream.GetArray().constData(), "that i1234s is", 14), 0);

    stream.Seek(-6, AZ::IO::GenericStream::ST_SEEK_END); // write in end, negative offset, without overrunning
    EXPECT_EQ(stream.GetCurPos(), 8);
    EXPECT_EQ(stream.Write(4, "5555"), 4);
    EXPECT_EQ(stream.GetCurPos(), 12);
    EXPECT_EQ(stream.GetLength(), 14);
    EXPECT_EQ(memcmp(stream.GetArray().constData(), "that i125555is", 14), 0);

    stream.Seek(2, AZ::IO::GenericStream::ST_SEEK_BEGIN); // write at begin offset, with overrun:
    EXPECT_EQ(stream.GetCurPos(), 2);
    EXPECT_EQ(stream.GetLength(), 14);
    EXPECT_EQ(stream.Write(14, "xxxxxxxxxxxxxx"), 14);
    EXPECT_EQ(stream.GetLength(), 16);
    EXPECT_EQ(stream.GetCurPos(), 16);
    EXPECT_EQ(memcmp(stream.GetArray().constData(), "thxxxxxxxxxxxxxx", 16), 0);

    stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
    stream.Seek(14, AZ::IO::GenericStream::ST_SEEK_CUR); // write in middle, with overrunning:
    EXPECT_EQ(stream.GetCurPos(), 14);
    EXPECT_EQ(stream.GetLength(), 16);
    EXPECT_EQ(stream.Write(4, "yyyy"), 4);
    EXPECT_EQ(stream.GetCurPos(), 18);
    EXPECT_EQ(stream.GetLength(), 18);
    EXPECT_EQ(memcmp(stream.GetArray().constData(), "thxxxxxxxxxxxxyyyy", 18), 0);

    stream.Seek(-2, AZ::IO::GenericStream::ST_SEEK_END); // write in end, negative offset, with overrunning
    EXPECT_EQ(stream.GetCurPos(), 16);
    EXPECT_EQ(stream.GetLength(), 18);
    EXPECT_EQ(stream.Write(4, "ZZZZ"), 4);
    EXPECT_EQ(stream.GetCurPos(), 20);
    EXPECT_EQ(stream.GetLength(), 20);
    EXPECT_EQ(memcmp(stream.GetArray().constData(), "thxxxxxxxxxxxxyyZZZZ", 20), 0);

    // read test.
    stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
    EXPECT_EQ(stream.Read(20, tempReadBuffer), 20);
    EXPECT_EQ(memcmp(tempReadBuffer, "thxxxxxxxxxxxxyyZZZZ", 20), 0);
    EXPECT_EQ(stream.Read(20, tempReadBuffer), 0); // because its already at end.
    EXPECT_EQ(memcmp(tempReadBuffer, "thxxxxxxxxxxxxyyZZZZ", 20), 0); // it should not have disturbed the buffer.
    stream.Seek(2, AZ::IO::GenericStream::ST_SEEK_BEGIN);
    EXPECT_EQ(stream.Read(20, tempReadBuffer), 18);
    EXPECT_EQ(memcmp(tempReadBuffer, "xxxxxxxxxxxxyyZZZZZZ", 20), 0); // it should not have disturbed the buffer bits that it was not asked to touch.
}

TEST_F(UtilitiesUnitTests, MatchFilePattern_FeedDifferentPatternsAndFilePaths_Succeeds)
{
    {
        AssetBuilderSDK::FilePatternMatcher extensionWildcardTest(AssetBuilderSDK::AssetBuilderPattern("*.cfg", AssetBuilderSDK::AssetBuilderPattern::Wildcard));
        EXPECT_TRUE(extensionWildcardTest.MatchesPath(AZStd::string("foo.cfg")));
        EXPECT_TRUE(extensionWildcardTest.MatchesPath(AZStd::string("abcd/foo.cfg")));
        EXPECT_FALSE(extensionWildcardTest.MatchesPath(AZStd::string("abcd/foo.cfd")));
    }

    {
        AssetBuilderSDK::FilePatternMatcher prefixWildcardTest(AssetBuilderSDK::AssetBuilderPattern("abf*.llm", AssetBuilderSDK::AssetBuilderPattern::Wildcard));
        EXPECT_TRUE(prefixWildcardTest.MatchesPath(AZStd::string("abf.llm")));
        EXPECT_TRUE(prefixWildcardTest.MatchesPath(AZStd::string("abf12345.llm")));
        EXPECT_FALSE(prefixWildcardTest.MatchesPath(AZStd::string("foo/abf12345.llm")));
        EXPECT_FALSE(prefixWildcardTest.MatchesPath(AZStd::string("foo/abf12345.lls")));
        EXPECT_FALSE(prefixWildcardTest.MatchesPath(AZStd::string("foo/ab2345.llm")));
    }

    {
        AssetBuilderSDK::FilePatternMatcher extensionPrefixWildcardTest(AssetBuilderSDK::AssetBuilderPattern("sdf.c*", AssetBuilderSDK::AssetBuilderPattern::Wildcard));
        EXPECT_TRUE(extensionPrefixWildcardTest.MatchesPath(AZStd::string("sdf.cpp")));
        EXPECT_TRUE(extensionPrefixWildcardTest.MatchesPath(AZStd::string("sdf.cxx")));
        EXPECT_TRUE(extensionPrefixWildcardTest.MatchesPath(AZStd::string("sdf.c")));
        EXPECT_FALSE(extensionPrefixWildcardTest.MatchesPath(AZStd::string("abcd/sdf.cpp")));
        EXPECT_FALSE(extensionPrefixWildcardTest.MatchesPath(AZStd::string("s:\\asd/abcd/sdf.cpp")));
        EXPECT_FALSE(extensionPrefixWildcardTest.MatchesPath(AZStd::string("sdc.c")));
        EXPECT_FALSE(extensionPrefixWildcardTest.MatchesPath(AZStd::string("sdf.hxx")));
        EXPECT_FALSE(extensionPrefixWildcardTest.MatchesPath(AZStd::string("s:\\asd/abcd/sdf.hxx")));
    }

    {
        AssetBuilderSDK::FilePatternMatcher prefixExtensionPrefixWildcardTest(AssetBuilderSDK::AssetBuilderPattern("s*.c*", AssetBuilderSDK::AssetBuilderPattern::Wildcard));
        EXPECT_TRUE(prefixExtensionPrefixWildcardTest.MatchesPath(AZStd::string("sdf.cpp")));
        EXPECT_TRUE(prefixExtensionPrefixWildcardTest.MatchesPath(AZStd::string("sdf.cxx")));
        EXPECT_FALSE(prefixExtensionPrefixWildcardTest.MatchesPath(AZStd::string("abcd/sdf.cpp")));
        EXPECT_FALSE(prefixExtensionPrefixWildcardTest.MatchesPath(AZStd::string("c:\\asd/abcd/sdf.cpp")));
        EXPECT_FALSE(prefixExtensionPrefixWildcardTest.MatchesPath(AZStd::string("sdf.hxx")));
        EXPECT_FALSE(prefixExtensionPrefixWildcardTest.MatchesPath(AZStd::string("s:\\asd/abcd/sdf.hxx")));
    }

    {
        AssetBuilderSDK::FilePatternMatcher fixedNameTest(AssetBuilderSDK::AssetBuilderPattern("a.bcd", AssetBuilderSDK::AssetBuilderPattern::Wildcard));
        EXPECT_TRUE(fixedNameTest.MatchesPath(AZStd::string("a.bcd")));
        EXPECT_FALSE(fixedNameTest.MatchesPath(AZStd::string("foo\\a.bcd")));
        EXPECT_FALSE(fixedNameTest.MatchesPath(AZStd::string("foo/a.bcd")));
        EXPECT_FALSE(fixedNameTest.MatchesPath(AZStd::string("c:/foo/a.bcd")));
        EXPECT_FALSE(fixedNameTest.MatchesPath(AZStd::string("c:\\foo/a.bcd")));
        EXPECT_FALSE(fixedNameTest.MatchesPath(AZStd::string("sdf.hxx")));
    }

    {
        AssetBuilderSDK::FilePatternMatcher midMatchExtensionPrefixTest(AssetBuilderSDK::AssetBuilderPattern("s*f.c*", AssetBuilderSDK::AssetBuilderPattern::Wildcard));
        EXPECT_TRUE(midMatchExtensionPrefixTest.MatchesPath(AZStd::string("sdf.cpp")));
        EXPECT_TRUE(midMatchExtensionPrefixTest.MatchesPath(AZStd::string("sef.cxx")));
        EXPECT_TRUE(midMatchExtensionPrefixTest.MatchesPath(AZStd::string("sf.c")));
        EXPECT_FALSE(midMatchExtensionPrefixTest.MatchesPath(AZStd::string("c:\\asd/abcd/sdf.cpp")));
        EXPECT_FALSE(midMatchExtensionPrefixTest.MatchesPath(AZStd::string("abcd/sdf.cpp")));
        EXPECT_FALSE(midMatchExtensionPrefixTest.MatchesPath(AZStd::string("sdc.c")));
        EXPECT_FALSE(midMatchExtensionPrefixTest.MatchesPath(AZStd::string("sdf.hxx")));
        EXPECT_FALSE(midMatchExtensionPrefixTest.MatchesPath(AZStd::string("s:\\asd/abcd/sdf.hxx")));
    }

    {
        AssetBuilderSDK::FilePatternMatcher subFolderExtensionWildcardTest(AssetBuilderSDK::AssetBuilderPattern("abcd/*.cfg", AssetBuilderSDK::AssetBuilderPattern::Wildcard));
        EXPECT_TRUE(subFolderExtensionWildcardTest.MatchesPath(AZStd::string("abcd/sdf.cfg")));
        EXPECT_FALSE(subFolderExtensionWildcardTest.MatchesPath(AZStd::string("c://abcd/sdf.cfg")));
        EXPECT_FALSE(subFolderExtensionWildcardTest.MatchesPath(AZStd::string("sdf.cfg")));
        EXPECT_FALSE(subFolderExtensionWildcardTest.MatchesPath(AZStd::string("abcs/sdf.cfg")));
        EXPECT_FALSE(subFolderExtensionWildcardTest.MatchesPath(AZStd::string("abcd/sdf.cfx")));
    }

    {
        AssetBuilderSDK::FilePatternMatcher subFolderPatternTest(AssetBuilderSDK::AssetBuilderPattern(".*\\/savebackup\\/.*", AssetBuilderSDK::AssetBuilderPattern::Regex));
        EXPECT_TRUE(subFolderPatternTest.MatchesPath(AZStd::string("abcd/savebackup/sdf.cfg")));
        EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("abcd/savebackup")));
        EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("savebackup/sdf.cfg")));
        EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("c://abcd/sdf.cfg")));
        EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("sdf.cfg")));
        EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("abcs/sdf.cfg")));
        EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("abcd/sdf.cfx")));
    }

    {
        AssetBuilderSDK::FilePatternMatcher subFolderPatternTest(AssetBuilderSDK::AssetBuilderPattern(".*\\/Presets\\/GeomCache\\/.*", AssetBuilderSDK::AssetBuilderPattern::Regex));
        EXPECT_TRUE(subFolderPatternTest.MatchesPath(AZStd::string("something/Presets/GeomCache/sdf.cfg")));
        EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("Presets/GeomCache/sdf.cfg"))); // should not match because it demands that there is a slash
        EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("abcd/savebackup")));
        EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("savebackup/sdf.cfg")));
        EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("c://abcd/sdf.cfg")));
        EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("sdf.cfg")));
        EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("abcs/sdf.cfg")));
        EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("abcd/sdf.cfx")));
    }
}

TEST_F(UtilitiesUnitTests, GetFileHashFromStream_NullPath_Returns0)
{
    AZ::u64 result = GetFileHash(nullptr);
    EXPECT_EQ(result, 0);
}

TEST_F(UtilitiesUnitTests, GetFileHashFromStreamSmallFile_ReturnsExpectedHash)
{
    QDir dir(m_assetDatabaseRequestsHandler->GetAssetRootDir().c_str());
    QString fileName(dir.filePath("test.txt"));
    CreateDummyFile(fileName);
    AZ::u64 result = GetFileHash(fileName.toUtf8());
    EXPECT_EQ(result, AZ::u64(17241709254077376921ul));
}

TEST_F(UtilitiesUnitTests, GetFileHashFromStream_SmallFileForced_ReturnsExpectedHash)
{
    QDir dir(m_assetDatabaseRequestsHandler->GetAssetRootDir().c_str());
    QString fileName(dir.filePath("test.txt"));
    CreateDummyFile(fileName);
    AZ::u64 result = GetFileHash(fileName.toUtf8(), true);
    EXPECT_EQ(result, AZ::u64(17241709254077376921ul));
}

// This tests a race condition where one process was writing to a file and the Asset Processor started hashing that file
// in a rare edge case, the end of file check used within the FileIOStream was reporting an incorrect end of file.
// This job runs in a separate thread at the same time the hashing is run, and replicates this edge case.
class FileWriteThrashTestJob : public AZ::Job
{
public:
    AZ_CLASS_ALLOCATOR(FileWriteThrashTestJob, AZ::ThreadPoolAllocator, 0);

    FileWriteThrashTestJob(bool deleteWhenDone, AZ::JobContext* jobContext, AZ::IO::HandleType fileHandle, AZStd::string_view bufferToWrite)
        : Job(deleteWhenDone, jobContext),
        m_fileHandle(fileHandle),
        m_bufferToWrite(bufferToWrite)
    {
        for (; m_initialWriteCount >= 0; --m_initialWriteCount)
        {
            AZ::IO::FileIOBase::GetInstance()->Write(m_fileHandle, m_bufferToWrite.c_str(), m_bufferToWrite.length());
        }
    }

    void Process() override
    {
        for (; m_writeLoopCount >= 0; --m_writeLoopCount)
        {
            AZ::IO::FileIOBase::GetInstance()->Write(m_fileHandle, m_bufferToWrite.c_str(), m_bufferToWrite.length());

            // Writing this unsigned int triggers the race condition more often.
            unsigned int uintToWrite = 10;
            AZ::IO::FileIOBase::GetInstance()->Write(m_fileHandle, &uintToWrite, sizeof(uintToWrite));
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(1));
        }
        AZ::IO::FileIOBase::GetInstance()->Close(m_fileHandle);
    }

    AZStd::string m_bufferToWrite;
    AZ::IO::HandleType m_fileHandle;
    int m_writeLoopCount = 1023 * 10; // Write enough times to trigger the race condition, a bit more than 10 seconds.
    int m_initialWriteCount = 1021 * 10; // Start with a larger file to make sure the hash operation won't finish immediately.
};

// This is a regression test for a bug found with this repro:
//  In the editor, export a level, the asset processor shuts down.
// This occured because the asset processor's file hashing picked up the temporary navigation mesh file
// when it was created, and started hashing it. At the same time, the editor was still writing to this file.
// When the file hash called AZ::IO::FileIOStream's read function with a buffer to read in this file,
// and it hit the end of the file and it read less than the buffer, the end of file checked used by the
// AZ::IO::FileIOStream would sometimes incorrectly report that it did not hit the end of the file.
// The hash function was updated to use a more accurate check. Instead of always requesting to read the buffer size,
// it requests to read either the buffer size, or the remaining length of the file.
// This test replicates that behavior by starting a job on another thread that writes to a file,
// and at the same time it runs the file hashing process. With the fix reverted, this test fails.
// This test purposely does not force a failure state to verify the assert would occur because
// it's a race condition, and this test should not fail due to timing issues on different machines.
class MockTraceMessageBusHandler
    : public AZ::Debug::TraceMessageBus::Handler
{
public:
    bool OnPreAssert(const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* /*message*/) override
    {
        m_assertTriggered = true;
        return true;
    }
    bool m_assertTriggered = false;
};

TEST_F(UtilitiesUnitTests, GetFileHashFromStream_LargeFileForcedAnotherThreadWritesToFile_ReturnsExpectedHash)
{
    MockTraceMessageBusHandler traceMessageBusHandler;
    traceMessageBusHandler.BusConnect();
    QDir dir(m_assetDatabaseRequestsHandler->GetAssetRootDir().c_str());
    QString fileName(dir.filePath("test.txt"));
    CreateDummyFile(fileName);

    // Use a small buffer to frequently write a lot of data into the file, to help force the race condition.
    char buffer[10];
    memset(buffer, 'a', AZ_ARRAY_SIZE(buffer));
    AZ::IO::HandleType writeHandle;
    // Using a file handle and not a file stream because the navigation mesh system used this same interface for writing the file.
    AZ::IO::FileIOBase::GetInstance()->Open(fileName.toUtf8(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary, writeHandle);

    // The job will close the stream
    FileWriteThrashTestJob* job = aznew FileWriteThrashTestJob(true, nullptr, writeHandle, buffer);
    job->m_writeLoopCount = 100; // compensate for artificial hashing delay below, keeping this test duration similar to others
    job->Start();

    // Use an artificial delay on hashing to ensure the race condition actually occurs.
    AZ::u64 result =
        GetFileHash(fileName.toUtf8(), true, nullptr, /*hashMsDelay*/ 20);
    // This test will result in different hash results on different machines, because writing to the stream
    // and reading from the stream to generate the hash happen at different speeds in different setups.
    // Just make sure it returns some result here.
    EXPECT_NE(result, 0);
    EXPECT_FALSE(traceMessageBusHandler.m_assertTriggered);
    traceMessageBusHandler.BusDisconnect();
}

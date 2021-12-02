/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include "UtilitiesUnitTests.h"


#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>
#include "native/utilities/assetUtils.h"
#include "native/utilities/ByteArrayStream.h"
#include <AzCore/Component/Entity.h>
#include <AzCore/Jobs/Job.h>
#include <AzTest/AzTest.h>
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

#if !AZ_TRAIT_DISABLE_FAILED_ASSET_PROCESSOR_TESTS
REGISTER_UNIT_TEST(UtilitiesUnitTests)
#endif // AZ_TRAIT_DISABLE_FAILED_ASSET_PROCESSOR_TESTS

void UtilitiesUnitTests::StartTest()
{
    using namespace AzToolsFramework::AssetDatabase;

    // do not change case
    // do not chop extension
    // do not make full path
    UNIT_TEST_EXPECT_TRUE(NormalizeFilePath("a/b\\c\\d/E.txt") == "a/b/c/d/E.txt");

    // do not erase full path
#if defined(AZ_PLATFORM_WINDOWS)
    UNIT_TEST_EXPECT_TRUE(NormalizeFilePath("c:\\a/b\\c\\d/E.txt") == "C:/a/b/c/d/E.txt");
#else
    UNIT_TEST_EXPECT_TRUE(NormalizeFilePath("c:\\a/b\\c\\d/E.txt") == "c:/a/b/c/d/E.txt");
#endif // defined(AZ_PLATFORM_WINDOWS)


    // same tests but for directories:
#if defined(AZ_PLATFORM_WINDOWS)
    UNIT_TEST_EXPECT_TRUE(NormalizeDirectoryPath("c:\\a/b\\c\\d") == "C:/a/b/c/d");
#else
    UNIT_TEST_EXPECT_TRUE(NormalizeDirectoryPath("c:\\a/b\\c\\d") == "c:/a/b/c/d");
#endif // defined(AZ_PLATFORM_WINDOWS)

    UNIT_TEST_EXPECT_TRUE(NormalizeDirectoryPath("a/b\\c\\d") == "a/b/c/d");

    // directories automatically chop slashes:
#if defined(AZ_PLATFORM_WINDOWS)
    UNIT_TEST_EXPECT_TRUE(NormalizeDirectoryPath("c:\\a/b\\c\\d\\") == "C:/a/b/c/d");
    UNIT_TEST_EXPECT_TRUE(NormalizeDirectoryPath("c:\\a/b\\c\\d//") == "C:/a/b/c/d");
#else
    UNIT_TEST_EXPECT_TRUE(NormalizeDirectoryPath("c:\\a/b\\c\\d\\") == "c:/a/b/c/d");
    UNIT_TEST_EXPECT_TRUE(NormalizeDirectoryPath("c:\\a/b\\c\\d//") == "c:/a/b/c/d");
#endif // defined(AZ_PLATFORM_WINDOWS)

    QTemporaryDir tempdir;
    QDir dir(tempdir.path());
    QString fileName (dir.filePath("test.txt"));
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
    UNIT_TEST_EXPECT_TRUE(AssetUtilities::MakeFileWritable(fileName));

    // ------------- Test NormalizeAndRemoveAlias --------------

    UNIT_TEST_EXPECT_TRUE(AssetUtilities::NormalizeAndRemoveAlias("@test@\\my\\file.txt") == QString("my/file.txt"));
    UNIT_TEST_EXPECT_TRUE(AssetUtilities::NormalizeAndRemoveAlias("@test@my\\file.txt") == QString("my/file.txt"));
    UNIT_TEST_EXPECT_TRUE(AssetUtilities::NormalizeAndRemoveAlias("@TeSt@my\\file.txt") == QString("my/file.txt")); // case sensitivity test!


    //-----------------------Test CopyFileWithTimeout---------------------

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
        UNIT_TEST_EXPECT_FALSE(CopyFileWithTimeout(fileName, outputFileName, 1));
        UNIT_TEST_EXPECT_TRUE(absorb.m_numWarningsAbsorbed == 2); // 2 for each fail
        //Trying to move when the output file is open for reading
        UNIT_TEST_EXPECT_FALSE(MoveFileWithTimeout(fileName, outputFileName, 1));
        UNIT_TEST_EXPECT_TRUE(absorb.m_numWarningsAbsorbed == 4);
    }
#endif // AZ_PLATFORM_WINDOWS ONLY

    inputFile.close();
    outputFile.close();

    //Trying to copy when the output file is not open
    UNIT_TEST_EXPECT_TRUE(CopyFileWithTimeout(fileName, outputFileName, 1));
    UNIT_TEST_EXPECT_TRUE(CopyFileWithTimeout(fileName, outputFileName, aznumeric_caster(-1)));//invalid timeout time
    // Trying to move when the output file is not open
    UNIT_TEST_EXPECT_TRUE(MoveFileWithTimeout(fileName, outputFileName, 1));
    UNIT_TEST_EXPECT_TRUE(MoveFileWithTimeout(outputFileName, fileName, 1));

    AZStd::atomic_bool setupDone{ false };
    AssetProcessor::AutoThreadJoiner joiner(new AZStd::thread(
        [&]()
        {
            //opening file
            outputFile.open(QFile::WriteOnly);
            setupDone = true;
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(1000));
            //closing file
            outputFile.close();
        }));

    while (!setupDone.load())
    {
        QThread::msleep(1);
    }

    UNIT_TEST_EXPECT_TRUE(outputFile.isOpen());

    //Trying to copy when the output file is open,but will close before the timeout inputted
    {
        UnitTestUtils::AssertAbsorber absorb;
        UNIT_TEST_EXPECT_TRUE(CopyFileWithTimeout(fileName, outputFileName, 3));
#if defined(AZ_PLATFORM_WINDOWS)
        // only windows has an issue with moving files out that are in use.
        // other platforms do so without issue.
        UNIT_TEST_EXPECT_TRUE(absorb.m_numWarningsAbsorbed > 0);
#endif // windows platform.
    }
  
    // ------------- Test CheckCanLock --------------
    {
        QTemporaryDir lockTestTempDir;
        QDir lockTestDir(lockTestTempDir.path());
        QString lockTestFileName(lockTestDir.filePath("lockTest.txt"));

        UNIT_TEST_EXPECT_FALSE(AssetUtilities::CheckCanLock(lockTestFileName));

        CreateDummyFile(lockTestFileName);
        UNIT_TEST_EXPECT_TRUE(AssetUtilities::CheckCanLock(lockTestFileName));

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
        UNIT_TEST_EXPECT_FALSE(AssetUtilities::CheckCanLock(lockTestFileName));
        lockTestFile.close();
#else
        if (handle != -1)
        {
            close(handle);
        }
#endif // windows/other platforms ifdef 
    }

    // --------------- TEST BYTEARRAYSTREAM

    {
        AssetProcessor::ByteArrayStream stream;
        UNIT_TEST_EXPECT_TRUE(stream.CanSeek());
        UNIT_TEST_EXPECT_TRUE(stream.IsOpen());
        UNIT_TEST_EXPECT_TRUE(stream.GetLength() == 0);
        UNIT_TEST_EXPECT_TRUE(stream.GetCurPos() == 0);
        char tempReadBuffer[24];
        azstrcpy(tempReadBuffer, 24, "This is a Test String");
        UNIT_TEST_EXPECT_TRUE(stream.Read(100, tempReadBuffer) == 0);

        // reserving does not alter the length.
        stream.Reserve(128);
        UNIT_TEST_EXPECT_TRUE(stream.GetLength() == 0);
        UNIT_TEST_EXPECT_TRUE(stream.Write(7, tempReadBuffer) == 7);
        UNIT_TEST_EXPECT_TRUE(stream.GetCurPos() == 7);
        UNIT_TEST_EXPECT_TRUE(stream.GetLength() == 7);
        UNIT_TEST_EXPECT_TRUE(memcmp(stream.GetArray().constData(), tempReadBuffer, 7) == 0);
        UNIT_TEST_EXPECT_TRUE(stream.Write(7, tempReadBuffer) == 7);
        UNIT_TEST_EXPECT_TRUE(stream.GetLength() == 14);
        UNIT_TEST_EXPECT_TRUE(stream.GetCurPos() == 14);
        UNIT_TEST_EXPECT_TRUE(memcmp(stream.GetArray().constData(), "This isThis is", 14) == 0);


        stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN); // write at  begin, without overrunning
        UNIT_TEST_EXPECT_TRUE(stream.GetCurPos() == 0);
        UNIT_TEST_EXPECT_TRUE(stream.Write(4, "that") == 4);
        UNIT_TEST_EXPECT_TRUE(stream.GetLength() == 14);
        UNIT_TEST_EXPECT_TRUE(stream.GetCurPos() == 4);
        UNIT_TEST_EXPECT_TRUE(memcmp(stream.GetArray().constData(), "that isThis is", 14) == 0);

        stream.Seek(2, AZ::IO::GenericStream::ST_SEEK_CUR); // write in middle, without overrunning
        UNIT_TEST_EXPECT_TRUE(stream.GetCurPos() == 6);
        UNIT_TEST_EXPECT_TRUE(stream.Write(4, "1234") == 4);
        UNIT_TEST_EXPECT_TRUE(stream.GetLength() == 14);
        UNIT_TEST_EXPECT_TRUE(stream.GetCurPos() == 10);
        UNIT_TEST_EXPECT_TRUE(memcmp(stream.GetArray().constData(), "that i1234s is", 14) == 0);

        stream.Seek(-6, AZ::IO::GenericStream::ST_SEEK_END); // write in end, negative offset, without overrunning
        UNIT_TEST_EXPECT_TRUE(stream.GetCurPos() == 8);
        UNIT_TEST_EXPECT_TRUE(stream.Write(4, "5555") == 4);
        UNIT_TEST_EXPECT_TRUE(stream.GetCurPos() == 12);
        UNIT_TEST_EXPECT_TRUE(stream.GetLength() == 14);
        UNIT_TEST_EXPECT_TRUE(memcmp(stream.GetArray().constData(), "that i125555is", 14) == 0);

        stream.Seek(2, AZ::IO::GenericStream::ST_SEEK_BEGIN); // write at begin offset, with overrun:
        UNIT_TEST_EXPECT_TRUE(stream.GetCurPos() == 2);
        UNIT_TEST_EXPECT_TRUE(stream.GetLength() == 14);
        UNIT_TEST_EXPECT_TRUE(stream.Write(14, "xxxxxxxxxxxxxx") == 14);
        UNIT_TEST_EXPECT_TRUE(stream.GetLength() == 16);
        UNIT_TEST_EXPECT_TRUE(stream.GetCurPos() == 16);
        UNIT_TEST_EXPECT_TRUE(memcmp(stream.GetArray().constData(), "thxxxxxxxxxxxxxx", 16) == 0);

        stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        stream.Seek(14, AZ::IO::GenericStream::ST_SEEK_CUR); // write in middle, with overrunning:
        UNIT_TEST_EXPECT_TRUE(stream.GetCurPos() == 14);
        UNIT_TEST_EXPECT_TRUE(stream.GetLength() == 16);
        UNIT_TEST_EXPECT_TRUE(stream.Write(4, "yyyy") == 4);
        UNIT_TEST_EXPECT_TRUE(stream.GetCurPos() == 18);
        UNIT_TEST_EXPECT_TRUE(stream.GetLength() == 18);
        UNIT_TEST_EXPECT_TRUE(memcmp(stream.GetArray().constData(), "thxxxxxxxxxxxxyyyy", 18) == 0);

        stream.Seek(-2, AZ::IO::GenericStream::ST_SEEK_END); // write in end, negative offset, with overrunning
        UNIT_TEST_EXPECT_TRUE(stream.GetCurPos() == 16);
        UNIT_TEST_EXPECT_TRUE(stream.GetLength() == 18);
        UNIT_TEST_EXPECT_TRUE(stream.Write(4, "ZZZZ") == 4);
        UNIT_TEST_EXPECT_TRUE(stream.GetCurPos() == 20);
        UNIT_TEST_EXPECT_TRUE(stream.GetLength() == 20);
        UNIT_TEST_EXPECT_TRUE(memcmp(stream.GetArray().constData(), "thxxxxxxxxxxxxyyZZZZ", 20) == 0);

        // read test.
        stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        UNIT_TEST_EXPECT_TRUE(stream.Read(20, tempReadBuffer) == 20);
        UNIT_TEST_EXPECT_TRUE(memcmp(tempReadBuffer, "thxxxxxxxxxxxxyyZZZZ", 20) == 0);
        UNIT_TEST_EXPECT_TRUE(stream.Read(20, tempReadBuffer) == 0); // because its already at end.
        UNIT_TEST_EXPECT_TRUE(memcmp(tempReadBuffer, "thxxxxxxxxxxxxyyZZZZ", 20) == 0); // it should not have disturbed the buffer.
        stream.Seek(2, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        UNIT_TEST_EXPECT_TRUE(stream.Read(20, tempReadBuffer) == 18);
        UNIT_TEST_EXPECT_TRUE(memcmp(tempReadBuffer, "xxxxxxxxxxxxyyZZZZZZ", 20) == 0); // it should not have disturbed the buffer bits that it was not asked to touch.
    }

    // --------------- TEST FilePatternMatcher
    {
        {
            AssetBuilderSDK::FilePatternMatcher extensionWildcardTest(AssetBuilderSDK::AssetBuilderPattern("*.cfg", AssetBuilderSDK::AssetBuilderPattern::Wildcard));
            UNIT_TEST_EXPECT_TRUE(extensionWildcardTest.MatchesPath(AZStd::string("foo.cfg")));
            UNIT_TEST_EXPECT_TRUE(extensionWildcardTest.MatchesPath(AZStd::string("abcd/foo.cfg")));
            UNIT_TEST_EXPECT_FALSE(extensionWildcardTest.MatchesPath(AZStd::string("abcd/foo.cfd")));
        }

        {
            AssetBuilderSDK::FilePatternMatcher prefixWildcardTest(AssetBuilderSDK::AssetBuilderPattern("abf*.llm", AssetBuilderSDK::AssetBuilderPattern::Wildcard));
            UNIT_TEST_EXPECT_TRUE(prefixWildcardTest.MatchesPath(AZStd::string("abf.llm")));
            UNIT_TEST_EXPECT_TRUE(prefixWildcardTest.MatchesPath(AZStd::string("abf12345.llm")));
            UNIT_TEST_EXPECT_FALSE(prefixWildcardTest.MatchesPath(AZStd::string("foo/abf12345.llm")));
            UNIT_TEST_EXPECT_FALSE(prefixWildcardTest.MatchesPath(AZStd::string("foo/abf12345.lls")));
            UNIT_TEST_EXPECT_FALSE(prefixWildcardTest.MatchesPath(AZStd::string("foo/ab2345.llm")));
        }

        {
            AssetBuilderSDK::FilePatternMatcher extensionPrefixWildcardTest(AssetBuilderSDK::AssetBuilderPattern("sdf.c*", AssetBuilderSDK::AssetBuilderPattern::Wildcard));
            UNIT_TEST_EXPECT_TRUE(extensionPrefixWildcardTest.MatchesPath(AZStd::string("sdf.cpp")));
            UNIT_TEST_EXPECT_TRUE(extensionPrefixWildcardTest.MatchesPath(AZStd::string("sdf.cxx")));
            UNIT_TEST_EXPECT_TRUE(extensionPrefixWildcardTest.MatchesPath(AZStd::string("sdf.c")));
            UNIT_TEST_EXPECT_FALSE(extensionPrefixWildcardTest.MatchesPath(AZStd::string("abcd/sdf.cpp")));
            UNIT_TEST_EXPECT_FALSE(extensionPrefixWildcardTest.MatchesPath(AZStd::string("s:\\asd/abcd/sdf.cpp")));
            UNIT_TEST_EXPECT_FALSE(extensionPrefixWildcardTest.MatchesPath(AZStd::string("sdc.c")));
            UNIT_TEST_EXPECT_FALSE(extensionPrefixWildcardTest.MatchesPath(AZStd::string("sdf.hxx")));
            UNIT_TEST_EXPECT_FALSE(extensionPrefixWildcardTest.MatchesPath(AZStd::string("s:\\asd/abcd/sdf.hxx")));
        }

        {
            AssetBuilderSDK::FilePatternMatcher prefixExtensionPrefixWildcardTest(AssetBuilderSDK::AssetBuilderPattern("s*.c*", AssetBuilderSDK::AssetBuilderPattern::Wildcard));
            UNIT_TEST_EXPECT_TRUE(prefixExtensionPrefixWildcardTest.MatchesPath(AZStd::string("sdf.cpp")));
            UNIT_TEST_EXPECT_TRUE(prefixExtensionPrefixWildcardTest.MatchesPath(AZStd::string("sdf.cxx")));
            UNIT_TEST_EXPECT_FALSE(prefixExtensionPrefixWildcardTest.MatchesPath(AZStd::string("abcd/sdf.cpp")));
            UNIT_TEST_EXPECT_FALSE(prefixExtensionPrefixWildcardTest.MatchesPath(AZStd::string("c:\\asd/abcd/sdf.cpp")));
            UNIT_TEST_EXPECT_FALSE(prefixExtensionPrefixWildcardTest.MatchesPath(AZStd::string("sdf.hxx")));
            UNIT_TEST_EXPECT_FALSE(prefixExtensionPrefixWildcardTest.MatchesPath(AZStd::string("s:\\asd/abcd/sdf.hxx")));
        }

        {
            AssetBuilderSDK::FilePatternMatcher fixedNameTest(AssetBuilderSDK::AssetBuilderPattern("a.bcd", AssetBuilderSDK::AssetBuilderPattern::Wildcard));
            UNIT_TEST_EXPECT_TRUE(fixedNameTest.MatchesPath(AZStd::string("a.bcd")));
            UNIT_TEST_EXPECT_FALSE(fixedNameTest.MatchesPath(AZStd::string("foo\\a.bcd")));
            UNIT_TEST_EXPECT_FALSE(fixedNameTest.MatchesPath(AZStd::string("foo/a.bcd")));
            UNIT_TEST_EXPECT_FALSE(fixedNameTest.MatchesPath(AZStd::string("c:/foo/a.bcd")));
            UNIT_TEST_EXPECT_FALSE(fixedNameTest.MatchesPath(AZStd::string("c:\\foo/a.bcd")));
            UNIT_TEST_EXPECT_FALSE(fixedNameTest.MatchesPath(AZStd::string("sdf.hxx")));
        }

        {
            AssetBuilderSDK::FilePatternMatcher midMatchExtensionPrefixTest(AssetBuilderSDK::AssetBuilderPattern("s*f.c*", AssetBuilderSDK::AssetBuilderPattern::Wildcard));
            UNIT_TEST_EXPECT_TRUE(midMatchExtensionPrefixTest.MatchesPath(AZStd::string("sdf.cpp")));
            UNIT_TEST_EXPECT_TRUE(midMatchExtensionPrefixTest.MatchesPath(AZStd::string("sef.cxx")));
            UNIT_TEST_EXPECT_TRUE(midMatchExtensionPrefixTest.MatchesPath(AZStd::string("sf.c")));
            UNIT_TEST_EXPECT_FALSE(midMatchExtensionPrefixTest.MatchesPath(AZStd::string("c:\\asd/abcd/sdf.cpp")));
            UNIT_TEST_EXPECT_FALSE(midMatchExtensionPrefixTest.MatchesPath(AZStd::string("abcd/sdf.cpp")));
            UNIT_TEST_EXPECT_FALSE(midMatchExtensionPrefixTest.MatchesPath(AZStd::string("sdc.c")));
            UNIT_TEST_EXPECT_FALSE(midMatchExtensionPrefixTest.MatchesPath(AZStd::string("sdf.hxx")));
            UNIT_TEST_EXPECT_FALSE(midMatchExtensionPrefixTest.MatchesPath(AZStd::string("s:\\asd/abcd/sdf.hxx")));
        }

        {
            AssetBuilderSDK::FilePatternMatcher subFolderExtensionWildcardTest(AssetBuilderSDK::AssetBuilderPattern("abcd/*.cfg", AssetBuilderSDK::AssetBuilderPattern::Wildcard));
            UNIT_TEST_EXPECT_TRUE(subFolderExtensionWildcardTest.MatchesPath(AZStd::string("abcd/sdf.cfg")));
            UNIT_TEST_EXPECT_FALSE(subFolderExtensionWildcardTest.MatchesPath(AZStd::string("c://abcd/sdf.cfg")));
            UNIT_TEST_EXPECT_FALSE(subFolderExtensionWildcardTest.MatchesPath(AZStd::string("sdf.cfg")));
            UNIT_TEST_EXPECT_FALSE(subFolderExtensionWildcardTest.MatchesPath(AZStd::string("abcs/sdf.cfg")));
            UNIT_TEST_EXPECT_FALSE(subFolderExtensionWildcardTest.MatchesPath(AZStd::string("abcd/sdf.cfx")));
        }

        {
            AssetBuilderSDK::FilePatternMatcher subFolderPatternTest(AssetBuilderSDK::AssetBuilderPattern(".*\\/savebackup\\/.*", AssetBuilderSDK::AssetBuilderPattern::Regex));
            UNIT_TEST_EXPECT_TRUE(subFolderPatternTest.MatchesPath(AZStd::string("abcd/savebackup/sdf.cfg")));
            UNIT_TEST_EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("abcd/savebackup")));
            UNIT_TEST_EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("savebackup/sdf.cfg")));
            UNIT_TEST_EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("c://abcd/sdf.cfg")));
            UNIT_TEST_EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("sdf.cfg")));
            UNIT_TEST_EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("abcs/sdf.cfg")));
            UNIT_TEST_EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("abcd/sdf.cfx")));
        }

        {
            AssetBuilderSDK::FilePatternMatcher subFolderPatternTest(AssetBuilderSDK::AssetBuilderPattern(".*\\/Presets\\/GeomCache\\/.*", AssetBuilderSDK::AssetBuilderPattern::Regex));
            UNIT_TEST_EXPECT_TRUE(subFolderPatternTest.MatchesPath(AZStd::string("something/Presets/GeomCache/sdf.cfg")));
            UNIT_TEST_EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("Presets/GeomCache/sdf.cfg"))); // should not match because it demands that there is a slash
            UNIT_TEST_EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("abcd/savebackup")));
            UNIT_TEST_EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("savebackup/sdf.cfg")));
            UNIT_TEST_EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("c://abcd/sdf.cfg")));
            UNIT_TEST_EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("sdf.cfg")));
            UNIT_TEST_EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("abcs/sdf.cfg")));
            UNIT_TEST_EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("abcd/sdf.cfx")));
        }
    }

    Q_EMIT UnitTestPassed();
}

class GetFileHashFromStream_NullPath_Returns0
    : public UnitTestRun
{
public:
    void StartTest() override
    {
        AZ::u64 result = GetFileHash(nullptr);
        UNIT_TEST_EXPECT_TRUE(result == 0);
        Q_EMIT UnitTestPassed();
    }
};

REGISTER_UNIT_TEST(GetFileHashFromStream_NullPath_Returns0)

class GetFileHashFromStreamSmallFile_ReturnsExpectedHash
    : public UnitTestRun
{
public:
    void StartTest() override
    {
        QTemporaryDir tempdir;
        QDir dir(tempdir.path());
        QString fileName(dir.filePath("test.txt"));
        CreateDummyFile(fileName);
        AZ::u64 result = GetFileHash(fileName.toUtf8());
        UNIT_TEST_EXPECT_TRUE(result == AZ::u64(17241709254077376921ul));
        Q_EMIT UnitTestPassed();
    }
};
REGISTER_UNIT_TEST(GetFileHashFromStreamSmallFile_ReturnsExpectedHash)

class GetFileHashFromStream_SmallFileForced_ReturnsExpectedHash
    : public UnitTestRun
{
public:
    void StartTest() override
    {
        QTemporaryDir tempdir;
        QDir dir(tempdir.path());
        QString fileName(dir.filePath("test.txt"));
        CreateDummyFile(fileName);
        AZ::u64 result = GetFileHash(fileName.toUtf8(), true);
        UNIT_TEST_EXPECT_TRUE(result == AZ::u64(17241709254077376921ul));
        Q_EMIT UnitTestPassed();
    }
};

REGISTER_UNIT_TEST(GetFileHashFromStream_SmallFileForced_ReturnsExpectedHash)

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
class GetFileHashFromStream_LargeFileForcedAnotherThreadWritesToFile_ReturnsExpectedHash
    : public UnitTestRun
    , public AZ::Debug::TraceMessageBus::Handler
{
public:
    void StartTest() override
    {
        AZ::Debug::TraceMessageBus::Handler::BusConnect();
        QTemporaryDir tempdir;
        QDir dir(tempdir.path());
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
        job->Start();

        // Use an artificial delay on hashing to ensure the race condition actually occurs.
        AZ::u64 result =
            GetFileHash(fileName.toUtf8(), true, nullptr, /*hashMsDelay*/ 20);
        // This test will result in different hash results on different machines, because writing to the stream
        // and reading from the stream to generate the hash happen at different speeds in different setups.
        // Just make sure it returns some result here.
        UNIT_TEST_EXPECT_TRUE(result != 0);
        UNIT_TEST_EXPECT_FALSE(m_assertTriggered);
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
        Q_EMIT UnitTestPassed();
    }

    bool OnPreAssert(const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* /*message*/) override
    {
        m_assertTriggered = true;
        return true;
    }
    bool m_assertTriggered = false;
};

REGISTER_UNIT_TEST(GetFileHashFromStream_LargeFileForcedAnotherThreadWritesToFile_ReturnsExpectedHash)

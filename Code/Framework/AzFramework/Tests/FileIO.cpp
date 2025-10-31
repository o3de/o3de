/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/string/string.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/IO/FileOperations.h>
#include <time.h>
#include <AzTest/Utils.h>

#include <AzFrameworkTests_Traits_Platform.h>

#if AZ_TRAIT_USE_WINDOWS_FILE_API
#include <sys/stat.h>
#include <io.h>
#endif

using namespace AZ;
using namespace AZ::IO;
using namespace AZ::Debug;


namespace UnitTest
{
    class NameMatchesFilterTest
        : public LeakDetectionFixture
    {
    public:
        void run()
        {
            AZ_TEST_ASSERT(NameMatchesFilter("hello", "hello") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("hello", "he?l?") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("hello", "he???") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("hello", "he*") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("hello", "he*o") == true);

            AZ_TEST_ASSERT(NameMatchesFilter("hello", "?*?o") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("hello", "h?*?") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("hello", "h?*?o") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("hello", "h?*?o?") == false);
            AZ_TEST_ASSERT(NameMatchesFilter("hello", "h***o*") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("something", "some??") == false);

            AZ_TEST_ASSERT(NameMatchesFilter("hello", "?????*") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("hello", "????*") == true);

            AZ_TEST_ASSERT(NameMatchesFilter("hello", "h??*") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("hello", "??L*") == true);

            AZ_TEST_ASSERT(NameMatchesFilter("anything", "**") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("any.thing", "*") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("anything", "") == false);
            AZ_TEST_ASSERT(NameMatchesFilter("system.pak", "*.pak") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("system.pakx", "*.pak") == false);
            AZ_TEST_ASSERT(NameMatchesFilter("system.pa", "*.pak") == false);
            AZ_TEST_ASSERT(NameMatchesFilter("system.pak.3", "*.pak.*") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("system.pa.pak", "*.pak") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("log1234.log", "log????.log") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("log1234.log", "log?????.log") == false);
            AZ_TEST_ASSERT(NameMatchesFilter("log151234.log", "log*.log") == true);
            AZ_TEST_ASSERT(NameMatchesFilter(".pak", "*.pak") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("", "*.pak") == false);
            AZ_TEST_ASSERT(NameMatchesFilter("", "") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("test.test", "????.????") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("testatest", "????.????") == false);
        }
    };

    TEST_F(NameMatchesFilterTest, Test)
    {
        run();
    }

    /**
     * FileIOStream test
     */
    class FileIOStreamTest
        : public LeakDetectionFixture
    {
    public:
        AZ::IO::LocalFileIO m_fileIO;
        AZ::IO::FileIOBase* m_prevFileIO;

        FileIOStreamTest()
        {
        }

        void SetUp() override
        {
            LeakDetectionFixture::SetUp();
            m_prevFileIO = AZ::IO::FileIOBase::GetInstance();
            AZ::IO::FileIOBase::SetInstance(&m_fileIO);
        }

        ~FileIOStreamTest() override
        {
        }

        void TearDown() override
        {
            AZ::IO::FileIOBase::SetInstance(m_prevFileIO);
            LeakDetectionFixture::TearDown();
        }

        void run()
        {
            AZ::Test::ScopedAutoTempDirectory tempDir;

            char fileIOTestPath[AZ::IO::MaxPathLength];
            azsnprintf(fileIOTestPath, AZ::IO::MaxPathLength, "%s/fileiotest.txt", tempDir.GetDirectory());

            FileIOStream stream(fileIOTestPath, AZ::IO::OpenMode::ModeWrite);
            AZ_TEST_ASSERT(stream.IsOpen());
            char output[256];
            azsnprintf(output, sizeof(output), "magic string");
            AZ_TEST_ASSERT(strlen(output) + 1 == stream.Write(strlen(output) + 1, output));
            stream.Close();

            stream.Open(fileIOTestPath, AZ::IO::OpenMode::ModeRead);
            AZ_TEST_ASSERT(stream.IsOpen());
            AZ_TEST_ASSERT(strlen(output) + 1 == stream.Read(strlen(output) + 1, output));
            AZ_TEST_ASSERT(strcmp(output, "magic string") == 0);
            stream.Close();
        }
    };

    TEST_F(FileIOStreamTest, Test)
    {
        run();
    }

    namespace LocalFileIOTest
    {
        class FolderFixture
            : public LeakDetectionFixture
        {
        public:
            AZ::Test::ScopedAutoTempDirectory m_tempDir;
            AZ::IO::Path m_root;
            AZ::IO::Path m_folderName;
            AZ::IO::Path m_deepFolder;
            AZ::IO::Path m_extraFolder;

            AZ::IO::Path m_fileRoot;
            AZ::IO::Path m_file01Name;
            AZ::IO::Path m_file02Name;
            AZ::IO::Path m_file03Name;
            int m_randomFolderKey = 0;

            FolderFixture()
            {
            }


            void ChooseRandomFolder()
            {
                m_root =  m_tempDir.GetDirectory();
                m_folderName = m_root / AZStd::string::format("tmp%08x", m_randomFolderKey);
                m_deepFolder = m_folderName / "test"  / "subdir";
                m_extraFolder = m_deepFolder / "subdir2";

                // make a couple files there, and in the root:
                m_fileRoot = m_extraFolder;
            }

            void SetUp() override
            {
                // lets use a random temp folder name
                srand(static_cast<unsigned int>(clock()));
                m_randomFolderKey = rand();

                LocalFileIO local;
                do
                {
                    ChooseRandomFolder();
                    ++m_randomFolderKey;
                } while (local.IsDirectory(m_fileRoot.c_str()));

                m_file01Name = m_fileRoot / "file01.txt";
                m_file02Name = m_fileRoot / "file02.asdf";
                m_file03Name = m_fileRoot / "test123.wha";
            }

            void TearDown() override
            {
            }
            void CreateTestFiles()
            {
                constexpr auto openMode = SystemFile::OpenMode::SF_OPEN_WRITE_ONLY
                    | SystemFile::OpenMode::SF_OPEN_CREATE
                    | SystemFile::OpenMode::SF_OPEN_CREATE_NEW;
                constexpr AZStd::string_view testContent("this is just a test");

                LocalFileIO local;
                AZ_TEST_ASSERT(local.CreatePath(m_fileRoot.c_str()));
                AZ_TEST_ASSERT(local.IsDirectory(m_fileRoot.c_str()));
                for (const AZ::IO::Path& filename : { m_file01Name, m_file02Name, m_file03Name })
                {
                    SystemFile tempFile;
                    tempFile.Open(filename.c_str(), openMode);

                    tempFile.Write(testContent.data(), testContent.size());
                    tempFile.Close();
                }
            }
        };

        class DirectoryTest
            : public FolderFixture
        {
        public:
            void run()
            {
                LocalFileIO local;

                AZ_TEST_ASSERT(!local.Exists(m_folderName.c_str()));

                AZ::IO::Path longPathCreateTest = m_folderName / "one" / "two" / "three";

                AZ_TEST_ASSERT(!local.Exists(longPathCreateTest.c_str()));
                AZ_TEST_ASSERT(!local.IsDirectory(longPathCreateTest.c_str()));
                AZ_TEST_ASSERT(local.CreatePath(longPathCreateTest.c_str()));
                AZ_TEST_ASSERT(local.IsDirectory(longPathCreateTest.c_str()));

                AZ_TEST_ASSERT(!local.Exists(m_deepFolder.c_str()));
                AZ_TEST_ASSERT(!local.IsDirectory(m_deepFolder.c_str()));
                AZ_TEST_ASSERT(local.CreatePath(m_deepFolder.c_str()));
                AZ_TEST_ASSERT(local.IsDirectory(m_deepFolder.c_str()));

                AZ_TEST_ASSERT(local.Exists(m_deepFolder.c_str()));
                AZ_TEST_ASSERT(local.CreatePath(m_deepFolder.c_str()));
                AZ_TEST_ASSERT(local.Exists(m_deepFolder.c_str()));
            }
        };

        TEST_F(DirectoryTest, Test)
        {
            run();
        }

        class ReadWriteTest
            : public FolderFixture
        {
        public:
            void run()
            {
                LocalFileIO local;

                AZ_TEST_ASSERT(!local.Exists(m_fileRoot.c_str()));
                AZ_TEST_ASSERT(!local.IsDirectory(m_fileRoot.c_str()));
                AZ_TEST_ASSERT(local.CreatePath(m_fileRoot.c_str()));
                AZ_TEST_ASSERT(local.IsDirectory(m_fileRoot.c_str()));

                constexpr auto openMode = SystemFile::OpenMode::SF_OPEN_WRITE_ONLY
                    | SystemFile::OpenMode::SF_OPEN_CREATE
                    | SystemFile::OpenMode::SF_OPEN_CREATE_NEW;
                SystemFile tempFile;
                tempFile.Open(m_file01Name.c_str(), openMode);
                constexpr AZStd::string_view testContent("this is just a test");
                tempFile.Write(testContent.data(), testContent.size());
                tempFile.Close();

                AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
                AZ_TEST_ASSERT(!local.Open("", AZ::IO::OpenMode::ModeWrite, fileHandle));
                AZ_TEST_ASSERT(fileHandle == AZ::IO::InvalidHandle);

                // test size without opening:
                AZ::u64 fs = 0;
                AZ_TEST_ASSERT(local.Size(m_file01Name.c_str(), fs));
                AZ_TEST_ASSERT(fs == 19);

                fileHandle = AZ::IO::InvalidHandle;

                AZ::u64 modTimeA = local.ModificationTime(m_file01Name.c_str());
                AZ_TEST_ASSERT(modTimeA != 0);

                // test invalid handle ops:
                AZ_TEST_ASSERT(!local.Seek(fileHandle, 0, AZ::IO::SeekType::SeekFromStart));
                AZ_TEST_ASSERT(!local.Close(fileHandle));
                AZ_TEST_ASSERT(!local.Eof(fileHandle));
                AZ_TEST_ASSERT(!local.Flush(fileHandle));
                AZ_TEST_ASSERT(!local.ModificationTime(fileHandle));
                AZ_TEST_ASSERT(!local.Read(fileHandle, nullptr, 0, false));
                AZ_TEST_ASSERT(!local.Tell(fileHandle, fs));

                AZ_TEST_ASSERT(!local.Exists((m_file01Name.Native() + "notexist").c_str()));

                AZ_TEST_ASSERT(local.Exists(m_file01Name.c_str()));
                AZ_TEST_ASSERT(!local.IsReadOnly(m_file01Name.c_str()));
                AZ_TEST_ASSERT(!local.IsDirectory(m_file01Name.c_str()));

                // test reads and seeks.
                AZ_TEST_ASSERT(local.Open(m_file01Name.c_str(), AZ::IO::OpenMode::ModeRead, fileHandle));
                AZ_TEST_ASSERT(fileHandle != AZ::IO::InvalidHandle);

                // use this again later...
                AZ::u64 modTimeB = local.ModificationTime(fileHandle);
                AZ_TEST_ASSERT(modTimeB != 0);

                static const size_t testStringLen = 256;
                char testString[testStringLen] = { 0 };

                // test size on open handle:
                fs = 0;
                AZ_TEST_ASSERT(local.Size(fileHandle, fs));
                AZ_TEST_ASSERT(fs == 19);

                // test size without opening, after its already open:
                fs = 0;
                AZ_TEST_ASSERT(local.Size(m_file01Name.c_str(), fs));
                AZ_TEST_ASSERT(fs == 19);

                AZ::u64 offs = 0;
                AZ_TEST_ASSERT(local.Tell(fileHandle, offs));
                AZ_TEST_ASSERT(offs == 0);
                AZ_TEST_ASSERT(local.Seek(fileHandle, 5, AZ::IO::SeekType::SeekFromStart));
                AZ_TEST_ASSERT(!local.Eof(fileHandle));
                AZ::u64 actualBytesRead = 0;
                // situation
                // this is just a test
                //      ^-------------
                // 15 chars
                AZ_TEST_ASSERT(local.Tell(fileHandle, offs));
                AZ_TEST_ASSERT(offs == 5);
                AZ_TEST_ASSERT(!local.Eof(fileHandle));
                AZ_TEST_ASSERT(local.Read(fileHandle, testString, testStringLen, false, &actualBytesRead));
                AZ_TEST_ASSERT(actualBytesRead == 14);
                AZ_TEST_ASSERT(strncmp(testString, "is just a test", 14) == 0);
                AZ_TEST_ASSERT(local.Eof(fileHandle));

                // this is just a test
                //                    ^
                AZ_TEST_ASSERT(local.Seek(fileHandle, -5, AZ::IO::SeekType::SeekFromCurrent));
                // this is just a test
                //               ^----
                AZ_TEST_ASSERT(local.Tell(fileHandle, offs));
                AZ_TEST_ASSERT(offs == 14);
                AZ_TEST_ASSERT(!local.Eof(fileHandle));
                AZ_TEST_ASSERT(local.Read(fileHandle, testString, testStringLen, false, &actualBytesRead));
                AZ_TEST_ASSERT(actualBytesRead == 5);
                AZ_TEST_ASSERT(strncmp(testString, " test", 5) == 0);
                AZ_TEST_ASSERT(local.Eof(fileHandle));
                // this is just a test
                //                    ^
                AZ_TEST_ASSERT(local.Seek(fileHandle, -6, AZ::IO::SeekType::SeekFromEnd));
                // this is just a test
                //              ^---
                AZ_TEST_ASSERT(local.Tell(fileHandle, offs));
                AZ_TEST_ASSERT(offs == 13);
                AZ_TEST_ASSERT(!local.Eof(fileHandle));
                AZ_TEST_ASSERT(local.Read(fileHandle, testString, 4, true, &actualBytesRead));
                AZ_TEST_ASSERT(actualBytesRead == 4);
                AZ_TEST_ASSERT(strncmp(testString, "a te", 4) == 0);
                AZ_TEST_ASSERT(local.Tell(fileHandle, offs));
                AZ_TEST_ASSERT(offs == 17);
                AZ_TEST_ASSERT(!local.Eof(fileHandle));

                // fail when not enough bytes:
                AZ_TEST_ASSERT(!local.Read(fileHandle, testString, testStringLen, true, &actualBytesRead));
                AZ_TEST_ASSERT(local.Eof(fileHandle));
                AZ_TEST_ASSERT(local.Close(fileHandle));
                fileHandle = AZ::IO::InvalidHandle;
            }
        };

        TEST_F(ReadWriteTest, Test)
        {
            run();
        }

        class PermissionsTest
            : public FolderFixture
        {
        public:
            void run()
            {
                LocalFileIO local;

                CreateTestFiles();

#if AZ_TRAIT_AZFRAMEWORKTEST_PERFORM_CHMOD_TEST

#if AZ_TRAIT_USE_WINDOWS_FILE_API
                _chmod(m_file01Name.c_str(), _S_IREAD);
#else
                chmod(m_file01Name.c_str(), S_IRUSR | S_IRGRP | S_IROTH);
#endif

                AZ_TEST_ASSERT(local.IsReadOnly(m_file01Name.c_str()));

#if AZ_TRAIT_USE_WINDOWS_FILE_API
                _chmod(m_file01Name.c_str(), _S_IREAD | _S_IWRITE);
#else
                chmod(m_file01Name.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
#endif

#endif

                AZ_TEST_ASSERT(!local.IsReadOnly(m_file01Name.c_str()));
            }
        };

        TEST_F(PermissionsTest, Test)
        {
            run();
        }

        class CopyMoveTests
            : public FolderFixture
        {
        public:
            void run()
            {
                LocalFileIO local;

                AZ_TEST_ASSERT(local.CreatePath(m_fileRoot.c_str()));
                AZ_TEST_ASSERT(local.IsDirectory(m_fileRoot.c_str()));
                {
#ifdef AZ_COMPILER_MSVC
                    FILE* tempFile;
                    fopen_s(&tempFile, m_file01Name.c_str(), "wb");
#else
                    FILE* tempFile = fopen(m_file01Name.c_str(), "wb");
#endif
                    fwrite("this is just a test", 1, 19, tempFile);
                    fclose(tempFile);
                }

                // make sure attributes are copied, such as original modtime:
                AZStd::this_thread::sleep_for(AZStd::chrono::seconds(1));
                AZ_TEST_ASSERT(local.Copy(m_file01Name.c_str(), m_file02Name.c_str()));
                AZ_TEST_ASSERT(local.Copy(m_file01Name.c_str(), m_file03Name.c_str()));

                AZ_TEST_ASSERT(local.Exists(m_file01Name.c_str()));
                AZ_TEST_ASSERT(local.Exists(m_file02Name.c_str()));
                AZ_TEST_ASSERT(local.Exists(m_file03Name.c_str()));
                AZ_TEST_ASSERT(!local.DestroyPath(m_file01Name.c_str())); // you may not destroy files.
                AZ_TEST_ASSERT(!local.DestroyPath(m_file02Name.c_str()));
                AZ_TEST_ASSERT(!local.DestroyPath(m_file03Name.c_str()));
                AZ_TEST_ASSERT(local.Exists(m_file01Name.c_str()));
                AZ_TEST_ASSERT(local.Exists(m_file02Name.c_str()));
                AZ_TEST_ASSERT(local.Exists(m_file03Name.c_str()));

                AZ::u64 f1s = 0;
                AZ::u64 f2s = 0;
                AZ::u64 f3s = 0;
                AZ_TEST_ASSERT(local.Size(m_file01Name.c_str(), f1s));
                AZ_TEST_ASSERT(local.Size(m_file02Name.c_str(), f2s));
                AZ_TEST_ASSERT(local.Size(m_file03Name.c_str(), f3s));
                AZ_TEST_ASSERT(f1s == f2s);
                AZ_TEST_ASSERT(f1s == f3s);

                // Copying over top other files is allowed

                SystemFile file;
                EXPECT_TRUE(file.Open(m_file01Name.c_str(), SystemFile::SF_OPEN_WRITE_ONLY));
                file.Write("this is just a test that is longer", 34);
                file.Close();

                EXPECT_TRUE(local.Copy(m_file01Name.c_str(), m_file02Name.c_str()));

                f1s = 0;
                f2s = 0;
                f3s = 0;
                EXPECT_TRUE(local.Size(m_file01Name.c_str(), f1s));
                EXPECT_TRUE(local.Size(m_file02Name.c_str(), f2s));
                EXPECT_TRUE(local.Size(m_file03Name.c_str(), f3s));
                EXPECT_EQ(f1s, f2s);
                EXPECT_NE(f1s, f3s);
            }
        };

        TEST_F(CopyMoveTests, Test)
        {
            run();
        }

        class ModTimeTest
            : public FolderFixture
        {
        public:
            void run()
            {
                AZ::IO::LocalFileIO local;

                CreateTestFiles();

                AZ::u64 modTimeC = 0;
                AZ::u64 modTimeD = 0;
                modTimeC = local.ModificationTime(m_file02Name.c_str());
                modTimeD = local.ModificationTime(m_file03Name.c_str());

                // make sure modtimes are in ascending order (at least)
                AZ_TEST_ASSERT(modTimeD >= modTimeC);

                // now touch some of the files.   This is also how we test append mode, and write mode.
                AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
                AZ_TEST_ASSERT(local.Open(m_file02Name.c_str(), AZ::IO::OpenMode::ModeAppend | AZ::IO::OpenMode::ModeBinary, fileHandle));
                AZ_TEST_ASSERT(fileHandle != AZ::IO::InvalidHandle);
                AZ_TEST_ASSERT(local.Write(fileHandle, "more", 4));
                AZ_TEST_ASSERT(local.Close(fileHandle));

                // No-append-mode
                AZ_TEST_ASSERT(local.Open(m_file03Name.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary, fileHandle));
                AZ_TEST_ASSERT(fileHandle != AZ::IO::InvalidHandle);
                AZ_TEST_ASSERT(local.Write(fileHandle, "more", 4));
                AZ_TEST_ASSERT(local.Close(fileHandle));

                modTimeC = local.ModificationTime(m_file02Name.c_str());
                modTimeD = local.ModificationTime(m_file03Name.c_str());

                AZ_TEST_ASSERT(modTimeD >= modTimeC); // should never be descending

                AZ::u64 f1s = 0;
                AZ::u64 f2s = 0;
                AZ::u64 f3s = 0;
                AZ_TEST_ASSERT(local.Size(m_file01Name.c_str(), f1s));
                AZ_TEST_ASSERT(local.Size(m_file02Name.c_str(), f2s));
                AZ_TEST_ASSERT(local.Size(m_file03Name.c_str(), f3s));
                AZ_TEST_ASSERT(f2s == f1s + 4);
                AZ_TEST_ASSERT(f3s == 4);
            }
        };

        TEST_F(ModTimeTest, Test)
        {
            run();
        }

        class FindFilesTest
            : public FolderFixture
        {
        public:
            void run()
            {
                AZ::IO::LocalFileIO local;

                CreateTestFiles();

                AZStd::vector<AZ::IO::Path> resultFiles;
                bool foundOK = local.FindFiles(m_fileRoot.c_str(), "*",
                        [&](const char* filePath) -> bool
                        {
                            resultFiles.push_back(filePath);
                            return false; // early out!
                        });

                AZ_TEST_ASSERT(foundOK);
                AZ_TEST_ASSERT(resultFiles.size() == 1);

                resultFiles.clear();

                foundOK = local.FindFiles(m_fileRoot.c_str(), "*",
                        [&](const char* filePath) -> bool
                        {
                            resultFiles.push_back(filePath);
                            return true; // continue iterating
                        });

                AZ_TEST_ASSERT(foundOK);
                AZ_TEST_ASSERT(resultFiles.size() == 3);

                // note: following tests accumulate more files without clearing resultfiles.
                foundOK = local.FindFiles(m_fileRoot.c_str(), "*.txt",
                        [&](const char* filePath) -> bool
                        {
                            resultFiles.push_back(filePath);
                            return true; // continue iterating
                        });

                AZ_TEST_ASSERT(foundOK);
                AZ_TEST_ASSERT(resultFiles.size() == 4);

                foundOK = local.FindFiles(m_fileRoot.c_str(), "file*.asdf",
                        [&](const char* filePath) -> bool
                        {
                            resultFiles.push_back(filePath);
                            return true; // continue iterating
                        });

                AZ_TEST_ASSERT(foundOK);
                AZ_TEST_ASSERT(resultFiles.size() == 5);

                foundOK = local.FindFiles(m_fileRoot.c_str(), "asaf.asdf",
                        [&](const char* filePath) -> bool
                        {
                            resultFiles.push_back(filePath);
                            return true; // continue iterating
                        });

                AZ_TEST_ASSERT(foundOK);
                AZ_TEST_ASSERT(resultFiles.size() == 5);

                resultFiles.clear();

                // test to make sure directories show up:
                foundOK = local.FindFiles(m_deepFolder.c_str(), "*",
                        [&](const char* filePath) -> bool
                        {
                            resultFiles.push_back(filePath);
                            return true; // continue iterating
                        });

                // canonicalize the name in the same way that find does.
                //AZStd::replace() m_extraFolder.replace('\\', '/');  FIXME PPATEL

                AZ_TEST_ASSERT(foundOK);
                AZ_TEST_ASSERT(resultFiles.size() == 1);
                AZ_TEST_ASSERT(resultFiles[0] == m_extraFolder);
                resultFiles.clear();
                foundOK = local.FindFiles("o:137787621!@#$%^&&**())_+[])_", "asaf.asdf",
                        [&](const char* filePath) -> bool
                        {
                            resultFiles.push_back(filePath);
                            return true; // continue iterating
                        });

                AZ_TEST_ASSERT(!foundOK);
                AZ_TEST_ASSERT(resultFiles.size() == 0);

                AZ::IO::Path file04Name = m_fileRoot / "test.wha";
                // test rename
                AZ_TEST_ASSERT(local.Rename(m_file03Name.c_str(), file04Name.c_str()));
                AZ_TEST_ASSERT(!local.Rename(m_file03Name.c_str(), file04Name.c_str()));
                AZ_TEST_ASSERT(local.Rename(file04Name.c_str(), file04Name.c_str())); // this is valid and ok
                AZ_TEST_ASSERT(local.Exists(file04Name.c_str()));
                AZ_TEST_ASSERT(!local.Exists(m_file03Name.c_str()));
                AZ_TEST_ASSERT(!local.IsDirectory(file04Name.c_str()));

                AZ::u64 f3s = 0;
                AZ_TEST_ASSERT(local.Size(file04Name.c_str(), f3s));
                AZ_TEST_ASSERT(f3s == 19);

                // deep destroy directory:
                AZ_TEST_ASSERT(local.DestroyPath(m_folderName.c_str()));
                AZ_TEST_ASSERT(!local.Exists(m_folderName.c_str()));
            }
        };

        TEST_F(FindFilesTest, Test)
        {
            run();
        }

        using AliasTest = FolderFixture;

        TEST_F(AliasTest, Test)
        {
            AZ::IO::LocalFileIO local;

            // test aliases
            local.SetAlias("@test@", m_folderName.c_str());
            const char* testDest1 = local.GetAlias("@test@");
            AZ_TEST_ASSERT(testDest1 != nullptr);
            const char* testDest2 = local.GetAlias("@NOPE@");
            AZ_TEST_ASSERT(testDest2 == nullptr);
            testDest1 = local.GetAlias("@test@"); // try with different case
            AZ_TEST_ASSERT(testDest1 != nullptr);

            // test resolving
            const char* aliasTestPath = "@test@\\some\\path\\somefile.txt";
            char aliasResolvedPath[AZ::IO::MaxPathLength];
            bool resolveDidWork = local.ResolvePath(aliasTestPath, aliasResolvedPath, AZ::IO::MaxPathLength);
            AZ_TEST_ASSERT(resolveDidWork);
            AZ::IO::Path expectedResolvedPath = m_folderName / "some/path/somefile.txt";
            AZ_TEST_ASSERT(aliasResolvedPath == expectedResolvedPath);

            // more resolve path tests with invalid inputs
            const char* testPath = nullptr;
            char* testResolvedPath = nullptr;
            resolveDidWork = local.ResolvePath(testPath, aliasResolvedPath, AZ::IO::MaxPathLength);
            AZ_TEST_ASSERT(!resolveDidWork);
            resolveDidWork = local.ResolvePath(aliasTestPath, testResolvedPath, AZ::IO::MaxPathLength);
            AZ_TEST_ASSERT(!resolveDidWork);
            resolveDidWork = local.ResolvePath(aliasTestPath, aliasResolvedPath, 0);
            AZ_TEST_ASSERT(!resolveDidWork);

            // Test that sending in a too small output path fails,
            // if the output buffer is smaller than the string being resolved
            size_t SMALLER_THAN_PATH_BEING_RESOLVED = strlen(aliasTestPath) - 1;
            AZ_TEST_START_TRACE_SUPPRESSION;
            resolveDidWork = local.ResolvePath(aliasTestPath, aliasResolvedPath, SMALLER_THAN_PATH_BEING_RESOLVED);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            AZ_TEST_ASSERT(!resolveDidWork);

            // Test that sending in a too small output path fails,
            // if the output buffer is too small to hold the resolved path
            size_t SMALLER_THAN_FINAL_RESOLVED_PATH = expectedResolvedPath.Native().length() - 1;
            AZ_TEST_START_TRACE_SUPPRESSION;
            resolveDidWork = local.ResolvePath(aliasTestPath, aliasResolvedPath, SMALLER_THAN_FINAL_RESOLVED_PATH);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            AZ_TEST_ASSERT(!resolveDidWork);

            // test clearing an alias
            local.ClearAlias("@test@");
            testDest1 = local.GetAlias("@test@");
            AZ_TEST_ASSERT(testDest1 == nullptr);;
        }

        TEST_F(AliasTest, ResolvePath_PathViewOverload_Succeeds)
        {
            AZ::IO::LocalFileIO local;
            local.SetAlias("@test@", m_folderName.c_str());
            AZ::IO::PathView aliasTestPath = "@test@\\some\\path\\somefile.txt";
            AZ::IO::FixedMaxPath aliasResolvedPath;
            ASSERT_TRUE(local.ResolvePath(aliasResolvedPath, aliasTestPath));
            AZ::IO::Path expectedResolvedPath = m_folderName / "some" / "path" / "somefile.txt";

            EXPECT_EQ(expectedResolvedPath, aliasResolvedPath);

            AZStd::optional<AZ::IO::FixedMaxPath> optionalResolvedPath = local.ResolvePath(aliasTestPath);
            ASSERT_TRUE(optionalResolvedPath);
            EXPECT_EQ(expectedResolvedPath, optionalResolvedPath.value());
        }

        TEST_F(AliasTest, ResolvePath_PathViewOverloadWithEmptyPath_Fails)
        {
            AZ::IO::LocalFileIO local;
            local.SetAlias("@test@", m_folderName.c_str());
            AZ::IO::FixedMaxPath aliasResolvedPath;
            EXPECT_FALSE(local.ResolvePath(aliasResolvedPath, {}));
        }

        TEST_F(AliasTest, ConvertToAlias_PathViewOverloadContainingExactAliasPath_Succeeds)
        {
            AZ::IO::LocalFileIO local;

            AZ::IO::FixedMaxPathString aliasFolder;
            EXPECT_TRUE(local.ConvertToAbsolutePath("/temp", aliasFolder.data(), aliasFolder.capacity()));
            aliasFolder.resize_no_construct(AZStd::char_traits<char>::length(aliasFolder.data()));
            local.SetAlias("@test@", aliasFolder.c_str());
            AZ::IO::FixedMaxPath aliasPath;
            ASSERT_TRUE(local.ConvertToAlias(aliasPath, AZ::IO::PathView(aliasFolder)));
            EXPECT_STREQ("@test@", aliasPath.c_str());

            AZStd::optional<AZ::IO::FixedMaxPath> optionalAliasPath = local.ConvertToAlias(AZ::IO::PathView(aliasFolder));
            ASSERT_TRUE(optionalAliasPath);
            EXPECT_STREQ("@test@", optionalAliasPath->c_str());
        }

        TEST_F(AliasTest, ConvertToAlias_PathViewOverloadStartingWithAliasPath_Succeeds)
        {
            AZ::IO::LocalFileIO local;
            AZ::IO::FixedMaxPathString aliasFolder;
            EXPECT_TRUE(local.ConvertToAbsolutePath("/temp", aliasFolder.data(), aliasFolder.capacity()));
            aliasFolder.resize_no_construct(AZStd::char_traits<char>::length(aliasFolder.data()));
            local.SetAlias("@test@", aliasFolder.c_str());

            const auto testPath = AZ::IO::FixedMaxPathString::format("%s/Dir", aliasFolder.c_str());
            AZ::IO::FixedMaxPath aliasPath;
            ASSERT_TRUE(local.ConvertToAlias(aliasPath, AZ::IO::PathView(testPath)));
            EXPECT_STREQ("@test@/Dir", aliasPath.c_str());
        }

        TEST_F(AliasTest, ConvertToAlias_PathViewOverloadInputPathWithoutPathSeparatorAndStartWithAliasPath_DoesNotSubstituteAlias)
        {
            AZ::IO::LocalFileIO local;
            AZ::IO::FixedMaxPathString aliasFolder;
            EXPECT_TRUE(local.ConvertToAbsolutePath("/temp", aliasFolder.data(), aliasFolder.capacity()));
            aliasFolder.resize_no_construct(AZStd::char_traits<char>::length(aliasFolder.data()));
            local.SetAlias("@test@", aliasFolder.c_str());

            // Because there is no trailing path separator, the input path is really "/tempDir"
            // Therefore the "/temp" alias shouldn't match as an alias should match a full directory
            const auto testPath = AZ::IO::FixedMaxPathString::format("%sDir", aliasFolder.c_str());
            AZ::IO::FixedMaxPath aliasPath{ testPath };
            EXPECT_TRUE(local.ConvertToAlias(aliasPath, AZ::IO::PathView(testPath)));
            EXPECT_STREQ(testPath.c_str(), aliasPath.c_str());
        }

        TEST_F(AliasTest, ConvertToAlias_PathViewOverloadWithTooLongPath_ReturnsFalse)
        {
            AZ::IO::LocalFileIO local;
            AZ::IO::FixedMaxPathString aliasFolder;
            EXPECT_TRUE(local.ConvertToAbsolutePath("/temp", aliasFolder.data(), aliasFolder.capacity()));
            aliasFolder.resize_no_construct(AZStd::char_traits<char>::length(aliasFolder.data()));
            local.SetAlias("@LongAliasThatIsLong@", aliasFolder.c_str());
            AZStd::string path = static_cast<AZStd::string_view>(aliasFolder);
            path.push_back(AZ_CORRECT_FILESYSTEM_SEPARATOR);
            // The length of "@alias@" is longer than the aliased path
            // Therefore ConvertToAlias should fail due to not being able to fit the alias in the buffer
            path.append(AZ::IO::MaxPathLength, 'a');

            AZ::IO::FixedMaxPath aliasPath;
            AZ_TEST_START_TRACE_SUPPRESSION;
            EXPECT_FALSE(local.ConvertToAlias(aliasPath, AZ::IO::PathView(path)));
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }

        TEST_F(AliasTest, ReplaceAlias_CanReplaceAliasInplace_Succeeds)
        {
            AZ::IO::LocalFileIO local;
            AZ::IO::FixedMaxPathString aliasFolder;
            auto CreateTestAbsPath = [&local](char* ptr, size_t size) -> size_t
            {
                // If the path was successfully converted to an absolute path
                // then take the string length of it to figure out where to resize
                // the fixed_string downwards
                return local.ConvertToAbsolutePath("/temp", ptr, size) ?
                    AZStd::char_traits<char>::length(ptr) : 0;
            };
            aliasFolder.resize_and_overwrite(aliasFolder.max_size(), CreateTestAbsPath);
            local.SetAlias("@test@", aliasFolder.c_str());

            // First replace the alias into a new FixedMaxPath
            const AZ::IO::FixedMaxPath testValue = "@test@/Assets/Materials/Types/MaterialInputs/BaseColorPropertyGroup.json";
            AZ::IO::FixedMaxPath expectedResolvedValue;
            EXPECT_TRUE(local.ReplaceAlias(expectedResolvedValue, testValue));

            // Make a copy of the testValue into a FixedMaxPath and resolve it inplace
            AZ::IO::FixedMaxPath testPath = testValue;
            EXPECT_TRUE(local.ReplaceAlias(testPath, testPath));
            EXPECT_EQ(expectedResolvedValue, testPath);
        }

        TEST_F(AliasTest, GetAlias_LogsError_WhenAccessingDeprecatedAlias_Succeeds)
        {
            AZ::IO::LocalFileIO local;

            AZ::IO::FixedMaxPathString aliasFolder;
            EXPECT_TRUE(local.ConvertToAbsolutePath("/temp", aliasFolder.data(), aliasFolder.capacity()));
            aliasFolder.resize_no_construct(AZStd::char_traits<char>::length(aliasFolder.data()));

            local.SetAlias("@test@", aliasFolder.c_str());
            local.SetDeprecatedAlias("@deprecated@", "@test@");
            local.SetDeprecatedAlias("@deprecatednonexistent@", "@nonexistent@");
            local.SetDeprecatedAlias("@deprecatedsecond@", "@deprecated@");
            local.SetDeprecatedAlias("@deprecatednonaliaspath@", aliasFolder);

            AZ_TEST_START_TRACE_SUPPRESSION;
            const char* testAlias = local.GetAlias("@test@");
            ASSERT_NE(nullptr, testAlias);
            EXPECT_EQ(AZ::IO::PathView(aliasFolder), AZ::IO::PathView(testAlias));
            AZ_TEST_STOP_TRACE_SUPPRESSION(0);

            // Validate that accessing Deprecated Alias results in AZ_Error
            AZ_TEST_START_TRACE_SUPPRESSION;
            testAlias = local.GetAlias("@deprecated@");
            ASSERT_NE(nullptr, testAlias);
            EXPECT_EQ(AZ::IO::PathView(aliasFolder), AZ::IO::PathView(testAlias));
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);

            AZ_TEST_START_TRACE_SUPPRESSION;
            testAlias = local.GetAlias("@deprecatednonexistent@");
            EXPECT_EQ(nullptr, testAlias);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);

            AZ_TEST_START_TRACE_SUPPRESSION;
            testAlias = local.GetAlias("@deprecatedsecond@");
            ASSERT_NE(nullptr, testAlias);
            EXPECT_EQ(AZ::IO::PathView(aliasFolder), AZ::IO::PathView(testAlias));
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);

            AZ_TEST_START_TRACE_SUPPRESSION;
            testAlias = local.GetAlias("@deprecatednonaliaspath@");
            ASSERT_NE(nullptr, testAlias);
            EXPECT_EQ(AZ::IO::PathView(aliasFolder), AZ::IO::PathView(testAlias));
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }

        class SmartMoveTests
            : public FolderFixture
        {
        public:
            void run()
            {
                LocalFileIO localFileIO;
                AZ::IO::FileIOBase::SetInstance(&localFileIO);
                AZ::IO::Path path = m_file01Name.ParentPath();
                AZ_TEST_ASSERT(localFileIO.CreatePath(path.c_str()));
                path = m_file01Name.ParentPath();
                AZ_TEST_ASSERT(localFileIO.CreatePath(path.c_str()));

                AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
                localFileIO.Open(m_file01Name.c_str(), OpenMode::ModeWrite | OpenMode::ModeText, fileHandle);
                localFileIO.Write(fileHandle, "DummyFile", 9);
                localFileIO.Close(fileHandle);

                AZ::IO::HandleType fileHandle1 = AZ::IO::InvalidHandle;
                localFileIO.Open(m_file02Name.c_str(), OpenMode::ModeWrite | OpenMode::ModeText, fileHandle1);
                localFileIO.Write(fileHandle1, "TestFile", 8);
                localFileIO.Close(fileHandle1);

                fileHandle1 = AZ::IO::InvalidHandle;
                localFileIO.Open(m_file02Name.c_str(), OpenMode::ModeRead | OpenMode::ModeText, fileHandle1);
                static const size_t testStringLen = 256;
                char testString[testStringLen] = { 0 };
                localFileIO.Read(fileHandle1, testString, testStringLen);
                localFileIO.Close(fileHandle1);
                AZ_TEST_ASSERT(strncmp(testString, "TestFile", 8) == 0);

                // try swapping files when none of the files are in use
                AZ_TEST_ASSERT(AZ::IO::SmartMove(m_file01Name.c_str(), m_file02Name.c_str()));

                fileHandle1 = AZ::IO::InvalidHandle;
                localFileIO.Open(m_file02Name.c_str(), OpenMode::ModeRead | OpenMode::ModeText, fileHandle1);
                testString[0] = '\0';
                localFileIO.Read(fileHandle1, testString, testStringLen);
                localFileIO.Close(fileHandle1);
                AZ_TEST_ASSERT(strncmp(testString, "DummyFile", 9) == 0);

                //try swapping files when source file is not present, this should fail
                AZ_TEST_ASSERT(!AZ::IO::SmartMove(m_file01Name.c_str(), m_file02Name.c_str()));

                fileHandle = AZ::IO::InvalidHandle;
                localFileIO.Open(m_file01Name.c_str(), OpenMode::ModeWrite | OpenMode::ModeText, fileHandle);
                localFileIO.Write(fileHandle, "TestFile", 8);
                localFileIO.Close(fileHandle);

#if AZ_TRAIT_AZFRAMEWORKTEST_MOVE_WHILE_OPEN
                fileHandle1 = AZ::IO::InvalidHandle;
                localFileIO.Open(m_file02Name.c_str(), OpenMode::ModeRead | OpenMode::ModeText, fileHandle1);
                testString[0] = '\0';
                localFileIO.Read(fileHandle1, testString, testStringLen);

                // try swapping files when the destination file is open for read only,
                // since window is unable to move files that are open for read, this will fail.
                AZ_TEST_ASSERT(!AZ::IO::SmartMove(m_file01Name.c_str(), m_file02Name.c_str()));
                localFileIO.Close(fileHandle1);
#endif
                fileHandle = AZ::IO::InvalidHandle;
                localFileIO.Open(m_file01Name.c_str(), OpenMode::ModeRead | OpenMode::ModeText, fileHandle);

                // try swapping files when the source file is open for read only
                AZ_TEST_ASSERT(AZ::IO::SmartMove(m_file01Name.c_str(), m_file02Name.c_str()));
                localFileIO.Close(fileHandle);

                fileHandle1 = AZ::IO::InvalidHandle;
                localFileIO.Open(m_file02Name.c_str(), OpenMode::ModeRead | OpenMode::ModeText, fileHandle1);
                testString[0] = '\0';
                localFileIO.Read(fileHandle1, testString, testStringLen);
                AZ_TEST_ASSERT(strncmp(testString, "TestFile", 8) == 0);
                localFileIO.Close(fileHandle1);

                localFileIO.Remove(m_file01Name.c_str());
                localFileIO.Remove(m_file02Name.c_str());
                localFileIO.DestroyPath(m_root.c_str());

                AZ::IO::FileIOBase::SetInstance(nullptr);
            }
        };

        TEST_F(SmartMoveTests, Test)
        {
            run();
        }
    }
}   // namespace UnitTest

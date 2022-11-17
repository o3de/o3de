/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UnitTest/UnitTest.h>

#include <AzCore/IO/SystemFile.h> // for max path decl
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/semaphore.h>
#include <AzCore/std/functional.h> // for function<> in the find files callback.
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzFramework/Application/Application.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/Archive/ArchiveFileIO.h>
#include <AzFramework/Archive/Archive.h>
#include <AzFramework/Archive/ArchiveVars.h>
#include <AzFramework/Archive/INestedArchive.h>

namespace UnitTest
{
    class ArchiveTestFixture
        : public ScopedAllocatorSetupFixture
    {
    public:
        ArchiveTestFixture()
            : ScopedAllocatorSetupFixture()
            , m_application{ AZStd::make_unique<AzFramework::Application>() }
        {
        }

        void SetUp() override
        {
            AZ::SettingsRegistryInterface* registry = AZ::SettingsRegistry::Get();

            auto projectPathKey =
                AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + "/project_path";
            AZ::IO::FixedMaxPath enginePath;
            registry->Get(enginePath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
            registry->Set(projectPathKey, (enginePath / "AutomatedTesting").Native());
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(*registry);

            m_application->Start({});
            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);
        }

        void TearDown() override
        {
            m_application->Stop();
        }

    protected:
        bool IsPackValid(const char* path)
        {
            AZ::IO::IArchive* archive = AZ::Interface<AZ::IO::IArchive>::Get();
            if (!archive)
            {
                return false;
            }

            return archive->OpenPack(path) && archive->ClosePack(path);
        }

        template <class Function>
        void RunConcurrentUnitTest(AZ::u32 numIterations, AZ::u32 numThreads, Function testFunction)
        {
            AZStd::atomic_int successCount{};
            constexpr size_t maxTestThreads = 16;

            for (AZ::u32 testIteration = 0; testIteration < numIterations; ++testIteration)
            {
                AZStd::fixed_vector<AZStd::thread, maxTestThreads> testThreads;
                successCount = 0;

                for (AZ::u32 threadIdx = 0; threadIdx < numThreads; ++threadIdx)
                {
                    auto threadFunctor = [&testFunction, &successCount]()
                    {
                        // Add some variability to thread timing by yielding each thread
                        AZStd::this_thread::yield();

                        if (testFunction())
                        {
                            ++successCount;
                        }
                    };
                    testThreads.emplace_back(threadFunctor);
                }

                for (AZStd::thread& testThread : testThreads)
                {
                    testThread.join();
                }

                EXPECT_EQ(numThreads, successCount);
            }
        }

        void TestFGetCachedFileData(const char* testFilePath, size_t dataLen, const char* testData)
        {
            AZ::IO::IArchive* archive = AZ::Interface<AZ::IO::IArchive>::Get();
            ASSERT_NE(nullptr, archive);

            constexpr uint32_t numThreadedIterations = 1;
            constexpr uint32_t numTestThreads = 5;

            {
                // Canary tests first
                AZ::IO::HandleType fileHandle = archive->FOpen(testFilePath, "rb");

                ASSERT_NE(AZ::IO::InvalidHandle, fileHandle);

                size_t fileSize = 0;
                char* pFileBuffer = (char*)archive->FGetCachedFileData(fileHandle, fileSize);
                ASSERT_NE(nullptr, pFileBuffer);
                EXPECT_EQ(dataLen, fileSize);
                EXPECT_EQ(0, memcmp(pFileBuffer, testData, dataLen));

                // 2nd call to FGetCachedFileData, same file handle
                fileSize = 0;
                char* pFileBuffer2 = (char*)archive->FGetCachedFileData(fileHandle, fileSize);
                EXPECT_NE(nullptr, pFileBuffer2);
                EXPECT_EQ(pFileBuffer, pFileBuffer2);
                EXPECT_EQ(dataLen, fileSize);

                // open already open file and call FGetCachedFileData
                fileSize = 0;
                {
                    AZ::IO::HandleType fileHandle2 = archive->FOpen(testFilePath, "rb");
                    char* pFileBuffer3 = (char*)archive->FGetCachedFileData(fileHandle2, fileSize);
                    ASSERT_NE(nullptr, pFileBuffer3);
                    EXPECT_EQ(dataLen, fileSize);
                    EXPECT_EQ(0, memcmp(pFileBuffer3, testData, dataLen));
                    archive->FClose(fileHandle2);
                }

                // Multithreaded test #1 reading from the same file handle in parallel
                auto parallelArchiveFileReadFunc = [archive, fileHandle, pFileBuffer, dataLen, testFilePath]()
                {
                    size_t fileSize = 0;
                    auto pFileBufferThread = reinterpret_cast<const char*>(archive->FGetCachedFileData(fileHandle, fileSize));
                    if (pFileBufferThread == nullptr)
                    {
                        EXPECT_NE(nullptr, pFileBufferThread) << "FGetCachedFileData returned nullptr for file " << testFilePath;
                        return false;
                    }
                    if (pFileBuffer != pFileBufferThread)
                    {
                        EXPECT_EQ(pFileBufferThread, pFileBuffer) << "Read file data for file " << testFilePath << "Does not match expected file data";
                        return false;
                    }
                    if (fileSize != dataLen)
                    {
                        EXPECT_EQ(dataLen, fileSize) << "Read filesize does not match expected filesize for file " << testFilePath;
                        return false;
                    }
                    return true;
                };
                RunConcurrentUnitTest(numThreadedIterations, numTestThreads, parallelArchiveFileReadFunc);

                archive->FClose(fileHandle);
            }

            // Multithreaded Test #2 reading from the same file concurrently
            auto concurrentArchiveFileReadFunc = [archive, testFilePath, dataLen, testData]()
            {
                AZ::IO::HandleType threadFileHandle = archive->FOpen(testFilePath, "rb");

                if (threadFileHandle == AZ::IO::InvalidHandle)
                {
                    EXPECT_NE(AZ::IO::InvalidHandle, threadFileHandle) << "Failed to open file handle " << testFilePath;
                    return false;
                }
                size_t fileSize = 0;
                auto pFileBufferThread = reinterpret_cast<const char*>(archive->FGetCachedFileData(threadFileHandle, fileSize));
                if (pFileBufferThread == nullptr)
                {
                    EXPECT_NE(nullptr, pFileBufferThread) << "FGetCachedFileData returned nullptr for file " << testFilePath;
                    return false;
                }
                if (fileSize != dataLen)
                {
                    EXPECT_EQ(dataLen, fileSize) << "Read filesize does not match expected filesize for file " << testFilePath;
                    return false;
                }
                if (memcmp(pFileBufferThread, testData, dataLen) != 0)
                {
                    ADD_FAILURE() << "Read file data for file " << testFilePath << "Does not match expected file data";
                }
                archive->FClose(threadFileHandle);
                return true;
            };
            RunConcurrentUnitTest(numThreadedIterations, numTestThreads, concurrentArchiveFileReadFunc);
        }

        AZStd::unique_ptr<AzFramework::Application> m_application;
    };

    struct CVarIntValueScope
    {
        CVarIntValueScope(AZ::IConsole& console, const char* cvarName)
            : m_console{ console }
            , m_cvarName{ cvarName }
        {
            // Store current CVar value
            m_valueStored = m_cvarName != nullptr && m_console.GetCvarValue(cvarName, m_oldValue) == AZ::GetValueResult::Success;
        }
        ~CVarIntValueScope()
        {
            // Restore the old value if it was successfully stored
            if (m_valueStored)
            {
                m_console.PerformCommand(m_cvarName, { AZ::CVarFixedString::format("%d", m_oldValue) });
            }
        }
        AZ::IConsole& m_console;
        const char* m_cvarName{};
        int32_t m_oldValue{};
        bool m_valueStored{};
    };

    TEST_F(ArchiveTestFixture, TestArchiveFGetCachedFileData_PakFile)
    {
        // Test setup - from Archive
        constexpr const char* fileInArchiveFile = "levels\\mylevel\\levelinfo.xml";
        constexpr AZStd::string_view dataString = "HELLO WORLD"; // other unit tests make sure writing and reading is working, so don't test that here

        AZ::IO::IArchive* archive = AZ::Interface<AZ::IO::IArchive>::Get();
        ASSERT_NE(nullptr, archive);

        AZ::IO::FileIOBase* fileIo = AZ::IO::FileIOBase::GetInstance();
        ASSERT_NE(nullptr, fileIo);

        auto console = AZ::Interface<AZ::IConsole>::Get();
        ASSERT_NE(nullptr, console);

        {
            AZStd::string testArchivePath_withSubfolders = "@usercache@/immediate.pak";
            AZStd::string testArchivePath_withMountPoint = "@usercache@/levels/test/flatarchive.pak";

            // delete test files in case they already exist
            archive->ClosePack(testArchivePath_withSubfolders.c_str());
            fileIo->Remove(testArchivePath_withSubfolders.c_str());
            fileIo->Remove(testArchivePath_withMountPoint.c_str());
            fileIo->CreatePath("@usercache@/levels/test");

            // setup test archive and file
            AZStd::intrusive_ptr<AZ::IO::INestedArchive> pArchive = archive->OpenArchive(testArchivePath_withSubfolders.c_str(), {}, AZ::IO::INestedArchive::FLAGS_CREATE_NEW);
            EXPECT_NE(nullptr, pArchive);
            EXPECT_EQ(0, pArchive->UpdateFile(fileInArchiveFile, dataString.data(), dataString.size(), AZ::IO::INestedArchive::METHOD_COMPRESS, AZ::IO::INestedArchive::LEVEL_FASTEST));
            pArchive.reset();
            EXPECT_TRUE(IsPackValid(testArchivePath_withSubfolders.c_str()));

            EXPECT_TRUE(archive->OpenPack("@products@", testArchivePath_withSubfolders.c_str()));

            EXPECT_TRUE(archive->IsFileExist(fileInArchiveFile));
        }

        // Prevent Archive file searches from using the OS filesystem
        // Also enable extra verbosity in the AZ::IO::Archive code
        CVarIntValueScope previousLocationPriority{ *console, "sys_pakPriority" };
        CVarIntValueScope oldArchiveVerbosity{ *console, "az_archive_verbosity" };
        console->PerformCommand("sys_PakPriority", { AZ::CVarFixedString::format("%d", aznumeric_cast<int>(AZ::IO::FileSearchPriority::PakOnly)) });
        console->PerformCommand("az_archive_verbosity", { "1" });

        // ---- Archive FGetCachedFileDataTests (these leverage Archive CachedFile mechanism for caching data ---
        TestFGetCachedFileData(fileInArchiveFile, dataString.size(), dataString.data());
    }

    TEST_F(ArchiveTestFixture, TestArchiveOpenPacks_FindsMultiplePaks_Works)
    {
        AZ::IO::IArchive* archive = AZ::Interface<AZ::IO::IArchive>::Get();
        ASSERT_NE(nullptr, archive);

        AZ::IO::FileIOBase* fileIo = AZ::IO::FileIOBase::GetInstance();
        ASSERT_NE(nullptr, fileIo);

        auto resetArchiveFile = [archive, fileIo](const AZStd::string& filePath)
        {
            archive->ClosePack(filePath.c_str());
            fileIo->Remove(filePath.c_str());

            auto pArchive = archive->OpenArchive(filePath.c_str(), {}, AZ::IO::INestedArchive::FLAGS_CREATE_NEW);
            EXPECT_NE(nullptr, pArchive);
            pArchive.reset();
            archive->ClosePack(filePath.c_str());
        };

        AZStd::string testArchivePath_pakOne = "@usercache@/one.pak";
        AZStd::string testArchivePath_pakTwo = "@usercache@/two.pak";

        // reset test files in case they already exist
        resetArchiveFile(testArchivePath_pakOne);
        resetArchiveFile(testArchivePath_pakTwo);

        // open and fetch the opened pak file using a *.pak
        AZStd::vector<AZ::IO::FixedMaxPathString> fullPaths;
        archive->OpenPacks("@usercache@/*.pak", &fullPaths);
        EXPECT_TRUE(AZStd::any_of(fullPaths.cbegin(), fullPaths.cend(), [](auto& path) { return path.ends_with("one.pak"); }));
        EXPECT_TRUE(AZStd::any_of(fullPaths.cbegin(), fullPaths.cend(), [](auto& path) { return path.ends_with("two.pak"); }));
    }

    TEST_F(ArchiveTestFixture, TestArchiveFGetCachedFileData_LooseFile)
    {
        // ------setup loose file FGetCachedFileData tests -------------------------

        constexpr AZStd::string_view dataString = "HELLO WORLD";

        AZ::IO::IArchive* archive = AZ::Interface<AZ::IO::IArchive>::Get();
        ASSERT_NE(nullptr, archive);
        const char* testRootPath = "@log@/unittesttemp";
        const char* looseTestFilePath = "@log@/unittesttemp/realfileforunittest.txt";
        AZ::IO::ArchiveFileIO cpfio(archive);

        // remove existing
        EXPECT_EQ(AZ::IO::ResultCode::Success, cpfio.DestroyPath(testRootPath));

        // create test file
        EXPECT_EQ(AZ::IO::ResultCode::Success, cpfio.CreatePath(testRootPath));

        AZ::IO::HandleType normalFileHandle;
        EXPECT_EQ(AZ::IO::ResultCode::Success, cpfio.Open(looseTestFilePath, AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary, normalFileHandle));

        AZ::u64 bytesWritten = 0;
        EXPECT_EQ(AZ::IO::ResultCode::Success, cpfio.Write(normalFileHandle, dataString.data(), dataString.size(), &bytesWritten));
        EXPECT_EQ(dataString.size(), bytesWritten);

        EXPECT_EQ(AZ::IO::ResultCode::Success, cpfio.Close(normalFileHandle));
        EXPECT_TRUE(cpfio.Exists(looseTestFilePath));

        TestFGetCachedFileData(looseTestFilePath, dataString.size(), dataString.data());
    }

    // a bug was found that causes problems reading data from packs if they are immediately mounted after writing.
    // this unit test adjusts for that.
    TEST_F(ArchiveTestFixture, TestArchivePackImmediateReading)
    {
        // the strategy is to create a archive file similar to how the level system does
        // one which contains subfolders
        // and a file inside that subfolder
        // to be successful, it must be possible to write that pack, close it, then open it via Archive
        // and be able to IMMEDIATELY
        // * read the file in the subfolder
        // * enumerate the folders (including that subfolder) even though they are 'virtual', not real folders on physical media
        // * all of the above even though the mount point for the archive is @products@ wheras the physical pack lives in @usercache@
        // finally, we're going to repeat the above test but with files mounted with subfolders
        // so for example, the pack will contain levelinfo.xml at the root of it
        // but it will be mounted at a subfolder (levels/mylevel).
        // this must cause FindNext and Open to work for levels/mylevel/levelinfo.xml.

        constexpr const char* testArchivePath_withSubfolders = "@usercache@/immediate.pak";
        constexpr const char* testArchivePath_withMountPoint = "@usercache@/levels/test/flatarchive.pak";

        AZ::IO::FileIOBase* fileIo = AZ::IO::FileIOBase::GetInstance();
        ASSERT_NE(nullptr, fileIo);

        AZ::IO::IArchive* archive = AZ::Interface<AZ::IO::IArchive>::Get();
        ASSERT_NE(nullptr, archive);

        auto console = AZ::Interface<AZ::IConsole>::Get();
        ASSERT_NE(nullptr, console);

        constexpr AZStd::string_view dataString = "HELLO WORLD"; // other unit tests make sure writing and reading is working, so don't test that here


        // delete test files in case they already exist
        archive->ClosePack(testArchivePath_withSubfolders);
        archive->ClosePack(testArchivePath_withMountPoint);
        fileIo->Remove(testArchivePath_withSubfolders);
        fileIo->Remove(testArchivePath_withMountPoint);
        fileIo->CreatePath("@usercache@/levels/test");


        AZStd::intrusive_ptr<AZ::IO::INestedArchive> pArchive = archive->OpenArchive(testArchivePath_withSubfolders, {}, AZ::IO::INestedArchive::FLAGS_CREATE_NEW);
        EXPECT_NE(nullptr, pArchive);
        EXPECT_EQ(0, pArchive->UpdateFile("levels\\mylevel\\levelinfo.xml", dataString.data(), dataString.size(), AZ::IO::INestedArchive::METHOD_COMPRESS, AZ::IO::INestedArchive::LEVEL_FASTEST));
        pArchive.reset();
        EXPECT_TRUE(IsPackValid(testArchivePath_withSubfolders));

        EXPECT_TRUE(archive->OpenPack("@products@", testArchivePath_withSubfolders));
        // ---- BARRAGE OF TESTS
        EXPECT_TRUE(archive->IsFileExist("levels\\mylevel\\levelinfo.xml"));
        EXPECT_TRUE(archive->IsFileExist("levels//mylevel//levelinfo.xml"));

        bool found_mylevel_folder = false;
        AZ::IO::ArchiveFileIterator handle = archive->FindFirst("levels\\*");

        EXPECT_TRUE(static_cast<bool>(handle));
        if (handle)
        {
            do
            {
                if ((handle.m_fileDesc.nAttrib & AZ::IO::FileDesc::Attribute::Subdirectory) == AZ::IO::FileDesc::Attribute::Subdirectory)
                {
                    if (azstricmp(handle.m_filename.data(), "mylevel") == 0)
                    {
                        found_mylevel_folder = true;
                    }
                }
                else
                {
                    EXPECT_STRCASENE("levelinfo.xml", handle.m_filename.data()); // you may not find files inside the archive in this folder.
                }
            } while (handle = archive->FindNext(handle));

            archive->FindClose(handle);
        }

        EXPECT_TRUE(found_mylevel_folder);

        bool found_mylevel_file = false;

        handle = archive->FindFirst("levels\\mylevel\\*");

        EXPECT_TRUE(static_cast<bool>(handle));
        if (handle)
        {
            do
            {
                if ((handle.m_fileDesc.nAttrib & AZ::IO::FileDesc::Attribute::Subdirectory) != AZ::IO::FileDesc::Attribute::Subdirectory)
                {
                    if (azstricmp(handle.m_filename.data(), "levelinfo.xml") == 0)
                    {
                        found_mylevel_file = true;
                    }
                }
                else
                {
                    EXPECT_STRCASENE("mylevel", handle.m_filename.data()); // you may not find the level subfolder here since we're in the subfolder already
                    EXPECT_STRCASENE("levels\\mylevel", handle.m_filename.data());
                    EXPECT_STRCASENE("levels//mylevel", handle.m_filename.data());
                }
            } while (handle = archive->FindNext(handle));

            archive->FindClose(handle);
        }

        EXPECT_TRUE(found_mylevel_file);

        // now test clean-up
        archive->ClosePack(testArchivePath_withSubfolders);
        fileIo->Remove(testArchivePath_withSubfolders);

        EXPECT_FALSE(archive->IsFileExist("levels\\mylevel\\levelinfo.xml"));
        EXPECT_FALSE(archive->IsFileExist("levels//mylevel//levelinfo.xml"));

        // Once the archive has been deleted it should no longer be searched
        CVarIntValueScope previousLocationPriority{ *console, "sys_pakPriority" };
        console->PerformCommand("sys_PakPriority", { AZ::CVarFixedString::format("%d", aznumeric_cast<int>(AZ::IO::FileSearchPriority::PakOnly)) });

        handle = archive->FindFirst("levels\\*");
        EXPECT_FALSE(static_cast<bool>(handle));
    }
    TEST_F(ArchiveTestFixture, FilesInArchive_AreSearchable)
    {
        // ----------- SECOND TEST.  File in levels/mylevel/ showing up as searchable.
        // note that the actual file's folder has nothing to do with the mount point.

        AZ::IO::FileIOBase* fileIo = AZ::IO::FileIOBase::GetInstance();
        ASSERT_NE(nullptr, fileIo);

        AZ::IO::IArchive* archive = AZ::Interface<AZ::IO::IArchive>::Get();
        ASSERT_NE(nullptr, archive);

        constexpr AZStd::string_view dataString = "HELLO WORLD";
        constexpr const char* testArchivePath_withMountPoint = "@usercache@/levels/test/flatarchive.pak";
        bool found_mylevel_file{};
        bool found_mylevel_folder{};

        AZStd::intrusive_ptr<AZ::IO::INestedArchive> pArchive = archive->OpenArchive(testArchivePath_withMountPoint, {}, AZ::IO::INestedArchive::FLAGS_CREATE_NEW);
        EXPECT_NE(nullptr, pArchive);
        EXPECT_EQ(0, pArchive->UpdateFile("levelinfo.xml", dataString.data(), dataString.size(), AZ::IO::INestedArchive::METHOD_COMPRESS, AZ::IO::INestedArchive::LEVEL_FASTEST));
        pArchive.reset();
        EXPECT_TRUE(IsPackValid(testArchivePath_withMountPoint));

        EXPECT_TRUE(archive->OpenPack("@products@\\uniquename\\mylevel2", testArchivePath_withMountPoint));

        // ---- BARRAGE OF TESTS
        EXPECT_TRUE(archive->IsFileExist("uniquename\\mylevel2\\levelinfo.xml"));
        EXPECT_TRUE(archive->IsFileExist("uniquename//mylevel2//levelinfo.xml"));

        EXPECT_TRUE(!archive->IsFileExist("uniquename\\mylevel\\levelinfo.xml"));
        EXPECT_TRUE(!archive->IsFileExist("uniquename//mylevel//levelinfo.xml"));
        EXPECT_TRUE(!archive->IsFileExist("uniquename\\test\\levelinfo.xml"));
        EXPECT_TRUE(!archive->IsFileExist("uniquename//test//levelinfo.xml"));

        found_mylevel_folder = false;
        AZ::IO::ArchiveFileIterator handle = archive->FindFirst("uniquename\\*");

        EXPECT_TRUE(static_cast<bool>(handle));
        if (handle)
        {
            do
            {
                if ((handle.m_fileDesc.nAttrib & AZ::IO::FileDesc::Attribute::Subdirectory) == AZ::IO::FileDesc::Attribute::Subdirectory)
                {
                    if (azstricmp(handle.m_filename.data(), "mylevel2") == 0)
                    {
                        found_mylevel_folder = true;
                    }
                }
            } while (handle = archive->FindNext(handle));

            archive->FindClose(handle);
        }

        EXPECT_TRUE(found_mylevel_folder);

        found_mylevel_file = false;

        handle = archive->FindFirst("uniquename\\mylevel2\\*");

        EXPECT_TRUE(static_cast<bool>(handle));
        if (handle)
        {
            do
            {
                if ((handle.m_fileDesc.nAttrib & AZ::IO::FileDesc::Attribute::Subdirectory) != AZ::IO::FileDesc::Attribute::Subdirectory)
                {
                    if (azstricmp(handle.m_filename.data(), "levelinfo.xml") == 0)
                    {
                        found_mylevel_file = true;
                    }
                }
            } while (handle = archive->FindNext(handle));

            archive->FindClose(handle);
        }

        EXPECT_TRUE(found_mylevel_file);

        archive->ClosePack(testArchivePath_withMountPoint);

        // --- test to make sure that when you iterate only the first component is found, so bury it deep and ask for the root
        EXPECT_TRUE(archive->OpenPack("@products@\\uniquename\\mylevel2\\mylevel3\\mylevel4", testArchivePath_withMountPoint));

        found_mylevel_folder = false;
        handle = archive->FindFirst("uniquename\\*");

        int numFound = 0;
        EXPECT_TRUE(static_cast<bool>(handle));
        if (handle)
        {
            do
            {
                if ((handle.m_fileDesc.nAttrib & AZ::IO::FileDesc::Attribute::Subdirectory) == AZ::IO::FileDesc::Attribute::Subdirectory)
                {
                    ++numFound;
                    if (azstricmp(handle.m_filename.data(), "mylevel2") == 0)
                    {
                        found_mylevel_folder = true;
                    }
                }
            } while (handle = archive->FindNext(handle));

            archive->FindClose(handle);
        }

        EXPECT_EQ(1, numFound);
        EXPECT_TRUE(found_mylevel_folder);

        numFound = 0;
        found_mylevel_folder = false;

        // now make sure no red herrings appear
        // for example, if a file is mounted at "@products@\\uniquename\\mylevel2\\mylevel3\\mylevel4"
        // and the file "@products@\\somethingelse" is requested it should not be found
        // in addition if the file "@products@\\uniquename\\mylevel3" is requested it should not be found
        handle = archive->FindFirst("somethingelse\\*");
        EXPECT_FALSE(static_cast<bool>(handle));

        handle = archive->FindFirst("uniquename\\mylevel3*");
        EXPECT_FALSE(static_cast<bool>(handle));

        archive->ClosePack(testArchivePath_withMountPoint);
        fileIo->Remove(testArchivePath_withMountPoint);
    }

    // test that ArchiveFileIO class works as expected
    TEST_F(ArchiveTestFixture, TestArchiveViaFileIO)
    {
        // strategy:
        // create a loose file and a packed file
        // mount the pack
        // make sure that the pack and loose file both appear when the PaKFileIO interface is used.
        using namespace AZ::IO;
        AZ::IO::IArchive* archive = AZ::Interface<AZ::IO::IArchive>::Get();
        AZ::IO::ArchiveFileIO cpfio(archive);

        constexpr const char* genericArchiveFileName = "@usercache@/testarchiveio.pak";
        ASSERT_NE(nullptr, archive);
        const char* dataString = "HELLO WORLD"; // other unit tests make sure writing and reading is working, so don't test that here
        size_t dataLen = strlen(dataString);
        AZStd::vector<char> dataBuffer;
        dataBuffer.resize(dataLen);

        // delete test files in case they already exist
        archive->ClosePack(genericArchiveFileName);
        cpfio.Remove(genericArchiveFileName);

        // create the asset alias directory
        cpfio.CreatePath("@products@");

        // create generic file

        HandleType normalFileHandle;
        AZ::u64 bytesWritten = 0;
        EXPECT_EQ(ResultCode::Success, cpfio.DestroyPath("@log@/unittesttemp"));
        EXPECT_TRUE(!cpfio.Exists("@log@/unittesttemp"));
        EXPECT_TRUE(!cpfio.IsDirectory("@log@/unittesttemp"));
        EXPECT_EQ(ResultCode::Success, cpfio.CreatePath("@log@/unittesttemp"));
        EXPECT_TRUE(cpfio.Exists("@log@/unittesttemp"));
        EXPECT_TRUE(cpfio.IsDirectory("@log@/unittesttemp"));
        EXPECT_EQ(ResultCode::Success, cpfio.Open("@log@/unittesttemp/realfileforunittest.xml", OpenMode::ModeWrite | OpenMode::ModeBinary, normalFileHandle));
        EXPECT_EQ(ResultCode::Success, cpfio.Write(normalFileHandle, dataString, dataLen, &bytesWritten));
        EXPECT_EQ(dataLen, bytesWritten);
        EXPECT_EQ(ResultCode::Success, cpfio.Close(normalFileHandle));
        normalFileHandle = InvalidHandle;
        EXPECT_TRUE(cpfio.Exists("@log@/unittesttemp/realfileforunittest.xml"));

        AZStd::intrusive_ptr<AZ::IO::INestedArchive> pArchive = archive->OpenArchive(genericArchiveFileName, {}, AZ::IO::INestedArchive::FLAGS_CREATE_NEW);
        EXPECT_NE(nullptr, pArchive);
        EXPECT_EQ(0, pArchive->UpdateFile("testfile.xml", dataString, aznumeric_cast<uint32_t>(dataLen), AZ::IO::INestedArchive::METHOD_COMPRESS, AZ::IO::INestedArchive::LEVEL_FASTEST));
        pArchive.reset();
        EXPECT_TRUE(IsPackValid(genericArchiveFileName));

        EXPECT_TRUE(archive->OpenPack("@products@", genericArchiveFileName));

        // ---- BARRAGE OF TESTS
        EXPECT_TRUE(cpfio.Exists("testfile.xml"));
        EXPECT_TRUE(cpfio.Exists("@products@/testfile.xml"));  // this should be hte same file
        EXPECT_TRUE(!cpfio.Exists("@log@/testfile.xml"));
        EXPECT_TRUE(!cpfio.Exists("@usercache@/testfile.xml"));
        EXPECT_TRUE(cpfio.Exists("@log@/unittesttemp/realfileforunittest.xml"));

        // --- Coverage test ----
        AZ::u64 fileSize = 0;
        AZ::u64 bytesRead = 0;
        AZ::u64 currentOffset = 0;
        char fileNameBuffer[AZ_MAX_PATH_LEN] = { 0 };
        EXPECT_EQ(ResultCode::Success, cpfio.Size("testfile.xml", fileSize));
        EXPECT_EQ(dataLen, fileSize);
        EXPECT_EQ(ResultCode::Success, cpfio.Open("testfile.xml", OpenMode::ModeRead | OpenMode::ModeBinary, normalFileHandle));
        EXPECT_NE(InvalidHandle, normalFileHandle);
        EXPECT_EQ(ResultCode::Success, cpfio.Size(normalFileHandle, fileSize));
        EXPECT_EQ(dataLen, fileSize);
        EXPECT_EQ(ResultCode::Success, cpfio.Read(normalFileHandle, dataBuffer.data(), 2, true, &bytesRead));
        EXPECT_EQ(2, bytesRead);
        EXPECT_EQ('H', dataBuffer[0]);
        EXPECT_EQ('E', dataBuffer[1]);
        EXPECT_EQ(ResultCode::Success, cpfio.Tell(normalFileHandle, currentOffset));
        EXPECT_EQ(2, currentOffset);
        EXPECT_EQ(ResultCode::Success, cpfio.Seek(normalFileHandle, 2, SeekType::SeekFromCurrent));
        EXPECT_EQ(ResultCode::Success, cpfio.Tell(normalFileHandle, currentOffset));
        EXPECT_EQ(4, currentOffset);
        EXPECT_TRUE(!cpfio.Eof(normalFileHandle));
        EXPECT_EQ(ResultCode::Success, cpfio.Seek(normalFileHandle, 2, SeekType::SeekFromStart));
        EXPECT_EQ(ResultCode::Success, cpfio.Tell(normalFileHandle, currentOffset));
        EXPECT_EQ(2, currentOffset);
        EXPECT_EQ(ResultCode::Success, cpfio.Seek(normalFileHandle, -2, SeekType::SeekFromEnd));
        EXPECT_EQ(ResultCode::Success, cpfio.Tell(normalFileHandle, currentOffset));
        EXPECT_EQ(dataLen - 2, currentOffset);
        EXPECT_NE(ResultCode::Success, cpfio.Read(normalFileHandle, dataBuffer.data(), 4, true, &bytesRead));
        EXPECT_EQ(2, bytesRead);
        EXPECT_TRUE(cpfio.Eof(normalFileHandle));
        EXPECT_TRUE(cpfio.GetFilename(normalFileHandle, fileNameBuffer, AZ_MAX_PATH_LEN));
        EXPECT_NE(AZStd::string_view::npos, AZStd::string_view(fileNameBuffer).find("testfile.xml"));
        // just coverage-call this function:
        EXPECT_EQ(archive->GetModificationTime(normalFileHandle), cpfio.ModificationTime(normalFileHandle));
        EXPECT_EQ(archive->GetModificationTime(normalFileHandle), cpfio.ModificationTime("testfile.xml"));
        EXPECT_NE(0, cpfio.ModificationTime("testfile.xml"));
        EXPECT_NE(0, cpfio.ModificationTime("@log@/unittesttemp/realfileforunittest.xml"));

        EXPECT_EQ(ResultCode::Success, cpfio.Close(normalFileHandle));

        EXPECT_TRUE(!cpfio.IsDirectory("testfile.xml"));
        EXPECT_TRUE(cpfio.IsDirectory("@products@"));
        EXPECT_TRUE(cpfio.IsReadOnly("testfile.xml"));
        EXPECT_TRUE(cpfio.IsReadOnly("@products@/testfile.xml"));
        EXPECT_TRUE(!cpfio.IsReadOnly("@log@/unittesttemp/realfileforunittest.xml"));


        // copy file from inside archive:
        EXPECT_EQ(ResultCode::Success, cpfio.Copy("testfile.xml", "@log@/unittesttemp/copiedfile.xml"));
        EXPECT_TRUE(cpfio.Exists("@log@/unittesttemp/copiedfile.xml"));

        // make sure copy is ok

        EXPECT_EQ(ResultCode::Success, cpfio.Open("@log@/unittesttemp/copiedfile.xml", OpenMode::ModeRead | OpenMode::ModeBinary, normalFileHandle));
        EXPECT_EQ(ResultCode::Success, cpfio.Size(normalFileHandle, fileSize));
        EXPECT_EQ(dataLen, fileSize);
        EXPECT_EQ(ResultCode::Success, cpfio.Read(normalFileHandle, dataBuffer.data(), dataLen + 10, false, &bytesRead)); // allowed to read less
        EXPECT_EQ(dataLen, bytesRead);
        EXPECT_EQ(0, memcmp(dataBuffer.data(), dataString, dataLen));
        EXPECT_EQ(ResultCode::Success, cpfio.Close(normalFileHandle));

        // make sure file does not exist, since copy will NOT overwrite:
        cpfio.Remove("@log@/unittesttemp/copiedfile2.xml");

        EXPECT_EQ(ResultCode::Success, cpfio.Rename("@log@/unittesttemp/copiedfile.xml", "@log@/unittesttemp/copiedfile2.xml"));
        EXPECT_TRUE(!cpfio.Exists("@log@/unittesttemp/copiedfile.xml"));
        EXPECT_TRUE(cpfio.Exists("@log@/unittesttemp/copiedfile2.xml"));

        // find files test.
        AZ::IO::FixedMaxPath resolvedTestFilePath;
        EXPECT_TRUE(cpfio.ResolvePath(resolvedTestFilePath, AZ::IO::PathView("@products@/testfile.xml")));
        bool foundIt = false;
        // note that this file exists only in the archive.
        cpfio.FindFiles("@products@", "*.xml", [&foundIt, &cpfio, &resolvedTestFilePath](const char* foundName)
            {
                AZ::IO::FixedMaxPath resolvedFoundPath;
                EXPECT_TRUE(cpfio.ResolvePath(resolvedFoundPath, AZ::IO::PathView(foundName)));
                // according to the contract stated in the FileIO.h file, we expect full paths. (Aliases are full paths)
                if (resolvedTestFilePath == resolvedFoundPath)
                {
                    foundIt = true;
                    return false;
                }
                return true;
            });

        EXPECT_TRUE(foundIt);


        // The following test is disabled because it will trigger an AZ_ERROR which will affect the outcome of this entire test
        // EXPECT_NE(ResultCode::Success, cpfio.Remove("@products@/testfile.xml")); // may not delete archive files

        // make sure it works with and without alias:
        EXPECT_TRUE(cpfio.Exists("@products@/testfile.xml"));
        EXPECT_TRUE(cpfio.Exists("testfile.xml"));

        EXPECT_TRUE(cpfio.Exists("@log@/unittesttemp/realfileforunittest.xml"));
        EXPECT_EQ(ResultCode::Success, cpfio.Remove("@log@/unittesttemp/realfileforunittest.xml"));
        EXPECT_TRUE(!cpfio.Exists("@log@/unittesttemp/realfileforunittest.xml"));

        // now test clean-up
        archive->ClosePack(genericArchiveFileName);
        cpfio.Remove(genericArchiveFileName);

        EXPECT_EQ(ResultCode::Success, cpfio.DestroyPath("@log@/unittesttemp"));
        EXPECT_TRUE(!cpfio.Exists("@log@/unittesttemp/realfileforunittest.xml"));
        EXPECT_TRUE(!cpfio.Exists("@log@/unittesttemp"));
        EXPECT_TRUE(!archive->IsFileExist("testfile.xml"));
    }

    TEST_F(ArchiveTestFixture, TestArchiveFolderAliases)
    {
        AZ::IO::FileIOBase* fileIo = AZ::IO::FileIOBase::GetInstance();
        ASSERT_NE(nullptr, fileIo);

        // test whether aliasing works as expected.  We'll create a archive in the cache, but we'll map it to a bunch of folders
        constexpr const char* testArchivePath = "@usercache@/archivetest.pak";

        char realNameBuf[AZ_MAX_PATH_LEN] = { 0 };
        EXPECT_TRUE(fileIo->ResolvePath(testArchivePath, realNameBuf, AZ_MAX_PATH_LEN));

        AZ::IO::IArchive* archive = AZ::Interface<AZ::IO::IArchive>::Get();
        ASSERT_NE(nullptr, archive);

        // delete test files in case they already exist
        archive->ClosePack(testArchivePath);
        fileIo->Remove(testArchivePath);

        // ------------ BASIC TEST:  Create and read Empty Archive ------------
        AZStd::intrusive_ptr<AZ::IO::INestedArchive> pArchive = archive->OpenArchive(testArchivePath, {}, AZ::IO::INestedArchive::FLAGS_CREATE_NEW);
        EXPECT_NE(nullptr, pArchive);

        EXPECT_EQ(0, pArchive->UpdateFile("foundit.dat", const_cast<char*>("test"), 4, AZ::IO::INestedArchive::METHOD_COMPRESS, AZ::IO::INestedArchive::LEVEL_BEST));

        pArchive.reset();
        EXPECT_TRUE(IsPackValid(testArchivePath));

        EXPECT_TRUE(archive->OpenPack("@usercache@", realNameBuf));
        EXPECT_TRUE(archive->IsFileExist("@usercache@/foundit.dat"));
        EXPECT_FALSE(archive->IsFileExist("@usercache@/foundit.dat", AZ::IO::FileSearchLocation::OnDisk));
        EXPECT_FALSE(archive->IsFileExist("@usercache@/notfoundit.dat"));
        EXPECT_TRUE(archive->ClosePack(realNameBuf));

        // change its actual location:
        EXPECT_TRUE(archive->OpenPack("@products@", realNameBuf));
        EXPECT_TRUE(archive->IsFileExist("@products@/foundit.dat"));
        EXPECT_FALSE(archive->IsFileExist("@usercache@/foundit.dat")); // do not find it in the previous location!
        EXPECT_FALSE(archive->IsFileExist("@products@/foundit.dat", AZ::IO::FileSearchLocation::OnDisk));
        EXPECT_FALSE(archive->IsFileExist("@products@/notfoundit.dat"));
        EXPECT_TRUE(archive->ClosePack(realNameBuf));

        // try sub-folders
        EXPECT_TRUE(archive->OpenPack("@products@/mystuff", realNameBuf));
        EXPECT_TRUE(archive->IsFileExist("@products@/mystuff/foundit.dat"));
        EXPECT_FALSE(archive->IsFileExist("@products@/foundit.dat")); // do not find it in the previous locations!
        EXPECT_FALSE(archive->IsFileExist("@usercache@/foundit.dat")); // do not find it in the previous locations!
        EXPECT_FALSE(archive->IsFileExist("@products@/foundit.dat", AZ::IO::FileSearchLocation::OnDisk));
        EXPECT_FALSE(archive->IsFileExist("@products@/mystuff/foundit.dat", AZ::IO::FileSearchLocation::OnDisk));
        EXPECT_FALSE(archive->IsFileExist("@products@/notfoundit.dat")); // non-existent file
        EXPECT_FALSE(archive->IsFileExist("@products@/mystuff/notfoundit.dat")); // non-existent file
        EXPECT_TRUE(archive->ClosePack(realNameBuf));
    }

    TEST_F(ArchiveTestFixture, IResourceList_Add_EmptyFileName_DoesNotInsert)
    {
        AZ::IO::IResourceList* reslist = AZ::Interface<AZ::IO::IArchive>::Get()->GetResourceList(AZ::IO::IArchive::RFOM_EngineStartup);
        ASSERT_NE(nullptr, reslist);

        reslist->Clear();
        reslist->Add("");
        EXPECT_EQ(nullptr, reslist->GetFirst());
    }

    TEST_F(ArchiveTestFixture, IResourceList_Add_RegularFileName_ResolvesAppropriately)
    {
        AZ::IO::IResourceList* reslist = AZ::Interface<AZ::IO::IArchive>::Get()->GetResourceList(AZ::IO::IArchive::RFOM_EngineStartup);
        ASSERT_NE(nullptr, reslist);

        AZ::IO::FileIOBase* ioBase = AZ::IO::FileIOBase::GetInstance();
        ASSERT_NE(nullptr, ioBase);
        AZ::IO::FixedMaxPath resolvedTestPath;
        EXPECT_TRUE(ioBase->ResolvePath(resolvedTestPath, "blah/blah/abcde"));

        reslist->Clear();
        reslist->Add("blah\\blah/AbCDE");
        AZ::IO::FixedMaxPath resolvedAddedPath;
        EXPECT_TRUE(ioBase->ResolvePath(resolvedAddedPath, reslist->GetFirst()));
        EXPECT_EQ(resolvedTestPath, resolvedAddedPath);
        reslist->Clear();
    }

    TEST_F(ArchiveTestFixture, IResourceList_Add_ReallyShortFileName_ResolvesAppropriately)
    {
        AZ::IO::IResourceList* reslist = AZ::Interface<AZ::IO::IArchive>::Get()->GetResourceList(AZ::IO::IArchive::RFOM_EngineStartup);
        ASSERT_NE(nullptr, reslist);

        AZ::IO::FileIOBase* ioBase = AZ::IO::FileIOBase::GetInstance();
        ASSERT_NE(nullptr, ioBase);
        AZ::IO::FixedMaxPath resolvedTestPath;
        EXPECT_TRUE(ioBase->ResolvePath(resolvedTestPath, "a"));

        reslist->Clear();
        reslist->Add("A");
        AZ::IO::FixedMaxPath resolvedAddedPath;
        EXPECT_TRUE(ioBase->ResolvePath(resolvedAddedPath, reslist->GetFirst()));
        EXPECT_EQ(resolvedTestPath, resolvedAddedPath);
        reslist->Clear();
    }

    TEST_F(ArchiveTestFixture, IResourceList_Add_AbsolutePath_RemovesAndReplacesWithAlias)
    {
        AZ::IO::IResourceList* reslist = AZ::Interface<AZ::IO::IArchive>::Get()->GetResourceList(AZ::IO::IArchive::RFOM_EngineStartup);
        ASSERT_NE(nullptr, reslist);

        AZ::IO::FileIOBase* ioBase = AZ::IO::FileIOBase::GetInstance();
        ASSERT_NE(nullptr, ioBase);

        const char* assetsPath = ioBase->GetAlias("@products@");
        ASSERT_NE(nullptr, assetsPath);

        auto stringToAdd = AZ::IO::Path(assetsPath) / "textures" / "test.dds";

        reslist->Clear();
        reslist->Add(stringToAdd.Native());

        // it normalizes the string, so the slashes flip and everything is lowercased.
        AZ::IO::FixedMaxPath resolvedAddedPath;
        AZ::IO::FixedMaxPath resolvedResourcePath;
        EXPECT_TRUE(ioBase->ReplaceAlias(resolvedAddedPath, "@products@/textures/test.dds"));
        EXPECT_TRUE(ioBase->ReplaceAlias(resolvedResourcePath, reslist->GetFirst()));
        EXPECT_EQ(resolvedAddedPath, resolvedResourcePath);
        reslist->Clear();
    }
}

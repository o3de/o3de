/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "CrySystem_precompiled.h"

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/UnitTest.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/IO/SystemFile.h> // for max path decl
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/semaphore.h>
#include <AzCore/std/functional.h> // for function<> in the find files callback.
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/Archive/ArchiveFileIO.h>
#include <AzFramework/Archive/Archive.h>
#include <AzFramework/Archive/INestedArchive.h>
#include <ILevelSystem.h>

namespace CryPakUnitTests
{

#if defined(AZ_PLATFORM_WINDOWS)

    // Note:  none of the below is really a unit test, its all basic feature tests
    // for critical functionality

    class Integ_CryPakUnitTests
        : public ::testing::Test
    {
    protected:
        bool IsPackValid(const char* path)
        {
            AZ::IO::IArchive* pak = gEnv->pCryPak;
            if (!pak)
            {
                return false;
            }

            if (!pak->OpenPack(path, AZ::IO::IArchive::FLAGS_PATH_REAL))
            {
                return false;
            }

            pak->ClosePack(path);
            return true;
        }
    };

    TEST_F(Integ_CryPakUnitTests, TestCryPakArchiveContainingLevels)
    {
        AZ::IO::FileIOBase* fileIo = AZ::IO::FileIOBase::GetInstance();
        ASSERT_NE(nullptr, fileIo);

        constexpr const char* testPakPath = "@usercache@/archivecontainerlevel.pak";

        char resolvedArchivePath[AZ_MAX_PATH_LEN] = { 0 };
        EXPECT_TRUE(fileIo->ResolvePath(testPakPath, resolvedArchivePath, AZ_MAX_PATH_LEN));

        AZ::IO::IArchive* pak = gEnv->pCryPak;

        ASSERT_NE(nullptr, pak);

        // delete test files in case they already exist
        pak->ClosePack(testPakPath);
        fileIo->Remove(testPakPath);

        ILevelSystem* levelSystem = gEnv->pSystem->GetILevelSystem();
        EXPECT_NE(nullptr, levelSystem);

        // ------------ Create an archive with a dummy level in it ------------
        AZStd::intrusive_ptr<AZ::IO::INestedArchive> pArchive = pak->OpenArchive(testPakPath, nullptr, AZ::IO::INestedArchive::FLAGS_CREATE_NEW);
        EXPECT_NE(nullptr, pArchive);

        const char levelInfoFile[] = "levelInfo.xml";
        AZStd::string relativeLevelPakPath = AZStd::string::format("levels/dummy/%s", ILevelSystem::LevelPakName);
        AZStd::string relativeLevelInfoPath = AZStd::string::format("levels/dummy/%s", levelInfoFile);

        EXPECT_EQ(0, pArchive->UpdateFile(relativeLevelPakPath.c_str(), const_cast<char*>("test"), 4, AZ::IO::INestedArchive::METHOD_COMPRESS, AZ::IO::INestedArchive::LEVEL_BEST));
        EXPECT_EQ(0, pArchive->UpdateFile(relativeLevelInfoPath.c_str(), const_cast<char*>("test"), 4, AZ::IO::INestedArchive::METHOD_COMPRESS, AZ::IO::INestedArchive::LEVEL_BEST));

        pArchive.reset();
        EXPECT_TRUE(IsPackValid(testPakPath));
        AZStd::fixed_string<AZ::IO::IArchive::MaxPath> fullLevelPakPath;
        bool addLevel = true;
        EXPECT_TRUE(pak->OpenPack("@assets@", resolvedArchivePath, AZ::IO::IArchive::FLAGS_LEVEL_PAK_INSIDE_PAK, nullptr, &fullLevelPakPath, addLevel));

        ILevelInfo* levelInfo = nullptr;
        // Since the archive was open, we should be able to find the level "dummy"
        levelInfo = levelSystem->GetLevelInfo("dummy");
        EXPECT_NE(nullptr, levelInfo);
        EXPECT_TRUE(pak->ClosePack(resolvedArchivePath));

        // After closing the archive we should not be able to find the level "dummy"
        levelInfo = levelSystem->GetLevelInfo("dummy");
        EXPECT_EQ(nullptr, levelInfo);
    }

    TEST_F(Integ_CryPakUnitTests, TestCryPakModTime)
    {
        AZ::IO::FileIOBase* fileIo = AZ::IO::FileIOBase::GetInstance();
        ASSERT_NE(nullptr, fileIo);

        AZ::IO::IArchive* pak = gEnv->pCryPak;
        // repeat the following test multiple times, since timing (seconds) can affect it and it involves time!
        for (int iteration = 0; iteration < 10; ++iteration)
        {
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds{ 100 });

            // helper paths and strings
            AZStd::string gameFolder = fileIo->GetAlias("@usercache@");

            AZStd::string testFile = "unittest.bin";
            AZStd::string testFilePath = gameFolder + "\\" + testFile;
            AZStd::string testPak = "unittest.pak";
            AZStd::string testPakPath = gameFolder + "\\" + testPak;
            AZStd::string zipCmd = "-zip=" + testPakPath;

            // delete test files in case they already exist
            fileIo->Remove(testFilePath.c_str());
            pak->ClosePack(testPakPath);
            fileIo->Remove(testPakPath.c_str());

            // create a test file
            char data[] = "unittest";
            FILE* f = nullptr;
            azfopen(&f, testFilePath.c_str(), "wb");
            EXPECT_TRUE(f != nullptr);                                              // file successfully opened for writing
            EXPECT_TRUE(fwrite(data, sizeof(char), sizeof(data), f) == sizeof(data)); // file written to successfully
            EXPECT_TRUE(fclose(f) == 0);                                 // file closed successfully

            AZ::IO::HandleType fDisk = pak->FOpen(testFilePath.c_str(), "rb");
            EXPECT_TRUE(fDisk > 0);                                          // opened file on disk successfully
            uint64_t modTimeDisk = pak->GetModificationTime(fDisk);       // high res mod time extracted from file on disk
            EXPECT_TRUE(pak->FClose(fDisk) == 0);              // file closed successfully

            // create a low res copy of disk file's mod time
            uint64_t absDiff, maxDiff = 20000000ul;
            uint16_t dosDate, dosTime;
            FILETIME ft;
            LARGE_INTEGER lt;

            ft.dwHighDateTime = modTimeDisk >> 32;
            ft.dwLowDateTime = modTimeDisk & 0xFFFFFFFF;
            EXPECT_TRUE(FileTimeToDosDateTime(&ft, &dosDate, &dosTime) != FALSE); // converted to DOSTIME successfully
            ft.dwHighDateTime = 0;
            ft.dwLowDateTime = 0;
            EXPECT_TRUE(DosDateTimeToFileTime(dosDate, dosTime, &ft) != FALSE);   // converted to FILETIME successfully
            lt.HighPart = ft.dwHighDateTime;
            lt.LowPart = ft.dwLowDateTime;
            uint64_t modTimeDiskLowRes = lt.QuadPart;

            absDiff = modTimeDiskLowRes >= modTimeDisk ? modTimeDiskLowRes - modTimeDisk : modTimeDisk - modTimeDiskLowRes;
            EXPECT_LE(absDiff, maxDiff);                             // FILETIME (high res) and DOSTIME (low res) should be at most 2 seconds apart

            gEnv->pResourceCompilerHelper->CallResourceCompiler(testFilePath.c_str(), zipCmd.c_str());
            EXPECT_EQ(AZ::IO::ResultCode::Success, fileIo->Remove(testFilePath.c_str()));                      // test file on disk deleted successfully

            EXPECT_TRUE(pak->OpenPack(testPakPath));           // opened pak successfully

            AZ::IO::HandleType fPak = pak->FOpen(testFilePath.c_str(), "rb");
            EXPECT_GT(fPak, 0);                                           // file (in pak) opened correctly
            uint64_t modTimePak = pak->GetModificationTime(fPak);         // low res mod time extracted from file in pak
            EXPECT_EQ(0, pak->FClose(fPak));               // file closed successfully

            EXPECT_TRUE(pak->ClosePack(testPakPath));          // closed pak successfully
            EXPECT_EQ(AZ::IO::ResultCode::Success, fileIo->Remove(testPakPath.c_str()));                       // test pak file deleted successfully

            absDiff = modTimePak >= modTimeDisk ? modTimePak - modTimeDisk : modTimeDisk - modTimePak;
            // compare mod times.  They are allowed to be up to 2 seconds apart but no more
            EXPECT_LE(absDiff, maxDiff);                             // FILETIME (disk) and DOSTIME (pak) should be at most 2 seconds apart
            // note:  Do not directly compare the disk time and pack time, the resolution drops the last digit off in some cases in pak
            // it only has a 2 second resolution.  you may compare to make sure that the pak time is WITHIN 2 seconds (as above) but not equal.

            // we depend on the fact that crypak is rounding up, instead of down
            EXPECT_GE(modTimePak, modTimeDisk);
        }
    }

  
#endif
}

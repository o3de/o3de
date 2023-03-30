/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>

#include <AzFramework/Application/Application.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/Archive/ArchiveFileIO.h>
#include <AzFramework/Archive/Archive.h>
#include <AzFramework/Archive/INestedArchive.h>

namespace UnitTest
{
    using ArchiveCompressionParamInterface = ::testing::WithParamInterface<AZStd::tuple<
        AZ::IO::INestedArchive::EPakFlags,
        AZ::IO::INestedArchive::ECompressionMethods,
        AZ::IO::INestedArchive::ECompressionLevels,
        int, int, int>>;

    class ArchiveCompressionTestFixture
        : public LeakDetectionFixture
        , public ArchiveCompressionParamInterface
    {
    public:
        ArchiveCompressionTestFixture()
            : m_application { AZStd::make_unique<AzFramework::Application>() }
        {
            // Create a unique alias to the user cache directory to avoid race conditions between
            // concurrent invocations of this test target running these tests
            AZ::IO::FileIOBase* fileIo = AZ::IO::FileIOBase::GetInstance();
            fileIo->SetAlias("@usercache@", m_tempDirectory.GetDirectory());
        }

    private:
        AZStd::unique_ptr<AzFramework::Application> m_application;
        AZ::Test::ScopedAutoTempDirectory m_tempDirectory;
    };

    auto IsPackValid(const char* path)
    {
        AZ::IO::IArchive* archive = AZ::Interface<AZ::IO::IArchive>::Get();
        if (!archive)
        {
            return false;
        }

        if (!archive->OpenPack(path))
        {
            return false;
        }

        archive->ClosePack(path);
        return true;
    }

    TEST_P(ArchiveCompressionTestFixture, TestArchivePacking_CompressionEmptyArchiveTest_PackIsValid)
    {
        // this also coincidentally tests to make sure packs inside aliases work.
        AZStd::string testArchivePath = "@usercache@/archivetest.pak";

        AZ::IO::FileIOBase* fileIo = AZ::IO::FileIOBase::GetInstance();
        ASSERT_NE(nullptr, fileIo);

        AZ::IO::IArchive* archive = AZ::Interface<AZ::IO::IArchive>::Get();

        ASSERT_NE(nullptr, archive);

        // delete test files in case they already exist
        archive->ClosePack(testArchivePath.c_str());
        fileIo->Remove(testArchivePath.c_str());

        // ------------ BASIC TEST:  Create and read Empty Archive ------------
        AZStd::intrusive_ptr<AZ::IO::INestedArchive> pArchive = archive->OpenArchive(testArchivePath.c_str(), {}, AZ::IO::INestedArchive::FLAGS_CREATE_NEW);
        EXPECT_NE(nullptr, pArchive);
        pArchive.reset();
        EXPECT_TRUE(IsPackValid(testArchivePath.c_str()));
    }

    TEST_P(ArchiveCompressionTestFixture, TestArchivePacking_CompressionFullArchive_PackIsValid)
    {
        // ------------ BASIC TEST:  Create archive full of standard sizes (including 0)   ----------------
        AZStd::string testArchivePath = "@usercache@/archivetest.pak";
        AZ::IO::IArchive* archive = AZ::Interface<AZ::IO::IArchive>::Get();

        auto openFlags = AZStd::get<0>(GetParam());
        auto compressionMethod = AZStd::get<1>(GetParam());
        auto compressionLevel = AZStd::get<2>(GetParam());
        auto stepSize = AZStd::get<3>(GetParam());
        auto numSteps = AZStd::get<4>(GetParam());
        auto iterations = AZStd::get<5>(GetParam());

        int maxSize = numSteps * stepSize;

        AZStd::vector<uint8_t> checkSums;
        checkSums.resize_no_construct(maxSize);
        for (int pos = 0; pos < maxSize; ++pos)
        {
            checkSums[pos] = static_cast<uint8_t>(pos % 256);
        }

        auto pArchive = archive->OpenArchive(testArchivePath.c_str(), {}, AZ::IO::INestedArchive::FLAGS_CREATE_NEW);
        EXPECT_NE(nullptr, pArchive);

        // the strategy here is to find errors related to file sizes, alignment, overwrites
        // so the first test will just repeatedly write files into the pack file with varying lengths (in odd number increments from a couple KB down to 0, including 0)
        AZStd::vector<uint8_t> orderedData;

        for (int j = 0; j < iterations; ++j)
        {
            for (int currentSize = maxSize; currentSize >= 0; currentSize -= stepSize)
            {
                auto fnBuffer = AZ::StringFunc::Path::FixedString::format("file-%i-%i.dat", currentSize, j);
                EXPECT_TRUE(pArchive->UpdateFile(fnBuffer, checkSums.data(), currentSize, compressionMethod, compressionLevel) == 0);
            }
        }

        pArchive.reset();
        EXPECT_TRUE(IsPackValid(testArchivePath.c_str()));


        // --------------------------------------------- read it back and verify
        pArchive = archive->OpenArchive(testArchivePath.c_str(), {}, openFlags);
        EXPECT_NE(nullptr, pArchive);

        for (int j = 0; j < iterations; ++j)
        {
            for (int currentSize = maxSize; currentSize >= 0; currentSize -= stepSize)
            {
                auto fnBuffer = AZ::StringFunc::Path::FixedString::format("file-%i-%i.dat", currentSize, j);
                AZ::IO::INestedArchive::Handle hand = pArchive->FindFile(fnBuffer);
                EXPECT_NE(nullptr, hand);
                EXPECT_EQ(currentSize, pArchive->GetFileSize(hand));
                EXPECT_EQ(0, pArchive->ReadFile(hand, checkSums.data()));
                for (int pos = 0; pos < currentSize; ++pos)
                {
                    EXPECT_EQ(pos % 256, checkSums[pos]);
                }
            }
        }

        pArchive.reset();
        EXPECT_TRUE(IsPackValid(testArchivePath.c_str()));
    }

    TEST_P(ArchiveCompressionTestFixture, TestArchivePacking_CompressionWithOverridenArchiveData_PackIsValid)
    {
        // ---------------- MORE COMPLICATED TEST which involves overwriting elements ----------------
        AZStd::string testArchivePath = "@usercache@/archivetest.pak";
        AZ::IO::IArchive* archive = AZ::Interface<AZ::IO::IArchive>::Get();

        auto openFlags = AZStd::get<0>(GetParam());
        auto compressionMethod = AZStd::get<1>(GetParam());
        auto compressionLevel = AZStd::get<2>(GetParam());
        auto stepSize = AZStd::get<3>(GetParam());
        auto numSteps = AZStd::get<4>(GetParam());
        auto iterations = AZStd::get<5>(GetParam());

        int maxSize = numSteps * stepSize;
        AZStd::vector<uint8_t> checkSums;
        checkSums.resize_no_construct(maxSize);
        for (int pos = 0; pos < maxSize; ++pos)
        {
            checkSums[pos] = static_cast<uint8_t>(pos % 256);
        }

        auto pArchive = archive->OpenArchive(testArchivePath.c_str());
        EXPECT_NE(nullptr, pArchive);

        for (int j = 0; j < iterations; ++j)
        {
            for (int currentSize = maxSize; currentSize >= 0; currentSize -= stepSize)
            {
                auto fnBuffer = AZ::StringFunc::Path::FixedString::format("file-%i-%i.dat", currentSize, j);
                EXPECT_TRUE(pArchive->UpdateFile(fnBuffer, checkSums.data(), currentSize, compressionMethod, compressionLevel) == 0);
            }
        }

        // overwrite the first and last iterations with files that are half their original size.
        for (int j = 0; j < iterations; ++j)
        {
            for (int currentSize = maxSize; currentSize >= 0; currentSize -= stepSize)
            {
                int newSize = currentSize; // more will become zero
                if (j != 1)
                {
                    newSize = newSize / 2; // the second iteration overwrites files with exactly the same size.
                }

                auto fnBuffer = AZ::StringFunc::Path::FixedString::format("file-%i-%i.dat", currentSize, j);

                // before we overwrite it, ensure that the element is correctly resized:
                AZ::IO::INestedArchive::Handle hand = pArchive->FindFile(fnBuffer);
                EXPECT_NE(nullptr, hand);
                EXPECT_EQ(currentSize, pArchive->GetFileSize(hand));
                EXPECT_EQ(0, pArchive->ReadFile(hand, checkSums.data()));
                for (int pos = 0; pos < currentSize; ++pos)
                {
                    EXPECT_EQ(pos % 256, checkSums[pos]);
                }

                // now overwrite it:
                EXPECT_EQ(0, pArchive->UpdateFile(fnBuffer, checkSums.data(), newSize, compressionMethod, compressionLevel));

                // after overwriting it ensure that the pack contains the updated info:
                hand = pArchive->FindFile(fnBuffer);
                EXPECT_NE(nullptr, hand);
                EXPECT_EQ(newSize, pArchive->GetFileSize(hand));
                EXPECT_EQ(0, pArchive->ReadFile(hand, checkSums.data()));
                for (int pos = 0; pos < newSize; ++pos)
                {
                    EXPECT_EQ(pos % 256, checkSums[pos]);
                }
            }
        }
        pArchive.reset();
        EXPECT_TRUE(IsPackValid(testArchivePath.c_str()));

        // -------------------------------------------------------------------------------------------
        // read it back and verify
        pArchive = archive->OpenArchive(testArchivePath.c_str(), {}, openFlags);
        EXPECT_NE(nullptr, pArchive);

        for (int j = 0; j < iterations; ++j)
        {
            for (int currentSize = maxSize; currentSize >= 0; currentSize -= stepSize)
            {
                auto fnBuffer = AZ::StringFunc::Path::FixedString::format("file-%i-%i.dat", currentSize, j);

                int newSize = currentSize; // more will become zero
                if (j != 1)
                {
                    newSize = newSize / 2; // the middle iteration overwrites files with exactly the same size.
                }

                AZ::IO::INestedArchive::Handle hand = pArchive->FindFile(fnBuffer);
                EXPECT_NE(nullptr, hand);
                EXPECT_EQ(newSize, pArchive->GetFileSize(hand));
                EXPECT_EQ(0, pArchive->ReadFile(hand, checkSums.data()));

                for (int pos = 0; pos < newSize; ++pos)
                {
                    EXPECT_EQ(pos % 256, checkSums[pos]);
                }
            }
        }

        pArchive.reset();
        EXPECT_TRUE(IsPackValid(testArchivePath.c_str()));
    }

    TEST_P(ArchiveCompressionTestFixture, TestArchivePacking_CompressionWithScatteredUpdatesAndNewFiles_PackIsValid)
    {
        // ---------- scattered test --------------
        // in this next test, we're going to update only some elements, to make sure it reads existing data okay
        // we want to make at least one element shrink and one element grow, adjacent to other files
        // this will include files that become zero size, and also includes new files that were not there before

        AZStd::string testArchivePath = "@usercache@/archivetest.pak";
        AZ::IO::IArchive* archive = AZ::Interface<AZ::IO::IArchive>::Get();

        auto openFlags = AZStd::get<0>(GetParam());
        auto compressionMethod = AZStd::get<1>(GetParam());
        auto compressionLevel = AZStd::get<2>(GetParam());
        auto stepSize = AZStd::get<3>(GetParam());
        auto numSteps = AZStd::get<4>(GetParam());
        auto iterations = AZStd::get<5>(GetParam());

        int maxSize = numSteps * stepSize;
        AZStd::vector<uint8_t> checkSums;
        checkSums.resize_no_construct(maxSize);
        for (int pos = 0; pos < maxSize; ++pos)
        {
            checkSums[pos] = static_cast<uint8_t>(pos % 256);
        }

        // first, reset the pack to the original state:
        auto pArchive = archive->OpenArchive(testArchivePath.c_str(), {}, AZ::IO::INestedArchive::FLAGS_CREATE_NEW);
        EXPECT_NE(nullptr, pArchive);

        for (int j = 0; j < iterations; ++j)
        {
            for (int currentSize = maxSize; currentSize >= 0; currentSize -= stepSize)
            {
                char fnBuffer[AZ_MAX_PATH_LEN];

                azsnprintf(fnBuffer, AZ_MAX_PATH_LEN, "file-%i-%i.dat", static_cast<int>(currentSize), j);
                EXPECT_TRUE(pArchive->UpdateFile(fnBuffer, checkSums.data(), currentSize, compressionMethod, compressionLevel) == 0);
            }
        }

        pArchive.reset();
        EXPECT_TRUE(IsPackValid(testArchivePath.c_str()));

        pArchive = archive->OpenArchive(testArchivePath.c_str());
        EXPECT_NE(nullptr, pArchive);
        // replace a scattering of the files:

        int writeCount = 0;
        for (int j = 0; j < iterations + 1; ++j) // note:  an extra iteration to generate new files
        {
            char fnBuffer[AZ_MAX_PATH_LEN];
            for (int currentSize = maxSize; currentSize >= 0; currentSize -= stepSize)
            {
                azsnprintf(fnBuffer, AZ_MAX_PATH_LEN, "file-%i-%i.dat", static_cast<int>(currentSize), j);

                ++writeCount;
                if (writeCount % 4 == 0)
                {
                    if (j != iterations) // the last one wont be there
                    {
                        // don't do anything for every fourth file, but we do make sure its there:
                        AZ::IO::INestedArchive::Handle hand = pArchive->FindFile(fnBuffer);
                        EXPECT_NE(nullptr, hand);
                        EXPECT_EQ(currentSize, pArchive->GetFileSize(hand));
                        EXPECT_EQ(0, pArchive->ReadFile(hand, checkSums.data()));
                    }

                    continue;
                }

                int newSize = currentSize;

                if (writeCount % 4 == 1)
                {
                    newSize = newSize * 2;
                }
                else if (writeCount % 4 == 2)
                {
                    newSize = newSize / 2;
                }
                else if (writeCount % 4 == 3)
                {
                    newSize = 0;
                }

                if (newSize > maxSize)
                {
                    newSize = maxSize; // don't blow our buffer!
                }



                // overwrite it:
                EXPECT_TRUE(pArchive->UpdateFile(fnBuffer, checkSums.data(), newSize, compressionMethod, compressionLevel) == 0);

                // after overwriting it ensure that the pack contains the updated info:
                AZ::IO::INestedArchive::Handle hand = pArchive->FindFile(fnBuffer);
                EXPECT_NE(nullptr, hand);
                EXPECT_EQ(newSize, pArchive->GetFileSize(hand));
                EXPECT_EQ(0, pArchive->ReadFile(hand, checkSums.data()));
                for (int pos = 0; pos < newSize; ++pos)
                {
                    EXPECT_EQ(pos % 256, checkSums[pos]);
                }
            }
        }
        EXPECT_TRUE(IsPackValid(testArchivePath.c_str()));

        // -------------------------------------------------------------------------------------------
        // read it back and verify
        pArchive = archive->OpenArchive(testArchivePath.c_str(), {}, openFlags);
        EXPECT_NE(nullptr, pArchive);

        writeCount = 0;
        for (int j = 0; j < iterations + 1; ++j) // make sure the extra iteration is there.
        {
            char fnBuffer[AZ_MAX_PATH_LEN];
            for (int currentSize = maxSize; currentSize >= 0; currentSize -= stepSize)
            {
                ++writeCount;

                int newSize = currentSize;

                if (writeCount % 4 == 1)
                {
                    newSize = newSize * 2;
                }
                else if (writeCount % 4 == 2)
                {
                    newSize = newSize / 2;
                }
                else if (writeCount % 4 == 3)
                {
                    newSize = 0;
                }
                else if (writeCount % 4 == 0)
                {
                    if (j == iterations) // the last one wont be there
                    {
                        continue;
                    }
                }

                if (newSize > maxSize)
                {
                    newSize = maxSize; // don't blow our buffer!
                }



                azsnprintf(fnBuffer, AZ_MAX_PATH_LEN, "file-%i-%i.dat", static_cast<int>(currentSize), j);

                // check it:
                AZ::IO::INestedArchive::Handle hand = pArchive->FindFile(fnBuffer);
                EXPECT_NE(nullptr, hand);
                EXPECT_EQ(newSize, pArchive->GetFileSize(hand));
                EXPECT_EQ(0, pArchive->ReadFile(hand, checkSums.data()));
                for (int pos = 0; pos < newSize; ++pos)
                {
                    EXPECT_TRUE(checkSums[pos] == (pos % 256));
                }
            }
        }
        pArchive.reset();
        EXPECT_TRUE(IsPackValid(testArchivePath.c_str()));
    }

    INSTANTIATE_TEST_CASE_P(
        ArchiveCompression,
        ArchiveCompressionTestFixture,
        ::testing::Values(
            std::tuple(AZ::IO::INestedArchive::FLAGS_READ_ONLY, AZ::IO::INestedArchive::METHOD_COMPRESS, AZ::IO::INestedArchive::LEVEL_BETTER, 777, 7, 1),
            std::tuple(static_cast<AZ::IO::INestedArchive::EPakFlags>(0), AZ::IO::INestedArchive::METHOD_COMPRESS, AZ::IO::INestedArchive::LEVEL_BETTER, 777, 7, 1),
            std::tuple(AZ::IO::INestedArchive::FLAGS_READ_ONLY, AZ::IO::INestedArchive::METHOD_COMPRESS, AZ::IO::INestedArchive::LEVEL_FASTEST, 777, 7, 1),
            std::tuple(AZ::IO::INestedArchive::FLAGS_READ_ONLY, AZ::IO::INestedArchive::METHOD_COMPRESS, AZ::IO::INestedArchive::LEVEL_FASTER, 777, 7, 1),
            std::tuple(AZ::IO::INestedArchive::FLAGS_READ_ONLY, AZ::IO::INestedArchive::METHOD_COMPRESS, AZ::IO::INestedArchive::LEVEL_NORMAL, 777, 7, 1),
            std::tuple(AZ::IO::INestedArchive::FLAGS_READ_ONLY, AZ::IO::INestedArchive::METHOD_COMPRESS, AZ::IO::INestedArchive::LEVEL_BETTER, 777, 7, 1),
            std::tuple(AZ::IO::INestedArchive::FLAGS_READ_ONLY, AZ::IO::INestedArchive::METHOD_COMPRESS, AZ::IO::INestedArchive::LEVEL_BEST, 777, 7, 1),
            std::tuple(static_cast<AZ::IO::INestedArchive::EPakFlags>(0), AZ::IO::INestedArchive::METHOD_COMPRESS, AZ::IO::INestedArchive::LEVEL_FASTEST, 777, 7, 1),
            std::tuple(static_cast<AZ::IO::INestedArchive::EPakFlags>(0), AZ::IO::INestedArchive::METHOD_COMPRESS, AZ::IO::INestedArchive::LEVEL_FASTER, 777, 7, 1),
            std::tuple(static_cast<AZ::IO::INestedArchive::EPakFlags>(0), AZ::IO::INestedArchive::METHOD_COMPRESS, AZ::IO::INestedArchive::LEVEL_NORMAL, 777, 7, 1),
            std::tuple(static_cast<AZ::IO::INestedArchive::EPakFlags>(0), AZ::IO::INestedArchive::METHOD_COMPRESS, AZ::IO::INestedArchive::LEVEL_BETTER, 777, 7, 1),
            std::tuple(static_cast<AZ::IO::INestedArchive::EPakFlags>(0), AZ::IO::INestedArchive::METHOD_COMPRESS, AZ::IO::INestedArchive::LEVEL_BEST, 777, 7, 1),
            std::tuple(AZ::IO::INestedArchive::FLAGS_READ_ONLY, AZ::IO::INestedArchive::METHOD_STORE, AZ::IO::INestedArchive::LEVEL_BETTER, 777, 7, 1),
            std::tuple(static_cast<AZ::IO::INestedArchive::EPakFlags>(0), AZ::IO::INestedArchive::METHOD_STORE, AZ::IO::INestedArchive::LEVEL_BETTER, 777, 7, 1),
            std::tuple(AZ::IO::INestedArchive::FLAGS_READ_ONLY, AZ::IO::INestedArchive::METHOD_COMPRESS, AZ::IO::INestedArchive::LEVEL_BEST, 1111, 10, 1),
            std::tuple(static_cast<AZ::IO::INestedArchive::EPakFlags>(0), AZ::IO::INestedArchive::METHOD_COMPRESS, AZ::IO::INestedArchive::LEVEL_BEST, 1111, 10, 1)
        ));
}

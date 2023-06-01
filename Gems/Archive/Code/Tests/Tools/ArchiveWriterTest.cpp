/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>

#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/std/ranges/ranges_algorithm.h>

#include <Archive/ArchiveWriterAPI.h>

namespace Archive::Test
{
    // The ArchiveEditorTestEnvironment is tracking memory
    // via the GemTestEnvironment::SetupEnvironment function
    // so the LeakDetectionFixture should not be used
    class ArchiveWriterFixture
        : public ::testing::Test
    {
    public:
        ArchiveWriterFixture() = default;
        ~ArchiveWriterFixture() = default;

    protected:
    };

    TEST_F(ArchiveWriterFixture, CreateArchiveWriter_Succeeds)
    {
        {
            auto archiveWriter = CreateArchiveWriter();
            ASSERT_NE(nullptr, archiveWriter);
        }

        {
            auto archiveWriter = CreateArchiveWriter(ArchiveWriterSettings{});
            ASSERT_NE(nullptr, archiveWriter);
        }
    }

    TEST_F(ArchiveWriterFixture, EmptyArchive_Create_Succeeds)
    {
        AZStd::vector<AZStd::byte> archiveBuffer;
        AZ::IO::ByteContainerStream archiveStream(&archiveBuffer);

        // Set the ArchiveStreamDeleter to not delete the stack ByteContainerStream
        IArchiveWriter::ArchiveStreamPtr archiveStreamPtr(&archiveStream, { false });
        AZStd::unique_ptr<IArchiveWriter> archiveWriter = CreateArchiveWriter(AZStd::move(archiveStreamPtr));

        IArchiveWriter::CommitResult commitResult = archiveWriter->Commit();
        EXPECT_TRUE(commitResult);

        // The empty archive should be have a header equal to the default constructed ArchiveHeader
        ArchiveHeader defaultArchiveHeader;
        auto defaultArchiveHeaderSpan = AZStd::span(reinterpret_cast<AZStd::byte*>(&defaultArchiveHeader), sizeof(defaultArchiveHeader));
        // The ArchiveHeader is written out as a 512-byte aligned
        auto archiveBufferSpan = AZStd::span(archiveBuffer).first(sizeof(ArchiveHeader));
        EXPECT_TRUE(AZStd::ranges::equal(archiveBufferSpan, defaultArchiveHeaderSpan));
    }

    TEST_F(ArchiveWriterFixture, Existing_Archive_CanBeWritten_Succeeds)
    {
        AZStd::vector<AZStd::byte> archiveBuffer;
        AZ::IO::ByteContainerStream archiveStream(&archiveBuffer);
        // Write an empty archive
        {
            // Set the ArchiveStreamDeleter to not delete the stack ByteContainerStream
            IArchiveWriter::ArchiveStreamPtr archiveStreamPtr(&archiveStream, { false });
            AZStd::unique_ptr<IArchiveWriter> archiveWriter = CreateArchiveWriter(AZStd::move(archiveStreamPtr));

            IArchiveWriter::CommitResult commitResult = archiveWriter->Commit();
            EXPECT_TRUE(commitResult);

            // The empty archive should be have a header equal to the default constructed ArchiveHeader
            ArchiveHeader defaultArchiveHeader;
            auto defaultArchiveHeaderSpan = AZStd::span(reinterpret_cast<AZStd::byte*>(&defaultArchiveHeader), sizeof(defaultArchiveHeader));
            // The ArchiveHeader is written out as a 512-byte aligned
            auto archiveBufferSpan = AZStd::span(archiveBuffer).first(sizeof(ArchiveHeader));
            EXPECT_TRUE(AZStd::ranges::equal(archiveBufferSpan, defaultArchiveHeaderSpan));
        }

        // Seek back to the beginning of the archiveStream and re-use it
        archiveStream.Seek(0, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);

        {
            // Set the ArchiveStreamDeleter to not delete the stack ByteContainerStream
            IArchiveWriter::ArchiveStreamPtr archiveStreamPtr(&archiveStream, { false });
            AZStd::unique_ptr<IArchiveWriter> archiveWriter = CreateArchiveWriter(AZStd::move(archiveStreamPtr));

            // Recommit the existing archive with no changes
            IArchiveWriter::CommitResult commitResult = archiveWriter->Commit();
            EXPECT_TRUE(commitResult);

            ArchiveHeader defaultArchiveHeader;
            auto defaultArchiveHeaderSpan = AZStd::span(reinterpret_cast<AZStd::byte*>(&defaultArchiveHeader), sizeof(defaultArchiveHeader));
            auto archiveBufferSpan = AZStd::span(archiveBuffer).first(sizeof(ArchiveHeader));
            // As no changes have been made to the existing archive empty archive it should still compare equal to a
            // default constructed archive header
            EXPECT_TRUE(AZStd::ranges::equal(archiveBufferSpan, defaultArchiveHeaderSpan));
        }
    }
}

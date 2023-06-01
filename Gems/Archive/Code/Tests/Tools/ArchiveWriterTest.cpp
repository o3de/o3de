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

    // Helper function for converting a string view to a byte span
    // This performs a reinterpret_cast on the string_view as buffer as pointer
    // to a contiguous sequence of AZStd::byte objects
    auto StringToByteSpan(AZStd::string_view textData) -> AZStd::span<const AZStd::byte>
    {
        return AZStd::as_bytes(AZStd::span(textData));
    }

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

    TEST_F(ArchiveWriterFixture, Archive_WithSingleFileAdded_Suceeds)
    {
        AZStd::vector<AZStd::byte> archiveBuffer;
        AZ::IO::ByteContainerStream archiveStream(&archiveBuffer);

        // Set the ArchiveStreamDeleter to not delete the stack ByteContainerStream
        IArchiveWriter::ArchiveStreamPtr archiveStreamPtr(&archiveStream, { false });
        AZStd::unique_ptr<IArchiveWriter> archiveWriter = CreateArchiveWriter(AZStd::move(archiveStreamPtr));

        // Add an uncompressed file to the Archive
        AZStd::string_view fileContent = "Hello World";
        ArchiveWriterFileSettings fileSettings;
        fileSettings.m_relativeFilePath = "Sanity/test.txt";

        auto addFileResult = archiveWriter->AddFileToArchive(StringToByteSpan(fileContent), fileSettings);

        EXPECT_TRUE(addFileResult);
        // A successfully added file should not return InvalidArchiveFileToken
        EXPECT_NE(InvalidArchiveFileToken, addFileResult.m_filePathToken);

        // the ArchiveWriterFileSettings defaults to lowercasing paths added to the archive
        AZ::IO::Path loweredCaseFilePath = fileSettings.m_relativeFilePath;
        AZStd::to_lower(loweredCaseFilePath.Native());
        EXPECT_EQ(loweredCaseFilePath, addFileResult.m_relativeFilePath);
        // The file should not be compressed
        EXPECT_EQ(Compression::Uncompressed, addFileResult.m_compressionAlgorithm);

        IArchiveWriter::CommitResult commitResult = archiveWriter->Commit();
        EXPECT_TRUE(commitResult);


        ArchiveHeader expectedArchiveHeader;
        // As there is one file in the archive which is less than 512 bytes
        // The expected Table of Contents offset would be
        // 512 to account for the table of contents + 512 for the single file block that is aligned to the
        // next multiple of 512
        // So the Table of Contents should start at offset 1024
        expectedArchiveHeader.m_tocOffset = ArchiveDefaultBlockAlignment + ArchiveDefaultBlockAlignment;
        expectedArchiveHeader.m_fileCount = 1;
        // Update the uncompressed sizes for the table of contents based on the number of files in the toc
        // plus the number of block lines used by that file
        expectedArchiveHeader.m_tocFileMetadataTableUncompressedSize = sizeof(ArchiveTocFileMetadata) * 1;
        expectedArchiveHeader.m_tocPathIndexTableUncompressedSize = sizeof(ArchiveTocFilePathIndex) * 1;
        // The path blob should only contain the single filename
        expectedArchiveHeader.m_tocPathBlobUncompressedSize = static_cast<AZ::u32>(fileSettings.m_relativeFilePath.Native().size());
        // As the file is not compressed the block offset table should not have an entry in it
        expectedArchiveHeader.m_tocBlockOffsetTableUncompressedSize = 0;

        auto expectedArchiveHeaderSpan = AZStd::span(reinterpret_cast<AZStd::byte*>(&expectedArchiveHeader), sizeof(expectedArchiveHeader));
        // The ArchiveHeader is written out as a 512-byte aligned
        auto archiveBufferSpan = AZStd::span(archiveBuffer).first(sizeof(ArchiveHeader));
        EXPECT_TRUE(AZStd::ranges::equal(archiveBufferSpan, expectedArchiveHeaderSpan));
    }
}

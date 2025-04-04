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

#include <Archive/Tools/ArchiveWriterAPI.h>

#include <Compression/CompressionLZ4API.h>

// Archive Gem private implementation includes
#include <Tools/ArchiveWriterFactory.h>

namespace Archive::Test
{
    // The ArchiveEditorTestEnvironment is tracking memory
    // via the GemTestEnvironment::SetupEnvironment function
    // so the LeakDetectionFixture should not be used
    class ArchiveWriterFixture
        : public ::testing::Test
    {
    public:
        ArchiveWriterFixture()
        {
            m_archiveWriterFactory = AZStd::make_unique<ArchiveWriterFactory>();
            AZ::Interface<IArchiveWriterFactory>::Register(m_archiveWriterFactory.get());
        }
        ~ArchiveWriterFixture()
        {
            AZ::Interface<IArchiveWriterFactory>::Unregister(m_archiveWriterFactory.get());
        }

    protected:
        // Create and register Archive Writer factory
        // to allow IArchiveWriter instances to be created
        AZStd::unique_ptr<IArchiveWriterFactory> m_archiveWriterFactory;
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
            auto createArchiveWriterResult = CreateArchiveWriter();
            ASSERT_TRUE(createArchiveWriterResult);
            AZStd::unique_ptr<IArchiveWriter> archiveWriter = AZStd::move(createArchiveWriterResult.value());
            ASSERT_NE(nullptr, archiveWriter);
        }

        {
            auto createArchiveWriterResult = CreateArchiveWriter(ArchiveWriterSettings{});
            ASSERT_TRUE(createArchiveWriterResult);
            AZStd::unique_ptr<IArchiveWriter> archiveWriter = AZStd::move(createArchiveWriterResult.value());
            ASSERT_NE(nullptr, archiveWriter);
        }
    }

    TEST_F(ArchiveWriterFixture, EmptyArchive_Create_Succeeds)
    {
        AZStd::vector<AZStd::byte> archiveBuffer;
        AZ::IO::ByteContainerStream archiveStream(&archiveBuffer);

        // Set the ArchiveStreamDeleter to not delete the stack ByteContainerStream
        IArchiveWriter::ArchiveStreamPtr archiveStreamPtr(&archiveStream, { false });
        auto createArchiveWriterResult = CreateArchiveWriter(AZStd::move(archiveStreamPtr));
        ASSERT_TRUE(createArchiveWriterResult);
        AZStd::unique_ptr<IArchiveWriter> archiveWriter = AZStd::move(createArchiveWriterResult.value());

        IArchiveWriter::CommitResult commitResult = archiveWriter->Commit();
        ASSERT_TRUE(commitResult);

        // The empty archive should be have a header equal to the default constructed ArchiveHeader
        ArchiveHeader defaultArchiveHeader;
        auto defaultArchiveHeaderSpan = AZStd::span(reinterpret_cast<AZStd::byte*>(&defaultArchiveHeader), sizeof(defaultArchiveHeader));
        // The ArchiveHeader is written out as a 512-byte aligned
        auto archiveBufferSpan = AZStd::span(archiveBuffer).first(sizeof(ArchiveHeader));
        EXPECT_TRUE(AZStd::ranges::equal(archiveBufferSpan, defaultArchiveHeaderSpan));
    }

    TEST_F(ArchiveWriterFixture, ExistingArchive_CanBeWritten_Succeeds)
    {
        AZStd::vector<AZStd::byte> archiveBuffer;
        AZ::IO::ByteContainerStream archiveStream(&archiveBuffer);
        // Write an empty archive
        {
            // Set the ArchiveStreamDeleter to not delete the stack ByteContainerStream
            IArchiveWriter::ArchiveStreamPtr archiveStreamPtr(&archiveStream, { false });
            auto createArchiveWriterResult = CreateArchiveWriter(AZStd::move(archiveStreamPtr));
            ASSERT_TRUE(createArchiveWriterResult);
            AZStd::unique_ptr<IArchiveWriter> archiveWriter = AZStd::move(createArchiveWriterResult.value());

            IArchiveWriter::CommitResult commitResult = archiveWriter->Commit();
            ASSERT_TRUE(commitResult);

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
            auto createArchiveWriterResult = CreateArchiveWriter(AZStd::move(archiveStreamPtr));
            ASSERT_TRUE(createArchiveWriterResult);
            AZStd::unique_ptr<IArchiveWriter> archiveWriter = AZStd::move(createArchiveWriterResult.value());

            // Recommit the existing archive with no changes
            IArchiveWriter::CommitResult commitResult = archiveWriter->Commit();
            ASSERT_TRUE(commitResult);

            ArchiveHeader defaultArchiveHeader;
            auto defaultArchiveHeaderSpan = AZStd::span(reinterpret_cast<AZStd::byte*>(&defaultArchiveHeader), sizeof(defaultArchiveHeader));
            auto archiveBufferSpan = AZStd::span(archiveBuffer).first(sizeof(ArchiveHeader));
            // As no changes have been made to the existing archive empty archive it should still compare equal to a
            // default constructed archive header
            EXPECT_TRUE(AZStd::ranges::equal(archiveBufferSpan, defaultArchiveHeaderSpan));
        }
    }

    TEST_F(ArchiveWriterFixture, Archive_WithSingleUncompressedFileAdded_Succeeds)
    {
        AZStd::vector<AZStd::byte> archiveBuffer;
        AZ::IO::ByteContainerStream archiveStream(&archiveBuffer);

        // Set the ArchiveStreamDeleter to not delete the stack ByteContainerStream
        IArchiveWriter::ArchiveStreamPtr archiveStreamPtr(&archiveStream, { false });
        auto createArchiveWriterResult = CreateArchiveWriter(AZStd::move(archiveStreamPtr));
        ASSERT_TRUE(createArchiveWriterResult);
        AZStd::unique_ptr<IArchiveWriter> archiveWriter = AZStd::move(createArchiveWriterResult.value());

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

        // Commit the archive Header and Table of Contents to the stream
        IArchiveWriter::CommitResult commitResult = archiveWriter->Commit();
        ASSERT_TRUE(commitResult);

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

    TEST_F(ArchiveWriterFixture, Archive_WithSingleLZ4CompressedFileAdded_Succeeds)
    {
        AZStd::vector<AZStd::byte> archiveBuffer;
        AZ::IO::ByteContainerStream archiveStream(&archiveBuffer);

        // Set the ArchiveStreamDeleter to not delete the stack ByteContainerStream
        IArchiveWriter::ArchiveStreamPtr archiveStreamPtr(&archiveStream, { false });
        auto createArchiveWriterResult = CreateArchiveWriter(AZStd::move(archiveStreamPtr));
        ASSERT_TRUE(createArchiveWriterResult);
        AZStd::unique_ptr<IArchiveWriter> archiveWriter = AZStd::move(createArchiveWriterResult.value());

        // Add an uncompressed file to the Archive
        AZStd::string_view fileContent = "Hello World";
        ArchiveWriterFileSettings fileSettings;
        // For this test also validate the upper casing of an added file
        fileSettings.m_fileCase = ArchiveFilePathCase::Uppercase;
        fileSettings.m_relativeFilePath = "Sanity/test.txt";
        fileSettings.m_compressionAlgorithm = CompressionLZ4::GetLZ4CompressionAlgorithmId();

        auto addFileResult = archiveWriter->AddFileToArchive(StringToByteSpan(fileContent), fileSettings);

        EXPECT_TRUE(addFileResult);
        // A successfully added file should not return InvalidArchiveFileToken
        EXPECT_NE(InvalidArchiveFileToken, addFileResult.m_filePathToken);

        // the file settings indicate that the relative path should be added as uppercased
        AZ::IO::Path upperCaseFilePath = fileSettings.m_relativeFilePath;
        AZStd::to_upper(upperCaseFilePath.Native());
        EXPECT_EQ(upperCaseFilePath, addFileResult.m_relativeFilePath);
        // The file should be compressed using the LZ4 compression algorithm
        EXPECT_EQ(CompressionLZ4::GetLZ4CompressionAlgorithmId(), addFileResult.m_compressionAlgorithm);

        IArchiveWriter::CommitResult commitResult = archiveWriter->Commit();
        ASSERT_TRUE(commitResult);

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
        // The file is compressed in this case, so there should be single block line entry as the file uncompressed size
        // is below Archive::MaxBlockLineSize which is made up of 3 * 2 MiB blocks encoded in a single AZ::u64
        expectedArchiveHeader.m_tocBlockOffsetTableUncompressedSize = sizeof(ArchiveBlockLineUnion) * 1;

        // Since a compression algorithm is being used set the first entry in the ArchiveHeader m_compressionAlgorithmIds array
        size_t currentCompressionAlgorithmIndex{};
        expectedArchiveHeader.m_compressionAlgorithmsIds[currentCompressionAlgorithmIndex++] = CompressionLZ4::GetLZ4CompressionAlgorithmId();

        auto expectedArchiveHeaderSpan = AZStd::span(reinterpret_cast<AZStd::byte*>(&expectedArchiveHeader), sizeof(expectedArchiveHeader));
        // The ArchiveHeader is written out as a 512-byte aligned
        auto archiveBufferSpan = AZStd::span(archiveBuffer).first(sizeof(ArchiveHeader));
        EXPECT_TRUE(AZStd::ranges::equal(archiveBufferSpan, expectedArchiveHeaderSpan));
    }

    TEST_F(ArchiveWriterFixture, FindFile_OnlyReturnsFilesInsideArchive)
    {
        AZStd::vector<AZStd::byte> archiveBuffer;
        AZ::IO::ByteContainerStream archiveStream(&archiveBuffer);

        // Set the ArchiveStreamDeleter to not delete the stack ByteContainerStream
        IArchiveWriter::ArchiveStreamPtr archiveStreamPtr(&archiveStream, { false });
        auto createArchiveWriterResult = CreateArchiveWriter(AZStd::move(archiveStreamPtr));
        ASSERT_TRUE(createArchiveWriterResult);
        AZStd::unique_ptr<IArchiveWriter> archiveWriter = AZStd::move(createArchiveWriterResult.value());

        AZStd::array<ArchiveAddFileResult, 2> addedFileResults;

        {
            // Add first file to archive
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

            addedFileResults[0] = AZStd::move(addFileResult);
        }

        {
            // Add second file to archive
            AZStd::string_view fileContent = "Goodbye World";
            ArchiveWriterFileSettings fileSettings;
            fileSettings.m_relativeFilePath = "Sanity/chess.txt";
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

            addedFileResults[1] = AZStd::move(addFileResult);
        }

        // Query the file using the path
        EXPECT_EQ(addedFileResults[0].m_filePathToken, archiveWriter->FindFile(addedFileResults[0].m_relativeFilePath));
        EXPECT_TRUE(archiveWriter->ContainsFile(addedFileResults[0].m_relativeFilePath));
        EXPECT_EQ(addedFileResults[1].m_filePathToken, archiveWriter->FindFile(addedFileResults[1].m_relativeFilePath));
        EXPECT_TRUE(archiveWriter->ContainsFile(addedFileResults[1].m_relativeFilePath));

        // Validate an empty path is not found
        EXPECT_EQ(InvalidArchiveFileToken, archiveWriter->FindFile(""));
        EXPECT_FALSE(archiveWriter->ContainsFile(""));

        // Validate a path not in the archive is not found
        constexpr AZ::IO::PathView notInArchivePath = "NotInArchive";
        EXPECT_EQ(InvalidArchiveFileToken, archiveWriter->FindFile(notInArchivePath));
        EXPECT_FALSE(archiveWriter->ContainsFile(notInArchivePath));

        // Commit the archive Header and Table of Contents to the stream
        IArchiveWriter::CommitResult commitResult = archiveWriter->Commit();
        ASSERT_TRUE(commitResult);

        // Second check - Make sure commiting the table of contents to the Archive Stream
        // does affect the ability to search files within the archive

        EXPECT_EQ(addedFileResults[0].m_filePathToken, archiveWriter->FindFile(addedFileResults[0].m_relativeFilePath));
        EXPECT_TRUE(archiveWriter->ContainsFile(addedFileResults[0].m_relativeFilePath));
        EXPECT_EQ(addedFileResults[1].m_filePathToken, archiveWriter->FindFile(addedFileResults[1].m_relativeFilePath));
        EXPECT_TRUE(archiveWriter->ContainsFile(addedFileResults[1].m_relativeFilePath));

        // empty path
        EXPECT_EQ(InvalidArchiveFileToken, archiveWriter->FindFile(""));
        EXPECT_FALSE(archiveWriter->ContainsFile(""));

        // not in archive path
        EXPECT_EQ(InvalidArchiveFileToken, archiveWriter->FindFile(notInArchivePath));
        EXPECT_FALSE(archiveWriter->ContainsFile(notInArchivePath));
    }

    TEST_F(ArchiveWriterFixture, Archive_MountAndUnmount_Succeeds)
    {
        AZStd::vector<AZStd::byte> archiveBuffer;
        AZ::IO::ByteContainerStream archiveStream(&archiveBuffer);

        // Set the ArchiveStreamDeleter to not delete the stack ByteContainerStream
        IArchiveWriter::ArchiveStreamPtr archiveStreamPtr(&archiveStream, { false });
        auto createArchiveWriterResult = CreateArchiveWriter(AZStd::move(archiveStreamPtr));
        ASSERT_TRUE(createArchiveWriterResult);
        AZStd::unique_ptr<IArchiveWriter> archiveWriter = AZStd::move(createArchiveWriterResult.value());

        EXPECT_TRUE(archiveWriter->IsMounted());

        ArchiveHeader expectedArchiveHeader;
        {
            // Write a uncompressed file to the mounted archive
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

            // Unmounting the archive also commits the archive header and table of contents
            // to the stream
            archiveWriter->UnmountArchive();

            EXPECT_FALSE(archiveWriter->IsMounted());

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
            // The ArchiveHeader is written out as a 512-byte aligned objefct
            // trunacate the span to the size the ArchiveHeader
            auto archiveBufferSpan = AZStd::span(archiveBuffer).first(sizeof(ArchiveHeader));
            EXPECT_TRUE(AZStd::ranges::equal(archiveBufferSpan, expectedArchiveHeaderSpan));
        }

        // Make a copy of archive buffer after being unmounted
        auto copiedArchiveBuffer = archiveBuffer;

        // Reset the archive StreamPtr and remount it
        {
            archiveStream.Seek(0, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);
            archiveStreamPtr.reset(&archiveStream);
            EXPECT_TRUE(archiveWriter->MountArchive(AZStd::move(archiveStreamPtr)));
            EXPECT_TRUE(archiveWriter->IsMounted());
            archiveWriter->UnmountArchive();

            // Verify that the archive contents are the same after the Commit call
            // via the second UnmountArchive operation
            EXPECT_EQ(copiedArchiveBuffer, archiveBuffer);
        }

        // Mount the archive a third time, this time remove a file
        {
            archiveStream.Seek(0, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);
            archiveStreamPtr.reset(&archiveStream);
            EXPECT_TRUE(archiveWriter->MountArchive(AZStd::move(archiveStreamPtr)));
            EXPECT_TRUE(archiveWriter->IsMounted());

            // Remove using file path
            ArchiveRemoveFileResult removeResult = archiveWriter->RemoveFileFromArchive("sanity/test.txt");
            EXPECT_TRUE(removeResult);

            // Unmount the archive again to update the header and TOC
            archiveWriter->UnmountArchive();

            // Updated the expected archive header to account for the removed file
            // The file block data has not been removed, so the Table of Contents
            // offset hasn't changed.
            // However the file count and the table of content uncompressed sizes
            // should been reduced back to 0.
            expectedArchiveHeader.m_fileCount = 0;
            expectedArchiveHeader.m_tocFileMetadataTableUncompressedSize = 0;
            expectedArchiveHeader.m_tocPathIndexTableUncompressedSize = 0;
            expectedArchiveHeader.m_tocPathBlobUncompressedSize = 0;

            // As a file has been removed, the deleted block offset table has been updated
            // It should point to the lowest offset where a deleted file was located
            expectedArchiveHeader.m_firstDeletedBlockOffset = removeResult.m_offset;

            auto expectedArchiveHeaderSpan = AZStd::span(reinterpret_cast<AZStd::byte*>(&expectedArchiveHeader), sizeof(expectedArchiveHeader));
            auto archiveBufferSpan = AZStd::span(archiveBuffer).first(sizeof(ArchiveHeader));
            EXPECT_TRUE(AZStd::ranges::equal(archiveBufferSpan, expectedArchiveHeaderSpan));
        }
    }

    TEST_F(ArchiveWriterFixture, Archive_AddAndRemoveContentFile_InSameMountedArchive_Succeeds)
    {
        AZStd::vector<AZStd::byte> archiveBuffer;
        AZ::IO::ByteContainerStream archiveStream(&archiveBuffer);

        // Set the ArchiveStreamDeleter to not delete the stack ByteContainerStream
        IArchiveWriter::ArchiveStreamPtr archiveStreamPtr(&archiveStream, { false });
        auto createArchiveWriterResult = CreateArchiveWriter(AZStd::move(archiveStreamPtr));
        ASSERT_TRUE(createArchiveWriterResult);
        AZStd::unique_ptr<IArchiveWriter> archiveWriter = AZStd::move(createArchiveWriterResult.value());

        AZStd::array<ArchiveAddFileResult, 2> addedFileResults;

        {
            // Add an uncompressed file to archive
            AZStd::string_view fileContent = "Hello World";
            ArchiveWriterFileSettings fileSettings;
            fileSettings.m_relativeFilePath = "Sanity/test.txt";
            auto addFileResult = archiveWriter->AddFileToArchive(StringToByteSpan(fileContent), fileSettings);

            EXPECT_TRUE(addFileResult);
            EXPECT_NE(InvalidArchiveFileToken, addFileResult.m_filePathToken);

            // the ArchiveWriterFileSettings defaults to lowercasing paths added to the archive
            AZ::IO::Path loweredCaseFilePath = fileSettings.m_relativeFilePath;
            AZStd::to_lower(loweredCaseFilePath.Native());
            EXPECT_EQ(loweredCaseFilePath, addFileResult.m_relativeFilePath);

            EXPECT_EQ(Compression::Uncompressed, addFileResult.m_compressionAlgorithm);

            addedFileResults[0] = AZStd::move(addFileResult);
        }

        {
            // Add a compressed file to archive
            AZStd::string_view fileContent = "Goodbye World";
            ArchiveWriterFileSettings fileSettings;
            fileSettings.m_compressionAlgorithm = CompressionLZ4::GetLZ4CompressionAlgorithmId();
            fileSettings.m_relativeFilePath = "Sanity/chess.txt";
            auto addFileResult = archiveWriter->AddFileToArchive(StringToByteSpan(fileContent), fileSettings);

            EXPECT_TRUE(addFileResult);
            EXPECT_NE(InvalidArchiveFileToken, addFileResult.m_filePathToken);

            // the ArchiveWriterFileSettings defaults to lowercasing paths added to the archive
            AZ::IO::Path loweredCaseFilePath = fileSettings.m_relativeFilePath;
            AZStd::to_lower(loweredCaseFilePath.Native());
            EXPECT_EQ(loweredCaseFilePath, addFileResult.m_relativeFilePath);

            EXPECT_EQ(CompressionLZ4::GetLZ4CompressionAlgorithmId(), addFileResult.m_compressionAlgorithm);

            addedFileResults[1] = AZStd::move(addFileResult);
        }

        // Commit the archive header and table of contents to the stream
        IArchiveWriter::CommitResult commitResult = archiveWriter->Commit();
        ASSERT_TRUE(commitResult);

        // Since the header is aligned on 512-byte boundary + the first 2 files
        // are aligned on a 512-byte boundary the TOC start offset is
        // 512 + (2 * 512)
        AZ::u64 tocStartOffset = ArchiveDefaultBlockAlignment +
            (ArchiveDefaultBlockAlignment * addedFileResults.size());
        // The Archive should contain 2 files
        {
            ArchiveHeader expectedArchiveHeader;
            expectedArchiveHeader.m_fileCount = 2;
            expectedArchiveHeader.m_tocOffset = tocStartOffset;
            // Update the uncompressed sizes for the table of contents based on the number of files in the TOC
            expectedArchiveHeader.m_tocFileMetadataTableUncompressedSize = sizeof(ArchiveTocFileMetadata) * 2;
            expectedArchiveHeader.m_tocPathIndexTableUncompressedSize = sizeof(ArchiveTocFilePathIndex) * 2;
            // The path blob should contain the both file paths back-to-back with bytes in-between
            AZ::u32 filePathBlobTableSize = static_cast<AZ::u32>(addedFileResults[0].m_relativeFilePath.Native().size());
            filePathBlobTableSize += static_cast<AZ::u32>(addedFileResults[1].m_relativeFilePath.Native().size());

            expectedArchiveHeader.m_tocPathBlobUncompressedSize = filePathBlobTableSize;
            // Since one of the files are compressed and is under 6 MiB, there should be 1 block line in
            // the block offset table
            expectedArchiveHeader.m_tocBlockOffsetTableUncompressedSize = sizeof(ArchiveBlockLineUnion) * 1;

            // Since a compression algorithm is being used set the first entry in the ArchiveHeader m_compressionAlgorithmIds array
            size_t currentCompressionAlgorithmIndex{};
            expectedArchiveHeader.m_compressionAlgorithmsIds[currentCompressionAlgorithmIndex++] = CompressionLZ4::GetLZ4CompressionAlgorithmId();

            auto expectedArchiveHeaderSpan = AZStd::span(reinterpret_cast<AZStd::byte*>(&expectedArchiveHeader), sizeof(expectedArchiveHeader));
            // The ArchiveHeader is written out as a 512-byte aligned
            auto archiveBufferSpan = AZStd::span(archiveBuffer).first(sizeof(ArchiveHeader));
            EXPECT_TRUE(AZStd::ranges::equal(archiveBufferSpan, expectedArchiveHeaderSpan));
        }

        // Now remove the compressed file from the Archive
        ArchiveRemoveFileResult removeFileResult;
        {
            ArchiveAddFileResult& compressedFileAddResult = addedFileResults[1];
            // Use the token to remove the file in this case
            removeFileResult = archiveWriter->RemoveFileFromArchive(compressedFileAddResult.m_filePathToken);
            EXPECT_TRUE(removeFileResult);
        }

        // Commit the update archive header and table of contents to stream
        commitResult = archiveWriter->Commit();
        ASSERT_TRUE(commitResult);

        // The Archive should now only contain 1 file
        {
            ArchiveHeader expectedArchiveHeader;
            expectedArchiveHeader.m_fileCount = 1;
            // NOTE: Removing a file that does remove it's deleted blocks from the archive data
            // Therefore the TOC Offset never decreases
            expectedArchiveHeader.m_tocOffset = expectedArchiveHeader.m_tocOffset = tocStartOffset;
            expectedArchiveHeader.m_tocFileMetadataTableUncompressedSize = sizeof(ArchiveTocFileMetadata) * 1;
            expectedArchiveHeader.m_tocPathIndexTableUncompressedSize = sizeof(ArchiveTocFilePathIndex) * 1;
            // The path blob should contain only the uncompressed file
            AZ::u32 filePathBlobTableSize = static_cast<AZ::u32>(addedFileResults[0].m_relativeFilePath.Native().size());

            expectedArchiveHeader.m_tocPathBlobUncompressedSize = filePathBlobTableSize;
            // The block offset table DOES NOT remove deleted entries in order to improve performance
            // So there should still be a single block line entry in the table
            expectedArchiveHeader.m_tocBlockOffsetTableUncompressedSize = sizeof(ArchiveBlockLineUnion) * 1;

            // The compression algorithm IDs registered in the ArchiveHeader are not removed form the archive
            // If the desire is to clear out unneeded compression algorithm ID, then a new archive can be
            // created and the contents of the old archive should be copied to it
            size_t currentCompressionAlgorithmIndex{};
            expectedArchiveHeader.m_compressionAlgorithmsIds[currentCompressionAlgorithmIndex++] = CompressionLZ4::GetLZ4CompressionAlgorithmId();

            // As the compressed file was removed, the TOC first deleted block offset is updated to point
            // to the offset of the lowest deleted file block
            // Use the remove file result local to update the expected value with that offset
            expectedArchiveHeader.m_firstDeletedBlockOffset = removeFileResult.m_offset;

            auto expectedArchiveHeaderSpan = AZStd::span(reinterpret_cast<AZStd::byte*>(&expectedArchiveHeader), sizeof(expectedArchiveHeader));
            // The ArchiveHeader is written out as a 512-byte aligned
            auto archiveBufferSpan = AZStd::span(archiveBuffer).first(sizeof(ArchiveHeader));
            EXPECT_TRUE(AZStd::ranges::equal(archiveBufferSpan, expectedArchiveHeaderSpan));
        }

        // Now remove the uncompressed file from the Archive
        {
            ArchiveAddFileResult& uncompressedFileAddResult = addedFileResults[0];
            // NOTE: This time use the relative file path
            removeFileResult = archiveWriter->RemoveFileFromArchive(uncompressedFileAddResult.m_filePathToken);
            EXPECT_TRUE(removeFileResult);
        }

        // Commit the update archive header and table of contents to stream
        commitResult = archiveWriter->Commit();
        ASSERT_TRUE(commitResult);

        // The Archive should now be empty
        {
            ArchiveHeader expectedArchiveHeader;
            expectedArchiveHeader.m_fileCount = 0;
            // NOTE: Removing a file that does remove it's deleted blocks from the archive data
            // Therefore the TOC Offset never decreases
            expectedArchiveHeader.m_tocOffset = expectedArchiveHeader.m_tocOffset = tocStartOffset;
            expectedArchiveHeader.m_tocFileMetadataTableUncompressedSize = 0;
            expectedArchiveHeader.m_tocPathIndexTableUncompressedSize = 0;
            // The path blob table should now be empty
            AZ::u32 filePathBlobTableSize = 0;

            expectedArchiveHeader.m_tocPathBlobUncompressedSize = filePathBlobTableSize;
            // The block offset table DOES NOT remove deleted entries in order to improve performance
            // So there should still be a single block line entry in the table
            expectedArchiveHeader.m_tocBlockOffsetTableUncompressedSize = sizeof(ArchiveBlockLineUnion) * 1;

            // The compression algorithm IDs registered in the ArchiveHeader are not removed form the archive
            // If the desire is to clear out unneeded compression algorithm ID, then a new archive can be
            // created and the contents of the old archive should be copied to it
            size_t currentCompressionAlgorithmIndex{};
            expectedArchiveHeader.m_compressionAlgorithmsIds[currentCompressionAlgorithmIndex++] = CompressionLZ4::GetLZ4CompressionAlgorithmId();

            // The Deleted block offset should now be 0x200 as the all the files in the archive have been deleted
            expectedArchiveHeader.m_firstDeletedBlockOffset = removeFileResult.m_offset;

            auto expectedArchiveHeaderSpan = AZStd::span(reinterpret_cast<AZStd::byte*>(&expectedArchiveHeader), sizeof(expectedArchiveHeader));
            // The ArchiveHeader is written out as a 512-byte aligned
            auto archiveBufferSpan = AZStd::span(archiveBuffer).first(sizeof(ArchiveHeader));
            EXPECT_TRUE(AZStd::ranges::equal(archiveBufferSpan, expectedArchiveHeaderSpan));
        }

        // Finally add back a file with the same name as the compressed file
        {
            // Add a compressed file to archive
            AZStd::string_view fileContent = "Big World";
            ArchiveWriterFileSettings fileSettings;
            fileSettings.m_compressionAlgorithm = CompressionLZ4::GetLZ4CompressionAlgorithmId();
            fileSettings.m_relativeFilePath = "Sanity/chess.txt";
            auto addFileResult = archiveWriter->AddFileToArchive(StringToByteSpan(fileContent), fileSettings);

            EXPECT_TRUE(addFileResult);
            EXPECT_NE(InvalidArchiveFileToken, addFileResult.m_filePathToken);

            // the ArchiveWriterFileSettings defaults to lowercasing paths added to the archive
            AZ::IO::Path loweredCaseFilePath = fileSettings.m_relativeFilePath;
            AZStd::to_lower(loweredCaseFilePath.Native());
            EXPECT_EQ(loweredCaseFilePath, addFileResult.m_relativeFilePath);

            EXPECT_EQ(CompressionLZ4::GetLZ4CompressionAlgorithmId(), addFileResult.m_compressionAlgorithm);

            addedFileResults[1] = AZStd::move(addFileResult);
        }

        // Commit the update archive header and table of contents to stream
        commitResult = archiveWriter->Commit();
        ASSERT_TRUE(commitResult);

        // The should be back at 1 file again
        {
            ArchiveHeader expectedArchiveHeader;
            expectedArchiveHeader.m_fileCount = 1;
            // NOTE: Removing a file that does remove it's deleted blocks from the archive data
            // Therefore the TOC Offset never decreases
            expectedArchiveHeader.m_tocOffset = expectedArchiveHeader.m_tocOffset = tocStartOffset;
            expectedArchiveHeader.m_tocFileMetadataTableUncompressedSize = sizeof(ArchiveTocFileMetadata) * 1;
            expectedArchiveHeader.m_tocPathIndexTableUncompressedSize = sizeof(ArchiveTocFilePathIndex) * 1;
            // The path blob should contain only the uncompressed file
            AZ::u32 filePathBlobTableSize = static_cast<AZ::u32>(addedFileResults[1].m_relativeFilePath.Native().size());

            expectedArchiveHeader.m_tocPathBlobUncompressedSize = filePathBlobTableSize;
            // Since a second compressed file have been added
            // the TOC block offset table uncompressed size should have grown by 1 block line
            // As existing block lines aren't reused in the TOC, it is ever increasing
            expectedArchiveHeader.m_tocBlockOffsetTableUncompressedSize = sizeof(ArchiveBlockLineUnion) * 2;

            // The compression algorithm IDs registered in the ArchiveHeader are not removed form the archive
            // If the desire is to clear out unneeded compression algorithm ID, then a new archive can be
            // created and the contents of the old archive should be copied to it
            size_t currentCompressionAlgorithmIndex{};
            expectedArchiveHeader.m_compressionAlgorithmsIds[currentCompressionAlgorithmIndex++] = CompressionLZ4::GetLZ4CompressionAlgorithmId();

            // The deleted block offset should now be 0x400 as the previous deleted block offset of 0x200
            // had the compressed data written to it from the string of "Big World"
            // That compressed data is less than 512-bytes, therefore the next deleted block offset
            // is rounded up to the nearest multiple of 512 which is 0x400 or 1024
            expectedArchiveHeader.m_firstDeletedBlockOffset = removeFileResult.m_offset + ArchiveDefaultBlockAlignment;

            auto expectedArchiveHeaderSpan = AZStd::span(reinterpret_cast<AZStd::byte*>(&expectedArchiveHeader), sizeof(expectedArchiveHeader));
            // The ArchiveHeader is written out as a 512-byte aligned
            auto archiveBufferSpan = AZStd::span(archiveBuffer).first(sizeof(ArchiveHeader));
            EXPECT_TRUE(AZStd::ranges::equal(archiveBufferSpan, expectedArchiveHeaderSpan));
        }
    }

    TEST_F(ArchiveWriterFixture, AddFileToArchive_CanUpdateExistingFile_Succeeds)
    {
        AZStd::vector<AZStd::byte> archiveBuffer;
        AZ::IO::ByteContainerStream archiveStream(&archiveBuffer);

        // Set the ArchiveStreamDeleter to not delete the stack ByteContainerStream
        IArchiveWriter::ArchiveStreamPtr archiveStreamPtr(&archiveStream, { false });
        auto createArchiveWriterResult = CreateArchiveWriter(AZStd::move(archiveStreamPtr));
        ASSERT_TRUE(createArchiveWriterResult);
        AZStd::unique_ptr<IArchiveWriter> archiveWriter = AZStd::move(createArchiveWriterResult.value());

        // Add an compressed file to archive
        AZStd::string_view fileContent = "Hello World";
        ArchiveWriterFileSettings fileSettings;
        fileSettings.m_relativeFilePath = "Sanity/test.txt";
        auto addFileResult = archiveWriter->AddFileToArchive(StringToByteSpan(fileContent), fileSettings);

        EXPECT_TRUE(addFileResult);
        EXPECT_NE(InvalidArchiveFileToken, addFileResult.m_filePathToken);

        // the ArchiveWriterFileSettings defaults to lowercasing paths added to the archive
        AZ::IO::Path loweredCaseFilePath = fileSettings.m_relativeFilePath;
        AZStd::to_lower(loweredCaseFilePath.Native());
        EXPECT_EQ(loweredCaseFilePath, addFileResult.m_relativeFilePath);

        EXPECT_EQ(Compression::Uncompressed, addFileResult.m_compressionAlgorithm);

        // Write the archive header and table of contents to stream
        IArchiveWriter::CommitResult commitResult = archiveWriter->Commit();
        ASSERT_TRUE(commitResult);

        // There should be a single file in the archive
        {
            ArchiveHeader expectedArchiveHeader;
            expectedArchiveHeader.m_tocOffset = ArchiveDefaultBlockAlignment + ArchiveDefaultBlockAlignment;
            expectedArchiveHeader.m_fileCount = 1;
            expectedArchiveHeader.m_tocFileMetadataTableUncompressedSize = sizeof(ArchiveTocFileMetadata) * 1;
            expectedArchiveHeader.m_tocPathIndexTableUncompressedSize = sizeof(ArchiveTocFilePathIndex) * 1;
            // The path blob should contain only the compressed file path
            AZ::u32 filePathBlobTableSize = static_cast<AZ::u32>(addFileResult.m_relativeFilePath.Native().size());

            expectedArchiveHeader.m_tocPathBlobUncompressedSize = filePathBlobTableSize;

            auto expectedArchiveHeaderSpan = AZStd::span(reinterpret_cast<AZStd::byte*>(&expectedArchiveHeader), sizeof(expectedArchiveHeader));
            // The ArchiveHeader is written out as a 512-byte aligned
            auto archiveBufferSpan = AZStd::span(archiveBuffer).first(sizeof(ArchiveHeader));
            EXPECT_TRUE(AZStd::ranges::equal(archiveBufferSpan, expectedArchiveHeaderSpan));
        }

        // Update the existing file and add compression to it
        fileSettings.m_fileMode = ArchiveWriterFileMode::AddNewOrUpdateExisting;
        fileSettings.m_compressionAlgorithm = CompressionLZ4::GetLZ4CompressionAlgorithmId();
        fileContent = "Big Little Tea Time";
        addFileResult = archiveWriter->AddFileToArchive(StringToByteSpan(fileContent), fileSettings);
        EXPECT_TRUE(addFileResult);

        EXPECT_NE(InvalidArchiveFileToken, addFileResult.m_filePathToken);
        EXPECT_EQ(loweredCaseFilePath, addFileResult.m_relativeFilePath);

        EXPECT_EQ(CompressionLZ4::GetLZ4CompressionAlgorithmId(), addFileResult.m_compressionAlgorithm);

        // Cast the first sizeof(ArchiveHeader) bytes to an ArchiveHeader
        auto archiveBufferSpan = AZStd::span(archiveBuffer).first(sizeof(ArchiveHeader));
        auto archiveHeader = reinterpret_cast<ArchiveHeader*>(archiveBufferSpan.data());

        // Write toe updated archive header and table of contents out
        commitResult = archiveWriter->Commit();
        ASSERT_TRUE(commitResult);

        // Compare against the known expected values
        EXPECT_EQ(1, archiveHeader->m_fileCount);
        EXPECT_EQ(sizeof(ArchiveTocFileMetadata) * 1, archiveHeader->m_tocFileMetadataTableUncompressedSize);
        EXPECT_EQ(sizeof(ArchiveTocFilePathIndex) * 1, archiveHeader->m_tocPathIndexTableUncompressedSize);
        EXPECT_EQ(fileSettings.m_relativeFilePath.Native().size(), archiveHeader->m_tocPathBlobUncompressedSize);
        EXPECT_EQ(sizeof(ArchiveBlockLineUnion), archiveHeader->m_tocBlockOffsetTableUncompressedSize);
        EXPECT_EQ(CompressionLZ4::GetLZ4CompressionAlgorithmId(), archiveHeader->m_compressionAlgorithmsIds[0]);
    }

    TEST_F(ArchiveWriterFixture, FileWithSizeGreaterThanMaxBlockLineSize_WritesMultipleBlockLines_WhenUsingCompression_ToBlockOffsetTable)
    {
        using namespace Archive::literals;
        // Generate a 7 MiB file
        AZStd::vector<AZStd::byte> largeFileBuffer;
        largeFileBuffer.resize_no_construct(7_mib);

        // Lambda is marked as mutable to allow the member `currentValue`
        // capture to be modified in place
        auto RepeatingByteSequenceGenerator = [currentValue = 0]() mutable
        {
            return static_cast<AZStd::byte>(currentValue % 256);
        };
        AZStd::generate(largeFileBuffer.begin(), largeFileBuffer.end(),
            RepeatingByteSequenceGenerator);

        // Create the Archive stream
        AZStd::vector<AZStd::byte> archiveBuffer;
        AZ::IO::ByteContainerStream archiveStream(&archiveBuffer);

        // Set the ArchiveStreamDeleter to not delete the stack ByteContainerStream
        IArchiveWriter::ArchiveStreamPtr archiveStreamPtr(&archiveStream, { false });
        auto createArchiveWriterResult = CreateArchiveWriter(AZStd::move(archiveStreamPtr));
        ASSERT_TRUE(createArchiveWriterResult);
        AZStd::unique_ptr<IArchiveWriter> archiveWriter = AZStd::move(createArchiveWriterResult.value());

        // Add a compressed file to archive
        ArchiveWriterFileSettings fileSettings;
        fileSettings.m_compressionAlgorithm = CompressionLZ4::GetLZ4CompressionAlgorithmId();
        fileSettings.m_relativeFilePath = "Sanity/chess.txt";
        auto addFileResult = archiveWriter->AddFileToArchive(largeFileBuffer, fileSettings);

        EXPECT_TRUE(addFileResult);
        EXPECT_NE(InvalidArchiveFileToken, addFileResult.m_filePathToken);

        // the ArchiveWriterFileSettings defaults to lowercasing paths added to the archive
        AZ::IO::Path loweredCaseFilePath = fileSettings.m_relativeFilePath;
        AZStd::to_lower(loweredCaseFilePath.Native());
        EXPECT_EQ(loweredCaseFilePath, addFileResult.m_relativeFilePath);

        EXPECT_EQ(CompressionLZ4::GetLZ4CompressionAlgorithmId(), addFileResult.m_compressionAlgorithm);

        // Commit the archive writer to update the header and table of contents in the stream
        ASSERT_TRUE(archiveWriter->Commit());

        // Cast the first sizeof(ArchiveHeader) bytes to an ArchiveHeader
        auto archiveBufferSpan = AZStd::span(archiveBuffer).first(sizeof(ArchiveHeader));
        auto archiveHeader = reinterpret_cast<ArchiveHeader*>(archiveBufferSpan.data());

        // Compare against the known expected values
        EXPECT_EQ(1, archiveHeader->m_fileCount);
        EXPECT_EQ(sizeof(ArchiveTocFileMetadata) * 1, archiveHeader->m_tocFileMetadataTableUncompressedSize);
        EXPECT_EQ(sizeof(ArchiveTocFilePathIndex) * 1, archiveHeader->m_tocPathIndexTableUncompressedSize);
        EXPECT_EQ(fileSettings.m_relativeFilePath.Native().size(), archiveHeader->m_tocPathBlobUncompressedSize);
        // Since the file being written is 7 MiB uncompressed it uses 4 2-MiB blocks for compression
        // (Well really 3 2-MiB blocks and the remaining block caps out at 1 MiB)
        // Since a block line can only represent 3 2-MiB block, multiple block lines must be written
        EXPECT_EQ(sizeof(ArchiveBlockLineUnion) * GetBlockLineCountIfCompressed(static_cast<AZ::u64>(largeFileBuffer.size())),
            archiveHeader->m_tocBlockOffsetTableUncompressedSize);

        EXPECT_EQ(CompressionLZ4::GetLZ4CompressionAlgorithmId(), archiveHeader->m_compressionAlgorithmsIds[0]);
    }

    TEST_F(ArchiveWriterFixture, CanWriteCompressedTOC_AndReadCompressedBackIn_Succeeds)
    {
        AZStd::vector<AZStd::byte> archiveBuffer;
        AZ::IO::ByteContainerStream archiveStream(&archiveBuffer);

        // Set the ArchiveStreamDeleter to not delete the stack ByteContainerStream
        IArchiveWriter::ArchiveStreamPtr archiveStreamPtr(&archiveStream, { false });

        ArchiveWriterSettings writerSettings;
        writerSettings.m_tocCompressionAlgorithm = CompressionLZ4::GetLZ4CompressionAlgorithmId();
        auto createArchiveWriterResult = CreateArchiveWriter(AZStd::move(archiveStreamPtr),
            writerSettings);
        ASSERT_TRUE(createArchiveWriterResult);
        AZStd::unique_ptr<IArchiveWriter> archiveWriter = AZStd::move(createArchiveWriterResult.value());

        // Add an compressed file to archive
        AZStd::string_view fileContent = "Hello World";
        ArchiveWriterFileSettings fileSettings;
        fileSettings.m_relativeFilePath = "Sanity/test.txt";
        auto addFileResult = archiveWriter->AddFileToArchive(StringToByteSpan(fileContent), fileSettings);

        EXPECT_TRUE(addFileResult);
        EXPECT_NE(InvalidArchiveFileToken, addFileResult.m_filePathToken);

        // the ArchiveWriterFileSettings defaults to lowercasing paths added to the archive
        AZ::IO::Path loweredCaseFilePath = fileSettings.m_relativeFilePath;
        AZStd::to_lower(loweredCaseFilePath.Native());
        EXPECT_EQ(loweredCaseFilePath, addFileResult.m_relativeFilePath);
        EXPECT_EQ(Compression::Uncompressed, addFileResult.m_compressionAlgorithm);

        // Write the archive header and the compressed table of contents to stream
        IArchiveWriter::CommitResult commitResult = archiveWriter->Commit();
        ASSERT_TRUE(commitResult);


        // Write the updated archive header and table of contents out
        commitResult = archiveWriter->Commit();
        ASSERT_TRUE(commitResult);

        // Compare against the known expected values
        // Verify the Table of Contents is compressed
        {
            // Cast the first sizeof(ArchiveHeader) bytes to an ArchiveHeader
            auto archiveBufferSpan = AZStd::span(archiveBuffer).first(sizeof(ArchiveHeader));
            auto archiveHeader = reinterpret_cast<ArchiveHeader*>(archiveBufferSpan.data());
            EXPECT_EQ(CompressionLZ4::GetLZ4CompressionAlgorithmId(), archiveHeader->m_compressionAlgorithmsIds[0]);
            EXPECT_EQ(0, archiveHeader->m_tocCompressionAlgoIndex);
            EXPECT_GT(archiveHeader->m_tocCompressedSize, 0);

            EXPECT_EQ(1, archiveHeader->m_fileCount);
            EXPECT_EQ(sizeof(ArchiveTocFileMetadata) * 1, archiveHeader->m_tocFileMetadataTableUncompressedSize);
            EXPECT_EQ(sizeof(ArchiveTocFilePathIndex) * 1, archiveHeader->m_tocPathIndexTableUncompressedSize);
            EXPECT_EQ(fileSettings.m_relativeFilePath.Native().size(), archiveHeader->m_tocPathBlobUncompressedSize);
            EXPECT_EQ(0, archiveHeader->m_tocBlockOffsetTableUncompressedSize);
        }

        // Reset the ArchiveWriter instance and make a new instance
        // that loads the Archive stream now containing a compressed TOC
        archiveWriter.reset();

        archiveStreamPtr.reset(&archiveStream);
        createArchiveWriterResult = CreateArchiveWriter(AZStd::move(archiveStreamPtr));
        ASSERT_TRUE(createArchiveWriterResult);
        archiveWriter = AZStd::move(createArchiveWriterResult.value());

        // Validate the Table of Contents by updating an existing file
        fileSettings.m_fileMode = ArchiveWriterFileMode::AddNewOrUpdateExisting;
        fileSettings.m_compressionAlgorithm = CompressionLZ4::GetLZ4CompressionAlgorithmId();
        fileContent = "Land data to compress";
        addFileResult = archiveWriter->AddFileToArchive(StringToByteSpan(fileContent), fileSettings);

        EXPECT_TRUE(addFileResult);
        EXPECT_EQ(CompressionLZ4::GetLZ4CompressionAlgorithmId(), addFileResult.m_compressionAlgorithm);

        // Re-write the archive header and Table of contents
        commitResult = archiveWriter->Commit();
        ASSERT_TRUE(commitResult);

        // Compare against the known expected values
        // Verify the Table of Contents is compressed
        // Cast the first sizeof(ArchiveHeader) bytes to an ArchiveHeader
        {
            auto archiveBufferSpan = AZStd::span(archiveBuffer).first(sizeof(ArchiveHeader));
            auto archiveHeader = reinterpret_cast<ArchiveHeader*>(archiveBufferSpan.data());
            EXPECT_EQ(CompressionLZ4::GetLZ4CompressionAlgorithmId(), archiveHeader->m_compressionAlgorithmsIds[0]);
            EXPECT_EQ(0, archiveHeader->m_tocCompressionAlgoIndex);
            EXPECT_GT(archiveHeader->m_tocCompressedSize, 0);

            // There should still only be a single file in the archive
            // However as the file is compressed now, there should be block line entries
            EXPECT_EQ(1, archiveHeader->m_fileCount);
            EXPECT_EQ(sizeof(ArchiveTocFileMetadata) * 1, archiveHeader->m_tocFileMetadataTableUncompressedSize);
            EXPECT_EQ(sizeof(ArchiveTocFilePathIndex) * 1, archiveHeader->m_tocPathIndexTableUncompressedSize);
            EXPECT_EQ(fileSettings.m_relativeFilePath.Native().size(), archiveHeader->m_tocPathBlobUncompressedSize);
            EXPECT_EQ(sizeof(ArchiveBlockLineUnion), archiveHeader->m_tocBlockOffsetTableUncompressedSize);
        }
    }
}

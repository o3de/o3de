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

#include <Archive/Clients/ArchiveReaderAPI.h>
#include <Archive/Tools/ArchiveWriterAPI.h>

#include <Compression/CompressionLZ4API.h>

// Archive Gem private implementation includes
#include <Clients/ArchiveReaderFactory.h>
#include <Tools/ArchiveWriterFactory.h>

namespace Archive::Test
{
    // Note the ArchiveReader unit test are placed in the Archive.Editor.Tests
    // Tools module to have to the ArchiveWriter which is used to create
    // test archives to be read
    // In theory if all archives were written to a file on disk
    // the test could be placed in the Archive.Tests client module

    // The ArchiveEditorTestEnvironment is tracking memory
    // via the GemTestEnvironment::SetupEnvironment function
    // so the LeakDetectionFixture should not be used
    class ArchiveReaderFixture
        : public ::testing::Test
    {
    public:
        ArchiveReaderFixture()
        {
            m_archiveReaderFactory = AZStd::make_unique<ArchiveReaderFactory>();
            AZ::Interface<IArchiveReaderFactory>::Register(m_archiveReaderFactory.get());
            m_archiveWriterFactory = AZStd::make_unique<ArchiveWriterFactory>();
            AZ::Interface<IArchiveWriterFactory>::Register(m_archiveWriterFactory.get());
        }
        ~ArchiveReaderFixture()
        {
            AZ::Interface<IArchiveWriterFactory>::Unregister(m_archiveWriterFactory.get());
            AZ::Interface<IArchiveReaderFactory>::Unregister(m_archiveReaderFactory.get());
        }

    protected:
        // Create and register an Archive Reader and Archive Writer factory
        // to allow IArchiveReader and IArchiveWriter instances to be created
        // The Archive Writer is needed to create the Archives in order to test the
        // Archive Reader code
        AZStd::unique_ptr<IArchiveReaderFactory> m_archiveReaderFactory;
        AZStd::unique_ptr<IArchiveWriterFactory> m_archiveWriterFactory;
    };


    TEST_F(ArchiveReaderFixture, CreateArchiveReader_Succeeds)
    {
        {
            auto createArchiveReaderResult = CreateArchiveReader();
            ASSERT_TRUE(createArchiveReaderResult);
            AZStd::unique_ptr<IArchiveReader> archiveReader = AZStd::move(createArchiveReaderResult.value());
            ASSERT_NE(nullptr, archiveReader);
        }

        {
            auto createArchiveReaderResult = CreateArchiveReader(ArchiveReaderSettings{});
            ASSERT_TRUE(createArchiveReaderResult);
            AZStd::unique_ptr<IArchiveReader> archiveReader = AZStd::move(createArchiveReaderResult.value());
            ASSERT_NE(nullptr, archiveReader);
        }
    }

    TEST_F(ArchiveReaderFixture, MountingEmptyFile_Fails)
    {
        AZStd::vector<AZStd::byte> archiveBuffer;
        AZ::IO::ByteContainerStream archiveStream(&archiveBuffer);

        // Set the ArchiveStreamDeleter to not delete the stack ByteContainerStream
        IArchiveReader::ArchiveStreamPtr archiveStreamPtr(&archiveStream, { false });
        ArchiveReaderSettings readerSettings;
        bool mountErrorOccurred{};
        readerSettings.m_errorCallback = [&mountErrorOccurred](const ArchiveReaderError&)
        {
            mountErrorOccurred = true;
        };
        auto createArchiveReaderResult = CreateArchiveReader(AZStd::move(archiveStreamPtr),
            readerSettings);
        ASSERT_TRUE(createArchiveReaderResult);
        AZStd::unique_ptr<IArchiveReader> archiveReader = AZStd::move(createArchiveReaderResult.value());

        EXPECT_TRUE(mountErrorOccurred);
        EXPECT_FALSE(archiveReader->IsMounted());

        // reset the mountErrorOccurred boolean
        mountErrorOccurred = false;

        // now try explicitly mounting an archive using the MountArchive method
        archiveStreamPtr.reset(&archiveStream);
        EXPECT_FALSE(archiveReader->MountArchive(AZStd::move(archiveStreamPtr)));
        EXPECT_TRUE(mountErrorOccurred);
        EXPECT_FALSE(archiveReader->IsMounted());
    }

    TEST_F(ArchiveReaderFixture, MountingFailsForInvalidArchive)
    {
        AZStd::vector<AZStd::byte> archiveBuffer;
        AZ::IO::ByteContainerStream archiveStream(&archiveBuffer);
        // Write random data to the archiveStream
        constexpr AZStd::string_view testData = "The slow gray fox hid under the hyperactive cat";
        archiveStream.Write(testData.size(), testData.data());
        archiveStream.Seek(0, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);

        // Set the ArchiveStreamDeleter to not delete the stack ByteContainerStream
        IArchiveReader::ArchiveStreamPtr archiveStreamPtr(&archiveStream, { false });
        ArchiveReaderSettings readerSettings;
        bool mountErrorOccurred{};
        readerSettings.m_errorCallback = [&mountErrorOccurred](const ArchiveReaderError&)
        {
            mountErrorOccurred = true;
        };
        auto createArchiveReaderResult = CreateArchiveReader(AZStd::move(archiveStreamPtr),
            readerSettings);
        ASSERT_TRUE(createArchiveReaderResult);
        AZStd::unique_ptr<IArchiveReader> archiveReader = AZStd::move(createArchiveReaderResult.value());

        EXPECT_TRUE(mountErrorOccurred);
        EXPECT_FALSE(archiveReader->IsMounted());

        // reset the mountErrorOccurred boolean
        mountErrorOccurred = false;

        // now try explicitly mounting an archive using the MountArchive method
        archiveStreamPtr.reset(&archiveStream);
        EXPECT_FALSE(archiveReader->MountArchive(AZStd::move(archiveStreamPtr)));
        EXPECT_TRUE(mountErrorOccurred);
        EXPECT_FALSE(archiveReader->IsMounted());
    }

    TEST_F(ArchiveReaderFixture, DefaultArchive_CreatedFromWriter_CanBeMounted)
    {
        AZStd::vector<AZStd::byte> archiveBuffer;
        AZ::IO::ByteContainerStream archiveStream(&archiveBuffer);

        {
            // Create an empty archive with no files in it
            IArchiveWriter::ArchiveStreamPtr archiveWriterStreamPtr(&archiveStream, { false });
            auto createArchiveWriterResult = CreateArchiveWriter(AZStd::move(archiveWriterStreamPtr));
            ASSERT_TRUE(createArchiveWriterResult);
            AZStd::unique_ptr<IArchiveWriter> archiveWriter = AZStd::move(createArchiveWriterResult.value());

            IArchiveWriter::CommitResult commitResult = archiveWriter->Commit();
            ASSERT_TRUE(commitResult);
        }

        // Set the ArchiveStreamDeleter to not delete the stack ByteContainerStream
        IArchiveReader::ArchiveStreamPtr archiveReaderStreamPtr(&archiveStream, { false });
        ArchiveReaderSettings readerSettings;
        bool mountErrorOccurred{};
        readerSettings.m_errorCallback = [&mountErrorOccurred](const ArchiveReaderError&)
        {
            mountErrorOccurred = true;
        };
        auto createArchiveReaderResult = CreateArchiveReader(AZStd::move(archiveReaderStreamPtr),
            readerSettings);
        ASSERT_TRUE(createArchiveReaderResult);
        AZStd::unique_ptr<IArchiveReader> archiveReader = AZStd::move(createArchiveReaderResult.value());

        // No error should occur and the archive should have been successfully mounted
        EXPECT_FALSE(mountErrorOccurred);
        EXPECT_TRUE(archiveReader->IsMounted());

        // Unmount the archive
        archiveReader->UnmountArchive();
        EXPECT_FALSE(archiveReader->IsMounted());

        // reset the mountErrorOccurred boolean
        mountErrorOccurred = false;

        // now try explicitly mounting an archive using the MountArchive method
        archiveReaderStreamPtr.reset(&archiveStream);
        EXPECT_TRUE(archiveReader->MountArchive(AZStd::move(archiveReaderStreamPtr)));
        EXPECT_FALSE(mountErrorOccurred);
        EXPECT_TRUE(archiveReader->IsMounted());
    }

    TEST_F(ArchiveReaderFixture, ListFileInArchive_ForExistingFile_Succeeds)
    {
        AZStd::vector<AZStd::byte> archiveBuffer;
        AZ::IO::ByteContainerStream archiveStream(&archiveBuffer);

        AZStd::string_view fooFileData;
        AZStd::string_view levelPrefabFileData;

        {
            // Create an archive with a several files in it
            // At least one of the files are compressed
            // and at one is not compressed
            IArchiveWriter::ArchiveStreamPtr archiveWriterStreamPtr(&archiveStream, { false });
            auto createArchiveWriterResult = CreateArchiveWriter(AZStd::move(archiveWriterStreamPtr));
            ASSERT_TRUE(createArchiveWriterResult);
            AZStd::unique_ptr<IArchiveWriter> archiveWriter = AZStd::move(createArchiveWriterResult.value());

            ArchiveWriterFileSettings fileSettings;

            // Write an uncompressed file with the contents of hello world
            AZStd::string_view fileData = "Hello World";
            fooFileData = fileData;
            fileSettings.m_relativeFilePath = "foo.txt";
            EXPECT_TRUE(archiveWriter->AddFileToArchive(AZStd::as_bytes(AZStd::span(fileData)), fileSettings));

            // Write a compressed file this time
            fileData = "My Prefab Data in an Archive";
            levelPrefabFileData = fileData;
            fileSettings.m_compressionAlgorithm = CompressionLZ4::GetLZ4CompressionAlgorithmId();
            fileSettings.m_relativeFilePath = "subdirectory/Level.prefab";
            EXPECT_TRUE(archiveWriter->AddFileToArchive(AZStd::as_bytes(AZStd::span(fileData)), fileSettings));

            IArchiveWriter::CommitResult commitResult = archiveWriter->Commit();
            ASSERT_TRUE(commitResult);
        }

        // Set the ArchiveStreamDeleter to not delete the stack ByteContainerStream
        IArchiveReader::ArchiveStreamPtr archiveReaderStreamPtr(&archiveStream, { false });
        auto createArchiveReaderResult = CreateArchiveReader(AZStd::move(archiveReaderStreamPtr));
        ASSERT_TRUE(createArchiveReaderResult);
        AZStd::unique_ptr<IArchiveReader> archiveReader = AZStd::move(createArchiveReaderResult.value());

        // No error should occur and the archive should have been successfully mounted
        EXPECT_TRUE(archiveReader->IsMounted());

        // The fooFileToken ArchiveFileToken instance will be used to validate
        // the overload of ListFileInArchive which accepts an ArchiveFileToken
        ArchiveFileToken fooFileToken = InvalidArchiveFileToken;
        {
            // Lookup the foo.txt file
            constexpr AZStd::string_view fooPath = "foo.txt";

            ArchiveListFileResult archiveListFileResult = archiveReader->ListFileInArchive(fooPath);
            EXPECT_TRUE(archiveListFileResult);
            EXPECT_NE(InvalidArchiveFileToken, archiveListFileResult.m_filePathToken);
            // Store of the file token for later lookup
            fooFileToken = archiveListFileResult.m_filePathToken;

            EXPECT_EQ(fooPath, archiveListFileResult.m_relativeFilePath);
            EXPECT_EQ(Compression::Uncompressed, archiveListFileResult.m_compressionAlgorithm);
            EXPECT_EQ(fooFileData.size(), archiveListFileResult.m_uncompressedSize);
            // As the file is not compressed, the ArchiveListFileResult compressed member is not check

            // The file is expected to have been written at offset 512-byte and aligned up to the next multiple of 512
            // which is 1024 since the file is <= 512 bytes in size
            AZ::u64 expectedFileOffset = ArchiveDefaultBlockAlignment;
            EXPECT_EQ(expectedFileOffset, archiveListFileResult.m_offset);

        }

        {
            // Lookup the subdirectory/level.prefab file

            // The default ArchiveWriterFileSettings used in this test lowercases the files paths that were added
            // So before looking up the Level.prefab in the "subdirectory" directory, lowercase the path
            AZ::IO::Path prefabPathLower = "subdirectory/Level.prefab";
            AZStd::to_lower(prefabPathLower.Native());
            ArchiveListFileResult archiveListFileResult = archiveReader->ListFileInArchive(prefabPathLower);
            EXPECT_TRUE(archiveListFileResult);
            EXPECT_NE(InvalidArchiveFileToken, archiveListFileResult.m_filePathToken);
            EXPECT_EQ(prefabPathLower, archiveListFileResult.m_relativeFilePath);
            EXPECT_EQ(CompressionLZ4::GetLZ4CompressionAlgorithmId(), archiveListFileResult.m_compressionAlgorithm);
            // The file should have been compressed
            // Just validate that its size > 0
            EXPECT_GT(archiveListFileResult.m_compressedSize, 0);
            EXPECT_EQ(levelPrefabFileData.size(), archiveListFileResult.m_uncompressedSize);

            // Since this is the second file written to the archive
            // the start offset should be at the next 512-byte offset a
            // which is 1024, as foo.txt was written at offset 512
            AZ::u64 expectedFileOffset = ArchiveDefaultBlockAlignment * 2;
            EXPECT_EQ(expectedFileOffset, archiveListFileResult.m_offset);
        }

        {
            // This time lookup the foo.txt file using the ArchiveFileToken
            constexpr AZStd::string_view fooPath = "foo.txt";

            ArchiveListFileResult archiveListFileResult = archiveReader->ListFileInArchive(fooFileToken);
            EXPECT_TRUE(archiveListFileResult);
            EXPECT_NE(InvalidArchiveFileToken, archiveListFileResult.m_filePathToken);
            EXPECT_EQ(fooPath, archiveListFileResult.m_relativeFilePath);
            EXPECT_EQ(Compression::Uncompressed, archiveListFileResult.m_compressionAlgorithm);
            EXPECT_EQ(fooFileData.size(), archiveListFileResult.m_uncompressedSize);
            // As the file is not compressed, the ArchiveListFileResult compressed member is not check

            // The file is expected to have been written at offset 512-byte and aligned up to the next multiple of 512
            // which is 1024 since the file is <= 512 bytes in size
            AZ::u64 expectedFileOffset = ArchiveDefaultBlockAlignment;
            EXPECT_EQ(expectedFileOffset, archiveListFileResult.m_offset);
        }

        // Finally validate the ContainsFile function succeeds
        EXPECT_TRUE(archiveReader->ContainsFile("foo.txt"));
    }

    TEST_F(ArchiveReaderFixture, ListFileInArchive_ForFileNotInArchive_Fails)
    {
        AZStd::vector<AZStd::byte> archiveBuffer;
        AZ::IO::ByteContainerStream archiveStream(&archiveBuffer);

        AZStd::string_view fooFileData;

        {
            // Create an archive with files in it
            IArchiveWriter::ArchiveStreamPtr archiveWriterStreamPtr(&archiveStream, { false });
            auto createArchiveWriterResult = CreateArchiveWriter(AZStd::move(archiveWriterStreamPtr));
            ASSERT_TRUE(createArchiveWriterResult);
            AZStd::unique_ptr<IArchiveWriter> archiveWriter = AZStd::move(createArchiveWriterResult.value());

            ArchiveWriterFileSettings fileSettings;

            // Write an uncompressed file with the contents of hello world
            AZStd::string_view fileData = "Hello World";
            fooFileData = fileData;
            fileSettings.m_relativeFilePath = "foo.txt";
            EXPECT_TRUE(archiveWriter->AddFileToArchive(AZStd::as_bytes(AZStd::span(fileData)), fileSettings));

            IArchiveWriter::CommitResult commitResult = archiveWriter->Commit();
            ASSERT_TRUE(commitResult);
        }

        // Set the ArchiveStreamDeleter to not delete the stack ByteContainerStream
        IArchiveReader::ArchiveStreamPtr archiveReaderStreamPtr(&archiveStream, { false });
        auto createArchiveReaderResult = CreateArchiveReader(AZStd::move(archiveReaderStreamPtr));
        ASSERT_TRUE(createArchiveReaderResult);
        AZStd::unique_ptr<IArchiveReader> archiveReader = AZStd::move(createArchiveReaderResult.value());

        // No error should occur and the archive should have been successfully mounted
        EXPECT_TRUE(archiveReader->IsMounted());

        {
            // Lookup the non-existent/foo.txt
            constexpr AZStd::string_view nonExistentPath = "non-existent/foo.txt";

            ArchiveListFileResult archiveListFileResult = archiveReader->ListFileInArchive(nonExistentPath);
            EXPECT_FALSE(archiveListFileResult);
            EXPECT_FALSE(archiveListFileResult.m_resultOutcome.has_value());

            // Also the ContainsFile function should fail as well
            EXPECT_FALSE(archiveReader->ContainsFile(nonExistentPath));
        }
    }

    TEST_F(ArchiveReaderFixture, EnumerateFilesInArchive_Visits_EachFileInTheArchive)
    {
        AZStd::vector<AZStd::byte> archiveBuffer;
        AZ::IO::ByteContainerStream archiveStream(&archiveBuffer);

        AZStd::string_view fooFileData;
        AZStd::string_view levelPrefabFileData;
        AZStd::string_view barFileData;

        {
            // Create an archive with a several files in it
            IArchiveWriter::ArchiveStreamPtr archiveWriterStreamPtr(&archiveStream, { false });
            auto createArchiveWriterResult = CreateArchiveWriter(AZStd::move(archiveWriterStreamPtr));
            ASSERT_TRUE(createArchiveWriterResult);
            AZStd::unique_ptr<IArchiveWriter> archiveWriter = AZStd::move(createArchiveWriterResult.value());

            ArchiveWriterFileSettings fileSettings;

            // Write an uncompressed file with the contents of hello world
            AZStd::string_view fileData = "Hello World";
            fooFileData = fileData;
            fileSettings.m_relativeFilePath = "foo.txt";
            EXPECT_TRUE(archiveWriter->AddFileToArchive(AZStd::as_bytes(AZStd::span(fileData)), fileSettings));

            // Write a compressed file this time
            fileData = "My Prefab Data in an Archive";
            levelPrefabFileData = fileData;
            fileSettings.m_compressionAlgorithm = CompressionLZ4::GetLZ4CompressionAlgorithmId();
            fileSettings.m_relativeFilePath = "subdirectory/Level.prefab";
            EXPECT_TRUE(archiveWriter->AddFileToArchive(AZStd::as_bytes(AZStd::span(fileData)), fileSettings));

            // Write a second text file
            fileData = "Box Box, Box Box";
            barFileData = fileData;
            // Don't compress the file this time
            fileSettings.m_compressionAlgorithm = Compression::Uncompressed;
            fileSettings.m_relativeFilePath = "subdirectory/bar.txt";
            EXPECT_TRUE(archiveWriter->AddFileToArchive(AZStd::as_bytes(AZStd::span(fileData)), fileSettings));

            IArchiveWriter::CommitResult commitResult = archiveWriter->Commit();
            ASSERT_TRUE(commitResult);
        }

        // Set the ArchiveStreamDeleter to not delete the stack ByteContainerStream
        IArchiveReader::ArchiveStreamPtr archiveReaderStreamPtr(&archiveStream, { false });
        auto createArchiveReaderResult = CreateArchiveReader(AZStd::move(archiveReaderStreamPtr));
        ASSERT_TRUE(createArchiveReaderResult);
        AZStd::unique_ptr<IArchiveReader> archiveReader = AZStd::move(createArchiveReaderResult.value());

        // No error should occur and the archive should have been successfully mounted
        EXPECT_TRUE(archiveReader->IsMounted());

        AZStd::vector<ArchiveListFileResult> filesInArchive;
        auto GetListResultsForFiles = [&filesInArchive](ArchiveListFileResult listFileResult) -> bool
        {
            filesInArchive.emplace_back(AZStd::move(listFileResult));
            // A return of true causes enumeration to continue
            return true;
        };
        EXPECT_TRUE(archiveReader->EnumerateFilesInArchive(GetListResultsForFiles));

        // The vector should have 3 entries in it
        ASSERT_EQ(3, filesInArchive.size());
        size_t fileIndex{};
        {
            // Validate the first entry in the vector
            // Lookup the foo.txt file
            constexpr AZStd::string_view fooPath = "foo.txt";

            const ArchiveListFileResult archiveListFileResult = filesInArchive[fileIndex++];
            EXPECT_TRUE(archiveListFileResult);
            EXPECT_NE(InvalidArchiveFileToken, archiveListFileResult.m_filePathToken);
            EXPECT_EQ(fooPath, archiveListFileResult.m_relativeFilePath);
            EXPECT_EQ(Compression::Uncompressed, archiveListFileResult.m_compressionAlgorithm);
            EXPECT_EQ(fooFileData.size(), archiveListFileResult.m_uncompressedSize);
            // As the file is not compressed, the ArchiveListFileResult compressed member is not check

            // The file is expected to have been written at offset 512-byte and aligned up to the next multiple of 512
            // which is 1024 since the file is <= 512 bytes in size
            AZ::u64 expectedFileOffset = ArchiveDefaultBlockAlignment;
            EXPECT_EQ(expectedFileOffset, archiveListFileResult.m_offset);
        }

        {
            // Validate the second entry in the vector
            // Lookup the subdirectory/level.prefab file

            // The default ArchiveWriterFileSettings used in this test lowercases the files paths that were added
            // So before looking up the Level.prefab in the "subdirectory" directory, lowercase the path
            AZ::IO::Path prefabPathLower = "subdirectory/Level.prefab";
            AZStd::to_lower(prefabPathLower.Native());

            const ArchiveListFileResult archiveListFileResult = filesInArchive[fileIndex++];
            EXPECT_TRUE(archiveListFileResult);
            EXPECT_NE(InvalidArchiveFileToken, archiveListFileResult.m_filePathToken);
            EXPECT_EQ(prefabPathLower, archiveListFileResult.m_relativeFilePath);
            EXPECT_EQ(CompressionLZ4::GetLZ4CompressionAlgorithmId(), archiveListFileResult.m_compressionAlgorithm);
            // The file should have been compressed
            // Just validate that its size > 0
            EXPECT_GT(archiveListFileResult.m_compressedSize, 0);
            EXPECT_EQ(levelPrefabFileData.size(), archiveListFileResult.m_uncompressedSize);

            // Since this is the second file written to the archive
            // the start offset should be at the next 512-byte offset a
            // which is 1024, as foo.txt was written at offset 512
            AZ::u64 expectedFileOffset = ArchiveDefaultBlockAlignment * 2;
            EXPECT_EQ(expectedFileOffset, archiveListFileResult.m_offset);
        }

        {
            // Validate the third entry in the vector
            // Lookup the subdirectory/bar.txt file

            // The default ArchiveWriterFileSettings used in this test lowercases the files paths that were added
            // So before looking up the Level.prefab in the "subdirectory" directory, lowercase the path
            AZ::IO::PathView barPath = "subdirectory/bar.txt";

            const ArchiveListFileResult archiveListFileResult = filesInArchive[fileIndex++];
            EXPECT_TRUE(archiveListFileResult);
            EXPECT_NE(InvalidArchiveFileToken, archiveListFileResult.m_filePathToken);
            EXPECT_EQ(barPath, archiveListFileResult.m_relativeFilePath);
            EXPECT_EQ(Compression::Uncompressed, archiveListFileResult.m_compressionAlgorithm);
            EXPECT_EQ(barFileData.size(), archiveListFileResult.m_uncompressedSize);

            // Since this is the third file written to the archive
            // and the first two files should have had an uncompressed size
            // and compressed size that is < 512-bytes
            // the start offset for bar.txt should be at 512 * 3 = 1536
            AZ::u64 expectedFileOffset = ArchiveDefaultBlockAlignment * 3;
            EXPECT_EQ(expectedFileOffset, archiveListFileResult.m_offset);
        }
    }

    TEST_F(ArchiveReaderFixture, EnumerateFilesInArchive_CanFilterFiles_Succeeds)
    {
        AZStd::vector<AZStd::byte> archiveBuffer;
        AZ::IO::ByteContainerStream archiveStream(&archiveBuffer);

        AZStd::string_view fooFileData;
        AZStd::string_view levelPrefabFileData;
        AZStd::string_view barFileData;

        {
            // Create an archive with a several files in it
            IArchiveWriter::ArchiveStreamPtr archiveWriterStreamPtr(&archiveStream, { false });

            auto createArchiveWriterResult = CreateArchiveWriter(AZStd::move(archiveWriterStreamPtr));
            ASSERT_TRUE(createArchiveWriterResult);
            AZStd::unique_ptr<IArchiveWriter> archiveWriter = AZStd::move(createArchiveWriterResult.value());

            ArchiveWriterFileSettings fileSettings;

            // Write an uncompressed file with the contents of hello world
            AZStd::string_view fileData = "Hello World";
            fooFileData = fileData;
            fileSettings.m_relativeFilePath = "foo.txt";
            EXPECT_TRUE(archiveWriter->AddFileToArchive(AZStd::as_bytes(AZStd::span(fileData)), fileSettings));

            // Write a compressed file this time
            fileData = "My Prefab Data in an Archive";
            levelPrefabFileData = fileData;
            fileSettings.m_compressionAlgorithm = CompressionLZ4::GetLZ4CompressionAlgorithmId();
            fileSettings.m_relativeFilePath = "subdirectory/Level.prefab";
            EXPECT_TRUE(archiveWriter->AddFileToArchive(AZStd::as_bytes(AZStd::span(fileData)), fileSettings));

            // Write a second text file
            fileData = "Box Box, Box Box";
            barFileData = fileData;
            // Don't compress the file this time
            fileSettings.m_compressionAlgorithm = Compression::Uncompressed;
            fileSettings.m_relativeFilePath = "subdirectory/bar.txt";
            EXPECT_TRUE(archiveWriter->AddFileToArchive(AZStd::as_bytes(AZStd::span(fileData)), fileSettings));

            IArchiveWriter::CommitResult commitResult = archiveWriter->Commit();
            ASSERT_TRUE(commitResult);
        }

        // Set the ArchiveStreamDeleter to not delete the stack ByteContainerStream
        IArchiveReader::ArchiveStreamPtr archiveReaderStreamPtr(&archiveStream, { false });
        auto createArchiveReaderResult = CreateArchiveReader(AZStd::move(archiveReaderStreamPtr));
        ASSERT_TRUE(createArchiveReaderResult);
        AZStd::unique_ptr<IArchiveReader> archiveReader = AZStd::move(createArchiveReaderResult.value());

        // No error should occur and the archive should have been successfully mounted
        EXPECT_TRUE(archiveReader->IsMounted());

        AZStd::vector<ArchiveListFileResult> filesInArchive;
        //!!! This time only add .txt files to the list file vector
        auto FilterListResultsForFiles = [&filesInArchive](ArchiveListFileResult listFileResult) -> bool
        {
            if (listFileResult.m_relativeFilePath.Match("*.txt"))
            {
                filesInArchive.emplace_back(AZStd::move(listFileResult));
            }
            return true;
        };
        EXPECT_TRUE(archiveReader->EnumerateFilesInArchive(FilterListResultsForFiles));

        // The vector should have 2 entries in it
        // as there are only two txt files in it
        ASSERT_EQ(2, filesInArchive.size());
        size_t fileIndex{};
        {
            // Validate the first entry in the vector
            // Lookup the foo.txt file
            constexpr AZStd::string_view fooPath = "foo.txt";

            const ArchiveListFileResult archiveListFileResult = filesInArchive[fileIndex++];
            EXPECT_TRUE(archiveListFileResult);
            EXPECT_NE(InvalidArchiveFileToken, archiveListFileResult.m_filePathToken);
            EXPECT_EQ(fooPath, archiveListFileResult.m_relativeFilePath);
            EXPECT_EQ(Compression::Uncompressed, archiveListFileResult.m_compressionAlgorithm);
            EXPECT_EQ(fooFileData.size(), archiveListFileResult.m_uncompressedSize);
            // As the file is not compressed, the ArchiveListFileResult compressed member is not check

            // The file is expected to have been written at offset 512-byte and aligned up to the next multiple of 512
            // which is 1024 since the file is <= 512 bytes in size
            AZ::u64 expectedFileOffset = ArchiveDefaultBlockAlignment;
            EXPECT_EQ(expectedFileOffset, archiveListFileResult.m_offset);
        }

        {
            // Validate the next entry in the vector
            // Lookup the subdirectory/bar.txt file

            // The default ArchiveWriterFileSettings used in this test lowercases the files paths that were added
            // So before looking up the Level.prefab in the "subdirectory" directory, lowercase the path
            AZ::IO::PathView barPath = "subdirectory/bar.txt";

            const ArchiveListFileResult archiveListFileResult = filesInArchive[fileIndex++];
            EXPECT_TRUE(archiveListFileResult);
            EXPECT_NE(InvalidArchiveFileToken, archiveListFileResult.m_filePathToken);
            EXPECT_EQ(barPath, archiveListFileResult.m_relativeFilePath);
            EXPECT_EQ(Compression::Uncompressed, archiveListFileResult.m_compressionAlgorithm);
            EXPECT_EQ(barFileData.size(), archiveListFileResult.m_uncompressedSize);

            // Since this is the third file written to the archive
            // and the first two files should have had an uncompressed size
            // and compressed size that is < 512-bytes
            // the start offset for bar.txt should be at 512 * 3 = 1536
            AZ::u64 expectedFileOffset = ArchiveDefaultBlockAlignment * 3;
            EXPECT_EQ(expectedFileOffset, archiveListFileResult.m_offset);
        }
    }

    TEST_F(ArchiveReaderFixture, ExtractFileFromArchive_ForExistingFile_Succeeds)
    {
        AZStd::vector<AZStd::byte> archiveBuffer;
        AZ::IO::ByteContainerStream archiveStream(&archiveBuffer);

        AZStd::string_view fooFileData;
        AZStd::string_view levelPrefabFileData;
        AZStd::string_view barFileData;

        {
            // Create an archive with a several files in it
            IArchiveWriter::ArchiveStreamPtr archiveWriterStreamPtr(&archiveStream, { false });
            auto createArchiveWriterResult = CreateArchiveWriter(AZStd::move(archiveWriterStreamPtr));
            ASSERT_TRUE(createArchiveWriterResult);
            AZStd::unique_ptr<IArchiveWriter> archiveWriter = AZStd::move(createArchiveWriterResult.value());

            ArchiveWriterFileSettings fileSettings;

            // Write an uncompressed file with the contents of hello world
            AZStd::string_view fileData = "Hello World";
            fooFileData = fileData;
            fileSettings.m_relativeFilePath = "foo.txt";
            EXPECT_TRUE(archiveWriter->AddFileToArchive(AZStd::as_bytes(AZStd::span(fileData)), fileSettings));

            // Write a compressed file this time
            fileData = "My Prefab Data in an Archive";
            levelPrefabFileData = fileData;
            fileSettings.m_compressionAlgorithm = CompressionLZ4::GetLZ4CompressionAlgorithmId();
            fileSettings.m_relativeFilePath = "subdirectory/Level.prefab";
            EXPECT_TRUE(archiveWriter->AddFileToArchive(AZStd::as_bytes(AZStd::span(fileData)), fileSettings));

            // Write a second text file
            fileData = "Box Box, Box Box";
            barFileData = fileData;
            // Don't compress the file this time
            fileSettings.m_compressionAlgorithm = Compression::Uncompressed;
            fileSettings.m_relativeFilePath = "subdirectory/bar.txt";
            EXPECT_TRUE(archiveWriter->AddFileToArchive(AZStd::as_bytes(AZStd::span(fileData)), fileSettings));

            IArchiveWriter::CommitResult commitResult = archiveWriter->Commit();
            ASSERT_TRUE(commitResult);
        }

        // Set the ArchiveStreamDeleter to not delete the stack ByteContainerStream
        IArchiveReader::ArchiveStreamPtr archiveReaderStreamPtr(&archiveStream, { false });
        auto createArchiveReaderResult = CreateArchiveReader(AZStd::move(archiveReaderStreamPtr));
        ASSERT_TRUE(createArchiveReaderResult);
        AZStd::unique_ptr<IArchiveReader> archiveReader = AZStd::move(createArchiveReaderResult.value());

        // No error should occur and the archive should have been successfully mounted
        EXPECT_TRUE(archiveReader->IsMounted());

        {
            // Extract foo.txt
            constexpr AZStd::string_view fooPath = "foo.txt";

            // Use ArchiveListFileResult to lookup the file metadata to determine
            // its uncompressed size
            const ArchiveListFileResult archiveListFileResult = archiveReader->ListFileInArchive(fooPath);
            ASSERT_TRUE(archiveListFileResult);

            AZStd::vector<AZStd::byte> fileBuffer;
            // Resize the buffer to exact size needed
            fileBuffer.resize_no_construct(archiveListFileResult.m_uncompressedSize);
            // Populate settings structure needed to extract the file
            ArchiveReaderFileSettings fileSettings;
            fileSettings.m_filePathIdentifier = fooPath;
            const ArchiveExtractFileResult archiveExtractFileResult = archiveReader->ExtractFileFromArchive(
                fileBuffer, fileSettings);
            ASSERT_TRUE(archiveExtractFileResult);

            EXPECT_NE(InvalidArchiveFileToken, archiveExtractFileResult.m_filePathToken);
            EXPECT_EQ(fooPath, archiveExtractFileResult.m_relativeFilePath);
            EXPECT_EQ(Compression::Uncompressed, archiveExtractFileResult.m_compressionAlgorithm);
            EXPECT_EQ(fooFileData.size(), archiveExtractFileResult.m_uncompressedSize);
            // As the file is not compressed, the ArchiveListFileResult compressed member is not check

            // The file is expected to have been written at offset 512-byte and aligned up to the next multiple of 512
            // which is 1024 since the file is <= 512 bytes in size
            AZ::u64 expectedFileOffset = ArchiveDefaultBlockAlignment;
            EXPECT_EQ(expectedFileOffset, archiveExtractFileResult.m_offset);

            // Now check the file span from the archiveExtractFileResult to get the exact foo data
            // Reinterpret the byte data in file span as text
            AZStd::string_view textFileSpan(reinterpret_cast<const char*>(archiveExtractFileResult.m_fileSpan.data()),
                archiveExtractFileResult.m_fileSpan.size());
            EXPECT_THAT(textFileSpan, ::testing::ContainerEq(fooFileData));

            // Validate the CRC32 of the entire file contents
            // NOTE: This only works on when extracting the entire uncompressed file
            // If the file was extracted with the ArchiveReader::m_decompressFile option set to false
            // and the file was compressed, then the CRC does not apply
            // Also if a partial read of the file was done, the Crc32 also does not apply
            EXPECT_EQ(archiveExtractFileResult.m_crc32, AZ::Crc32(archiveExtractFileResult.m_fileSpan));
        }

        {
            // Extract subdirectory/Level.prefab

            // The default ArchiveWriterFileSettings used in this test lowercases the files paths that were added
            // So before looking up the Level.prefab in the "subdirectory" directory, lowercase the path
            AZ::IO::Path prefabPathLower = "subdirectory/Level.prefab";
            AZStd::to_lower(prefabPathLower.Native());

            // Use ArchiveListFileResult to lookup the file metadata to uncompressed size
            const ArchiveListFileResult archiveListFileResult = archiveReader->ListFileInArchive(prefabPathLower);
            ASSERT_TRUE(archiveListFileResult);

            AZStd::vector<AZStd::byte> fileBuffer;
            // Resize the buffer to exact size needed
            fileBuffer.resize_no_construct(archiveListFileResult.m_uncompressedSize);
            // Populate settings structure needed to extract the file
            ArchiveReaderFileSettings fileSettings;
            // Use the file path token this time to extract the file
            fileSettings.m_filePathIdentifier = archiveListFileResult.m_filePathToken;
            const ArchiveExtractFileResult archiveExtractFileResult = archiveReader->ExtractFileFromArchive(
                fileBuffer, fileSettings);
            ASSERT_TRUE(archiveExtractFileResult);

            EXPECT_NE(InvalidArchiveFileToken, archiveExtractFileResult.m_filePathToken);
            EXPECT_EQ(prefabPathLower, archiveExtractFileResult.m_relativeFilePath);
            EXPECT_EQ(CompressionLZ4::GetLZ4CompressionAlgorithmId(), archiveExtractFileResult.m_compressionAlgorithm);
            // The file should have been compressed
            // Just validate that its size > 0
            EXPECT_GT(archiveExtractFileResult.m_compressedSize, 0);
            EXPECT_EQ(levelPrefabFileData.size(), archiveExtractFileResult.m_uncompressedSize);

            // The subdirectory/level.prefab is the second file within the archive
            // So it its start offset should be at 1024 since the foo.txt start offset was at 512
            AZ::u64 expectedFileOffset = ArchiveDefaultBlockAlignment * 2;
            EXPECT_EQ(expectedFileOffset, archiveExtractFileResult.m_offset);

            // Now check the file span from the archiveExtractFileResult to get the exact
            // view of the file data for the level.prefab after running it through decompression
            AZStd::string_view textFileSpan(reinterpret_cast<const char*>(archiveExtractFileResult.m_fileSpan.data()),
                archiveExtractFileResult.m_fileSpan.size());
            EXPECT_THAT(textFileSpan, ::testing::ContainerEq(levelPrefabFileData));

            // Validate the CRC32 of the entire file contents
            // NOTE: This only works on when extracting the entire uncompressed file
            // If the file was extracted with the ArchiveReader::m_decompressFile option set to false
            // and the file was compressed, then the CRC does not apply
            // Also if a partial read of the file was done, the Crc32 also does not apply
            EXPECT_EQ(archiveExtractFileResult.m_crc32, AZ::Crc32(archiveExtractFileResult.m_fileSpan));
        }
    }

    //! This test validates the setting the ArchiveReaderFileSettings::m_decompressFile option to `false`
    //! will extract the compressed file WITHOUT decompressing it.
    TEST_F(ArchiveReaderFixture, ExtractFileFromArchive_ExtractionOfFile_ThatSkipsDecompressed_Succeeds)
    {
        AZStd::vector<AZStd::byte> archiveBuffer;
        AZ::IO::ByteContainerStream archiveStream(&archiveBuffer);

        AZStd::string_view levelPrefabFileData = "My Prefab Data in an Archive";
        AZStd::vector<AZStd::byte> expectedCompressedFileData;
        auto compressionRegistrar = Compression::CompressionRegistrar::Get();
        ASSERT_NE(nullptr, compressionRegistrar);
        auto lz4Compressor = compressionRegistrar->FindCompressionInterface(CompressionLZ4::GetLZ4CompressionAlgorithmId());
        ASSERT_NE(nullptr, lz4Compressor);

        // Resize the output buffer to be large enough to contain the compressed data
        expectedCompressedFileData.resize_no_construct(lz4Compressor->CompressBound(levelPrefabFileData.size()));
        // Compress the test data into the byte buffer
        Compression::CompressionResultData compressionResult = lz4Compressor->CompressBlock(expectedCompressedFileData,
            AZStd::as_bytes(AZStd::span(levelPrefabFileData)));
        ASSERT_TRUE(compressionResult);

        AZStd::span<AZStd::byte> actualCompressedDataSpan = compressionResult.m_compressedBuffer;
        {
            // Create an archive with a several files in it
            IArchiveWriter::ArchiveStreamPtr archiveWriterStreamPtr(&archiveStream, { false });
            auto createArchiveWriterResult = CreateArchiveWriter(AZStd::move(archiveWriterStreamPtr));
            ASSERT_TRUE(createArchiveWriterResult);
            AZStd::unique_ptr<IArchiveWriter> archiveWriter = AZStd::move(createArchiveWriterResult.value());

            ArchiveWriterFileSettings fileSettings;

            // Write a compressed file this time
            fileSettings.m_compressionAlgorithm = CompressionLZ4::GetLZ4CompressionAlgorithmId();
            fileSettings.m_relativeFilePath = "subdirectory/Level.prefab";
            EXPECT_TRUE(archiveWriter->AddFileToArchive(AZStd::as_bytes(AZStd::span(levelPrefabFileData)), fileSettings));

            IArchiveWriter::CommitResult commitResult = archiveWriter->Commit();
            ASSERT_TRUE(commitResult);
        }

        // Set the ArchiveStreamDeleter to not delete the stack ByteContainerStream
        IArchiveReader::ArchiveStreamPtr archiveReaderStreamPtr(&archiveStream, { false });
        auto createArchiveReaderResult = CreateArchiveReader(AZStd::move(archiveReaderStreamPtr));
        ASSERT_TRUE(createArchiveReaderResult);
        AZStd::unique_ptr<IArchiveReader> archiveReader = AZStd::move(createArchiveReaderResult.value());

        // No error should occur and the archive should have been successfully mounted
        EXPECT_TRUE(archiveReader->IsMounted());

        {
            // Extract subdirectory/Level.prefab

            // The default ArchiveWriterFileSettings used in this test lowercases the files paths that were added
            // So before looking up the Level.prefab in the "subdirectory" directory, lowercase the path
            AZ::IO::Path prefabPathLower = "subdirectory/Level.prefab";
            AZStd::to_lower(prefabPathLower.Native());

            // Use ArchiveListFileResult to lookup the file metadata
            const ArchiveListFileResult archiveListFileResult = archiveReader->ListFileInArchive(prefabPathLower);
            ASSERT_TRUE(archiveListFileResult);

            AZStd::vector<AZStd::byte> fileBuffer;
            // As the buffer is pointing to compressed data this time
            // It is resized to be large enough to store the compressed data
            fileBuffer.resize_no_construct(archiveListFileResult.m_compressedSize);
            // Populate settings structure needed to extract the file
            ArchiveReaderFileSettings fileSettings;
            // Use the file path token this time to extract the file
            fileSettings.m_filePathIdentifier = archiveListFileResult.m_filePathToken;
            // Skip Decompression of the file content
            fileSettings.m_decompressFile = false;
            const ArchiveExtractFileResult archiveExtractFileResult = archiveReader->ExtractFileFromArchive(
                fileBuffer, fileSettings);
            ASSERT_TRUE(archiveExtractFileResult);
            EXPECT_NE(InvalidArchiveFileToken, archiveExtractFileResult.m_filePathToken);
            EXPECT_EQ(prefabPathLower, archiveExtractFileResult.m_relativeFilePath);
            EXPECT_EQ(CompressionLZ4::GetLZ4CompressionAlgorithmId(), archiveExtractFileResult.m_compressionAlgorithm);
            // The file should have been compressed
            // Just validate that its size > 0
            EXPECT_GT(archiveExtractFileResult.m_compressedSize, 0);
            EXPECT_EQ(levelPrefabFileData.size(), archiveExtractFileResult.m_uncompressedSize);

            // The subdirectory/level.prefab is the only file within the archive
            // So it its start offset should be at 512
            AZ::u64 expectedFileOffset = ArchiveDefaultBlockAlignment;
            EXPECT_EQ(expectedFileOffset, archiveExtractFileResult.m_offset);

            // The file span should still be compressed at this point
            AZStd::span<AZStd::byte> compressedFileSpan = archiveExtractFileResult.m_fileSpan;
            EXPECT_TRUE(AZStd::ranges::equal(compressedFileSpan, actualCompressedDataSpan));
        }
    }

    TEST_F(ArchiveReaderFixture, ExtractFileFromArchive_PartialReadOfCompressedFile_Across3Blocks_Succeeds)
    {
        // Verify that the ArchiveReaderFileSettings can read content from a compressed file
        // where the data is are in different 2 MiB blocks of the file when uncompressed
        // The format of the data will be as follows
        // First 2-MiB Block to be compressed:
        //   (2-MiB - 1) bytes of '\0'
        //   1 byte of 'H'
        // Second 2-MiB Block to be compressed:
        //   10 bytes that are 'e', 'l', 'l' 'o', ' ', 'W', 'o', 'r', 'l', 'd'
        //   (2-MiB - 10) byte of '\0'
        // Final 2-MiB Block to be compressed (partial:
        //   1 byte 'A'
        //   7 bytes of "rchive"
        //
        // The test will attempt to start reading at offset (2MiB -1)
        // and will attempt to read (2 MiB + 2) bytes
        // This should result internally in 3 compressed blocks being read
        // and decompressed
        // What should be returned is the value of
        // "Hello World" + (2-MiB - 10) of '\0' + 'A'
        //

        // This will store to the expected uncompressed that is being tested
        constexpr AZStd::string_view firstBlockEnd = "H";
        constexpr AZStd::string_view secondBlockBegin = "ello World";
        constexpr AZStd::string_view finalBlockBegin = "Archive";
        AZStd::vector<AZStd::byte> expectedResultData;
        // The total size should be (2 MiB + 2)
        constexpr size_t PartialReadSize = firstBlockEnd.size() + ArchiveBlockSizeForCompression + 1;
        static_assert(PartialReadSize == 2_mib + 2);

        expectedResultData.reserve(PartialReadSize);

        auto expectedDataBackInserter = AZStd::back_inserter(expectedResultData);
        AZStd::ranges::copy(AZStd::as_bytes(AZStd::span(firstBlockEnd)), expectedDataBackInserter);
        AZStd::ranges::copy(AZStd::as_bytes(AZStd::span(secondBlockBegin)), expectedDataBackInserter);
        AZStd::fill_n(expectedDataBackInserter, ArchiveBlockSizeForCompression - secondBlockBegin.size(), AZStd::byte{});
        // Only copy the first character from the final block
        AZStd::ranges::copy(AZStd::as_bytes(AZStd::span(finalBlockBegin)).first(1), expectedDataBackInserter);
        ASSERT_EQ(PartialReadSize, expectedResultData.size());

        AZStd::vector<AZStd::byte> archiveBuffer;
        AZ::IO::ByteContainerStream archiveStream(&archiveBuffer);

        {
            // Create an archive with a several files in it
            IArchiveWriter::ArchiveStreamPtr archiveWriterStreamPtr(&archiveStream, { false });
            auto createArchiveWriterResult = CreateArchiveWriter(AZStd::move(archiveWriterStreamPtr));
            ASSERT_TRUE(createArchiveWriterResult);
            AZStd::unique_ptr<IArchiveWriter> archiveWriter = AZStd::move(createArchiveWriterResult.value());

            ArchiveWriterFileSettings fileSettings;

            // Generate the data for the file to compressed
            AZStd::vector<AZStd::byte> fileBuffer;
            // The file should be 4-MiB + the the size of the string "Archive"
            constexpr size_t FileSize = ArchiveBlockSizeForCompression * 2 + finalBlockBegin.size();
            fileBuffer.reserve(FileSize);
            auto fileBufferBackInserter = AZStd::back_inserter(fileBuffer);

            // Generate the data for the first block
            AZStd::fill_n(fileBufferBackInserter, ArchiveBlockSizeForCompression - firstBlockEnd.size(), AZStd::byte{});
            AZStd::ranges::copy(AZStd::as_bytes(AZStd::span(firstBlockEnd)), fileBufferBackInserter);
            // Generate the data for the second block
            AZStd::ranges::copy(AZStd::as_bytes(AZStd::span(secondBlockBegin)), fileBufferBackInserter);
            AZStd::fill_n(fileBufferBackInserter, ArchiveBlockSizeForCompression - secondBlockBegin.size(), AZStd::byte{});
            // Generate the data for the final block
            AZStd::ranges::copy(AZStd::as_bytes(AZStd::span(finalBlockBegin)), fileBufferBackInserter);
            ASSERT_EQ(FileSize, fileBuffer.size());

            fileSettings.m_compressionAlgorithm = CompressionLZ4::GetLZ4CompressionAlgorithmId();
            fileSettings.m_relativeFilePath = "MultiblockCompressed.bin";
            EXPECT_TRUE(archiveWriter->AddFileToArchive(fileBuffer, fileSettings));

            IArchiveWriter::CommitResult commitResult = archiveWriter->Commit();
            ASSERT_TRUE(commitResult);
        }

        // Set the ArchiveStreamDeleter to not delete the stack ByteContainerStream
        IArchiveReader::ArchiveStreamPtr archiveReaderStreamPtr(&archiveStream, { false });
        auto createArchiveReaderResult = CreateArchiveReader(AZStd::move(archiveReaderStreamPtr));
        ASSERT_TRUE(createArchiveReaderResult);
        AZStd::unique_ptr<IArchiveReader> archiveReader = AZStd::move(createArchiveReaderResult.value());

        // No error should occur and the archive should have been successfully mounted
        EXPECT_TRUE(archiveReader->IsMounted());

        {
            // Extract subdirectory/Level.prefab

            // The default ArchiveWriterFileSettings used in this test lowercases the files paths that were added
            // So before looking up the Level.prefab in the "subdirectory" directory, lowercase the path
            AZ::IO::Path filePathLower = "MultiblockCompressed.bin";
            AZStd::to_lower(filePathLower.Native());

            // Use ArchiveListFileResult to lookup the file metadata
            const ArchiveListFileResult archiveListFileResult = archiveReader->ListFileInArchive(filePathLower);
            ASSERT_TRUE(archiveListFileResult);

            AZStd::vector<AZStd::byte> fileBuffer;
            fileBuffer.resize_no_construct(archiveListFileResult.m_uncompressedSize);
            // Populate settings structure needed to extract the file
            ArchiveReaderFileSettings fileSettings;
            // Use the file path token this time to extract the file
            fileSettings.m_filePathIdentifier = archiveListFileResult.m_filePathToken;
            // Set the start offset to start reading from the byte of the first block
            fileSettings.m_startOffset = ArchiveBlockSizeForCompression - 1;
            // Read 2-Mib + 2 to read the final byte of the first block, the entire second block
            // and the first byte from the final block
            fileSettings.m_bytesToRead = ArchiveBlockSizeForCompression + 2;

            const ArchiveExtractFileResult archiveExtractFileResult = archiveReader->ExtractFileFromArchive(
                fileBuffer, fileSettings);
            ASSERT_TRUE(archiveExtractFileResult);
            EXPECT_NE(InvalidArchiveFileToken, archiveExtractFileResult.m_filePathToken);
            EXPECT_EQ(filePathLower, archiveExtractFileResult.m_relativeFilePath);
            EXPECT_EQ(CompressionLZ4::GetLZ4CompressionAlgorithmId(), archiveExtractFileResult.m_compressionAlgorithm);
            // The file should have been compressed
            // Just validate that its size > 0
            EXPECT_GT(archiveExtractFileResult.m_compressedSize, 0);
            EXPECT_EQ(fileBuffer.size(), archiveExtractFileResult.m_uncompressedSize);

            // Since this is the only file within the achive it should start at offset 512
            AZ::u64 expectedFileOffset = ArchiveDefaultBlockAlignment;
            EXPECT_EQ(expectedFileOffset, archiveExtractFileResult.m_offset);

            // The file span should only view the 2-MiB + 2 sequence within the uncompressed file
            // that buffer
            AZStd::span<AZStd::byte> requestedFileData = archiveExtractFileResult.m_fileSpan;
            EXPECT_TRUE(AZStd::ranges::equal(requestedFileData, expectedResultData));
        }
    }
}

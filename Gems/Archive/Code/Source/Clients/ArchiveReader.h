/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Archive/Clients/ArchiveBaseAPI.h>
#include <Archive/Clients/ArchiveReaderAPI.h>

#include <Clients/ArchiveTOCView.h>

#include <AzCore/Memory/Memory_fwd.h>
#include <AzCore/RTTI/RTTIMacros.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/utility/to_underlying.h>
#include <AzCore/Task/TaskExecutor.h>

namespace AZ
{
    class TaskGraphEvent;
}

namespace AZ::IO
{
    class GenericStream;
}

namespace Archive
{
    //! Implements the Archive Reader Interface
    //! This can be used to read and extract files from an archive
    class ArchiveReader
        : public IArchiveReader
    {
    public:
        AZ_TYPE_INFO_WITH_NAME_DECL(ArchiveReader);
        AZ_RTTI_NO_TYPE_INFO_DECL();
        AZ_CLASS_ALLOCATOR_DECL;

        ArchiveReader();
        //! Create an archive reader using the specified reader settings
        explicit ArchiveReader(const ArchiveReaderSettings& readerSettings);
        //! Open an file at the specified file path and takes sole ownership of it
        //! The ArchiveReader will close the file on Unmount
        explicit ArchiveReader(AZ::IO::PathView archivePath, const ArchiveReaderSettings& readerSettings = {});
        //! Takes ownership of the open stream and will optionally delete it based on the ArchiveFileDeleter
        explicit ArchiveReader(ArchiveStreamPtr archiveStream, const ArchiveReaderSettings& readerSettings = {});
        ~ArchiveReader();

        //! Opens the archive path and returns true if successful
        //! Will unmount any previously mounted archive
        bool MountArchive(AZ::IO::PathView archivePath) override;
        bool MountArchive(ArchiveStreamPtr archiveStream) override;

        //! Closes the handle to the mounted archive stream
        void UnmountArchive() override;

        //! Returns if an open archive that is mounted
        bool IsMounted() const override;

        //! Reads the content of the file specified in the ArchiveReadeFileSettings
        //! The file path identifier in the settings is used to locate the file to extract from the archive
        //! The outputSpan should be a pre-allocated buffer that is large enough
        //! to fit either the uncompressed size of the file if the `m_decompressFile` setting is true
        //! or the compressed size of the file if the `m_decompressFile` setting is false
        //!
        //! @param outputSpan pre-allocated buffer that should be large enough to store the extracted
        //! file
        //! @param fileSettings settings which can configure whether the file should be decompressed,
        //! the start offset where to start reading content within the file, how many bytes
        //! to read from the file, etc...
        //! @return ArchiveExtractFileResult structure which on success contains
        //! a span of the actual data extracted from the Archive.
        //! NOTE: The extracted data can be smaller than the outputSpan.size()
        //! On failure, the result outcome member contains the error that occurred
        ArchiveExtractFileResult ExtractFileFromArchive(AZStd::span<AZStd::byte> outputSpan,
            const ArchiveReaderFileSettings& fileSettings) override;

        //! List the file metadata from the archive using the ArchiveFileToken
        //! @param filePathToken identifier token that can be used to quickly lookup
        //! metadata about the file
        //! @return ArchiveListResult with metadata for the file if found
        ArchiveListFileResult ListFileInArchive(ArchiveFileToken filePathToken) const override;
        //! List the file metadata from the archive using the relative FilePath
        //! @param relativePath File path to lookup within the archive
        //! @return ArchiveListResult with metadata for the file if found
        ArchiveListFileResult ListFileInArchive(AZ::IO::PathView relativePath) const override;

        //! Returns if the archive contains a relative path
        //! @param relativePath Relative path within archive to search for
        //! @returns true if the relative path is contained with the Archive
        //! equivalent to `return FindFile(relativePath) != InvalidArchiveFileToken;`
        bool ContainsFile(AZ::IO::PathView relativePath) const override;

        //! Enumerates all files within the archive table of contents and invokes a callback
        //! function with the listing information about the file
        //! This function can be used to build filter files in the Archive based on any value
        //! supplied in the ArchiveListFileResult structure
        //! For example filtering can be done based on file path(such as globbing for all *.txt files)
        //! or filtering based on uncompressed size(such as locating all files > 2MiB, etc...)
        //! Alternatively this function can be used to list all of the files within the archive by
        //! binding a lambda that populates a vector
        //!
        //! @param listFileCallback Callback which is invoked for each file in the archive
        //! @return result structure that is convertible to a boolean value indicating if enumeration was successful
        EnumerateArchiveResult EnumerateFilesInArchive(ListFileCallback listFileCallback) const override;

        //! Dump metadata for the archive to the supplied generic stream
        //! @param metadataStream archive file metadata will be written to the stream
        //! @param metadataSettings settings using which control the file metadata to write to the stream
        //! @return true if metadata was successfully written
        bool DumpArchiveMetadata(AZ::IO::GenericStream& metadataStream,
            const ArchiveMetadataSettings& metadataSettings = {}) const override;

    private:
        //! Reads the Archive Header into memory.
        //! Afterwards the Archive Header is used to read the TOC into memory
        //! and build any structures for acceleration of lookups
        bool ReadArchiveHeaderAndToc();
        //! Reads the archive header from the generic stream
        bool ReadArchiveHeader(ArchiveHeader& archiveHeader, AZ::IO::GenericStream& archiveStream);
        //! Reads the archive table of contents from the generic stream by using the archive header
        //! to determine the offset and size of the table of contents
        struct ArchiveTableOfContentsReader;
        bool ReadArchiveTOC(ArchiveTableOfContentsReader& archiveToc, AZ::IO::GenericStream& archiveStream,
            const ArchiveHeader& archiveHeader);

        //! Creates a mapping of views to the file paths within the archive to the ArchiveFileToken
        //! The ArchiveFileToken currently corresponds to the index within the table of contents
        //! ArchiveTocFilePathIndex, ArchiveTocFileMetadata and ArchiveFilePath vector structures
        bool BuildFilePathMap(const ArchiveTableOfContentsView& archiveToc);

        //! Read data from offset within archive directly to span
        //! @param fileBuffer pre-allocated span to populate buffer with data
        //! @param offset absolute file within mounted archive to start reading data from
        //! @param fileSize the amount of data to read at the specified offset
        //! @param fileSettings setting structure which is used to configure where the
        //! start offset for reading within the extracted file and how many bytes to read from that
        //! start offset
        //! @return result outcome which contains an error messages related to reading the file
        //! if it fails
        using ReadRawFileOutcome = AZStd::expected<AZStd::span<AZStd::byte>, ResultString>;
        ReadRawFileOutcome ReadRawFileIntoBuffer(AZStd::span<AZStd::byte> fileBuffer,
            AZ::u64 offset, AZ::u64 fileSize,
            const ArchiveReaderFileSettings& fileSettings);

        //! Decompressed the content from the input buffer
        //! @param decompressionResultSpan span to populated with decompressed results
        //! @param fileSettings settings which indicate the max number of decompression task
        //! to use for decompressing the file content.
        //! This parameter also contains settings for selecting an offset within the decompressed
        //! file to start reading, as well as a cap on the number of bytes to read from that start offset
        //! @param extractFileResult Contains the compressed size, raw offset within the archive and uncompressed
        //! size of the file needed for extracting
        //! @return result outcome with a span containing a view of the decompressed file data
        //! within the offset range specified by
        //! [ArchiveReaderFileSettings::m_startOffset, ArchiveReaderFileSettings::m_startOffset + ArchiveReaderFileSettings::m_bytesToRead)
        //! Otherwise an error message string providing reasons why decompression failed
        using ReadCompressedFileOutcome = AZStd::expected<AZStd::span<AZStd::byte>, ResultString>;
        ReadCompressedFileOutcome ReadCompressedFileIntoBuffer(AZStd::span<AZStd::byte> decompressionResultSpan,
            const ArchiveReaderFileSettings& fileSettings,
            const ArchiveExtractFileResult& extractFileResult);


        // Private Member variables section

        //! Archive Reader specific settings
        //! Controls the number of tasks to use for reading and decompression of content
        //! from the archive
        //! Also contains an error callback that is invoked when error occurs in the constructor
        ArchiveReaderSettings m_settings;
        //! Archive header as read from the first sizeof(ArchiveHeader) blocks of the archive stream
        //! The header is not modified by the reader
        ArchiveHeader m_archiveHeader;
        //! View of the Archive TOC within the supplied archive stream
        //! Since the ArchiveReader doesn't mutate the archive, a Table of Contents View is used
        //! and paired with a raw buffer of the Table of Contents
        struct ArchiveTableOfContentsReader
        {
            // Default a table of contents reader that has an empty vector
            // and default constructed view
            ArchiveTableOfContentsReader();
            // Stores the buffer containing the Table of Contents raw data
            // and an ArchiveTableOfContentsView instance which is a read-only view into that raw data
            ArchiveTableOfContentsReader(AZStd::vector<AZStd::byte> tocBuffer, ArchiveTableOfContentsView tocView);

            ArchiveTableOfContentsView m_tocView;
        private:
            AZStd::vector<AZStd::byte> m_tocBuffer;
        };
        ArchiveTableOfContentsReader m_archiveToc;

        //! Stores mapping of FilePath to index within the file path table in the Archive TOC
        //! The index is used to as the ArchiveFileToken
        //! IMPORTANT: The PathView is a view into the m_archiveToc TOC buffer
        //! and therefore this map should be cleared before reading another archive TOC
        using FilePathTable = AZStd::unordered_map<AZ::IO::PathView, size_t>;
        FilePathTable m_pathMap;

        //! GenericStream pointer which stores the open archive
        ArchiveStreamPtr m_archiveStream;

        //! Protects reads within the archive stream
        //! NOTE: This does restrict read jobs to be done on one thread at a time
        //! if done using the AZ::IO::GenericStream API as it maintains a single seek position
        AZStd::mutex m_archiveStreamMutex;

        //! Task Executor used to decompress blocks of a file in parallel
        AZ::TaskExecutor m_taskExecutor;
    };
} // namespace Archive

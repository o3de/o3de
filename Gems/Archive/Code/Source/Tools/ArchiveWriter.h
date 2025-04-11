/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Archive/Clients/ArchiveBaseAPI.h>
#include <Archive/Tools/ArchiveWriterAPI.h>

#include <Clients/ArchiveTOC.h>

#include <AzCore/Memory/Memory_fwd.h>
#include <AzCore/RTTI/RTTIMacros.h>
#include <AzCore/std/containers/unordered_map.h>
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
    //! class which is used to write into a stream the O3DE Archive format
    //! The caller is required to supply a ArchiveWriterSettings structure
    //! which contains the ArchiveHeader and ArchiveTableOfContents data
    //! to use when writing to the Archive file
    //! The class can be initialized with a user supplied AZ::IO::GenericStream class
    //! in which case it should be open in stream should needs to be open with OpenMode::ModeUpdate
    //! The reason why is that to locate information about any content files in order to update an existing archive
    //! it read access is needed
    //! The recommend OpenMode value for opening a new archive or updating an existing archive
    //! are as follows
    //! constexpr OpenMode mode = OpenMode::Update | OpenMode::Append | OpenMode::Binary
    //! The Append option makes sure that the Archive is not truncated on open
    class ArchiveWriter
        : public IArchiveWriter
    {
    public:
        AZ_TYPE_INFO_WITH_NAME_DECL(ArchiveWriter);
        AZ_RTTI_NO_TYPE_INFO_DECL();
        AZ_CLASS_ALLOCATOR_DECL;

        ArchiveWriter();
        //! Create an archive writer using the specified writer settings
        explicit ArchiveWriter(const ArchiveWriterSettings& writerSettings);
        //! Open an file at the specified file path and takes sole ownership of it
        //! The ArchiveWriter will close the file on Unmount
        explicit ArchiveWriter(AZ::IO::PathView archivePath, const ArchiveWriterSettings& writerSettings = {});
        //! Takes ownership of the open stream and will optionally delete it based on the ArchiveFileDeleter
        explicit ArchiveWriter(ArchiveStreamPtr archiveStream, const ArchiveWriterSettings& writerSettings = {});
        ~ArchiveWriter();

        //! Opens the archive path and returns true if successful
        //! Will unmount any previously mounted archive
        bool MountArchive(AZ::IO::PathView archivePath) override;
        bool MountArchive(ArchiveStreamPtr archiveStream) override;

        //! Closes the handle to the mounted archive stream
        //! This will invoke the Commit() function to write the archive TOC
        //! to the stream before closing the stream
        void UnmountArchive() override;

        //! Returns if an open archive that is mounted
        bool IsMounted() const override;

        //! Write the updated ArchiveHeader to the beginning of the stream and
        //! Table of Contents to end of the stream
        //!
        //! If this call is successful, the archive TOC has been successfully written
        //! This function has been marked [[nodiscard]], to ensure the caller
        //! checks the return value
        //! @return A successful expectation if the TOC has been written
        using CommitResult = AZStd::expected<void, ResultString>;
        [[nodiscard]] CommitResult Commit() override;

        //! Adds the content from the stream to the relative path
        //! @param inputStream stream class where data for the file is source from
        //! The entire stream is read into the memory and written into archive
        //! @param fileSettings settings used to configure the relative path to
        //! write to the archive for the given file data.
        //! It also allows users to configure the compression algorithm to use,
        //! and whether the AddFileToArchive logic fails if an existing file is being added
        //! @return ArchiveAddFileResult containing the actual compression file path
        //! as saved to the Archive TOC, the compression algorithm used
        //! and an Archive File Token which can be used to remove the file if need be
        //! On failure, the result outcome contains any errors that have occurred
        ArchiveAddFileResult AddFileToArchive(AZ::IO::GenericStream& inputStream,
            const ArchiveWriterFileSettings& fileSettings) override;

        //! Use the span contents to add the file to the archive
        //! @param inputSpan view of data which will be written to the archive
        //! at the relative path supplied in the @fileSettings parameter
        //! @param fileSettings settings used to configure the relative path to
        //! write to the archive for the given file data.
        //! It also allows users to configure the compression algorithm to use,
        //! and whether the AddFileToArchive logic fails if an existing file is being added
        //! @return ArchiveAddFileResult containing the actual compression file path
        //! as saved to the Archive TOC, the compression algorithm used
        //! and an Archive File Token which can be used to remove the file if need be
        //! On failure, the result outcome contains any errors that have occurred
        ArchiveAddFileResult AddFileToArchive(AZStd::span<const AZStd::byte> inputSpan,
            const ArchiveWriterFileSettings& fileSettings) override;

        //! Searches for a relative path within the archive
        //! @param relativePath Relative path within archive to search for
        //! @return A token that identifies the Archive file if it exist
        //! if the with the specified path doesn't exist InvalidArchiveFileToken is returned
        ArchiveFileToken FindFile(AZ::IO::PathView relativePath) const override;
        //! Returns if the archive contains a relative path
        //! @param relativePath Relative path within archive to search for
        //! @returns true if the relative path is contained with the Archive
        //! equivalent to `return FindFile(relativePath) != InvalidArchiveFileToken;`
        bool ContainsFile(AZ::IO::PathView relativePath) const override;

        //! Removes the file from the archive using the ArchiveFileToken
        //! @param filePathToken Identifier queried using FindFile or AddFileToArchive
        //! NOTE: The entry in the table of contents is not actually removed
        //! The index where the file is located using the filePathToken
        //! is just added to the removed file indices set
        //! @return ArchiveRemoveResult with metadata about how the deleted file was
        //! stored in the Archive
        ArchiveRemoveFileResult RemoveFileFromArchive(ArchiveFileToken filePathToken) override;
        //! Removes the file from the archive using a relative path name
        //! @param relativePath relative path within archive to search for
        //! @return ArchiveRemoveResult with metadata about how the deleted file was
        //! stored in the Archive
        ArchiveRemoveFileResult RemoveFileFromArchive(AZ::IO::PathView relativePath) override;

        //! Dump metadata for the archive to the supplied generic stream
        //! @param metadataStream archive file metadata will be written to the stream
        //! @param metadataSettings settings using which control the file metadata to write to the stream
        //! @return true if metadata was successfully written
        bool DumpArchiveMetadata(AZ::IO::GenericStream& metadataStream,
            const ArchiveMetadataSettings& metadataSettings = {}) const override;

    private:

        bool ReadArchiveHeaderAndToc();
        //! Wraps an offset of the block to write plus the block size within the final buffer
        //! that will be written to the archive block section
        //! When the file is stored uncompressed, the offset is 0 and the size is the entire
        //! input span supplied to @AddFileToArchive
        struct BlockOffsetSizePair
        {
            size_t m_offset{ AZStd::numeric_limits<size_t>::max() };
            size_t m_size{};
        };
        //! Encapsulates the compression algorithm plus an output span from compressing the data
        struct ContentFileBlocks
        {
            //! Stores the index into the TOC of compression algorithm to use
            AZ::u8 m_compressionAlgorithmIndex{ UncompressedAlgorithmIndex };
            //! Stores a vector of offset, size pairs containing each block of the file to store in the
            //! archive raw block section
            AZStd::vector<BlockOffsetSizePair> m_blockOffsetSizePairs;
            //! Span which references the data to write
            //! The block offset size pairs are offsets into this span
            //! Each block is padded to be aligned to 512 byte boundaries
            //! Therefore this span will generally have a larger than size()
            //! than the m_totalUnalignedSize member
            AZStd::span<const AZStd::byte> m_writeSpan;
            //! Stores the total compressed size of all blocks of the file
            //! if they were stored without alignment
            AZ::u64 m_totalUnalignedSize{};

        };
        using CompressContentOutcome = AZStd::expected<ContentFileBlocks, ResultString>;

        //! Uses the AZ Task system to compress 2 MiB blocks of a content file in parallel
        //! @param compressionBuffer output buffer to write compressed content
        //! @oaram fileSettings settings controlling how to write the content data into the archive stream
        //! @param contentFileData input buffer of file content to write to as a file using the file path
        //!        specified in the fileSettings
        //! @return an outcome that contains a span if the content fileData was successfully compressed
        //!         otherwise a failure string containing the error that occurs when attempting to compressed
        //!         the asynchronous content
        CompressContentOutcome CompressContentFileAsync(AZStd::vector<AZStd::byte>& compressionBuffer,
            const ArchiveWriterFileSettings& fileSettings, AZStd::span<const AZStd::byte> inputDataSpan);

        //! In-memory structure which stores metadata about the file contents after being
        //! sent through any compression algorithm and path normalization
        struct ContentFileData
        {
            //! The file path to uses for the content being written to the archive
            //! This path has been posted processed to to account for any changes
            //! to file case due to the `ArchiveWriterFileSettings::m_fileCase` member
            AZ::IO::PathView m_relativeFilePath;
            //! stores block data about the file contents to write to block section of archive
            //! The block data contains offsets into the buffer to write
            ContentFileBlocks m_contentFileBlocks;
            //! Reference to the file contents span that was supplied to @AddFileToArchive
            //! This is used to retrieve the uncompressed size of the file contents
            //! and to perform a CRC32 over the uncompressed data
            AZStd::span<const AZStd::byte> m_uncompressedSpan;
        };
        //! Update the archive header with the new file count and the location
        //! of the file in the archive
        ArchiveFileToken WriteContentFileToArchive(const ArchiveWriterFileSettings& fileSettings,
            const ContentFileData& contentFileData);

        //! Helper function to update the TOC block offset table entries for the file
        //! being written
        //! @return Index into the block offset table where the compressed size
        //! of the 2 MiB blocks are stored
        AZ::u64 UpdateBlockOffsetEntryForFile(const ContentFileData& contentFileData);

        //! Reads the archive header from the generic stream
        bool ReadArchiveHeader(ArchiveHeader& archiveHeader, AZ::IO::GenericStream& archiveStream);
        //! Reads the archive table of contents from the generic stream by using the archive header
        //! to determine the offset and size of the table of contents
        bool ReadArchiveTOC(ArchiveTableOfContents& archiveToc, AZ::IO::GenericStream& archiveStream,
            const ArchiveHeader& archiveHeader);

        //! Builds in-memory acceleration structures for quick look up of deleted Archive files blocks
        //! This is done by starting from the first deleted block offset in the Archive Header
        //! and iterating through the blocks within the block section of the file
        //! The deleted block map provides a mapping of free blocks size to offset within the archive
        bool BuildDeletedFileBlocksMap(const ArchiveHeader& archiveHeader,
            AZ::IO::GenericStream& archiveStream);

        //! Iterates over the deleted Archive file block map and merges
        //! Any deleted blocks that are next to each other into a single entry
        //! of the combined sizes
        //! NOTE: This iterates over the entire deleted block map
        //! so the operation is takes longer the more deleted blocks there are
        void MergeContiguousDeletedBlocks();

        //! Creates a mapping of views to the file paths within the archive to the ArchiveFileToken
        //! The ArchiveFileToken currently corresponds to the index within the table of contents
        //! ArchiveTocFilePathIndex, ArchiveTocFileMetadata and ArchiveFilePath vector structures
        bool BuildFilePathMap(const ArchiveTableOfContents& archiveToc);

        //! Returns an offset to seek to in the archive stream, where the content file data should
        //! be written
        //! If the fileSize can fit within a deleted file block, it offset is extract from the deleted block map
        //! and returned
        //! Otherwise the table of contents start offset is returned and archive header updates that table of contents
        //! value by the amount to be written
        AZ::u64 ExtractWriteBlockOffset(AZ::u64 alignedFileSizeToWrite);

        //! Encapsulates the result of converting the ArchiveTableOfContents structure
        //! into a raw byte buffer
        struct WriteTocRawResult
        {
            // Return true of there is an error compressing the span
            explicit operator bool() const;

            //! Stores a span to the raw toc data if success
            AZStd::span<const AZStd::byte> m_tocSpan;
            //! Stores any error messages if writing the TOC data to a raw buffer has failed
            ResultString m_errorString;
        };
        //! Writes the Table of Contents into a raw buffer
        //! @param tocOutputBuffer output buffer to write uncompressed raw TOC data
        //! @param tocUncompressedInputSpan input buffer of the raw table of contents data to write
        //! @return a result structure containing a span in the output buffer containing
        //! the raw TOC data and its actual size
        WriteTocRawResult WriteTocRaw(AZStd::vector<AZStd::byte>& tocOutputBuffer);

        //! Encapsulates the result of compression a raw buffer of table of contents data
        //! data
        struct CompressTocRawResult
        {
            // Return true of there is an error compressing the span
            explicit operator bool() const;

            //! Stores a span to the compressed TOC if successful
            //! reference to the uncompressed TOC input span if not
            AZStd::span<const AZStd::byte> m_compressedTocSpan;
            //! Stores any error messages if compression fails
            ResultString m_errorString;
        };
        //! Compresses the raw Table of Contents using the table of contents compression
        //! algorithm specified in the Archive header
        //! @param tocCompressionBuffer output buffer to write compressed table of contents
        //! @param tocUncompressedInputSpan input buffer of the raw table of contents data to write
        //! @param compressionAlgorithmId the compression algorithm to use for compressing the Table of Contents
        //! @return a result structure containing a span within the compression buffer of the compressed TOC data
        //! if successful, otherwise a span to the original input span is returned
        CompressTocRawResult CompressTocRaw(AZStd::vector<AZStd::byte>& tocCompressionBuffer,
            AZStd::span<const AZStd::byte> uncompressedTocInputSpan,
            Compression::CompressionAlgorithmId compressionAlgorithmId);

        //! Archive Writer specific settings
        //! Controls the compression algorithm used to write the table of contents
        //! Also contains an error callback that is invoked with an ArchiveWriterError
        //! instance containing the error that occurs when using this class
        ArchiveWriterSettings m_settings;
        //! Archive header which is updated in place and written to the archive stream
        //! when the archive data is committed
        //! When a stream with an existing archive is supplied,
        //! this value is initialized using that archive
        ArchiveHeader m_archiveHeader;
        //! Archive TOC which manages in-memory file metadata about content within the archive
        //! The TOC is read from the archive stream, if an existing archive is supplied
        //! and the archive header was able to be successfully read
        //!
        //! NOTE: The File Metadata vector, File Path Index vector and File Path
        //! are never resized downwards.
        //! When a file is deleted, it is marked deleted by adding its index to the removedFileIndices set below
        //! When a file is added, then the following logic occurs
        //! If there is an entry in the removed file set, then the existing entry in the File Path Index vector and File Path vector
        //! at that index stored in the removed file set
        //! Otherwise a new entry is appended to the end of the those vectors
        ArchiveTableOfContents m_archiveToc;

        //! Stores mapping of FilePath to index within the file path table in the Archive TOC
        using FilePathTable = AZStd::unordered_map<AZ::IO::Path, size_t>;
        FilePathTable m_pathMap;
        //! Set containing the index of removed file entries in the table of contents
        //! for this specific ArchiveWriter instance
        //! The ArchiveWriter itself never writes out removed file entries and this set
        //! is only for in-memory use when updating an archive.
        //! NOTE: This is not an ArchiveTocFilePathIndex variable inside the File Path Index vector
        //! The value here is an integer index into a vector of ArchiveTocFilePathIndex instances
        //! The size of the
        using RemovedFileIndexSet = AZStd::set<AZ::u64>;
        RemovedFileIndexSet m_removedFileIndices;

        //! Stores a table that maps the unused size represented by the
        //! deleted raw block data to a sorted set of offsets into the mounted archive stream
        //! where the deleted block data starts
        //! This map is used to quickly lookup deleted blocks within an existing archive file
        //! which can be re-used to write the file data for file that is being added or updated
        using DeletedBlockMap = AZStd::map<AZ::u64, AZStd::set<AZ::u64>>;
        DeletedBlockMap m_deletedBlockSizeToOffsetMap;

        //! GenericStream pointer which stores the open archive
        ArchiveStreamPtr m_archiveStream;

        //! Protects reads and writes to the archive stream
        AZStd::mutex m_archiveStreamMutex;

        //! Task Executor used to compress blocks of a file in parallel
        AZ::TaskExecutor m_taskWriteExecutor;
    };

} // namespace Archive

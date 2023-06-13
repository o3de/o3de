/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>

#include <AzCore/IO/Path/Path.h>
#include <AzCore/Memory/Memory_fwd.h>
#include <AzCore/RTTI/RTTIMacros.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/parallel/thread.h>

#include <Archive/Clients/ArchiveBaseAPI.h>
#include <Archive/Clients/ArchiveInterfaceStructs.h>

#include <Compression/CompressionInterfaceStructs.h>

namespace AZ
{
    template<typename T>
    class Interface;
}

namespace AZ::IO
{
    class GenericStream;
}

namespace AZStd
{
    using std::unique_ptr;
}

namespace Compression
{
    struct DecompressionOptions;
}

namespace Archive
{
    //! ErrorCode structure which is used to indicate errors
    //! when reading from an archive
    //! The value of 0 is reserved to indicate no error
    enum class ArchiveReaderErrorCode
    {
        ErrorOpeningArchive = 1,
        ErrorReadingHeader,
        ErrorReadingTableOfContents,
    };
    using ArchiveReaderErrorString = AZStd::fixed_string<512>;

    //! Wraps an error code enum and a string containing an error message
    //! when performing archive reading operations
    struct ArchiveReaderError
    {
        explicit operator bool() const;

        ArchiveReaderErrorCode m_errorCode{};
        ArchiveReaderErrorString m_errorMessage;
    };

    //! Stores settings to configure how Archive Reader Settings
    struct ArchiveReaderSettings
    {
        //! Callback which is invoked by the ArchiveReader to inform users of errors that occurs
        //! This is used in by functions that can't return an error outcome such as constructors
        using ErrorCallback = AZStd::function<void(const ArchiveReaderError&)>;
        ErrorCallback m_errorCallback = [](const ArchiveReaderError&) {};

        //! Configures the maximum number of decompression task that can run in parallel
        //! If the value is 0, then a single decompression task that will be run
        //! at a given moment
        AZ::u32 m_maxDecompressTasks{ AZStd::thread::hardware_concurrency() };

        //! Configures the maximum number of read task that can run in parallel
        //! For a value of 0 maps to a single read task
        AZ::u32 m_maxReadTasks{ 1 };
    };

    //! Settings for controlling how an individual file is extracted from an archive.
    //! It supports specifying custom decompression options that is forwarded
    //! to the registered IDecompressionInterface used to decompress
    //! the file if it is compressed
    struct ArchiveReaderFileSettings
    {
        //! Variant which stores either a path view or an ArchiveFileToken
        //! It is used to supply the identifier that can be used
        //! to query the file contents
        AZStd::variant<AZ::IO::PathView, ArchiveFileToken> m_filePathIdentifier;
        //! Decompress the file content if compressed
        //! By default, a compressed content will be decompressed
        //! after being read from the Archive file
        bool m_decompressFile{ true };
        //! Pointer to a decompression options derived struct
        //! This can be used to supply custom decompression options
        const Compression::DecompressionOptions* m_decompressionOptions{};
        //! Offset within the file being extracted to start reading
        AZ::u64 m_startOffset{};
        //! The amount of bytes to read from the extracted file
        //! Defaults to AZStd::numeric_limits<AZ::u64>::max()
        //! which is used as sentinel value to indicate the entire
        //! file should be read
        AZ::u64 m_bytesToRead{ AZStd::numeric_limits<AZ::u64>::max() };
    };

    //! Returns result data around operation of adding a stream of content data
    //! to an archive file
    struct ArchiveExtractFileResult
    {
        //! returns if adding a stream of data to a file within the archive has succeeded
        //! it does by checking that the ArchiveFileToken != InvalidArchiveFileToken
        explicit operator bool() const;

        //! The file path of the extracted file
        AZ::IO::Path m_relativeFilePath;
        //! Identifier token that allows for quicker lookup of the file in the mounted
        //! archive TOC for the ArchiveReader instance the file was extracted from
        ArchiveFileToken m_filePathToken{ InvalidArchiveFileToken };
        //! The compression algorithm ID representing the compression algorithm used to store the file
        Compression::CompressionAlgorithmId m_compressionAlgorithm{ Compression::Uncompressed };
        //! The uncompressed size of the removed file
        AZ::u64 m_uncompressedSize{};
        //! the compressed size of the extracted file
        AZ::u64 m_compressedSize{};
        //! The raw offset of the file in the archive
        //! As the ArchiveHeader is 512-byte aligned to the beginning of the file
        //! this value is at least 512,
        //! NOTE: The TocOffsetU64 structure is used to enforce that the value is >= 512
        ArchiveHeader::TocOffsetU64 m_offset{};
        //! CRC32 checksum of the uncompressed file data
        AZ::Crc32 m_crc32{};
        //! Span which view of the extracted file data
        //! If the ArchiveReaderFileSettings specifies decompression should occur,
        //! Then the extracted file content will be the raw content
        AZStd::span<AZStd::byte> m_fileSpan;

        //! Stores any error messages related to extraction of the file from the archive
        ResultOutcome m_resultOutcome;
    };

    //! Returns a result structure that indicates if removal of a content file from the
    //! archive was successful
    //! Metadata about the file is returned, such as its file path, compressed algorithm ID
    //! offset from the beginning of the raw file data blocks, uncompressed size and compressed size
    struct ArchiveListFileResult
    {
        //! returns if adding a stream of data to a file within the archive has succeeded
        //! it does by checking that the ArchiveFileToken != InvalidArchiveFileToken
        explicit operator bool() const;

        //! The file path of the file being queried
        AZ::IO::Path m_relativeFilePath;
        //! Identifier token that allows for quicker lookup of the file in the archive TOC
        ArchiveFileToken m_filePathToken{ InvalidArchiveFileToken };
        //! The compression algorithm ID representing the compression algorithm used to store the file
        Compression::CompressionAlgorithmId m_compressionAlgorithm{ Compression::Uncompressed };
        //! The uncompressed size of the removed file
        AZ::u64 m_uncompressedSize{};
        //! the compressed size of the removed file
        //! INFO: This value will be a multiple of 512
        AZ::u64 m_compressedSize{};
        //! The raw offset of the file in the archive
        //! As the ArchiveHeader is 512-byte aligned to the beginning of the file
        //! this value is at least 512,
        //! NOTE: The TocOffsetU64 structure is used to enforce that the value is >= 512
        ArchiveHeader::TocOffsetU64 m_offset{};
        //! CRC32 checksum of the uncompressed file data
        AZ::Crc32 m_crc32{};

        //! Stores error and information messages with listing the contents of the file
        ResultOutcome m_resultOutcome;
    };

    //! Encapsulates the result of enumerating files with the archive
    //! If an error occurs the ResultOutcome unexpected value is set
    struct EnumerateArchiveResult
    {
        //! returns true if the ResultOutcome has a non-error value
        explicit constexpr operator bool() const;
        //! Stores error info about enumerating all files within the archive
        ResultOutcome m_resultOutcome;
    };


    //! Interface for the ArchiveReader of O3DE Archive format
    //! An ArchiveReaderSettings object can be used to customize how
    //! an archive is read.
    //! Such as the ability to specify the number of read and decompression task that
    //! can run in parallel

    //! The user can supply their own stream of archive data via the AZ::IO::GenericStream interface
    //! In that case the archive needs to be opened with at least OpenMode::Read
    //! The recommend OpenMode value for opening the archive are as follows
    //! constexpr OpenMode mode = OpenMode::Read | OpenMode::Binary
    class IArchiveReader
    {
    public:
        AZ_TYPE_INFO_WITH_NAME_DECL(IArchiveReader);
        AZ_RTTI_NO_TYPE_INFO_DECL();
        AZ_CLASS_ALLOCATOR_DECL;

        //! Unique pointer which wraps a stream of archive data
        //! The stream can be owned by the ArchiveReader depending
        //! on the m_delete value
        struct ArchiveStreamDeleter
        {
            ArchiveStreamDeleter();
            ArchiveStreamDeleter(bool shouldDelete);
            void operator()(AZ::IO::GenericStream* ptr) const;
            bool m_delete{ true };
        };
        using ArchiveStreamPtr = AZStd::unique_ptr<AZ::IO::GenericStream, ArchiveStreamDeleter>;

        virtual ~IArchiveReader();

        //! Opens the archive path and returns true if successful
        //! Will unmount any previously mounted archive
        virtual bool MountArchive(AZ::IO::PathView archivePath) = 0;
        virtual bool MountArchive(ArchiveStreamPtr archiveStream) = 0;

        //! Closes the handle to the mounted archive stream
        virtual void UnmountArchive() = 0;

        //! Returns if an open archive that is mounted
        virtual bool IsMounted() const = 0;

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
        virtual ArchiveExtractFileResult ExtractFileFromArchive(AZStd::span<AZStd::byte> outputSpan,
            const ArchiveReaderFileSettings& fileSettings) = 0;

        //! List the file metadata from the archive using the ArchiveFileToken
        //! @param filePathToken identifier token that can be used to quickly lookup
        //! metadata about the file
        //! @return ArchiveListResult with metadata for the file if found
        virtual ArchiveListFileResult ListFileInArchive(ArchiveFileToken filePathToken) const = 0;
        //! List the file metadata from the archive using the relative FilePath
        //! @param relativePath File path to lookup within the archive
        //! @return ArchiveListResult with metadata for the file if found
        virtual ArchiveListFileResult ListFileInArchive(AZ::IO::PathView relativePath) const = 0;

        //! Returns if the archive contains a relative path
        //! @param relativePath Relative path within archive to search for
        //! @returns true if the relative path is contained with the Archive
        //! equivalent to `return FindFile(relativePath) != InvalidArchiveFileToken;`
        virtual bool ContainsFile(AZ::IO::PathView relativePath) const = 0;

        //! Callback structure which is invoked with the metadata for each file in the archive
        //! table of contents section
        //! This can be used to perform filtering on files within the archive
        //! @return a value of `true` can be returned to continue enumeration of the archive
        using ListFileCallback = AZStd::function<bool(ArchiveListFileResult)>;

        //! Enumerates all files within the archive table of contents and invokes a callback
        //! function with the listing information about the file
        //! This function can be used to build filter files in the Archive based on any value
        //! supplied in the ArchiveListFileResult structure
        //! For example filtering can be done based on file path(such as globbing for all *.txt files)
        //! or filtering based on uncompressed size(such as locating all files > 2MiB, etc...)
        //! @param listFileCallback Callback which is invoked for each file in the archive
        //! @return result structure that is convertible to a boolean value indicating if enumeration was successful
        virtual EnumerateArchiveResult EnumerateFilesInArchive(ListFileCallback listFileCallback) const = 0;

        //! Dump metadata for the archive to the supplied generic stream
        //! @param metadataStream with human readable data about files within the archive will be written to the stream
        //! @param metadataStream archive file metadata will be written to the stream
        //! @param metadataSettings settings using which control the file metadata to write to the stream
        virtual bool DumpArchiveMetadata(AZ::IO::GenericStream& metadataStream,
            const ArchiveMetadataSettings& metadataSettings = {}) const = 0;
    };

    //! Factory which is used to creates instances of the ArchiveReader class
    //! The Create functions parameters are forwarded to the ArchiveReader constructor
    class IArchiveReaderFactory
    {
    public:
        AZ_TYPE_INFO_WITH_NAME_DECL(IArchiveReaderFactory);
        AZ_RTTI_NO_TYPE_INFO_DECL();
        AZ_CLASS_ALLOCATOR_DECL;

        virtual ~IArchiveReaderFactory();
        virtual AZStd::unique_ptr<IArchiveReader> Create() const = 0;
        virtual AZStd::unique_ptr<IArchiveReader> Create(const ArchiveReaderSettings& readerSettings) const = 0;
        virtual AZStd::unique_ptr<IArchiveReader> Create(AZ::IO::PathView archivePath,
            const ArchiveReaderSettings& readerSettings) const = 0;
        virtual AZStd::unique_ptr<IArchiveReader> Create(IArchiveReader::ArchiveStreamPtr archiveStream,
            const ArchiveReaderSettings& readerSettings) const = 0;
    };

    // Helper Alias for access the IArchiveReaderFactory instance
    using ArchiveReaderFactoryInterface = AZ::Interface<IArchiveReaderFactory>;

    // The CreateArchiveReader functions are utility functions that help outside gem modules creating an ArchiveReader
    // The return value is a CreateArchiveReaderResult, which will return a unique_ptr to the create ArchiveReader
    // on success or a failure result string indicating why the ArchiveReader could not be created on failure
    using CreateArchiveReaderResult = AZStd::expected<AZStd::unique_ptr<IArchiveReader>, ResultString>;
    CreateArchiveReaderResult CreateArchiveReader();
    CreateArchiveReaderResult CreateArchiveReader(const ArchiveReaderSettings& readerSettings);

    CreateArchiveReaderResult CreateArchiveReader(AZ::IO::PathView archivePath,
        const ArchiveReaderSettings& readerSettings = {});
    CreateArchiveReaderResult CreateArchiveReader(IArchiveReader::ArchiveStreamPtr archiveStream,
        const ArchiveReaderSettings& readerSettings = {});
} // namespace Archive

// Implementation for any struct functions
#include "ArchiveReaderAPI.inl"

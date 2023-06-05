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
#include <AzCore/std/functional.h>

#include <Archive/Clients/ArchiveBaseAPI.h>
#include <Archive/Clients/ArchiveInterfaceStructs.h>

#include <Compression/CompressionInterfaceStructs.h>
#include <Compression/CompressionInterfaceAPI.h>

namespace AZ::IO
{
    class GenericStream;
}

namespace Archive
{
    //! ErrorCode structure which is used to indicate errors
    //! when writing to an archive
    //! The value of 0 is reserved to indicate no error
    enum class ArchiveWriterErrorCode
    {
        ErrorOpeningArchive = 1,
        ErrorReadingHeader,
        ErrorReadingTableOfContents,
        ErrorWritingTableOfContents,
    };
    using ArchiveWriterErrorString = AZStd::fixed_string<512>;

    //! Wraps an error code enum and a string containing an error message
    //! when performing archive writer operations
    struct ArchiveWriterError
    {
        explicit operator bool() const;

        ArchiveWriterErrorCode m_errorCode{};
        ArchiveWriterErrorString m_errorMessage;
    };

    //! Stores settings to configure how Archive Writer Settings
    struct ArchiveWriterSettings
    {
        ArchiveWriterSettings();

        //! Optional Compression Algorithm to use when writing the Archive TOC to the archive
        //! If the optional is not engaged, then the compression algorithm stored in the
        //! `ArchiveHeader::m_tocCompressionAlgoIndex` field is used instead
        //!
        //! If the compression algorithm isn't registered with the CompressionRegistrar
        //! or if the compression algorithm cannot be added to the Archive Header compression algorithm array
        //! due to it being full, then the TOC will be written as uncompressed
        AZStd::optional<Compression::CompressionAlgorithmId> m_tocCompressionAlgorithm;

        //! Callback which is invoked by the ArchiveWriter to inform users of errors that occurs
        //! This is used in by functions that can't return an error outcome such as constructors
        using ErrorCallback = AZStd::function<void(const ArchiveWriterError&)>;
        ErrorCallback m_errorCallback = [](const ArchiveWriterError&) {};

        //! Configures the maximum number of compression task that can run in parallel
        //! If the value is 0, then a single compression task that will be run
        //! at a given moment
        AZ::u32 m_maxCompressTasks{ AZStd::thread::hardware_concurrency() };
    };

    enum class ArchiveWriterFileMode : bool
    {
        AddNew,
        AddNewOrUpdateExisting,
    };

    enum class ArchiveFilePathCase : AZ::u8
    {
        // Lowercase the file path when adding to the Archive
        Lowercase,
        // Uppercase the file path when adding to the Archive
        Uppercase,
        // Maintain the current file path case when adding to the Archive
        Keep
    };

    //! Settings for controlling how an individual file is added to an archive.
    //! It supports specification of the compression algorithm,
    //! the relative path it should be in the archive file located at within the archive,
    //! whether to allow updating an existing archive file at the same path, etc...
    //! NOTE: The relative file path will be lowercased by default based on the
    //! ArchiveFileCase enum
    //! This due to the Archiving System supporting both case-preserving(Windows, MacOS)
    //! and case-sensitive systems such as Linux
    struct ArchiveWriterFileSettings
    {
        AZ::IO::PathView m_relativeFilePath;
        Compression::CompressionAlgorithmId m_compressionAlgorithm{ Compression::Uncompressed };
        ArchiveWriterFileMode m_fileMode{ ArchiveWriterFileMode::AddNew };
        ArchiveFilePathCase m_fileCase{ ArchiveFilePathCase::Lowercase };
        //! Pointer to a compression options derived struct
        //! This can be used to supply custom compression options
        //! to the compressor the Archive Writer users
        const Compression::CompressionOptions* m_compressionOptions{};
    };


    //! Returns result data around operation of adding a stream of content data
    //! to an archive file
    struct ArchiveAddFileResult
    {
        //! returns if adding a stream of data to a file within the archive has succeeded
        //! it does by checking that the ArchiveFileToken != InvalidArchiveFileToken
        explicit operator bool() const;

        //! File path of the added file
        //! NOTE: This is the file path as added to the Archive FilePath Blob Table
        //! It will be different than the file path specified in
        //! `ArchiveWriterFileSettings::m_relativeFilePath` if the
        //! `ArchiveWriterFileSettings::m_fileCase` options causes the file path case to change
        AZ::IO::Path m_relativeFilePath;
        //! Token that can be used to query or remove the file added file from the mounted Archive
        //! This is only valid for the specific ArchiveWriter instance
        ArchiveFileToken m_filePathToken{ InvalidArchiveFileToken };
        //! Compression Algorithm ID that was used to compress the added file
        //! NOTE: This will be different that the `ArchiveWriterFileSettings::m_compressionAlgorithm`
        //! if the the compression algorithm is not registered or the CompressionRegistrar
        //! is not available.
        //! In that case, the file will be stored uncompressed
        Compression::CompressionAlgorithmId m_compressionAlgorithm{ Compression::Uncompressed };
        //! Stores any error messages that occur when adding the file from the archive
        ResultOutcome m_resultOutcome;
    };

    //! Returns a result structure that indicates if removal of a content file from the
    //! archive was successful
    //! Metadata about the file is returned, such as its file path, compressed algorithm ID
    //! offset from the beginning of the raw file data blocks, uncompressed size and compressed size
    struct ArchiveRemoveFileResult
    {
        //! returns if adding a stream of data to a file within the archive has succeeded
        //! it does by checking that the ArchiveFileToken != InvalidArchiveFileToken
        explicit operator bool() const;

        //! File path of the removed file
        AZ::IO::Path m_relativeFilePath;
        //! Compression algorithm ID that that file was compressed or Compression::Uncompressed if not
        Compression::CompressionAlgorithmId m_compressionAlgorithm{ Compression::Uncompressed };
        //! The uncompressed size of the removed file
        AZ::u64 m_uncompressedSize{};
        //! the compressed size of the removed file
        //! INFO: This value will be a multiple of 512
        AZ::u64 m_compressedSize{};
        //! The raw offset of the file in the ArchiveFile from the beginning of the raw file data block
        //! As the ArchiveHeader is 512-byte aligned to the beginning of the file
        //! this value is at least 512,
        //! NOTE: The TocOffsetU64 structure is used to enforce that the value is >= 512
        ArchiveHeader::TocOffsetU64 m_offset{};
        //! Stores any error messages that occur when removing the file from the archive
        ResultOutcome m_resultOutcome;
    };


    //! Interface for the ArchiveWriter of O3DE Archive format
    //! The caller is required to supply a ArchiveWriterSettings structure instance
    //! which contains the ArchiveHeader and ArchiveTableOfContents data
    //! to use when writing to the Archive file
    //! The class can be initialized with a user supplied AZ::IO::GenericStream class
    //! in which case the stream needs to be open with OpenMode::ModeUpdate
    //! The reason why is that to locate information about any content files in order to update an existing archive
    //! it read access is needed
    //! The recommend OpenMode value for opening a new archive or updating an existing archive
    //! are as follows
    //! constexpr OpenMode mode = OpenMode::Update | OpenMode::Append | OpenMode::Binary
    //! The Append option makes sure that the Archive is not truncated on open
    class IArchiveWriter
    {
    public:
        AZ_TYPE_INFO_WITH_NAME_DECL(IArchiveWriter);
        AZ_RTTI_NO_TYPE_INFO_DECL();
        AZ_CLASS_ALLOCATOR_DECL;

        //! Unique pointer which wraps a stream
        //! where Archive data will be written.
        //! The stream can be owned by the ArchiveWriter depending
        //! on the m_delete value
        struct ArchiveStreamDeleter
        {
            ArchiveStreamDeleter();
            ArchiveStreamDeleter(bool shouldDelete);
            void operator()(AZ::IO::GenericStream* ptr) const;
            bool m_delete{ true };
        };
        using ArchiveStreamPtr = AZStd::unique_ptr<AZ::IO::GenericStream, ArchiveStreamDeleter>;

        virtual ~IArchiveWriter();

        //! Opens the archive path and returns true if successful
        //! Will unmount any previously mounted archive
        virtual bool MountArchive(AZ::IO::PathView archivePath) = 0;
        virtual bool MountArchive(ArchiveStreamPtr archiveStream) = 0;

        //! Closes the handle to the mounted archive stream
        //! This will invoke the Commit() function to write the archive TOC
        //! to the stream before closing the stream
        virtual void UnmountArchive() = 0;

        //! Returns if an open archive that is mounted
        virtual bool IsMounted() const = 0;

        //! Write the updated ArchiveHeader to the beginning of the stream and
        //! Table of Contents to end of the stream
        //!
        //! If this call is successful, the archive TOC has been successfully written
        //! This function has been marked [[nodiscard]], to ensure the caller
        //! checks the return value
        //! @return A successful expectation if the TOC has been written
        using CommitResult = AZStd::expected<void, ResultString>;
        [[nodiscard]] virtual CommitResult Commit() = 0;

        //! Adds the content from the stream to the relative path
        //! based on the ArchiveWriterFileSettings
        virtual ArchiveAddFileResult AddFileToArchive(AZ::IO::GenericStream& inputStream,
            const ArchiveWriterFileSettings& fileSettings = {}) = 0;

        //! The span contents is used to supply file data for the file to the archive
        virtual ArchiveAddFileResult AddFileToArchive(AZStd::span<const AZStd::byte> inputSpan,
            const ArchiveWriterFileSettings& fileSettings = {}) = 0;

        //! Searches for a relative path within the archive
        //! @param relativePath Relative path within archive to search for
        //! @return A token that identifies the Archive file if it exist
        //! if the with the specified path doesn't exist InvalidArchiveFileToken is returned
        virtual ArchiveFileToken FindFile(AZ::IO::PathView relativePath) const = 0;
        //! Returns if the archive contains a relative path
        //! @param relativePath Relative path within archive to search for
        //! @returns true if the relative path is contained with the Archive
        //! equivalent to `return FindFile(relativePath) != InvalidArchiveFileToken;`
        virtual bool ContainsFile(AZ::IO::PathView relativePath) const = 0;

        //! Removes the file from the archive using the ArchiveFileToken
        //! @param filePathToken Relative path within archive to search for
        //! NOTE: The entry in the table of contents is not actually removed
        //! The index where the file is located using the filePathToken
        //! is just added to the removed file indices set
        //! //! @return ArchiveRemoveResult with metadata about how the deleted file was
        //! stored in the Archive
        virtual ArchiveRemoveFileResult RemoveFileFromArchive(ArchiveFileToken filePathToken) = 0;
        //! Removes the file from the archive using a relative path name
        //! @param relativePath Relative path within archive to search for
        //! //! @return ArchiveRemoveResult with metadata about how the deleted file was
        //! stored in the Archive
        virtual ArchiveRemoveFileResult RemoveFileFromArchive(AZ::IO::PathView relativePath) = 0;

        //! Writes metadata for the archive to the supplied generic stream
        //! @param metadataStream with human readable data about files within the archive will be written to the stream
        //! @return true if metadata was successfully written
        virtual bool WriteArchiveMetadata(AZ::IO::GenericStream& metadataStream,
            const ArchiveMetadataSettings& metadataSettings = {}) const = 0;
    };

    //! Creates an instance of the Concrete ArchiveWriter class
    //! The parameters are forwarded to the ArchiveWriter constructor and used to initialize the instance
    AZStd::unique_ptr<IArchiveWriter> CreateArchiveWriter();
    AZStd::unique_ptr<IArchiveWriter> CreateArchiveWriter(const ArchiveWriterSettings& writerSettings);

    AZStd::unique_ptr<IArchiveWriter> CreateArchiveWriter(AZ::IO::PathView archivePath,
        const ArchiveWriterSettings& writerSettings = {});
    AZStd::unique_ptr<IArchiveWriter> CreateArchiveWriter(IArchiveWriter::ArchiveStreamPtr archiveStream,
        const ArchiveWriterSettings& writerSettings = {});

} // namespace Archive

// Implementation for any struct functions
#include "ArchiveWriterAPI.inl"

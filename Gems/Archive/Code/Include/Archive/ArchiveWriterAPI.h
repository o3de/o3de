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

#include <Archive/ArchiveInterfaceStructs.h>

#include <Compression/CompressionInterfaceStructs.h>
#include <Compression/CompressionInterfaceAPI.h>

namespace Archive
{
    //! Stores settings to configure how Archive Writer Settings
    struct ArchiveWriterSettings
    {
        ArchiveWriterSettings() = default;
    };

    //! Specifies settings to use when retrieving the metadata
    //! about files within the archive
    struct ArchiveWriterMetadataSettings
    {
        //! Output total file count
        bool m_writeFileCount{ true };
        //! Outputs the relative file paths
        bool m_writeFilepaths{ true };
        //! Outputs the offsets of files within the archive
        bool m_writeFileOffsets{ true };
        //! Outputs the sizes of file as they are stored inside of an archive
        //! as well as the compression algorithm used for files
        //! This will include both uncompressed and compressed sizes
        bool m_writeFileSizesAndCompression{ true };
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
        AZ::IO::Path m_relativeFilePath;
        Compression::CompressionAlgorithmId m_compressionAlgorithm{ Compression::Uncompressed };
        ArchiveWriterFileMode m_fileMode{ ArchiveWriterFileMode::AddNew };
        ArchiveFilePathCase m_fileCase{ ArchiveFilePathCase::Lowercase };
        //! Pointer to a compression options derived struct
        //! This can be used to supply custom compression options
        //! to the compressor the Archive Writer users
        const Compression::CompressionOptions* m_compressionOptions{};
    };

    //! Stores outcome detailing result of operation
    using ResultString = AZStd::fixed_string<512>;
    using ResultOutcome = AZStd::expected<void, ResultString>;

    //! Returns result data around operation of adding a stream of content data
    //! to an archive file
    struct ArchiveAddToFileResult
    {
        //! returns if adding a stream of data to a file within the archive has succeeded
        //! it does by checking that the ArchiveFileToken != InvalidArchiveFileToken
        explicit operator bool() const;

        AZ::IO::PathView m_relativeFilePath;
        ArchiveFileToken m_filePathToken{ InvalidArchiveFileToken };
        Compression::CompressionAlgorithmId m_compressionAlgorithm{ Compression::Uncompressed };
        ResultOutcome m_resultOutcome;
    };

    //! Stores offset information about the Files added to the Archive for write
    struct ArchiveWriterFileMetadata
    {
        AZ::IO::Path m_relativeFilePath;
        Compression::CompressionAlgorithmId m_compressionAlgorithm{ Compression::Uncompressed };
    };

} // namespace Archive

// Implementation for any struct functions
#include "ArchiveWriterAPI.inl"

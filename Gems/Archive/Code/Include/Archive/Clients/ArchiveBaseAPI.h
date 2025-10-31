/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/string/fixed_string.h>
#include <AzCore/std/utility/expected.h>

#include <Compression/CompressionInterfaceStructs.h>

namespace Archive
{
    struct ArchiveHeader;

    //! Token that can be used to identify a file within an archive
    enum class ArchiveFileToken : AZ::u64 {};

    static constexpr ArchiveFileToken InvalidArchiveFileToken = static_cast<ArchiveFileToken>(AZStd::numeric_limits<AZ::u64>::max());

    //! Specifies settings to use when retrieving the metadata
    //! about files within the archive
    struct ArchiveMetadataSettings
    {
        //! Output total file count
        bool m_writeFileCount{ true };
        //! Outputs the relative file paths
        bool m_writeFilePaths{ true };
        //! Outputs the offsets of files within the archive
        //! m_writeFilePaths must be true for offsets to be written
        //! otherwise there would be no file path associated with the offset values
        bool m_writeFileOffsets{ true };
        //! Outputs the sizes of file as they are stored inside of an archive
        //! as well as the compression algorithm used for files
        //! This will include both uncompressed and compressed sizes
        //! m_writeFilePaths must be true for offsets to be written
        //! otherwise there would be no file path associated with the offset values
        bool m_writeFileSizesAndCompression{ true };
    };

    //! Stores outcome detailing result of operation
    using ResultString = AZStd::fixed_string<512>;
    using ResultOutcome = AZStd::expected<void, ResultString>;

    //! Updates the ArchiveHeader structure with compression algorithm ID if
    //! there is space in the ArchiveHeader compression algorithm ID array
    //! @param compressionAlgorithmId compression algorithm ID to add to the archive header
    //! @param archiveHeader modifiable archive header reference to update
    //! with compression algorithm ID if a slot is available
    //! @return true if the specified compression algorithm ID was added
    //! to the ArchiveHeader
    constexpr bool AddCompressionAlgorithmId(Compression::CompressionAlgorithmId compressionAlgorithmId,
        ArchiveHeader& archiveHeader);

    //! Queries the ArchiveHeader for the specified compression algorithm ID and return
    //! the index within that compression algorithm ID if found
    //! @param compressionAlgorithmId compression algorithm ID to search for
    //! @param archiveHeader archive header to search within for compression algorithm ID
    //! @return the index of the compression algorithm ID within the ArchiveHeader
    //! If compression algorithm ID cannot be found the InvalidAlgorithmIndex value is returned
    //! If the supplied compression algorithm ID is the special `Uncompressed` algorithm ID,
    //! then UncompressedAlgorithmIndex is returned
    constexpr size_t FindCompressionAlgorithmId(Compression::CompressionAlgorithmId compressionAlgorithmId,
        const ArchiveHeader& archiveHeader);
} // namespace Archive

#include "ArchiveBaseAPI.inl"

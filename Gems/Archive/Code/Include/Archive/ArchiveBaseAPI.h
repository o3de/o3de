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

#include <Compression/CompressionInterfaceStructs.h>

namespace Archive
{
    //! Token that can be used to identify a file within an archive
    enum class ArchiveFileToken : AZ::u64 {};

    static constexpr ArchiveFileToken InvalidArchiveFileToken = static_cast<ArchiveFileToken>(AZ::u64(0));


    // ! Returns metadata about an archived file
    struct ArchiveFileMetadata
    {
        //! Relative File path which represents the file in the Archive
        AZ::IO::PathView m_filePath{};
        //! Offset to the first block of the Archive File on disk
        //! If the compression algorithm = Uncompressed
        //! this represents a single contiguous block of file data
        AZ::u64 m_offset{};
        //! Uncompressed size of the file
        AZ::u64 m_uncompressedSize{};
        //! The size of the compressed file
        //! Note: This will be 0 if the compression algorithm = Uncompressed
        AZ::u64 m_compressedSize{};
        //! The compression algorithm used to compress this file
        //! the archive
        Compression::CompressionAlgorithmId m_compressionAlgorithm{ Compression::Uncompressed };
    };
} // namespace Archive

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace Archive
{
    // Implement byte storage multipliers
    inline namespace literals
    {
        constexpr AZ::u64 operator"" _kib(AZ::u64 value)
        {
            return value * (1 << 10);
        }

        constexpr AZ::u64 operator"" _mib(AZ::u64 value)
        {
            return value * (1 << 20);
        }

        constexpr AZ::u64 operator"" _gib(AZ::u64 value)
        {
            return value * (1 << 30);
        }
    }

    // ArchiveHeader constructor implementation
    ArchiveHeaderSection::ArchiveHeaderSection()
        : m_tocCompressedSize{}
        , m_tocCompressionAlgoIndex{ Compression::UncompressedAlgorithmIndex }
    {}

    // ArchiveFileMetadata constructor implementation
    ArchiveFileMetadataSection::ArchiveFileMetadataSection()
        : m_uncompressedSize{}
        , m_compressedSizeInSectors{}
        , m_compressionAlgoIndex{ Compression::UncompressedAlgorithmIndex }
        , m_blockTableIndex{}
        , m_offset{}
    {}

    // ArchiveFilePathIndexSection constructor implementation
    ArchiveFilePathIndexSection::ArchiveFilePathIndexSection() = default;

    // The ArchiveBlockLine constructor zero initializes each block line
    // Block lines are made up of 3 blocks at a time
    ArchiveBlockLine::ArchiveBlockLine()
        : m_block1{}
        , m_block2{}
        , m_block3{}
        , m_blockUsed{ 1 }
    {}

    // ArchiveBlockLineJump constructor
    ArchiveBlockLineJump::ArchiveBlockLineJump()
        : m_blockJump{}
        , m_block1{}
        , m_block2{}
        , m_blockUsed{ 1 }
    {}

    //! ArchiveBlockLineSection constructor which stores both types of block lines
    //! Union which can store either a block line without a jump entry or block line with a jump entry
    ArchiveBlockLineSection::ArchiveBlockLineSection()
    {
    }
} // namespace Archive


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

    // ArchiveHeader::TocOffsetU64
    inline ArchiveHeader::TocOffsetU64::TocOffsetU64() = default;

    // Constrain the TocOffset to be 512 or greater
    inline ArchiveHeader::TocOffsetU64::TocOffsetU64(AZ::u64 offset)
        : m_value(AZStd::max(offset, ArchiveDefaultBlockAlignment))
    {
    }

    inline auto ArchiveHeader::TocOffsetU64::operator=(AZ::u64 offset) -> TocOffsetU64&
    {
        m_value = AZStd::max(offset, ArchiveDefaultBlockAlignment);
        return *this;
    }

    inline ArchiveHeader::TocOffsetU64::operator AZ::u64() const
    {
        return m_value;
    }

    // ArchiveHeader constructor implementation
    inline ArchiveHeader::ArchiveHeader()
        : m_tocCompressedSize{}
        , m_tocCompressionAlgoIndex{ UncompressedAlgorithmIndex }
    {}

    // Get the total uncompressed size of the Table of Contents
    inline AZ::u64 ArchiveHeader::GetUncompressedTocSize() const
    {
        // The first 8 bytes of the Archive TOC section is the magic byte sequence
        AZ::u64 uncompressedSize = AZ_SIZE_ALIGN_UP(sizeof(ArchiveTocMagicBytes), sizeof(ArchiveTocFileMetadata));
        // The uncompressed FileMetadataTable is always a multiple
        // of sizeof(ArchiveTocFileMetadata) which is 16
        uncompressedSize += m_tocFileMetadataTableUncompressedSize;
        // The uncompressed FilePathIndex is always a multiple
        // of sizeof(ArchiveTocFilePathIndex) which is 8
        uncompressedSize += m_tocPathIndexTableUncompressedSize;
        // The uncompressed file path blob section
        // is the exact size of the blob section
        uncompressedSize += m_tocPathBlobUncompressedSize;

        // The BlockOffset table starts on 8-byte alignment
        // so aligned up the the current uncompressed size upwards
        // to the next multiple of 8
        uncompressedSize = AZ_SIZE_ALIGN_UP(uncompressedSize, 8);

        // As the block offset table is the last section of the
        // table of contents, no alignment constraints need to be accounted for
        // To close out the information loop however, each block offset table
        // entry stores a 8-byte integer which encodes either 3 2-MiB compressed block sizes
        // or a 16-bit block jump offset entry and 2 2-MiB compressed block sizes(21-bits each)
        uncompressedSize += m_tocBlockOffsetTableUncompressedSize;

        return uncompressedSize;
    }

    inline AZ::u64 ArchiveHeader::GetTocStoredSize() const
    {
        return m_tocCompressionAlgoIndex < UncompressedAlgorithmIndex
            ? m_tocCompressedSize : GetUncompressedTocSize();
    }

    constexpr ArchiveHeaderValidationResult::operator bool() const
    {
        return m_errorCode == ArchiveHeaderErrorCode{};
    }

    // Validates Archive Header by checking the magic bytes
    inline ArchiveHeaderValidationResult ValidateHeader(const ArchiveHeader& archiveHeader)
    {
        ArchiveHeaderValidationResult result{};
        if (archiveHeader.m_magicBytes != ArchiveHeaderMagicBytes)
        {
            result.m_errorCode = ArchiveHeaderErrorCode::InvalidMagicBytes;
            result.m_errorMessage = ArchiveHeaderValidationResult::ErrorString::format(
                "Archive header has invalid magic byte sequence %x",
                archiveHeader.m_magicBytes);
        }
        return result;
    }

    // ArchiveFileMetadata constructor implementation
    inline ArchiveTocFileMetadata::ArchiveTocFileMetadata()
        : m_uncompressedSize{}
        , m_compressedSizeInSectors{}
        , m_compressionAlgoIndex{ UncompressedAlgorithmIndex }
        , m_blockLineTableFirstIndex{}
        , m_offset{}
    {}

    // ArchiveTocFilePathIndex constructor implementation
    inline ArchiveTocFilePathIndex::ArchiveTocFilePathIndex()
        : m_size{}
        , m_unused{}
        , m_offset{}
    {

    }

    // The ArchiveBlockLine constructor zero initializes each block line
    // Block lines are made up of 3 blocks at a time
    inline ArchiveBlockLine::ArchiveBlockLine()
        : m_block1{}
        , m_block2{}
        , m_block3{}
        , m_blockUsed{ 1 }
    {}

    // ArchiveBlockLineJump constructor
    inline ArchiveBlockLineJump::ArchiveBlockLineJump()
        : m_blockJump{}
        , m_block1{}
        , m_block2{}
        , m_blockUsed{ 1 }
    {}

    //! ArchiveBlockLineUnion constructor which stores both types of block lines
    //! Union which can store either a block line without a jump entry or block line with a jump entry
    inline ArchiveBlockLineUnion::ArchiveBlockLineUnion()
    {
    }

    //! Calculates the block count for a file if it were to be compressed
    //! by dividing its uncompressed size by the archive block size(2 MiB)
    //! and rounding up to a multiple of the archive block size
    constexpr AZ::u32 GetBlockCountIfCompressed(AZ::u64 uncompressedSize)
    {
        auto DivideCeiling = [](AZ::u64 value, AZ::u64 divisor) constexpr
        {
            return (value + divisor - 1) / divisor;
        };
        return static_cast<AZ::u32>(DivideCeiling(uncompressedSize,  ArchiveBlockSizeForCompression));
    }

    static_assert(GetBlockCountIfCompressed(0) == 0, "Empty file should have 0 blocks");
    static_assert(GetBlockCountIfCompressed(1) == 1, "File with at least 1 byte, requires 1 block of storage");
    static_assert(GetBlockCountIfCompressed(ArchiveBlockSizeForCompression) == 1, "File that exactly matches the ArchiveBlockSizeForCompression"
        " of 2 MiB should fit within 1 block");
    static_assert(GetBlockCountIfCompressed(ArchiveBlockSizeForCompression + 1) == 2, "File that is one byte above the ArchiveBlockSizeForCompression"
        " of 2 MiB requires 2 blocks");

    //! Calculate the Block Lines using a piecewise function
    //! that takes into account any jump entries being used in the block offset table
    //! for each set of 3 block lines when the remaining file size > 18 MiB
    constexpr AZ::u32 GetBlockLineCountIfCompressed(AZ::u64 uncompressedSize)
    {
        // The number of blocks a file contains is based on how many 2 MiB chunks can be extracted
        // using it's uncompressed size
        // If the remaining file size > 18 MiB, then the next 3 block lines encodes
        // a single block line entry using 15-bits followed 2 compressed block sizes stored 21-bits each
        // followed by two more block line entries with 3 compressed block sizes each stored in 21-bits
        // i.e the data looks as follows
        // Block Line 0
        //     Jump Offset: 16
        //     Compressed Block 0 Size: 21
        //     Compressed Block 1 Size: 21
        // Block Line 1
        //     Compressed Block 2 Size: 21
        //     Compressed Block 3 Size: 21
        //     Compressed Block 4 Size: 21
        // Block Line 2
        //     Compressed Block 5 Size: 21
        //     Compressed Block 6 Size: 21
        //     Compressed Block 7 Size: 21
        //
        // Otherwise if remaining file size < 18 MiB, then the next 3 block lines encodes the compressed
        // sizes of 9 2 MiB blocks
        // Block Line 0
        //     Compressed Block 0 Size: 21
        //     Compressed Block 1 Size: 21
        //     Compressed Block 2 Size: 21
        // Block Line 1
        //     Compressed Block 3 Size: 21
        //     Compressed Block 4 Size: 21
        //     Compressed Block 5 Size: 21
        // Block Line 2
        //     Compressed Block 6 Size: 21
        //     Compressed Block 7 Size: 21
        //     Compressed Block 8 Size: 21
        // NOTE: Each block line is an uint64_t

        // The Piecewise function is defined as follows, where 'x' is represents the size value
        // f(x) = 0, if x = 0
        // f(x) = ceil((x + 0 * 2 MiB) / (2 MiB * 3)), if x > 0 MiB x <= 18 MiB
        // f(x) = ceil((x + 0 * 2 MiB) / (2 MiB * 3)), if x > 18 MiB and x <= 22 MiB
        // f(x) = ceil((x + 1 * 2 MiB) / (2 MiB * 3)), if x > 22 MiB and x <= 34 MiB
        // f(x) = ceil((x + 1 * 2 MiB) / (2 MiB * 3)), if x > 34 MiB and x <= 38 MiB
        // f(x) = ceil((x + 2 * 2 MiB) / (2 MiB * 3)), if x > 38 MiB and x <= 50 MiB
        // f(x) = ceil((x + 2 * 2 MiB) / (2 MiB * 3)), if x > 50 MiB and x <= 54 MiB
        //
        // Based on the cases above, mathematical induction can be used to come up with a formula that works for all cases
        // First several of the piecewise functions segments can be combined
        // f(x) = 0, if x = 0
        // f(x) = ceil((x + 0 * 2 MiB) / (2 MiB * 3)), if x > 0 MiB and x <= 22 MiB
        // f(x) = ceil((x + 1 * 2 MiB) / (2 MiB * 3)), if x > 22 MiB and x <= 38 MiB (+16)
        // f(x) = ceil((x + 2 * 2 MiB) / (2 MiB * 3)), if x > 50 MiB and x <= 54 MiB (+16)
        //
        // It can be seen that there is a correlation the multiplier to 2 MiB that gets added to x `(x + N * 2 MiB)`
        // and the range of the piecewise function values (0, 22] + N * 16 MiB, based on some value 'N'
        // Therefore the piecewise function can further be reduced to
        // f(x) = 0, if x = 0
        // f(x) = ceil((x + N * 2 MiB) / (2 MiB * 3)), if x is between `(0, 22] + N * 16` MiB
        //
        // The final part is then figuring out what 'N' represents
        // N is the number of 16 MiB s after the first 22 MiB
        // The first 22 MiB is the amount of 2 MiB chunks that can be stored in the
        // first 4 block lines when one of the block offsets represents a jump table entry
        // i.e 12 block offset slots - 1 jump slot = 11 block offset slots.
        // The 11 block offset slots can store the compressed size of the first 2 MiB * 11 = 22 MiB of a file
        // Mathematically N is then equivalent to taking the uncompressed size('x') substracting 22 MiB,
        // clamping the minimum value to be 0
        // Next that value is then divided by 16 MiB and rounded towards infinity
        // N = ceil((x - 22 MiB) / 16 MiB)
        //
        // So the piecewise function, now can be reduced as follows
        // f(x) = 0, if x = 0
        // f(x) = ceil((x + ceil(max(x - 22 MiB, 0) / 16 MiB) * 2 MiB) / (2 MiB * 3)), if x > 0

        auto DivideCeiling = [](AZ::u64 value, AZ::u64 divisor) constexpr
        {
            return (value + divisor - 1) / divisor;
        };
        const AZ::u64 uncompressed16MiBChunksAfterFirst4BlockLines = uncompressedSize > MaxFileSizeForMinBlockLinesWithJumpEntry
            ? DivideCeiling(uncompressedSize - MaxFileSizeForMinBlockLinesWithJumpEntry, FileSizeToSkipWithJumpEntry)
            : 0;
        const AZ::u64 blockLineCount = DivideCeiling(
            uncompressedSize + uncompressed16MiBChunksAfterFirst4BlockLines * ArchiveBlockSizeForCompression, MaxBlockLineSize);
        return static_cast<AZ::u32>(blockLineCount);
    }

    static_assert(GetBlockLineCountIfCompressed(0) == 0, "0 MiB file should have no block lines");
    static_assert(GetBlockLineCountIfCompressed(1) == 1, "1 byte file if compressed should have its compressed sizes fit in 1 block line");
    static_assert(GetBlockLineCountIfCompressed(MaxBlockLineSize) == 1,
        "6 MiB file if compressed should have its compressed sizes fit in 1 block line");
    static_assert(GetBlockLineCountIfCompressed(MaxBlockLineSize + 1) == 2,
        "6 MiB + 1 byte file if compressed should have its compressed sizes fit in 2 block lines");
    static_assert(GetBlockLineCountIfCompressed(2 * MaxBlockLineSize) == 2,
        "12 MiB file if compressed should have its compressed sizes fit in 2 block lines");
    static_assert(GetBlockLineCountIfCompressed((2 * MaxBlockLineSize) + 1) == 3,
        "12 MiB + 1 byte file if compressed should have its compressed sizes fit in 3 block lines");
    static_assert(GetBlockLineCountIfCompressed(MaxRemainingFileSizeNoJumpEntry) == 3,
        "18 MiB file if compressed should have its compressed sizes fit in 3 block lines");
    static_assert(GetBlockLineCountIfCompressed(MaxRemainingFileSizeNoJumpEntry + 1) == 4,
        "18 MiB + 1 byte file if compressed should have its compressed sizes fit in 4 block lines");
    static_assert(GetBlockLineCountIfCompressed(MaxFileSizeForMinBlockLinesWithJumpEntry) == 4,
        "22 MiB file if compressed should have its compressed sizes fit in 4 block lines");
    static_assert(GetBlockLineCountIfCompressed(MaxFileSizeForMinBlockLinesWithJumpEntry + 1) == 5,
        "22 MiB + 1 byte file if compressed should have its compressed sizes fit in 5 block lines");
    static_assert(GetBlockLineCountIfCompressed(MaxFileSizeForMinBlockLinesWithJumpEntry + MaxBlockLineSize) == 5,
        "28 MiB file if compressed should have its compressed sizes fit in 5 block lines");
    static_assert(GetBlockLineCountIfCompressed(MaxFileSizeForMinBlockLinesWithJumpEntry + MaxBlockLineSize + 1) == 6,
        "28 MiB + 1 byte file if compressed should have its compressed sizes fit in 6 block lines");
    static_assert(GetBlockLineCountIfCompressed(MaxFileSizeForMinBlockLinesWithJumpEntry + (2 * MaxBlockLineSize)) == 6,
        "34 MiB file if compressed should have its compressed sizes fit in 6 block lines");
} // namespace Archive


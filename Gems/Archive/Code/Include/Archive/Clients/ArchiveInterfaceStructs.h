/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>

#include <AzCore/Math/Crc.h>
#include <AzCore/IO/Path/Path_fwd.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/std/limits.h>
#include <AzCore/std/utility/expected.h>

#include <Compression/CompressionInterfaceStructs.h>

namespace Archive
{
    inline namespace literals
    {
        constexpr AZ::u64 operator"" _kib(AZ::u64 value);
        constexpr AZ::u64 operator"" _mib(AZ::u64 value);
        constexpr AZ::u64 operator"" _gib(AZ::u64 value);
    }

    // tag index that indicates the archived content being examined is uncompressed
    // It is set to the maximum value that can be stored in 3-bits = 7
    constexpr AZ::u8 UncompressedAlgorithmIndex = 0b111;

    // index which is returned when the compression algorithm Id is not registered
    // with the Archive header
    constexpr size_t InvalidAlgorithmIndex = AZStd::numeric_limits<size_t>::max();

    //! Represents the default block size for the Archive format
    //! It will be 2 MiB until more data is available that proves
    //! that a different block size is more ideal
    constexpr AZ::u64 ArchiveBlockSizeForCompression = 2 * (1 << 20);
    //! The alignment of blocks within an archive file
    //! It defaults to 512 bytes
    constexpr AZ::u64 ArchiveDefaultBlockAlignment = 512;

    static_assert((ArchiveBlockSizeForCompression % ArchiveDefaultBlockAlignment) == 0, "ArchiveBlockSizeForCompression"
        " should be aligned to ArchiveDefaultBlockAlignment");

    //! Sentinel which indicates the value written to the last block to indicate
    //! there are no further deleted blocks afterwards
    constexpr AZ::u64 DeletedBlockOffsetSentinel = AZStd::numeric_limits<AZ::u64>::max();

    //! O3DE only runs on little endian machines
    //! Therefore the bytes are added in little endian order
    //! The Magic Identifier for the archive format is "O3AR" for O3DE Archive
    constexpr AZ::u32 ArchiveHeaderMagicBytes = { 'O' | ('3' << 8) | ('A' << 16) | ('R' << 24) };
    constexpr AZ::u64 ArchiveTocMagicBytes = ArchiveHeaderMagicBytes;

    //! Fixed size Header struct for the Archive format
    //! This suitable for directly reading the archive header into
    struct ArchiveHeader
    {
        ArchiveHeader();

        //! An actual implementation of the ArchiveHeader copy constructor
        //! is required as the const AZ::u32 m_magicBytes member
        //! causes the copy constructor to be implicitly deleted
        ArchiveHeader(const ArchiveHeader& other);
        ArchiveHeader& operator=(const ArchiveHeader& other);

        //! Retrieves the Uncompressed Table of Contents(TOC) size
        //! by adding up the size of the TOC File Metadata table
        //! + TOC File Path Index table
        //! + TOC File Path Blob table
        //! + TOC Block Offset table
        AZ::u64 GetUncompressedTocSize() const;

        //! If on the Compression algorithm the TOC is using a compression algorithm
        //! index that is < UncompressedAlgorithmIndex
        //! then the compressed size of the toc is returned
        //! otherwise the uncompressed size of the toc is returned
        AZ::u64 GetTocStoredSize() const;

        //! The Magic Bytes used to identify the archive as being in the O3DE Archive format
        //! offset = 0
        const AZ::u32 m_magicBytes{ ArchiveHeaderMagicBytes };

        //! Version number of archive format. Supports up to 2^16 revisions per entry
        //! offset = 4
        AZ::u16 m_minorVersion{};
        AZ::u16 m_majorVersion{};
        AZ::u16 m_revision{};

        //! reserved for future memory configurations
        //! default layout is 2Mib blocks with 512 byte borders
        //! offset = 10
        AZ::u16 m_layout{};

        //! Represents the number of files stored within the Archive
        //! Caps out at (2^25) or ~33 million files that can be represented
        //! offset = 12
        AZ::u32 m_fileCount{};

        //! Wraps table of contents member
        //! to constrain the values of the table of contents offset
        //! The table of contents offset cannot be < 512
        //! as the archive header is reserved to the first 512 bytes of the archive file
        struct TocOffsetU64
        {
            //! default constructs the TocOffset with an offset
            //! equal to ArchiveDefaultBlockAlignment(or 512)
            TocOffsetU64();

            //! Implicit conversion constructor to store AZ::u64
            TocOffsetU64(AZ::u64 offset);

            //! assignment to store AZ::u64
            TocOffsetU64& operator=(AZ::u64 offset);

            //! implicit u64 operator that allows
            //! assignment of the TOC offset to a 64-bite int
            operator AZ::u64() const;
        private:
            AZ::u64 m_value{ ArchiveDefaultBlockAlignment };
        };

        //! The 64-bit offset from the start of the Archive File to the Table of Contents
        //! The table of contents offset also doubles as the offset to write new blocks of content
        //! that does not exist in any deleted blocks
        //! offset = 16
        TocOffsetU64 m_tocOffset{};

        //! Compressed Size of the Table of Contents
        //! Max size is 512MiB or 2^29 bytes
        //! The TOC offset + TOC compressed size is equal to total size of the Archive file
        //! If the Compression Algorithm is set to Uncompressed
        //! Then this value is 0
        //! offset = 24
        AZ::u32 m_tocCompressedSize : 29;
        //! Compression algorithm used for the Table of Contents
        //! The maximum 3-bit value of 7 is reserved for uncompressed
        //! Other values count as a offset in the Compression Algorithm ID table
        AZ::u32 m_tocCompressionAlgoIndex : 3;

        //! Uncompressed size of the Table of Contents File Metadata table.
        //! offset = 28
        AZ::u32 m_tocFileMetadataTableUncompressedSize{};
        //! Uncompressed size of the Table of Contents File Path index
        //! The File Path index is used to lookup the location for a file path
        //! within the archive
        //! offset = 32
        AZ::u32 m_tocPathIndexTableUncompressedSize{};
        //! Uncompressed size of the Table of Contents File Path table
        //! It contains a blob of FilePaths without any null-termination
        //! The File Path Index entries are used to look up a file path
        //! through using the path offset + size entry
        //! offset = 36
        AZ::u32 m_tocPathBlobUncompressedSize{};
        //! Uncompressed size of the Table of Contents File Block offset table
        //! Contains compressed sizes of individual blocks of a file
        //! In Archive V1 layout the block size is 2MiB
        //! offset = 40
        AZ::u32 m_tocBlockOffsetTableUncompressedSize{};

        //! Threshold value represents the cap on the size a block after it has been
        //! sent through the compression step to determine if it should be stored compressed
        //!
        //! Due to block size defaulting to 2MiB, any blocks that are larger than 2_mib
        //! after compression will be stored uncompressed.
        //! So the maximum limit of this value is the Block Size
        //! offset = 44
        AZ::u32 m_compressionThreshold{ static_cast<AZ::u32>(ArchiveBlockSizeForCompression) };

        //! Stores 32-bit IDS of up to 7 compression algorithms that this archive can use
        //! Each entry is initialized to the Invalid CompressionAlgorithmId
        //! The capacity of the array is the value of the uncompressed algorithm index
        //! offset = 48
        AZStd::array<Compression::CompressionAlgorithmId, UncompressedAlgorithmIndex> m_compressionAlgorithmsIds =
            []() constexpr
        {
            AZStd::array<Compression::CompressionAlgorithmId, UncompressedAlgorithmIndex> compressionIdInitArray{};
            compressionIdInitArray.fill(Compression::Invalid);
            return compressionIdInitArray;
        }();

        //! Padding bytes added to ArchiveHeader
        //! to ensure byte offsets 76-79 contains bytes with a value '\0'
        //! This allows equivalent ArchiveHeader to be memcmp
        //! offset = 76
        const AZ::u32 m_padding{};

        //! Offset from the beginning of the file block section to the first
        //! deleted block.
        //! The first 8 bytes of each deleted block will contain the offset to the next deleted block
        //! or 0xffff'ffff'ffff'ffff if this is the last deleted block
        //! offset = 80 (aligned on 8 byte boundary)
        AZ::u64 m_firstDeletedBlockOffset{ DeletedBlockOffsetSentinel };

        //! total offset = 88

        //! Max FileCount
        //! Up to 2^32 files can be stored,
        //! but is limited to 2^25 because around 640MiB of uncompressed data will need to be loaded into memory for an archive of containing ~33million files,
        //! and memory requirements would increase if the not limited.
        //! offset = 12
        static constexpr AZ::u32 MaxFileCount{ (1 << 25) - 1 };
    };

    static_assert(sizeof(ArchiveHeader) == 88, "Archive Header section should be less than 88 bytes per spec version 1");
    static_assert(sizeof(ArchiveHeader) <= ArchiveDefaultBlockAlignment, "Archive Header section should be less than 512 bytes");

    //! Error codes for when archive validation fails
    enum class ArchiveHeaderErrorCode
    {
        InvalidMagicBytes = 1
    };

    //! Stores the error code and any error messages related to failing
    //! to validate the archive header
    struct ArchiveHeaderValidationResult
    {
        explicit constexpr operator bool() const;
        ArchiveHeaderErrorCode m_errorCode{};
        using ErrorString = AZStd::basic_fixed_string<char, 512, AZStd::char_traits<char>>;
        ErrorString m_errorMessage;
    };

    //! Validates the ArchiveHeader data
    //! It currently validates that the first 4 bytes of the ArchiveHeader
    //! matches the magic byte sequence "O3AR"
    ArchiveHeaderValidationResult ValidateHeader(const ArchiveHeader& archiveHeader);

    //! File offset representing the beginning of the file
    //! It starts is at offset 512 within the archive file stream
    //! This is because the Archive header is aligned to 512 bytes
    constexpr AZ::u64 ContentDataOffsetStart = AZ_SIZE_ALIGN_UP(sizeof(ArchiveHeader), ArchiveDefaultBlockAlignment);
    static_assert(ContentDataOffsetStart == ArchiveDefaultBlockAlignment, "Offset where file content data in the archive should match"
        " the ArchiveDefaultBlockAlignment");

    //! Represents an entry of a single file within the Archive
    struct alignas(32) ArchiveTocFileMetadata
    {
        ArchiveTocFileMetadata();

        //! represents the file after it has been uncompressed on disk
        //! Can represent a file up to 2^35 = 32GiB
        AZ::u64 m_uncompressedSize : 35; // 35-bits
        //! Stores compressed blocks that stored aligned on 512-byte sectors
        //! Therefore this can represent bytes sizes up to 35-bits as well
        //! while the value actually being stored is a 512-byte sector size
        //! 2^26 sectors * 512 bytes = 2^26 * 2^9 = 2^35 bytes
        //! Each 2 MiB chunk of a file compressed sectors are aggregated in this member
        //! For example if a 4 MiB file was compressed and the 2 "2 MiB" block compressed down
        //! to 513 and 511 bytes each,
        //! then compressed size in 512-byte sectors would be 3 due to rounding up each block to the nearest 512 byte boundary
        //!   AlignUpToMultiple512(513) / 512 = 2
        //! + AlignUptoMultiple512(511) / 512 = 1
        //! --------------------------------------
        //!                                     3
        AZ::u64 m_compressedSizeInSectors : 26; // 61-bits
        //! Stores an index into Compression ID table to indicate
        //! the compression algorithm the file uses or UncompressedAlgorithmIndex
        //! if the value is set to UncompressedAlgorithmIndex,
        //! the block table index is not used
        AZ::u64 m_compressionAlgoIndex : 3; // 64-bits

        //! Index of the first block which line contains compressed data for this file
        //! Up to 2^25 ~ 33.55 million block lines can be referenced
        //! Each block line can represent up to 3 "2 MiB" blocks of content data that has been compressed
        //! Therefore a total of (2^25 * 3) ~ 100.66 million blocks can be stored
        //!
        //! NOTE: This is only used if the file is stored compressed
        //! If the compressionAlgorithm index is UncompressedAlgorithmIndex
        //! then the file is stored uncompressed and the `m_offset` member represents
        //! a contiguous block that is 512 byte aligned
        AZ::u64 m_blockLineTableFirstIndex : 25; // 25-bits
        //! Offset within the archive where the file actually starts
        //! Due to files within the archive being aligned on 512-byte boundaries
        //! This can represent a offset of up to (39 + 9) bits or 2^48 = 256TiB
        //! The actual cap for Archive V1 layout is around 64TiB, since the m_blockTable can only
        //! represent 2^25 "2 MiB" blocks, which is (2^25 * 2^21) = 2^46 = 64TiB
        AZ::u64 m_offset : 39; // 64-bits

        //! offset = 16
        //! Stores a checksum value of the file uncompressed data
        //! This can be used to validate that uncompressed file contents
        AZ::Crc32 m_crc32{};

        //! offset = 20
        //! Add padding bytes to fill the File Metadata structure
        //! with 0 bytes on construction
        AZStd::byte m_unused[12]{};
    };

    static_assert(sizeof(ArchiveTocFileMetadata) == 32, "File Metadata size should be 16 bytes");

    //! Stores the size of a file path and an offset into the file path blob table for a single file
    //! in the archive
    struct ArchiveTocFilePathIndex
    {
        ArchiveTocFilePathIndex();

        //! Size of the number of bytes until the end of the File Path entry
        //! Cap is 15-bits to allow relative paths with sizes up to 2^15
        AZ::u64 m_size : 15;

        //! Unused bit to pad to 16-bytes
        AZ::u64 m_unused : 1;

        //! Offset from the beginning of the File Path Table to the start of Archive File Path
        AZ::u64 m_offset : 48;
    };

    static_assert(sizeof(ArchiveTocFilePathIndex) == 8, "File Path Index entry should be 8 bytes");

    //! There are 3 blocks per block line as 3 "2 MiB" chunks can be encoded in a 64-bit integer
    //! This is done by storing the compressed block size using 21-bits
    constexpr AZ::u64 BlocksPerBlockLine = 3;
    //! For a block line with a jump entry, instead of having 3 21-bit compressed block sizes,
    //! one of the block entries is borrowed for the 16-bit jump offset entry
    constexpr AZ::u64 BlocksPerBlockLineWithJump = BlocksPerBlockLine - 1;
    //! Maximum block line size is 3 blocks * 2 MiB = 6 MiB
    constexpr AZ::u64 MaxBlockLineSize = ArchiveBlockSizeForCompression * BlocksPerBlockLine;
    //! Constant for the maximum number of block line entries for the file
    //! before a jump offset is used which is 3 block lines(9 blocks)
    constexpr AZ::u64 MaxBlocksNoJumpEntry = 9;
    constexpr AZ::u64 MaxBlockLinesNoJumpEntry = MaxBlocksNoJumpEntry / BlocksPerBlockLine;
    //! When the remaining size of a file is above 18 MiB, a jump offset is used to the block line
    //! to indicate where the next block starts
    constexpr AZ::u64 MaxRemainingFileSizeNoJumpEntry = MaxBlockLineSize * MaxBlockLinesNoJumpEntry;
    //! Specifies the number of block line entries that are skipped with a jump entry
    //! The value is 3 block lines(1 jump entry + 8 blocks)
    constexpr AZ::u64 BlocksToSkipWithJumpEntry = 8;
    constexpr AZ::u64 BlockLinesToSkipWithJumpEntry = (BlocksToSkipWithJumpEntry + 1) / BlocksPerBlockLine;
    //! The compressed size of 8 "2 MiB" blocks are stored by the next 3 blocks including the current block
    //! if the remaining uncompressed size of a file is >= 18 MiB
    //! Since 16-bits are used to store the jump entry, the first block in the current block
    //! line is unavailable and 16 MiB of uncompressed sizes can be skipped
    constexpr AZ::u64 FileSizeToSkipWithJumpEntry = BlocksToSkipWithJumpEntry * ArchiveBlockSizeForCompression;
    //! Represents the maximum uncompressed size in bytes of the minimum amount of block lines(4)
    //! that is required for a file with a jump entry
    //! A file that is > 18 MiB requires a jump entry in the first block offset entry of the first block line
    //! The 3 block lines in total from that beginning of the block offset table entry for the file
    //! contains
    //! 1 jump entry(16-bits)
    //! + compressed size values for first 2 2 MiB blocks of the file(21-bits) = 58 bits which can be encoded in AZ::u64
    //! + compressed size values for next 3 2 MiB blocks of the file(21-bits) = 63 bits which can be encoded in another AZ::u64
    //! + compressed size values for following 3 2 MiB blocks of the file(21-bits) = 63 bits which can be encoded in 3rd AZ::u64
    //! Therefore in 24-bytes, the compressed sizes for 16 MiB of uncompressed data can be stored
    //! Now the jump entry would point at a block line which can encode another 3 2 MiB blocks
    //! at minimum(63-bits)
    //! Therefore in 32-bytes, the first 4 block lines of a file can encode the compressed sizes of the first 11 2 MiB block
    //!  = the first 22 MiB of the file
    constexpr AZ::u64 MaxFileSizeForMinBlockLinesWithJumpEntry = FileSizeToSkipWithJumpEntry + MaxBlockLineSize;

    //! Block lines are made up of 3 blocks at a time
    //! This is used when a file uncompressed size is < 18 MiB
    struct ArchiveBlockLine
    {
        ArchiveBlockLine();

        //! Represents the compressed size of the first 2 MiB block in a block line
        AZ::u64 m_block0 : 21;
        //! Represents the compressed size of the middle 2 MiB block in a block line
        AZ::u64 m_block1 : 21;
        //! Represents the compressed size of the last 2 MiB block in a block line
        AZ::u64 m_block2 : 21;
        //! 1 if the block is used
        AZ::u64 m_blockUsed : 1;
    };

    //! Block line to represents the compressed size of a file in blocks
    //! when a file uncompressed size is >=18 MiB
    struct ArchiveBlockLineJump
    {
        ArchiveBlockLineJump();

        //! 16 bit entry which is used to skip the next 8 blocks
        //! by storing the next 8 block total size within the 16-bits
        //! As blocks are 512-byte aligned, a size of up to 25-bits can be represented
        //! 2^25 = 32 MiB > 18 MiB, therefore jumps of 18MiB can be represented
        AZ::u64 m_blockJump : 16;
        //! Represents the compressed size(non-aligned) of the first block in the block line
        //! containing the jump table
        AZ::u64 m_block0 : 21;
        //! Represents the compressed size(non-aligned) of the last block in the block line
        //! containing the jump table
        AZ::u64 m_block1 : 21;
        //! 1 if the block is used
        AZ::u64 m_blockUsed : 1;
    };

    static_assert(sizeof(ArchiveBlockLine) == sizeof(ArchiveBlockLineJump),
        "The Non-Jump Block Line and Jump Block line must be the same size");

    static_assert(sizeof(ArchiveBlockLine) == 8, "Block Line size should be 8 bytes");


    //! Union which can store either a block line without a jump entry
    //! or block line with a jump entry
    union ArchiveBlockLineUnion
    {
        ArchiveBlockLineUnion();

        //! A block line containing entries for up to 3 2MiB block
        //! It will be the only type used for files with a total uncompressed size < 18 MiB
        ArchiveBlockLine m_blockLine{};
        //! A block containing a 16-bit jump entry which is used to store the total
        //! compressed size of the next 8-blocks
        //! When the remaining uncompressed size >= 18 MiB, a block with a jump entry
        //! will exist for every 3 block lines until the remaining uncompressed size
        //! is < 18 MiB
        ArchiveBlockLineJump m_blockLineWithJump;
    };

    //! Returns the blocks needed for storing a file that would be compressed
    //! using the file uncompressed size in bytes
    //! NOTE: If the file is stored uncompressed, then there is no need to call this function
    //! as the file is written as one contiguous byte sequence of its uncompressed data
    //! There are 0 blocks in that scenario
    //!
    //! The maximum size of a single uncompressed file in an archive is 2^35 (32GiB)
    //! As the current block size is 2^21 (2MiB) the maximum number of blocks a file can have
    //! is 2^35 / 2^21 = 2^14 (16KiB) which fits in a 16-bit int
    //! Now as the block size might be altered in the future
    //! a 32-bit int is returned
    constexpr AZ::u32 GetBlockCountIfCompressed(AZ::u64 uncompressedSize);

    //! Returns the block lines needed for storing a file that would be compressed
    //! using the file uncompressed size in bytes
    //! NOTE: If the file is stored uncompressed, then there is no need to call this function
    //! as the file is written as one contiguous byte sequence of its uncompressed data
    //! There are 0 block lines in that scenario
    //!
    //! This does not have linear mapping with block values uncompressed files
    //! that have more than 18 MiB remaining in memory will have a 16-bit jump entry
    //! used in place of the first the compressed size entry for a block
    //! The function to calculate the block line count is actually a piecewise function
    //! Uncompressed File Size = 0; No block lines are stored
    //! Block Size = (0, 6] MiB; Uses 1 block line
    //! Block Size = (6, 12] MiB; Uses 2 block lines
    //! Block Size = (12, 18] MiB; Uses 3 block lines
    //! Block Size = (18, 22] MiB; Uses 4 block lines!
    //!     The first entry of the first block line used for a jump table entry
    //!     as the file still contains more than 18MiB remaining when the first block line set of 3 is processed
    //! Block Size = (22, 28] MiB; Uses 5 block lines
    //! Block Size = (28, 34] MiB; Uses 6 block lines
    //! Block Size = (34, 38] MiB; Uses 7 block lines!
    //!     The first entry of the first and fourth block line are used for a jump table entry
    //!     as the file still contains more than 18MiB remaining when the first 2 block line sets of 3 are processed
    //! Block Size = (38, 44] MiB; Uses 8 block lines
    //! Block Size = (44, 50] MiB; Uses 9 block lines
    //! ...
    constexpr AZ::u32 GetBlockLineCountIfCompressed(AZ::u64 uncompressedSize);

    //! Retrieve the block range of compressed blocks to read from
    //! a content file using the uncompressed offset to start reading +
    //! the amount of uncompressed bytes to read from the file
    constexpr AZStd::pair<AZ::u64, AZ::u64> GetBlockRangeToRead(AZ::u64 uncompressedOffset, AZ::u64 bytesToRead);

    //! Stores the result of the GetBlockLineIndexFromBlockIndex function below
    struct GetBlockLineIndexResult
    {
        explicit constexpr operator bool() const;
        size_t m_blockLineIndex{ AZStd::numeric_limits<size_t>::max() };
        size_t m_offsetInBlockLine{ AZStd::numeric_limits<size_t>::max() };
        AZStd::fixed_string<256> m_errorMessage;
    };
    //! Calculates the block line index from the block index given the block count of a file
    //! Both the block line index and the block offset within that index is returned
    //! The block offset value can be used to fine the exact compressed block size
    //! within the block line
    //! The files block line section in steps of 3 block lines at time
    //! For a file with <= 9 blocks
    //! there is a single step of 3 block lines = 9 blocks processed
    //!
    //! For a file between [10, 18) blocks
    //! the first step is 3 block lines = 8 blocks processed (1 entry is for the jump offset)
    //! the final step is up to 3 more blocks lines = 9 blocks processed
    //!
    //! For a file between [18, 26) blocks
    //! the first step is 3 block lines = 8 blocks processed (1 entry is for the jump offset)
    //! the second step is 3 more block lines = 8 blocks processed (1 entry is for the jump offset)
    //! the final step is up to 3 more blocks lines = 9 blocks processed
    //!
    //! If a file has 16 blocks and the block index value is 8,
    //! the block line index should be 3 as the fourth block line entry
    //! However if a file has 9 blocks and the block index value is 8
    //! the block line index should be 2 as the third block line entry
    //!
    //! If a file has 25 blocks and the block index value is 16,
    //! the block line index should be 6 as the seventh block line entry
    //! However if a file has 17 blocks and the block index value is 16
    //! the block line index should be 5 as the sixth block line index
    //!
    //! @param blockCount The amount of 2-MiB blocks for the file
    //! @param blockIndex index into a specific 2-MiB block of the file
    //! @return On success a result structure that contains the block line index and the offset within that block
    constexpr GetBlockLineIndexResult GetBlockLineIndexFromBlockIndex(AZ::u64 blockCount, AZ::u64 blockIndex);

} // namespace Archive

// Implementation for any struct functions
#include "ArchiveInterfaceStructs.inl"

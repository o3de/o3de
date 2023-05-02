/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>

#include <AzCore/IO/Path/Path_fwd.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/std/limits.h>

#include <Compression/CompressionInterfaceStructs.h>

namespace Archive
{
    inline namespace literals
    {
        constexpr AZ::u64 operator"" _kib(AZ::u64 value);
        constexpr AZ::u64 operator"" _mib(AZ::u64 value);
        constexpr AZ::u64 operator"" _gib(AZ::u64 value);
    }

    //! Represents the default block size for the Archive format
    //! It will be 2 MiB until their is data that proves
    //! that a different block size is ideal
    constexpr AZ::u64 ArchiveDefaultBlockSize = 2  * (1 << 20);
    //! The alignment of blocks within an archive file
    //! It defaults to 512 bytes
    constexpr AZ::u64 ArchiveDefaultBlockAlignment = 512;

    //! Sentinel which indicates the last entry in the deleted File Path Index list
    constexpr AZ::u32 DeletedPathIndexSentinel = AZStd::numeric_limits<AZ::u32>::max();

    //! Sentinel which indicates the value written to the last block to indicate
    //! there are no further deleted blocks afterwards
    constexpr AZ::u64 DeletedBlockOffsetSentinel = AZStd::numeric_limits<AZ::u64>::max();

    //! Fixed size Header struct for the Archive format
    //! This suitable for directly reading the archive header into
    struct ArchiveHeaderSection
    {
        ArchiveHeaderSection();

        //! O3DE only runs on little endian machines
        //! Therefore the bytes are added in little endian order
        //! The Magic Identifier for the archive format is "O3AR" for O3DE Archive
        //! offset = 0
        const AZ::u32 m_magicBytes{ 'O' | ('3' << 8) | ('A' << 16) | ('R' << 24) };

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

        //! The 64-bit offset from the start of the Archive File to the Table of Contents
        //! offset = 16
        AZ::u64 m_tocOffset{};

        //! Compressed Size of the Table of Contents
        //! Max size is 512MiB or 2^29 bytes
        //! The TOC offset + TOC offset is equal to total size of the Archive file
        //! offset = 24
        AZ::u32 m_tocCompressedSize : 29;
        //! Compression algorithm used for the Table of Contents
        //! The maximum 3-bit value of 7 is reserved for uncompressed
        //! Other values count as a offset in the Compression Algorithm ID table
        AZ::u32 m_tocCompressionAlgoIndex : 3;

        //! Uncompressed size of the Table of Contents File Metadata section.
        //! offset = 28
        AZ::u32 m_tocFileMetadataTableUncompressedSize{};
        //! Uncompressed size of the Table of Contents File Path index
        //! The File Path index is used to lookup the location for a file path
        //! within the archive
        //! offset = 32
        AZ::u32 m_tocPathIndexTableUncompressedSize{};
        //! Uncompressed size of the Table of Contents File Path section
        //! It contains a blob of FilePaths without any null-termination
        //! The File Path Index entries are used to look up a file path
        //! through using the path offset + size entry
        //! offset = 36
        AZ::u32 m_tocPathTableUncompressedSize{};
        //! Uncompressed size of the Table of Contents File Block section
        //! Contains compressed sizes of individual blocks of a file
        //! In Archive V1 layout the block size is 2MiB
        //! offset = 40
        AZ::u32 m_tocBlockTableSize{};

        //! Threshold value represents the cap on the size a block after it has been
        //! sent through the compression step to determine if it should be stored compressed
        //!
        //! Due to block size defaulting to 2MiB, any blocks that are larger than 2_mib
        //! after compression will be stored uncompressed.
        //! So the maximum limit of this value is the Block Size
        //! offset = 44
        AZ::u32 m_compressionThreshold{ static_cast<AZ::u32>(ArchiveDefaultBlockSize) };

        //! Stores 32-bit IDS of up to 7 compression algorithms that this archive can use
        //! offset = 48
        AZStd::array<Compression::CompressionAlgorithmId, 7> m_compressionAlgorithmsIds;

        //! Offset from the beginning of the File Path index table
        //! Where the first deleted block is located
        //! offset = 76
        AZ::u32 m_firstDeletedFileIndex{ DeletedPathIndexSentinel };

        //! Offset from the beginning of the file block section to the first
        //! deleted block.
        //! The first 8 bytes of each deleted block will contain the offset to the next deleted block
        //! or 0xffff'ffff'ffff'ffff if this is the last deleted block
        //! offset = 80
        AZ::u64 m_firstDeletedBlockOffset{ DeletedBlockOffsetSentinel };

        //! total offset = 88

        //! Max FileCount
        //! Up to 2^32 files can be stored,
        //! but is limited to 2^25 because around 640MiB of uncompressed data will need to be loaded into memory for an archive of containing ~33million files,
        //! and memory requirements would increase if the not limited.
        //! offset = 12
        static constexpr AZ::u32 MaxFileCount{(1 << 25) - 1 };
    };

    static_assert(sizeof(ArchiveHeaderSection) <= ArchiveDefaultBlockAlignment, "Archive Header section should be less than 512 bytes");

    //! Represents an entry of a single file within the Archive
    struct ArchiveFileMetadataSection
    {
        ArchiveFileMetadataSection();

        //! represents the file after it has been uncompressed on disk
        //! Can represent a file up to 2^35 = 32GiB
        AZ::u64 m_uncompressedSize : 35; // 35-bits
        //! compressed files are stored aligned on 512-byte sectors
        //! Therefore this can represent bytes sizes up to 35-bits as well
        //! while the value actually being stored is a 512-byte sector size
        //! 2^26 sectors * 512 bytes = 2^26 * 2^9 = 2^35 bytes
        AZ::u64 m_compressedSizeInSectors : 26; // 61-bits
        //! Stores an index into Compression ID table to indicate
        //! the compression algorithm the file uses or UncompressedAlgorithmIndex
        //! if the value is set to UncompressedAlgorithmIndex,
        //! the block Table Index is not used
        AZ::u64 m_compressionAlgoIndex : 3; // 64-bits

        //! Index for the first block which contains compressed data for this file
        //! As it is 25 bits, up to 2^25 ~ 33.55 Million blocks can be referenced
        //! If the compressionAlgorithm index is UncompressedAlgorithmIndex
        //! then the file is uncompressed in m_offset in a contiguous block
        //! that is 512 byte aligned
        AZ::u64 m_blockTableIndex : 25; // 25-bits
        //! Offset within the archive where the file actually starts
        //! Due to files within the archive being aligned on 512-byte boundaries
        //! This can represent a offset of up to (39 + 9) bits or 2^48 = 256TiB
        //! The actual cap for Archive V1 layout is around 64TiB, since the m_blockTable can only
        //! represent 2^25 "2 MiB" blocks, which is (2^25 * 2^21) = 2^46 = 64TiB
        AZ::u64 m_offset : 39; // 64-bits
    };

    //! Views an entry of a single File Path Index when the File Path Table of the Archive TOC
    struct ArchiveFilePathIndexSection
    {
        ArchiveFilePathIndexSection();

        //! Deleted flag to indicates if the file has been deleted from the archive
        bool m_deleted{};
        //! Because the previous entry was a bool there is 1 byte of padding here before the size

        //! Size of the number of bytes until the end of the File Path entry
        //! Cap is 16-bits to allow relative paths with sizes up to 2^16
        AZ::u16 m_size{};

        //! Offset from the beginning of the File Path Table to the start of Archive File Path
        AZ::u32 m_offset{};
    };


    //! Block lines are made up of 3 blocks at a time
    //! This is used when a file uncompressed size is < 18 MiB
    struct ArchiveBlockLine
    {
        ArchiveBlockLine();

        //! Represents the compressed size of the first 2 MiB block in a block line
        AZ::u64 m_block1 : 21;
        //! Represents the compressed size of the middle 2 MiB block in a block line
        AZ::u64 m_block2 : 21;
        //! Represents the compressed size of the last 2 MiB block in a block line
        AZ::u64 m_block3 : 21;
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
        AZ::u64 m_block1 : 21;
        //! Represents the compressed size(non-aligned) of the last block in the block line
        //! containing the jump table
        AZ::u64 m_block2 : 21;
        //! 1 if the block is used
        AZ::u64 m_blockUsed : 1;
    };

    static_assert(sizeof(ArchiveBlockLine) == sizeof(ArchiveBlockLineJump),
        "The Non-Jump Block Line and Jump Block line must be the same size");


    //! Union which can store either a block line without a jump entry
    //! or block line with a jump entry
    union ArchiveBlockLineSection
    {
        ArchiveBlockLineSection();

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

    //! Structure storing data from the Table of Contents at the end
    //! of the archive file
    struct ArchiveTableOfContents
    {
        //! The Archive Header is either uncompressed
        //! or compressed based on the compression algorithm index
        //! set within the Archive Header

        //! Pointer to the beginning of the Archive File Metadata Table
        //! It's length is based on the file count value in the Archive Header Section
        ArchiveFileMetadataSection* m_fileMetadataTable{};
        //! Pointer to the beginning of the Archive File Path Index Table
        //! It's length is based on the file count value in the Archive Header Section
        ArchiveFilePathIndexSection* m_filePathIndexTable{};

        //! ArchiveFilePathTable is a view into a blob of file paths
        using ArchiveFilePathTable = AZStd::span<AZ::IO::PathView>;
        ArchiveFilePathTable m_filePathBlob;

        //! pointer to block offset table which stores the compressed size of all blocks within the archive
        ArchiveBlockLineSection* m_archiveBlockTable{};
    };

} // namespace Archive

// Implementation for any struct functions
#include "ArchiveInterfaceStructs.inl"

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/IO/Path/Path_fwd.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/intrusive_base.h>
#include <AzCore/std/string/string.h>
#include <AzFramework/Archive/Codec.h>

namespace AZ::IO
{
    // This represents one particular archive.
    struct INestedArchive
        : public AZStd::intrusive_base
    {
        // Compression methods
        enum ECompressionMethods
        {
            METHOD_STORE = 0,
            METHOD_COMPRESS = 8,
            METHOD_DEFLATE = 8,
            METHOD_COMPRESS_AND_ENCRYPT = 11
        };

        // Compression levels
        enum ECompressionLevels
        {
            LEVEL_FASTEST = 0,
            LEVEL_FASTER = 2,
            LEVEL_NORMAL = 8,
            LEVEL_BETTER = 8,
            LEVEL_BEST = 9,
            LEVEL_DEFAULT = -1
        };

        enum EPakFlags
        {
            // support for absolute and other complex path specifications -
            // all paths will be treated relatively to the current directory
            FLAGS_ABSOLUTE_PATHS = 1,

            // if this is set, the object will only understand relative to the zip file paths,
            // but this can give an opportunity to optimize for frequent quick accesses
            // FLAGS_SIMPLE_RELATIVE_PATHS and FLAGS_ABSOLUTE_PATHS are mutually exclusive
            FLAGS_RELATIVE_PATHS_ONLY = 1 << 1,

            // if this flag is set, the archive update/remove operations will not work
            // this is useful when you open a read-only or already opened for reading files.
            // If FLAGS_OPEN_READ_ONLY | FLAGS_SIMPLE_RELATIVE_PATHS are set, IArchive
            // will try to return an object optimized for memory, with long life cycle
            FLAGS_READ_ONLY = 1 << 2,

            // if this flag is set, FLAGS_OPEN_READ_ONLY
            // flags are also implied. The returned object will be optimized for quick access and
            // memory footprint
            FLAGS_OPTIMIZED_READ_ONLY = (1 << 3),

            // if this is set, the existing file (if any) will be overwritten
            FLAGS_CREATE_NEW = 1 << 4,

            // if this flag is set, and the file is opened for writing, and some files were updated
            // so that the archive is no more continuous, the archive will nevertheless NOT be compacted
            // upon closing the archive file. This can be faster if you open/close the archive for writing
            // multiple times
            FLAGS_DONT_COMPACT = 1 << 5,

            // if this is set, validate header data when opening the archive
            FLAGS_VALIDATE_HEADERS = 1 << 9,

            // if this is set, validate header data when opening the archive and validate CRCs when decompressing
            // & reading files.
            FLAGS_FULL_VALIDATE = 1 << 10,

            // Disable a pak file without unloading it, this flag is used in combination with patches and multiplayer
            // to ensure that specific paks stay in the position(to keep the same priority) but being disabled
            // when running multiplayer
            FLAGS_DISABLE_PAK = 1 << 11,
        };

        using Handle = void*;

        virtual ~INestedArchive() = default;

        // Get archive's root folder
        virtual Handle GetRootFolderHandle() = 0;

        // Summary:
        //   Adds a new file to the zip or update an existing one.
        // Description:
        //   Adds a new file to the zip or update an existing one
        //   adds a directory (creates several nested directories if needed)
        //   compression methods supported are METHOD_STORE == 0 (store) and
        //   METHOD_DEFLATE == METHOD_COMPRESS == 8 (deflate) , compression
        //   level is LEVEL_FASTEST == 0 till LEVEL_BEST == 9 or LEVEL_DEFAULT == -1
        //   for default (like in zlib)
        virtual int UpdateFile(AZStd::string_view szRelativePath, const void* pUncompressed, uint64_t nSize, uint32_t nCompressionMethod = 0,
            int nCompressionLevel = -1, CompressionCodec::Codec codec = CompressionCodec::Codec::ZLIB) = 0;

        // Summary:
        //   Adds a new file to the zip or update an existing one if it is not compressed - just stored  - start a big file
        //   ( name might be misleading as if nOverwriteSeekPos is used the update is not continuous )
        // Description:
        //   First step for the UpdateFileConinouseSegment
        virtual int StartContinuousFileUpdate(AZStd::string_view szRelativePath, uint64_t nSize) = 0;

        // Summary:
        //   Adds a new file to the zip or update an existing segment if it is not compressed - just stored
        //   adds a directory (creates several nested directories if needed)
        //   ( name might be misleading as if nOverwriteSeekPos is used the update is not continuous )
        // Arguments:
        //   nOverwriteSeekPos - std::numeric_limits<uint64_t>::max(i.e uint64_t(-1)) means the seek pos should not be overwritten (then it needs UpdateFileCRC() to update CRC)
        virtual int UpdateFileContinuousSegment(AZStd::string_view szRelativePath, uint64_t nSize, const void* pUncompressed, uint64_t nSegmentSize,
            uint64_t nOverwriteSeekPos = (std::numeric_limits<uint64_t>::max)()) = 0;

        // Summary:
        //   needed to update CRC if UpdateFileContinuousSegment() was used with nOverwriteSeekPos
        virtual int UpdateFileCRC(AZStd::string_view szRelativePath, AZ::Crc32 dwCRC) = 0;

        // Summary:
        //   Deletes the file from the archive.
        virtual int RemoveFile(AZStd::string_view szRelativePath) = 0;

        // Summary:
        //   Deletes the directory, with all its descendants (files and subdirs).
        virtual int RemoveDir(AZStd::string_view szRelativePath) = 0;

        // Summary:
        //   Deletes all files and directories in the archive.
        virtual int RemoveAll() = 0;

        // Summary:
        //   Lists all the files in the archive.
        virtual int ListAllFiles(AZStd::vector<AZ::IO::Path>& outFileEntries) = 0;

        // Summary:
        //   Finds the file; you don't have to close the returned handle.
        // Returns:
        //   NULL if the file doesn't exist
        virtual Handle FindFile(AZStd::string_view szPath) = 0;
        // Summary:
        //   Get the file size (uncompressed).
        // Returns:
        //   The size of the file (unpacked) by the handle
        virtual uint64_t GetFileSize(Handle) = 0;
        // Summary:
        //   Reads the file into the preallocated buffer
        // Note:
        //    Must be at least the size returned by GetFileSize.
        virtual int ReadFile(Handle, void* pBuffer) = 0;

        // Summary:
        //   Get the full path to the archive file.
        virtual AZ::IO::PathView GetFullPath() const = 0;

        // Summary:
        //   Get the flags of this object.
        // Description:
        //   The possibles flags are defined in EPakFlags.
        // See Also:
        //   SetFlags, ResetFlags
        virtual uint32_t GetFlags() const = 0;

        // Summary:
        //   Sets the flags of this object.
        // Description:
        //   The possibles flags are defined in EPakFlags.
        // See Also:
        //   GetFlags, ResetFlags
        virtual bool SetFlags(uint32_t nFlagsToSet) = 0;

        // Summary:
        //   Resets the flags of this object.
        // See Also:
        //   SetFlags, GetFlags
        virtual bool ResetFlags(uint32_t nFlagsToSet) = 0;

        // Summary:
        //   Control if files in this pack can be accessed
        // Returns:
        //   true if archive state was changed
        virtual bool SetPackAccessible(bool bAccessible) = 0;

        // Summary:
        //   Determines if the archive is read only.
        // Returns:
        //   true if this archive is read-only
        inline bool IsReadOnly() const { return (GetFlags() & FLAGS_READ_ONLY) != 0; }
    };
}


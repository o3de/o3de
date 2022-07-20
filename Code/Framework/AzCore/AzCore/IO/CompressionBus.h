/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/Streamer/RequestPath.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/string/string_view.h>

namespace AZ
{
    namespace IO
    {
        union CompressionTag
        {
            uint32_t m_code;
            char m_name[4];
        };

        enum class ConflictResolution : uint8_t
        {
            PreferFile,
            PreferArchive,
            UseArchiveOnly
        };

        struct CompressionInfo;
        using DecompressionFunc = AZStd::function<bool(const CompressionInfo& info, const void* compressed, size_t compressedSize, void* uncompressed, size_t uncompressedBufferSize)>;

        struct CompressionInfo
        {
            CompressionInfo() = default;
            CompressionInfo(const CompressionInfo& rhs) = default;
            CompressionInfo& operator=(const CompressionInfo& rhs) = default;

            CompressionInfo(CompressionInfo&& rhs);
            CompressionInfo& operator=(CompressionInfo&& rhs);

            //! Relative path to the archive file.
            RequestPath m_archiveFilename;
            //< The function to use to decompress the data.
            DecompressionFunc m_decompressor;
            //< Tag that uniquely identifies the compressor responsible for decompressing the referenced data.
            CompressionTag m_compressionTag{ 0 };
            //! Offset into the archive file for the found file.
            size_t m_offset = 0;
            //! On disk size of the compressed file.
            size_t m_compressedSize = 0;
            //! Size after the file has been decompressed.
            size_t m_uncompressedSize = 0;
            //! Preferred solution when an archive is found in the archive and as a separate file.
            ConflictResolution m_conflictResolution = ConflictResolution::UseArchiveOnly;
            //! Whether or not the file is compressed. If the file is not compressed, the compressed and uncompressed sizes should match.
            bool m_isCompressed = false;
            //! Whether or not the pak file is used in multiple location or reads can be done exclusively.
            bool m_isSharedPak = false; 
        };

        class Compression
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
            using MutexType = AZStd::recursive_mutex;
            
            virtual ~Compression() = default;

            virtual void FindCompressionInfo(bool& found, CompressionInfo& info, const AZ::IO::PathView filePath) = 0;
        };

        using CompressionBus = AZ::EBus<Compression>;

        namespace CompressionUtils
        {
            bool FindCompressionInfo(CompressionInfo& info, const AZ::IO::PathView filePath);
        }
    }
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/CompressionBus.h>

namespace AZ::IO
{
    CompressionInfo::CompressionInfo(CompressionInfo&& rhs)
    {
        *this = AZStd::move(rhs);
    }

    CompressionInfo& CompressionInfo::operator=(CompressionInfo&& rhs)
    {
        m_decompressor = AZStd::move(rhs.m_decompressor);
        m_archiveFilename = AZStd::move(rhs.m_archiveFilename);
        m_compressionTag = rhs.m_compressionTag;
        m_offset = rhs.m_offset;
        m_compressedSize = rhs.m_compressedSize;
        m_uncompressedSize = rhs.m_uncompressedSize;
        m_conflictResolution = rhs.m_conflictResolution;
        m_isCompressed = rhs.m_isCompressed;
        m_isSharedPak = rhs.m_isSharedPak;

        return *this;
    }

    namespace CompressionUtils
    {
        bool FindCompressionInfo(CompressionInfo& info, const AZStd::string_view filename)
        {
            bool result = false;
            CompressionBus::Broadcast(&CompressionBus::Events::FindCompressionInfo, result, info, filename);
            return result;
        }
    }
} // namespace AZ::IO

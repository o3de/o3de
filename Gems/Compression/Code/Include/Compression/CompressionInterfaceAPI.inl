/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace Compression
{
    //! Returns a boolean true if decompression has completed
    constexpr inline CompressionResultData::operator bool() const
    {
        return m_result == CompressionResult::Complete;
    }

    //! Retrieves the compressed data size
    inline AZ::u64 CompressionResultData::GetCompressedByteCount() const
    {
        return m_compressedBuffer.size();
    }

    //! Retrieves the memory address of the compressed data
    inline AZStd::byte* CompressionResultData::GetCompressedByteData() const
    {
        return m_compressedBuffer.data();
    }
} // namespace Compression

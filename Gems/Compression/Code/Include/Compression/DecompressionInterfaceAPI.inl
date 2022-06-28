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
    //! Returns a boolean true if decompression has succeeded
    constexpr inline DecompressionResultData::operator bool() const
    {
        return m_result == DecompressionResult::Complete;
    }

    //! Retrieves the uncompressed size of the decompressed data
    inline AZ::u64 DecompressionResultData::GetUncompressedByteCount() const
    {
        return m_uncompressedBuffer.size();
    }

    //! Retrieves the memory address where the uncompressed data
    //! is stored
    inline AZStd::byte* DecompressionResultData::GetUncompressedByteData() const
    {
        return m_uncompressedBuffer.data();
    }
} // namespace Compression

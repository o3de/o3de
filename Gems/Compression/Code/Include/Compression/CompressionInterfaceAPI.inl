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
    AZ_TYPE_INFO_WITH_NAME_IMPL_INLINE(CompressionOptions, "CompressionOptions", "{037B2A25-E195-4C5D-B402-6108CE978280}");
    AZ_RTTI_NO_TYPE_INFO_IMPL_INLINE(CompressionOptions);
    AZ_TYPE_INFO_WITH_NAME_IMPL_INLINE(CompressionRegistrarInterface, "CompressionRegistrarInterface",  "{92251FE8-9D19-4A23-9A2B-F91D99D9491B}");
    AZ_RTTI_NO_TYPE_INFO_IMPL_INLINE(CompressionRegistrarInterface);

    inline CompressionOptions::~CompressionOptions() = default;

    //! Returns a boolean true if compression has completed
    constexpr inline CompressionOutcome::operator bool() const
    {
        return m_result == CompressionResult::Complete;
    }

    //! Returns a boolean true if compression has completed
    constexpr inline CompressionResultData::operator bool() const
    {
        return bool(m_compressionOutcome);
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

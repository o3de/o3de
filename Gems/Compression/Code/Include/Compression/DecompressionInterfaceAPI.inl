/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Compression/CompressionTypeIds.h>

namespace Compression
{
    AZ_TYPE_INFO_WITH_NAME_IMPL_INLINE(DecompressionOptions, "DecompressionOptions",
        DecompressionOptionsTypeId);
    AZ_RTTI_NO_TYPE_INFO_IMPL_INLINE(DecompressionOptions);
    AZ_TYPE_INFO_WITH_NAME_IMPL_INLINE(DecompressionRegistrarInterface, "DecompressionRegistrarInterface",
        DecompressionRegistrarInterfaceTypeId);
    AZ_RTTI_NO_TYPE_INFO_IMPL_INLINE(DecompressionRegistrarInterface);

    inline DecompressionOptions::~DecompressionOptions() = default;

    //! Returns a boolean true if decompression has completed
    constexpr inline DecompressionOutcome::operator bool() const
    {
        return m_result == DecompressionResult::Complete;
    }

    //! Returns a boolean true if decompression has succeeded
    constexpr inline DecompressionResultData::operator bool() const
    {
        return bool(m_decompressionOutcome);
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

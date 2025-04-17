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
    AZ_TYPE_INFO_WITH_NAME_IMPL_INLINE(CompressionOptions, "CompressionOptions", CompressionOptionsTypeId);
    AZ_RTTI_NO_TYPE_INFO_IMPL_INLINE(CompressionOptions);
    AZ_TYPE_INFO_WITH_NAME_IMPL_INLINE(CompressionRegistrarInterface, "CompressionRegistrarInterface", CompressionRegistrarInterfaceTypeId);
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

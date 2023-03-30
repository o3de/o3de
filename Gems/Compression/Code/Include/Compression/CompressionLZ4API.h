/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string_view.h>


namespace Compression
{
    enum class CompressionAlgorithmId : AZ::u32;
}

namespace CompressionLZ4
{
    //! Returns the CompressionAlgorithmId associated with the LZ4 Compressor
    //! @return LZ4 Compression AlgorithmId
    constexpr Compression::CompressionAlgorithmId GetLZ4CompressionAlgorithmId();

    constexpr Compression::CompressionAlgorithmId GetLZ4CompressionAlgorithmId()
    {
        constexpr Compression::CompressionAlgorithmId AlgorithmId{ AZ::u32(AZStd::hash<AZStd::string_view>{}("LZ4")) };
        return AlgorithmId;
    }
} // namespace CompressionLZ4

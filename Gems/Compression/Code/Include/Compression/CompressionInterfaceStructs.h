/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/limits.h>
#include <AzCore/std/typetraits/underlying_type.h>

namespace Compression
{
    // Structure represent a compression algorithmId
    enum class CompressionAlgorithmId : AZ::u32 {};
    constexpr CompressionAlgorithmId Uncompressed{};
    // tag index that indicates the content being examined is uncompressed
    // It is set to the maximum value that can be stored in 3-bits = 7
    constexpr AZ::u8 UncompressedAlgorithmIndex = 0b111;

    constexpr CompressionAlgorithmId Invalid{ AZStd::numeric_limits<AZ::u32>::max() };

    // Implements the Relational Operators for the CompressionAlgorithmId class
    // only for use in the CompressionFactoryImpl/DecompressionFactoryImpl class to allow it to be added to a set
    AZ_DEFINE_ENUM_RELATIONAL_OPERATORS(CompressionAlgorithmId);
} // namespace Compression

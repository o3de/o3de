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

namespace Compression
{
    // Structure represent a compression algorithmId
    enum class CompressionAlgorithmId : AZ::u32 {};
    constexpr CompressionAlgorithmId Uncompressed{};
    constexpr CompressionAlgorithmId Invalid{ AZStd::numeric_limits<AZ::u32>::max() };

} // namespace Compression

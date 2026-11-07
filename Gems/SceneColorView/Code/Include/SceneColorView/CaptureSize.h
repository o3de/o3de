/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <cstdint>

namespace SceneColorView
{
    struct CaptureSize
    {
        uint32_t m_width;
        uint32_t m_height;
    };

    constexpr uint32_t NormalizeDivisor(uint32_t divisor)
    {
        return divisor == 1 || divisor == 2 || divisor == 4 ? divisor : 2;
    }

    constexpr CaptureSize CalculateCaptureSize(uint32_t width, uint32_t height, uint32_t divisor)
    {
        divisor = NormalizeDivisor(divisor);
        // Ceiling division without overflow at UINT32_MAX.
        width = width / divisor + (width % divisor != 0);
        height = height / divisor + (height % divisor != 0);
        return {width ? width : 1, height ? height : 1};
    }
}

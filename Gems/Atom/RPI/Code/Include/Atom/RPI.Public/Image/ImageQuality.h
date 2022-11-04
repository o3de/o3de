/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>

namespace AZ
{
    namespace RPI
    {
        using ImageQuality = u32;

        constexpr ImageQuality ImageQualityHighest = 0;
        constexpr ImageQuality ImageQualityLowest = 0xFFFFFFFF;
    }
}

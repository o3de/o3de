/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiCanvasEditor_precompiled.h"

#include "PivotPresets.h"

namespace
{
    static AZ::Vector2 presets[ PivotPresets::PresetIndexCount ] =
    {
        AZ::Vector2(0.0f, 0.0f),
        AZ::Vector2(0.5f, 0.0f),
        AZ::Vector2(1.0f, 0.0f),
        AZ::Vector2(0.0f, 0.5f),
        AZ::Vector2(0.5f, 0.5f),
        AZ::Vector2(1.0f, 0.5f),
        AZ::Vector2(0.0f, 1.0f),
        AZ::Vector2(0.5f, 1.0f),
        AZ::Vector2(1.0f, 1.0f)
    };

    static_assert(PivotPresets::PresetIndexCount == AZ_ARRAY_SIZE(presets), "presets and PresetIndexCount MUST be the same size.");
} // anonymous namespace.

namespace PivotPresets
{
    const AZ::Vector2& PresetIndexToPivot(int i)
    {
        if ((i >= 0) && (i < PresetIndexCount))
        {
            return presets[ i ];
        }
        else
        {
            AZ_Assert(false, "Invalid pivot preset index");
            return presets[ 0 ];
        }
    }

    int PivotToPresetIndex(const AZ::Vector2& v)
    {
        float x = v.GetX();
        float y = v.GetY();

        if (x == 0.0f)
        {
            if (y == 0.0f)
            {
                // 0: AZ::Vector2(0.0f, 0.0f)
                return 0;
            }
            else if (y == 0.5f)
            {
                // 3: AZ::Vector2(0.0f, 0.5f)
                return 3;
            }
            else if (y == 1.0f)
            {
                // 6: AZ::Vector2(0.0f, 1.0f)
                return 6;
            }
            else
            {
                return -1;
            }
        }
        else if (x == 0.5f)
        {
            if (y == 0.0f)
            {
                // 1: AZ::Vector2(0.5f, 0.0f)
                return 1;
            }
            else if (y == 0.5f)
            {
                // 4: AZ::Vector2(0.5f, 0.5f)
                return 4;
            }
            else if (y == 1.0f)
            {
                // 7: AZ::Vector2(0.5f, 1.0f)
                return 7;
            }
            else
            {
                return -1;
            }
        }
        else if (x == 1.0f)
        {
            if (y == 0.0f)
            {
                // 2: AZ::Vector2(1.0f, 0.0f)
                return 2;
            }
            else if (y == 0.5f)
            {
                // 5: AZ::Vector2(1.0f, 0.5f)
                return 5;
            }
            else if (y == 1.0f)
            {
                // 8: AZ::Vector2(1.0f, 1.0f)
                return 8;
            }
            else
            {
                return -1;
            }
        }
        else
        {
            return -1;
        }
    }
}   // namespace PivotPresets

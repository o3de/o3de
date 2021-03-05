/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "UiCanvasEditor_precompiled.h"

#include "AnchorPresets.h"

namespace
{
    static AZ::Vector4 presets[ AnchorPresets::PresetIndexCount ] =
    {
        AZ::Vector4(0.0f, 0.0f, 0.0f, 0.0f),
        AZ::Vector4(0.5f, 0.0f, 0.5f, 0.0f),
        AZ::Vector4(1.0f, 0.0f, 1.0f, 0.0f),
        AZ::Vector4(0.0f, 0.0f, 1.0f, 0.0f),
        AZ::Vector4(0.0f, 0.5f, 0.0f, 0.5f),
        AZ::Vector4(0.5f, 0.5f, 0.5f, 0.5f),
        AZ::Vector4(1.0f, 0.5f, 1.0f, 0.5f),
        AZ::Vector4(0.0f, 0.5f, 1.0f, 0.5f),
        AZ::Vector4(0.0f, 1.0f, 0.0f, 1.0f),
        AZ::Vector4(0.5f, 1.0f, 0.5f, 1.0f),
        AZ::Vector4(1.0f, 1.0f, 1.0f, 1.0f),
        AZ::Vector4(0.0f, 1.0f, 1.0f, 1.0f),
        AZ::Vector4(0.0f, 0.0f, 0.0f, 1.0f),
        AZ::Vector4(0.5f, 0.0f, 0.5f, 1.0f),
        AZ::Vector4(1.0f, 0.0f, 1.0f, 1.0f),
        AZ::Vector4(0.0f, 0.0f, 1.0f, 1.0f)
    };

    static_assert(AnchorPresets::PresetIndexCount == AZ_ARRAY_SIZE(presets), "presets and PresetIndexCount MUST be the same size.");
} // anonymous namespace.

namespace AnchorPresets
{
    const AZ::Vector4& PresetIndexToAnchor(int i)
    {
        if ((i >= 0) && (i < PresetIndexCount))
        {
            return presets[ i ];
        }
        else
        {
            AZ_Assert(0, "Invalid preset index");
            return presets[ 0 ];
        }
    }

    int AnchorToPresetIndex(const AZ::Vector4& v)
    {
        float x = v.GetX();
        float y = v.GetY();
        float z = v.GetZ();
        float w = v.GetW();

        if (x == 0.0f)
        {
            if (y == 0.0f)
            {
                if (z == 0.0f)
                {
                    if (w == 0.0f)
                    {
                        // 0: AZ::Vector4( 0.0f, 0.0f, 0.0f, 0.0f )
                        return 0;
                    }
                    else if (w == 1.0f)
                    {
                        // 12: AZ::Vector4( 0.0f, 0.0f, 0.0f, 1.0f )
                        return 12;
                    }
                    else
                    {
                        return -1;
                    }
                }
                else if (z == 1.0f)
                {
                    if (w == 0.0f)
                    {
                        // 3: AZ::Vector4( 0.0f, 0.0f, 1.0f, 0.0f )
                        return 3;
                    }
                    else if (w == 1.0f)
                    {
                        // 15: AZ::Vector4( 0.0f, 0.0f, 1.0f, 1.0f )
                        return 15;
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
            else if (y == 0.5f)
            {
                if (w == 0.5f)
                {
                    if (z == 0.0f)
                    {
                        // 4: AZ::Vector4( 0.0f, 0.5f, 0.0f, 0.5f )
                        return 4;
                    }
                    else if (z == 1.0f)
                    {
                        // 7: AZ::Vector4( 0.0f, 0.5f, 1.0f, 0.5f )
                        return 7;
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
            else if (y == 1.0f)
            {
                if (w == 1.0f)
                {
                    if (z == 0.0f)
                    {
                        // 8: AZ::Vector4( 0.0f, 1.0f, 0.0f, 1.0f )
                        return 8;
                    }
                    else if (z == 1.0f)
                    {
                        // 11: AZ::Vector4( 0.0f, 1.0f, 1.0f, 1.0f )
                        return 11;
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
            else
            {
                return -1;
            }
        }
        else if (x == 0.5f)
        {
            if (y == 0.0f)
            {
                if (z == 0.5f)
                {
                    if (w == 0.0f)
                    {
                        // 1 AZ::Vector4( 0.5f, 0.0f, 0.5f, 0.0f )
                        return 1;
                    }
                    else if (w == 1.0f)
                    {
                        // 13 AZ::Vector4( 0.5f, 0.0f, 0.5f, 1.0f )
                        return 13;
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
            else if (z == 0.5f)
            {
                if (y == 0.5f)
                {
                    if (w == 0.5f)
                    {
                        // 5 AZ::Vector4( 0.5f, 0.5f, 0.5f, 0.5f )
                        return 5;
                    }
                    else
                    {
                        return -1;
                    }
                }
                else if (y == 1.0f)
                {
                    if (w == 1.0f)
                    {
                        // 9 AZ::Vector4( 0.5f, 1.0f, 0.5f, 1.0f )
                        return 9;
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
            else
            {
                return -1;
            }
        }
        else if (x == 1.0f)
        {
            if (z == 1.0f)
            {
                if (y == 0.0f)
                {
                    if (w == 0.0f)
                    {
                        // 2: AZ::Vector4( 1.0f, 0.0f, 1.0f, 0.0f )
                        return 2;
                    }
                    else if (w == 1.0f)
                    {
                        // 14: AZ::Vector4( 1.0f, 0.0f, 1.0f, 1.0f )
                        return 14;
                    }
                    else
                    {
                        return -1;
                    }
                }
                else if (y == 0.5f)
                {
                    if (w == 0.5f)
                    {
                        // 6: AZ::Vector4( 1.0f, 0.5f, 1.0f, 0.5f )
                        return 6;
                    }
                    else
                    {
                        return -1;
                    }
                }
                else if (y == 1.0f)
                {
                    if (w == 1.0f)
                    {
                        // 10: AZ::Vector4( 1.0f, 1.0f, 1.0f, 1.0f )
                        return 10;
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
}   // namespace AnchorPresets

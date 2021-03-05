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

#pragma once

#include <AzCore/base.h>

namespace AZ
{
    using FONT_TEXTURE_TYPE = uint8_t;

    // the number of slots in the glyph cache
    // each slot ocupies ((glyph_bitmap_width * glyph_bitmap_height) + 24) bytes
    #define AZ_FONT_GLYPH_CACHE_SIZE    (1)

    // the glyph spacing in font texels between characters in proportional font mode (more correct would be to take the value in the character)
    #define AZ_FONT_GLYPH_PROP_SPACING  (1)

    // the size of a rendered space, this value gets multiplied by the default characted width
    #define AZ_FONT_SPACE_SIZE          (0.5f)

    // don't draw this char (used to avoid drawing color codes)
    #define AZ_FONT_NOT_DRAWABLE_CHAR   (0xffff)

    // smoothing methods
    enum class FontSmoothMethod
    {
        None = 0,
        Blur = 1,
        SuperSample = 2,
    };

    // smoothing amounts
    enum class FontSmoothAmount
    {
        None = 0,
        x2 = 1,
        x4 = 2,
    };
};
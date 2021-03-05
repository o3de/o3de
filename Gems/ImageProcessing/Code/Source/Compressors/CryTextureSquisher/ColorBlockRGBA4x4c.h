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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once


#include "ColorTypes.h"

namespace ImageProcessing
{
    // Uncompressed 4x4 color block of 8bit integers.
    struct ColorBlockRGBA4x4c
    {
        ColorBlockRGBA4x4c()
        {
        }

        void setRGBA8(const void* imgBGRA8, unsigned int width, unsigned int height, unsigned int pitch, unsigned int x, unsigned int y);
        void getRGBA8(void* imgBGRA8, unsigned int width, unsigned int height, unsigned int pitch, unsigned int x, unsigned int y);

        void setA8(const void* imgA8, unsigned int width, unsigned int height, unsigned int pitch, unsigned int x, unsigned int y);
        void getA8(void* imgA8, unsigned int width, unsigned int height, unsigned int pitch, unsigned int x, unsigned int y);

        bool isSingleColorIgnoringAlpha() const;

        const ColorRGBA8* colors() const
        {
            return m_color;
        }

        ColorRGBA8* colors()
        {
            return m_color;
        }

        ColorRGBA8 color(unsigned int i) const
        {
            return m_color[i];
        }

        ColorRGBA8& color(unsigned int i)
        {
            return m_color[i];
        }

    private:
        static const unsigned int COLOR_COUNT = 4 * 4;

        ColorRGBA8 m_color[COLOR_COUNT];
    };

} // namespace ImageProcessing

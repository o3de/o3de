/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once


#include "ColorTypes.h"

namespace ImageProcessingAtom
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
} // namespace ImageProcessingAtom

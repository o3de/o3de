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
    // Uncompressed 4x4 color block of 16bit integers.
    struct ColorBlockRGBA4x4s
    {
        ColorBlockRGBA4x4s()
        {
        }

        void setRGBA16(const void* imgRGBA16, unsigned int width, unsigned int height, unsigned int pitch, unsigned int x, unsigned int y);
        void getRGBA16(void* imgRGBA16, unsigned int width, unsigned int height, unsigned int pitch, unsigned int x, unsigned int y);

        void setA16(const void* imgA16, unsigned int width, unsigned int height, unsigned int pitch, unsigned int x, unsigned int y);
        void getA16(void* imgA16, unsigned int width, unsigned int height, unsigned int pitch, unsigned int x, unsigned int y);

        bool isSingleColorIgnoringAlpha() const;

        const ColorRGBA16* colors() const
        {
            return m_color;
        }

        ColorRGBA16* colors()
        {
            return m_color;
        }

        ColorRGBA16 color(unsigned int i) const
        {
            return m_color[i];
        }

        ColorRGBA16& color(unsigned int i)
        {
            return m_color[i];
        }

    private:
        static const unsigned int COLOR_COUNT = 4 * 4;

        ColorRGBA16 m_color[COLOR_COUNT];
    };
} // namespace ImageProcessingAtom

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include "ColorTypes.h"

namespace ImageProcessingAtom
{
    // Uncompressed 4x4 color block of single precision floating points.
    struct ColorBlockRGBA4x4f
    {
        ColorBlockRGBA4x4f()
        {
        }

        void setRGBAf(const void* imgARGBf, unsigned int width, unsigned int height, unsigned int pitch, unsigned int x, unsigned int y);
        void getRGBAf(void* imgARGBf, unsigned int width, unsigned int height, unsigned int pitch, unsigned int x, unsigned int y);

        void setAf(const void* imgAf, unsigned int width, unsigned int height, unsigned int pitch, unsigned int x, unsigned int y);
        void getAf(void* imgAf, unsigned int width, unsigned int height, unsigned int pitch, unsigned int x, unsigned int y);

        const ColorRGBAf* colors() const
        {
            return m_color;
        }

        ColorRGBAf* colors()
        {
            return m_color;
        }

        ColorRGBAf color(unsigned int i) const
        {
            return m_color[i];
        }

        ColorRGBAf& color(unsigned int i)
        {
            return m_color[i];
        }

    private:
        static const unsigned int COLOR_COUNT = 4 * 4;

        ColorRGBAf m_color[COLOR_COUNT];
    };
} // namespace ImageProcessingAtom

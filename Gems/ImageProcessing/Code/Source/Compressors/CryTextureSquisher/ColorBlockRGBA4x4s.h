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

} // namespace ImageProcessing

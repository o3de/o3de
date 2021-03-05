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

#include <ImageProcessing_precompiled.h>

#include <AzCore/Math/MathUtils.h>

#include "ColorBlockRGBA4x4s.h"

namespace ImageProcessing
{
    static_assert(sizeof(int) == 4, "Expected size of int to be 4 bytes!");

    void ColorBlockRGBA4x4s::setRGBA16(const void* imgBGRA16, unsigned int const width, unsigned int const height, unsigned int const pitch, unsigned int x, unsigned int y)
    {
        AZ_Assert(imgBGRA16, "%s: Unexpected image pointer", __FUNCTION__);
        AZ_Assert((width & 3) == 0, "%s: Unexpected image width", __FUNCTION__);
        AZ_Assert((height & 3) == 0, "%s: Unexpected image height", __FUNCTION__);
        AZ_Assert(pitch >= width * sizeof(ColorRGBA16), "%s: Unexpected image pitch", __FUNCTION__);
        AZ_Assert(x < width, "%s: Unexpected pixel position x", __FUNCTION__);
        AZ_Assert(y < height, "%s: Unexpected pixel position y", __FUNCTION__);

        const unsigned int bw = AZ::GetMin(width  - x, 4U);
        const unsigned int bh = AZ::GetMin(height - y, 4U);

        // note: it's allowed for source data to be not aligned to 4 byte boundary
        // (so, we cannot cast source data pointer to ColorBGRA16* in code below)

        if ((bw == 4) && (bh == 4))
        {
            for (unsigned int row = 0; row < 4; ++row)
            {
                const uint16* const pSrc = (const uint16*)(((const uint8*)imgBGRA16) + (pitch * (y + row)) + (x * sizeof(ColorRGBA16)));

                ColorRGBA16* const pDst = &m_color[row << 2];

                pDst[0].setRGBA(&pSrc[0 * sizeof(ColorRGBA16) / sizeof(*pSrc)]);
                pDst[1].setRGBA(&pSrc[1 * sizeof(ColorRGBA16) / sizeof(*pSrc)]);
                pDst[2].setRGBA(&pSrc[2 * sizeof(ColorRGBA16) / sizeof(*pSrc)]);
                pDst[3].setRGBA(&pSrc[3 * sizeof(ColorRGBA16) / sizeof(*pSrc)]);
            }
        }
        else
        {
            // Rare case: block is smaller than 4x4.
            // Let's repeat pixels in this case.
            // It will keep frequency of colors, except the case
            // when width and/or height equals 3. But, this case
            // is very rare because images usually are "power of 2" sized, and even
            // if they are not, nobody will notice that the resulting encoding
            // for such block is not ideal.

            static unsigned int remainder[] =
            {
                0, 0, 0, 0,
                0, 1, 0, 1,
                0, 1, 2, 0,
                0, 1, 2, 3,
            };

            for (unsigned int row = 0; row < 4; ++row)
            {
                const unsigned int by = remainder[(bh - 1) * 4 + row];
                const uint16* const pSrc = (const uint16*)(((const uint8*)imgBGRA16) + pitch * (y + by));

                ColorRGBA16* pDst = &m_color[row * 4];

                for (unsigned int col = 0; col < 4; ++col)
                {
                    const unsigned int bx = remainder[(bw - 1) * 4 + col];

                    pDst[col].setRGBA(&pSrc[(x + bx) * sizeof(ColorRGBA16) / sizeof(*pSrc)]);
                }
            }
        }
    }

    void ColorBlockRGBA4x4s::getRGBA16(void* imgBGRA16, unsigned int const width, unsigned int const height, unsigned int const pitch, unsigned int x, unsigned int y)
    {
        AZ_Assert(imgBGRA16, "%s: Unexpected image pointer", __FUNCTION__);
        AZ_Assert((width & 3) == 0, "%s: Unexpected image width", __FUNCTION__);
        AZ_Assert((height & 3) == 0, "%s: Unexpected image height", __FUNCTION__);
        AZ_Assert(pitch >= width * sizeof(ColorRGBA16), "%s: Unexpected image pitch", __FUNCTION__);
        AZ_Assert(x < width, "%s: Unexpected pixel position x", __FUNCTION__);
        AZ_Assert(y < height, "%s: Unexpected pixel position y", __FUNCTION__);

        const unsigned int bw = AZ::GetMin(width  - x, 4U);
        const unsigned int bh = AZ::GetMin(height - y, 4U);

        // note: it's allowed for source data to be not aligned to 4 byte boundary
        // (so, we cannot cast source data pointer to ColorBGRA16* in code below)

        if ((bw == 4) && (bh == 4))
        {
            for (unsigned int row = 0; row < 4; ++row)
            {
                uint16* const pDst = (uint16*)(((uint8*)imgBGRA16) + (pitch * (y + row)) + (x * sizeof(ColorRGBA16)));

                const ColorRGBA16* const pSrc = &m_color[row << 2];

                pSrc[0].getRGBA(&pDst[0 * sizeof(ColorRGBA16) / sizeof(*pDst)]);
                pSrc[1].getRGBA(&pDst[1 * sizeof(ColorRGBA16) / sizeof(*pDst)]);
                pSrc[2].getRGBA(&pDst[2 * sizeof(ColorRGBA16) / sizeof(*pDst)]);
                pSrc[3].getRGBA(&pDst[3 * sizeof(ColorRGBA16) / sizeof(*pDst)]);
            }
        }
        else
        {
            // Rare case: block is smaller than 4x4.
            // Let's repeat pixels in this case.
            // It will keep frequency of colors, except the case
            // when width and/or height equals 3. But, this case
            // is very rare because images usually are "power of 2" sized, and even
            // if they are not, nobody will notice that the resulting encoding
            // for such block is not ideal.

            static unsigned int remainder[] =
            {
                0, 0, 0, 0,
                0, 1, 0, 1,
                0, 1, 2, 0,
                0, 1, 2, 3,
            };

            for (unsigned int row = 0; row < 4; ++row)
            {
                const unsigned int by = remainder[(bh - 1) * 4 + row];
                uint16* const pDst = (uint16*)(((uint8*)imgBGRA16) + pitch * (y + by));

                const ColorRGBA16* const pSrc = &m_color[row * 4];

                for (unsigned int col = 0; col < 4; ++col)
                {
                    const unsigned int bx = remainder[(bw - 1) * 4 + col];

                    pSrc[col].getRGBA(&pDst[(x + bx) * sizeof(ColorRGBA16) / sizeof(*pSrc)]);
                }
            }
        }
    }

    void ColorBlockRGBA4x4s::setA16(const void* imgA16, unsigned int const width, unsigned int const height, unsigned int const pitch, unsigned int x, unsigned int y)
    {
        AZ_Assert(imgA16, "%s: Unexpected image pointer", __FUNCTION__);
        AZ_Assert((width & 3) == 0, "%s: Unexpected image width", __FUNCTION__);
        AZ_Assert((height & 3) == 0, "%s: Unexpected image height", __FUNCTION__);
        AZ_Assert(pitch >= width * sizeof(uint8), "%s: Unexpected image pitch", __FUNCTION__);
        AZ_Assert(x < width, "%s: Unexpected pixel position x", __FUNCTION__);
        AZ_Assert(y < height, "%s: Unexpected pixel position y", __FUNCTION__);

        const unsigned int bw = AZ::GetMin(width  - x, 4U);
        const unsigned int bh = AZ::GetMin(height - y, 4U);

        // note: it's allowed for source data to be not aligned to 4 byte boundary
        // (so, we cannot cast source data pointer to ColorBGRA16* in code below)

        if ((bw == 4) && (bh == 4))
        {
            for (unsigned int row = 0; row < 4; ++row)
            {
                const uint16* const pSrc = (const uint16*)(((const uint8*)imgA16) + (pitch * (y + row)) + (x * sizeof(uint16)));

                ColorRGBA16* const pDst = &m_color[row << 2];

                pDst[0].setRGBA(0, 0, 0, pSrc[0]);
                pDst[1].setRGBA(0, 0, 0, pSrc[1]);
                pDst[2].setRGBA(0, 0, 0, pSrc[2]);
                pDst[3].setRGBA(0, 0, 0, pSrc[3]);
            }
        }
        else
        {
            // Rare case: block is smaller than 4x4.
            // Let's repeat pixels in this case.
            // It will keep frequency of colors, except the case
            // when width and/or height equals 3. But, this case
            // is very rare because images usually are "power of 2" sized, and even
            // if they are not, nobody will notice that the resulting encoding
            // for such block is not ideal.

            static unsigned int remainder[] =
            {
                0, 0, 0, 0,
                0, 1, 0, 1,
                0, 1, 2, 0,
                0, 1, 2, 3,
            };

            for (unsigned int row = 0; row < 4; ++row)
            {
                const unsigned int by = remainder[(bh - 1) * 4 + row];
                const uint16* const pSrc = (const uint16*)(((const uint8*)imgA16) + pitch * (y + by));

                ColorRGBA16* pDst = &m_color[row * 4];

                for (unsigned int col = 0; col < 4; ++col)
                {
                    const unsigned int bx = remainder[(bw - 1) * 4 + col];

                    pDst[col].setRGBA(0, 0, 0, pSrc[x + bx]);
                }
            }
        }
    }

    void ColorBlockRGBA4x4s::getA16(void* imgA16, unsigned int const width, unsigned int const height, unsigned int const pitch, unsigned int x, unsigned int y)
    {
        AZ_Assert(imgA16, "%s: Unexpected image pointer", __FUNCTION__);
        AZ_Assert((width & 3) == 0, "%s: Unexpected image width", __FUNCTION__);
        AZ_Assert((height & 3) == 0, "%s: Unexpected image height", __FUNCTION__);
        AZ_Assert(pitch >= width * sizeof(uint8), "%s: Unexpected image pitch", __FUNCTION__);
        AZ_Assert(x < width, "%s: Unexpected pixel position x", __FUNCTION__);
        AZ_Assert(y < height, "%s: Unexpected pixel position y", __FUNCTION__);

        const unsigned int bw = AZ::GetMin(width  - x, 4U);
        const unsigned int bh = AZ::GetMin(height - y, 4U);

        // note: it's allowed for source data to be not aligned to 4 byte boundary
        // (so, we cannot cast source data pointer to ColorBGRA16* in code below)

        if ((bw == 4) && (bh == 4))
        {
            for (unsigned int row = 0; row < 4; ++row)
            {
                uint16* const pDst = (uint16*)(((uint8*)imgA16) + (pitch * (y + row)) + (x * sizeof(uint16)));
                uint16 r, g, b;

                const ColorRGBA16* const pSrc = &m_color[row << 2];

                pSrc[0].getRGBA(r, g, b, pDst[0]);
                pSrc[1].getRGBA(r, g, b, pDst[1]);
                pSrc[2].getRGBA(r, g, b, pDst[2]);
                pSrc[3].getRGBA(r, g, b, pDst[3]);
            }
        }
        else
        {
            // Rare case: block is smaller than 4x4.
            // Let's repeat pixels in this case.
            // It will keep frequency of colors, except the case
            // when width and/or height equals 3. But, this case
            // is very rare because images usually are "power of 2" sized, and even
            // if they are not, nobody will notice that the resulting encoding
            // for such block is not ideal.

            static unsigned int remainder[] =
            {
                0, 0, 0, 0,
                0, 1, 0, 1,
                0, 1, 2, 0,
                0, 1, 2, 3,
            };

            for (unsigned int row = 0; row < 4; ++row)
            {
                const unsigned int by = remainder[(bh - 1) * 4 + row];
                uint16* const pDst = (uint16*)(((uint8*)imgA16) + pitch * (y + by));
                uint16 r, g, b;

                const ColorRGBA16* const pSrc = &m_color[row * 4];

                for (unsigned int col = 0; col < 4; ++col)
                {
                    const unsigned int bx = remainder[(bw - 1) * 4 + col];

                    pSrc[col].getRGBA(r, g, b, pDst[x + bx]);
                }
            }
        }
    }

    bool ColorBlockRGBA4x4s::isSingleColorIgnoringAlpha() const
    {
        for (unsigned int i = 1; i < COLOR_COUNT; ++i)
        {
            if ((m_color[0].b != m_color[i].b) ||
                (m_color[0].g != m_color[i].g) ||
                (m_color[0].r != m_color[i].r))
            {
                return false;
            }
        }

        return true;
    }

} // namespace ImageProcessing

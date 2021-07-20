/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */



#include <AzCore/Math/MathUtils.h>

#include "ColorBlockRGBA4x4f.h"

namespace ImageProcessingAtom
{
    static_assert(sizeof(int) == 4, "Expected size of int to be 4 bytes!");

    void ColorBlockRGBA4x4f::setRGBAf(const void* imgRGBAf, unsigned int const width, unsigned int const height, unsigned int const pitch, unsigned int x, unsigned int y)
    {
        AZ_Assert(imgRGBAf, "%s: Unexpected image pointer", __FUNCTION__);
        AZ_Assert((width & 3) == 0, "%s: Unexpected image width", __FUNCTION__);
        AZ_Assert((height & 3) == 0, "%s: Unexpected image height", __FUNCTION__);
        AZ_Assert(pitch >= width * sizeof(ColorRGBAf), "%s: Unexpected image pitch", __FUNCTION__);
        AZ_Assert(x < width, "%s: Unexpected pixel position x", __FUNCTION__);
        AZ_Assert(y < height, "%s: Unexpected pixel position y", __FUNCTION__);

        const unsigned int bw = AZ::GetMin(width  - x, 4U);
        const unsigned int bh = AZ::GetMin(height - y, 4U);

        // note: it's allowed for source data to be not aligned to 4 byte boundary
        // (so, we cannot cast source data pointer to ColorRGBAf* in code below)

        if ((bw == 4) && (bh == 4))
        {
            for (unsigned int row = 0; row < 4; ++row)
            {
                const float* const pSrc = (const float*)(((const uint8*)imgRGBAf) + (pitch * (y + row)) + (x * sizeof(ColorRGBAf)));

                ColorRGBAf* const pDst = &m_color[row << 2];

                pDst[0].setRGBA(&pSrc[0 * sizeof(ColorRGBAf) / sizeof(*pSrc)]);
                pDst[1].setRGBA(&pSrc[1 * sizeof(ColorRGBAf) / sizeof(*pSrc)]);
                pDst[2].setRGBA(&pSrc[2 * sizeof(ColorRGBAf) / sizeof(*pSrc)]);
                pDst[3].setRGBA(&pSrc[3 * sizeof(ColorRGBAf) / sizeof(*pSrc)]);
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
                const float* const pSrc = (const float*)(((const uint8*)imgRGBAf) + pitch * (y + by));

                ColorRGBAf* pDst = &m_color[row * 4];

                for (unsigned int col = 0; col < 4; ++col)
                {
                    const unsigned int bx = remainder[(bw - 1) * 4 + col];

                    pDst[col].setRGBA(&pSrc[(x + bx) * sizeof(ColorRGBAf) / sizeof(*pSrc)]);
                }
            }
        }
    }

    void ColorBlockRGBA4x4f::getRGBAf(void* imgRGBAf, unsigned int const width, unsigned int const height, unsigned int const pitch, unsigned int x, unsigned int y)
    {
        AZ_Assert(imgRGBAf, "%s: Unexpected image pointer", __FUNCTION__);
        AZ_Assert((width & 3) == 0, "%s: Unexpected image width", __FUNCTION__);
        AZ_Assert((height & 3) == 0, "%s: Unexpected image height", __FUNCTION__);
        AZ_Assert(pitch >= width * sizeof(ColorRGBAf), "%s: Unexpected image pitch", __FUNCTION__);
        AZ_Assert(x < width, "%s: Unexpected pixel position x", __FUNCTION__);
        AZ_Assert(y < height, "%s: Unexpected pixel position y", __FUNCTION__);

        const unsigned int bw = AZ::GetMin(width  - x, 4U);
        const unsigned int bh = AZ::GetMin(height - y, 4U);

        // note: it's allowed for source data to be not aligned to 4 byte boundary
        // (so, we cannot cast source data pointer to ColorRGBAf* in code below)

        if ((bw == 4) && (bh == 4))
        {
            for (unsigned int row = 0; row < 4; ++row)
            {
                float* const pDst = (float*)(((uint8*)imgRGBAf) + (pitch * (y + row)) + (x * sizeof(ColorRGBAf)));

                const ColorRGBAf* const pSrc = &m_color[row << 2];

                pSrc[0].getRGBA(&pDst[0 * sizeof(ColorRGBAf) / sizeof(*pDst)]);
                pSrc[1].getRGBA(&pDst[1 * sizeof(ColorRGBAf) / sizeof(*pDst)]);
                pSrc[2].getRGBA(&pDst[2 * sizeof(ColorRGBAf) / sizeof(*pDst)]);
                pSrc[3].getRGBA(&pDst[3 * sizeof(ColorRGBAf) / sizeof(*pDst)]);
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
                float* const pDst = (float*)(((uint8*)imgRGBAf) + pitch * (y + by));

                const ColorRGBAf* const pSrc = &m_color[row * 4];

                for (unsigned int col = 0; col < 4; ++col)
                {
                    const unsigned int bx = remainder[(bw - 1) * 4 + col];

                    pSrc[col].getRGBA(&pDst[(x + bx) * sizeof(ColorRGBAf) / sizeof(*pDst)]);
                }
            }
        }
    }

    void ColorBlockRGBA4x4f::setAf(const void* imgAf, unsigned int const width, unsigned int const height, unsigned int const pitch, unsigned int x, unsigned int y)
    {
        AZ_Assert(imgAf, "%s: Unexpected image pointer", __FUNCTION__);
        AZ_Assert((width & 3) == 0, "%s: Unexpected image width", __FUNCTION__);
        AZ_Assert((height & 3) == 0, "%s: Unexpected image height", __FUNCTION__);
        AZ_Assert(pitch >= width * sizeof(float), "%s: Unexpected image pitch", __FUNCTION__);
        AZ_Assert(x < width, "%s: Unexpected pixel position x", __FUNCTION__);
        AZ_Assert(y < height, "%s: Unexpected pixel position y", __FUNCTION__);

        const unsigned int bw = AZ::GetMin(width  - x, 4U);
        const unsigned int bh = AZ::GetMin(height - y, 4U);

        // note: it's allowed for source data to be not aligned to 4 byte boundary
        // (so, we cannot cast source data pointer to ColorRGBAf* in code below)

        if ((bw == 4) && (bh == 4))
        {
            for (unsigned int row = 0; row < 4; ++row)
            {
                const float* const pSrc = (const float*)(((const uint8*)imgAf) + (pitch * (y + row)) + (x * sizeof(float)));

                ColorRGBAf* const pDst = &m_color[row << 2];

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
                const float* const pSrc = (const float*)(((const uint8*)imgAf) + (pitch * (y + by)));

                ColorRGBAf* pDst = &m_color[row * 4];

                for (unsigned int col = 0; col < 4; ++col)
                {
                    const unsigned int bx = remainder[(bw - 1) * 4 + col];

                    pDst[col].setRGBA(0, 0, 0, pSrc[x + bx]);
                }
            }
        }
    }

    void ColorBlockRGBA4x4f::getAf(void* imgAf, unsigned int const width, unsigned int const height, unsigned int const pitch, unsigned int x, unsigned int y)
    {
        AZ_Assert(imgAf, "%s: Unexpected image pointer", __FUNCTION__);
        AZ_Assert((width & 3) == 0, "%s: Unexpected image width", __FUNCTION__);
        AZ_Assert((height & 3) == 0, "%s: Unexpected image height", __FUNCTION__);
        AZ_Assert(pitch >= width * sizeof(float), "%s: Unexpected image pitch", __FUNCTION__);
        AZ_Assert(x < width, "%s: Unexpected pixel position x", __FUNCTION__);
        AZ_Assert(y < height, "%s: Unexpected pixel position y", __FUNCTION__);

        const unsigned int bw = AZ::GetMin(width  - x, 4U);
        const unsigned int bh = AZ::GetMin(height - y, 4U);

        // note: it's allowed for source data to be not aligned to 4 byte boundary
        // (so, we cannot cast source data pointer to ColorRGBAf* in code below)

        if ((bw == 4) && (bh == 4))
        {
            for (unsigned int row = 0; row < 4; ++row)
            {
                float* const pDst = (float*)(((uint8*)imgAf) + (pitch * (y + row)) + (x * sizeof(float)));
                float r, g, b;

                const ColorRGBAf* const pSrc = &m_color[row << 2];

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
                float* const pDst = (float*)(((uint8*)imgAf) + pitch * (y + by));
                float r, g, b;

                const ColorRGBAf* const pSrc = &m_color[row * 4];

                for (unsigned int col = 0; col < 4; ++col)
                {
                    const unsigned int bx = remainder[(bw - 1) * 4 + col];

                    pSrc[col].getRGBA(r, g, b, pDst[x + bx]);
                }
            }
        }
    }
} // namespace ImageProcessingAtom

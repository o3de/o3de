/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Purpose:
//  - Hold a glyph bitmap and blit it to the main texture

#include <AtomLyIntegration/AtomFont/GlyphBitmap.h>
#include <math.h>
#include <CryCommon/Cry_Math.h>

//-------------------------------------------------------------------------------------------------
AZ::GlyphBitmap::GlyphBitmap()
    : m_width(0)
    , m_height(0)
    , m_buffer(nullptr)
{
}

//-------------------------------------------------------------------------------------------------
AZ::GlyphBitmap::~GlyphBitmap()
{
}

//-------------------------------------------------------------------------------------------------
int AZ::GlyphBitmap::Create(int width, int height)
{
    Release();

    m_buffer = AZStd::unique_ptr<uint8_t[]>(new uint8_t[width * height]);

    if (!m_buffer)
    {
        return 0;
    }

    m_width = width;
    m_height = height;

    return 1;
}

//-------------------------------------------------------------------------------------------------
int AZ::GlyphBitmap::Release()
{
    m_buffer = nullptr;
    m_width = m_height = 0;

    return 1;
}

//-------------------------------------------------------------------------------------------------
int AZ::GlyphBitmap::Blur(AZ::FontSmoothAmount smoothAmount)
{
    int iterationCount = 0;
    switch(smoothAmount)
    {
        case AZ::FontSmoothAmount::x2:
            iterationCount = 1;
            break;
        case AZ::FontSmoothAmount::x4:
            iterationCount = 2;
            break;
    }
    int colorSum;
    int yOffset;
    int yUpOffset;
    int yDownOffset;

    for (int i = 0; i < iterationCount; i++)
    {
        for (int y = 0; y < m_height; y++)
        {
            yOffset = y * m_width;

            if (y - 1 >= 0)
            {
                yUpOffset = (y - 1) * m_width;
            }
            else
            {
                yUpOffset = (y) * m_width;
            }

            if (y + 1 < m_height)
            {
                yDownOffset = (y + 1) * m_width;
            }
            else
            {
                yDownOffset = (y) * m_width;
            }

            for (int x = 0; x < m_width; x++)
            {
                colorSum = m_buffer[yUpOffset + x] + m_buffer[yDownOffset + x];

                if (x - 1 >= 0)
                {
                    colorSum += m_buffer[yOffset + x - 1];
                }
                else
                {
                    colorSum += m_buffer[yOffset + x];
                }

                if (x + 1 < m_width)
                {
                    colorSum += m_buffer[yOffset + x + 1];
                }
                else
                {
                    colorSum += m_buffer[yOffset + x];
                }

                m_buffer[yOffset + x] = static_cast<uint8_t>(colorSum >> 2);
            }
        }
    }

    return 1;
}

//-------------------------------------------------------------------------------------------------
int AZ::GlyphBitmap::Clear()
{
    memset(m_buffer.get(), 0, m_width * m_height);

    return 1;
}

//-------------------------------------------------------------------------------------------------
int AZ::GlyphBitmap::BlitTo8(unsigned char* destBuffer, int srcX, int srcY, int srcWidth, int srcHeight, int destX, int destY, int destWidth)
{
    int  ySrcOffset;
    int  yDestOffset;

    for (int y = 0; y < srcHeight; y++)
    {
        ySrcOffset = (srcY + y) * m_width;
        yDestOffset = (destY + y) * destWidth;

        for (int x = 0; x < srcWidth; x++)
        {
            destBuffer[yDestOffset + destX + x] = m_buffer[ySrcOffset + srcX + x];
        }
    }

    return 1;
}

//-------------------------------------------------------------------------------------------------
int AZ::GlyphBitmap::BlitScaledTo8(unsigned char* destBuffer, [[maybe_unused]] int srcReadXOffset, int srcReadYOffset, int srcWidth, int srcHeight, int destX, [[maybe_unused]] int destY, int destWidth, int destHeight, int destBufferWidth)
{
    int newWidth = (int)destWidth;
    int newHeight = (int)destHeight;

    float destToSrcXScale = srcWidth / (float)newWidth;
    float destToSrcYScale = srcHeight / (float)newHeight;

    float srcReadX;
    float srcReadY;
    float srcReadXFraction;
    float srcReadYFraction;
    float oneMinusX;
    float oneMinusY;
    float fR0;
    float fR1;

    int srcReadXCeil;
    int srcReadYCeil;
    int srcReadXFloor;
    int srcReadYFloor;
    int destOffsetY;

    uint8_t color0;
    uint8_t color1;
    uint8_t color2;
    uint8_t color3;

    for (int y = 0; y < newHeight; ++y)
    {
        srcReadY = y * destToSrcYScale;
        srcReadYFloor = (int)floor_tpl(srcReadY);
        srcReadYCeil = srcReadYFloor + 1;

        srcReadYFraction = srcReadY - srcReadYFloor;
        oneMinusY = 1.0f - srcReadYFraction;

        destOffsetY = y * destBufferWidth;

        srcReadYFloor += srcReadYOffset;
        srcReadYCeil += srcReadYOffset;

        if (srcReadYCeil >= m_height)
        {
            srcReadYCeil = srcReadYFloor;
        }

        for (int x = 0; x < newWidth; ++x)
        {
            srcReadX = x * destToSrcXScale;
            srcReadXFloor = (int)floor_tpl(srcReadX);
            srcReadXCeil = srcReadXFloor + 1;

            srcReadXFraction = srcReadX - srcReadXFloor;
            oneMinusX = 1.0f - srcReadXFraction;

            // possible bug from Cry, using the y offset here
            srcReadXFloor += srcReadYOffset;
            srcReadXCeil += srcReadYOffset;

            if (srcReadXCeil >= m_width)
            {
                srcReadXCeil = srcReadXFloor;
            }

            color0 = m_buffer[srcReadYFloor * m_width + srcReadXFloor];
            color1 = m_buffer[srcReadYFloor * m_width + srcReadXCeil];
            color2 = m_buffer[srcReadYCeil * m_width + srcReadXFloor];
            color3 = m_buffer[srcReadYCeil * m_width + srcReadXCeil];

            fR0 = (oneMinusX * color0 + srcReadXFraction * color1);
            fR1 = (oneMinusX * color2 + srcReadXFraction * color3);

            destBuffer[destOffsetY + x + destX] = (unsigned char)((oneMinusY * fR0) + (srcReadYFraction * fR1));
        }
    }

    return 1;
}
//-------------------------------------------------------------------------------------------------

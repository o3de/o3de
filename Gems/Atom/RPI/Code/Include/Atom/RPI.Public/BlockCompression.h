/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Base.h>
#include <AzCore/Math/Color.h>

namespace AZ
{
    namespace RPI
    {
        // Helper structure for decoding BC1 block compression.
        // BC1 consists of 8-byte blocks that encode 16 pixels arranged in a 4x4 square. The first 4 bytes are 2 16-bit colors,
        // and the second 4 bytes contain 4 2-bit color indices that represent individual pixels.
        // The 2-bit indices represent the following:
        // 00 - use color0
        // 01 - use color1
        // 10 - if color0 > color1, use 2/3 color0 and 1/3 color1, else use 1/2 color0 and 1/2 color1
        // 11 - if color0 > color1, use 1/3 color0 and 2/3 color1, else use transparent black
        struct BC1Block
        {
            uint16_t m_color0;
            uint16_t m_color1;
            uint8_t m_colorIndices[4];

            // Each block is 8 bytes in size.
            static constexpr size_t BlockBytes = 8;

            // Each block is 4x4 pixels in size.
            static constexpr size_t BlockPixelWidth = 4;
            static constexpr size_t BlockPixelHeight = 4;

            // Given an image width, and an XY location, return a pair of indices.
            // The first index is a block index when walking through an array of blocks.
            // The second index is the specific pixel index (0-15) within that block.
            static AZStd::pair<size_t, size_t> GetBlockIndices(uint32_t width, uint32_t x, uint32_t y)
            {
                size_t blockWidth = width / BlockPixelWidth;
                uint32_t blockX = x / BlockPixelWidth;
                uint32_t blockY = y / BlockPixelHeight;

                size_t blockIndex = (blockY * blockWidth + blockX);
                size_t pixelIndex = ((y % BlockPixelHeight) * BlockPixelHeight) + (x % BlockPixelWidth);
                return AZStd::pair<size_t, size_t>(blockIndex, pixelIndex);
            }

            // Given an index into the 4x4 block, return the color value in the 0-1 range.
            AZ::Color GetBlockColor(size_t pixelIndex) const
            {
                AZ_Assert(pixelIndex < 16, "Unsupported pixel index for BC1: %zu", pixelIndex);
                // The pixels are in a 4x4 block, so first we get the row of 4 2-bit indices that have the pixel we want.
                uint8_t colorRowIndices = m_colorIndices[(pixelIndex / 4)];
                // Extract the 2-bit index by shifting down in multiples of 2 bits and masking.
                uint8_t colorIndex = (colorRowIndices >> (2 * (pixelIndex % 4))) & 0x03;

                auto extractColor = [](uint16_t compressedColor) -> AZ::Color
                {
                    return AZ::Color(
                        ((compressedColor >> 11) & 0x1F) / aznumeric_cast<float>(0x1F),
                        ((compressedColor >> 5) & 0x3F) / aznumeric_cast<float>(0x3F),
                        ((compressedColor >> 0) & 0x1F) / aznumeric_cast<float>(0x1F),
                        1.0f);
                };

                // Using the pixel's color index, return the proper color value.
                switch (colorIndex)
                {
                case 0:
                    // Index 0 uses color 0
                    return extractColor(m_color0);
                case 1:
                    // Index 1 uses color 1
                    return extractColor(m_color1);
                    break;
                case 2:
                    // Index 2 either uses 2/3 of color 0 and 1/3 of color 1 or 1/2 of color 0 and 1/2 of color 1
                    return (m_color0 > m_color1) ? extractColor(m_color0).Lerp(extractColor(m_color1), (1.0f / 3.0f))
                                                    : extractColor(m_color0).Lerp(extractColor(m_color1), (1.0f / 2.0f));
                case 3:
                    // Index 3 either uses 1/3 of color 0 and 2/3 of color 1 or transparent black
                    return (m_color0 > m_color1) ? extractColor(m_color0).Lerp(extractColor(m_color1), (2.0f / 3.0f))
                                                    : AZ::Color::CreateZero();
                }

                return AZ::Color::CreateZero();
            }
        };

        // Helper structure for decoding BC4 block compression.
        // BC4 consists of 8-byte blocks that encode 16 pixels arranged in a 4x4 square. The first 2 bytes are 2 8-bit greyscale values,
        // and the next 6 bytes contain 6 3-bit indices that represent individual pixels.
        // The 3-bit index values of 000 and 111 directly reference the two greyscale values. The other 6 values either
        // represent 6 interpolated values between the two greyscale values or 4 interpolated values plus black and white.
        struct BC4Block
        {
            union
            {
                uint64_t m_block;

                struct
                {
                    uint8_t m_color0;
                    uint8_t m_color1;
                    uint8_t m_colorIndices[6];
                };
            };

            // Each block is 8 bytes in size.
            static constexpr size_t BlockBytes = 8;

            // Each block is 4x4 pixels in size.
            static constexpr size_t BlockPixelWidth = 4;
            static constexpr size_t BlockPixelHeight = 4;

            // Given an image width, and an XY location, return a pair of indices.
            // The first index is a block index when walking through an array of blocks.
            // The second index is the specific pixel index (0-15) within that block.
            static AZStd::pair<size_t, size_t> GetBlockIndices(uint32_t width, uint32_t x, uint32_t y)
            {
                size_t blockWidth = width / BlockPixelWidth;
                uint32_t blockX = x / BlockPixelWidth;
                uint32_t blockY = y / BlockPixelHeight;

                size_t blockIndex = (blockY * blockWidth + blockX);
                size_t pixelIndex = ((y % BlockPixelHeight) * BlockPixelHeight) + (x % BlockPixelWidth);
                return AZStd::pair<size_t, size_t>(blockIndex, pixelIndex);
            }

            // Given an index into the 4x4 block, return the color value in the 0-1 range.
            AZ::Color GetBlockColor(size_t pixelIndex) const
            {
                AZ_Assert(pixelIndex < 16, "Unsupported pixel index for BC4: %zu", pixelIndex);
                // Pull out the correct 3 bits from the BC4 block for this pixel.
                // The "16 +" is to skip over the two bytes of palette data to get to the start of the first pixel.
                uint8_t colorIndex = (m_block >> (16 + (3 * (pixelIndex & 0x0F)))) & 0x07;

                auto extractColor = [](uint8_t color) -> AZ::Color
                {
                    return AZ::Color(color / 255.0f, color / 255.0f, color / 255.0f, 1.0f);
                };

                if (m_color0 > m_color1)
                {
                    // When the first color palette entry is larger, the first two indices are the two palette entries,
                    // and the remaining 5 are interpolations from 1/7 to 6/7 between the palette entries.
                    switch (colorIndex)
                    {
                    case 0:
                        return extractColor(m_color0);
                    case 1:
                        return extractColor(m_color1);
                    case 2:
                    case 3:
                    case 4:
                    case 5:
                    case 6:
                    case 7:
                        // Index 2-7 lerps from 1/7 - 6/7 between color0 and color1
                        return extractColor(m_color0)
                            .Lerp(extractColor(m_color1), aznumeric_cast<float>(colorIndex - 1) / 7.0f);
                    }
                }
                else
                {
                    // When the second color palette entry is larger, the first two indices are the two palette entries,
                    // the next 4 are interpolations from 1/5 to 4/5 between the palette entries, and the last two are transparent
                    // black and opaque white.
                    switch (colorIndex)
                    {
                    case 0:
                        return extractColor(m_color0);
                    case 1:
                        return extractColor(m_color1);
                    case 2:
                    case 3:
                    case 4:
                    case 5:
                        // Index 2-5 lerps from 1/5 - 4/5 between color0 and color1
                        return extractColor(m_color0)
                            .Lerp(extractColor(m_color1), aznumeric_cast<float>(colorIndex - 1) / 5.0f);
                    case 6:
                        // Index 6 returns transparent black
                        return AZ::Color::CreateZero();
                    case 7:
                        // Index 7 returns opaque white
                        return AZ::Color::CreateOne();
                    }
                }

                return AZ::Color::CreateZero();
            }
        };


    }   // namespace RPI
}   // namespace AZ


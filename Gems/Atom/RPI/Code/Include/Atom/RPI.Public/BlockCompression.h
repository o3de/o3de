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
        // 10 - use 2/3 color0 and 1/3 color1
        // 11 - use 1/3 color0 and 2/3 color1
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
                    // Index 2 uses 2/3 of color 0 and 1/3 of color 1
                    return extractColor(m_color0).Lerp(extractColor(m_color1), (1.0f / 3.0f));
                case 3:
                    // Index 3 uses 1/3 of color 0 and 2/3 of color 1
                    return extractColor(m_color0).Lerp(extractColor(m_color1), (2.0f / 3.0f));
                }

                return AZ::Color::CreateZero();
            }
        };

    }   // namespace RPI
}   // namespace AZ


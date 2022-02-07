/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Purpose : Hold a glyph bitmap and blit it to the main texture

#pragma once

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AtomLyIntegration/AtomFont/FontCommon.h>

namespace AZ
{
    class GlyphBitmap
    {
    public:
        GlyphBitmap();
        ~GlyphBitmap();

        int Create(int width, int height);
        int Release();

        unsigned char* GetBuffer() { return m_buffer.get(); };

        int Blur(AZ::FontSmoothAmount smoothAmount);

        int Clear();

        int BlitTo8(unsigned char* buffer, int srcX, int srcY, int srcWidth, int srcHeight, int destX, int destY, int destWidth);

        int BlitScaledTo8(unsigned char* buffer, int srcX, int srcY, int srcWidth, int srcHeight, int destX, int destY, int destWidth, int destHeight, int destBufferWidth);

        int GetWidth() { return m_width; }
        int GetHeight() { return m_height; }

    private:

        AZStd::unique_ptr<uint8_t[]> m_buffer;
        int                          m_width;
        int                          m_height;
    };
}

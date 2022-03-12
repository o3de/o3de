/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

namespace AZ
{
    class FontBitmap
    {
    public:
        FontBitmap();
        ~FontBitmap();

        int Blur(int iterationCount);
        int Scale(float scaleX, float scaleY);

        int BlitFrom(FontBitmap* source, int srcX, int srcY, int destX, int destY, int width, int height);
        int BlitTo(FontBitmap* destination, int destX, int destY, int srcX, int srcY, int width, int height);

        int Create(int width, int height);
        int Release();

        int SaveBitmap(const AZStd::string& fileName);
        int Get32Bpp(unsigned int** buffer)
        {
            (*buffer) = new unsigned int[m_width * m_height];

            if (!(*buffer))
            {
                return 0;
            }

            int dataSize = m_width * m_height;

            for (int i = 0; i < dataSize; i++)
            {
                (*buffer)[i] = (m_data[i] << 24) | (m_data[i] << 16) | (m_data[i] << 8) | (m_data[i]);
            }

            return 1;
        }

        int GetWidth() { return m_width; }
        int GetHeight() { return m_height; }

        void SetRenderData(void* renderData) { m_renderData = renderData; };
        void* GetRenderData() { return m_renderData; };

        unsigned char* GetData() { return m_data; }

    public:

        int             m_width;
        int             m_height;

        unsigned char* m_data;
        void* m_renderData;
    };
}

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

// Purpose : Hold a glyph bitmap and blit it to the main texture

#ifndef CRYINCLUDE_CRYFONT_GLYPHBITMAP_H
#define CRYINCLUDE_CRYFONT_GLYPHBITMAP_H

#pragma once


class CGlyphBitmap
{
public:
    CGlyphBitmap();
    ~CGlyphBitmap();

    int Create(int iWidth, int iHeight);
    int Release();

    unsigned char* GetBuffer() { return m_pBuffer; };

    int Blur(int iIterations);
    int Scale(float fScaleX, float fScaleY);

    int Clear();

    int BlitTo8(unsigned char* pBuffer, int iSrcX, int iSrcY, int iSrcWidth, int iSrcHeight, int iDestX, int iDestY, int iDestWidth);
    int BlitTo32(unsigned int* pBuffer, int iSrcX, int iSrcY, int iSrcWidth, int iSrcHeight, int iDestX, int iDestY, int iDestWidth);

    int BlitScaledTo8(unsigned char* pBuffer, int iSrcX, int iSrcY, int iSrcWidth, int iSrcHeight, int iDestX, int iDestY, int iDestWidth, int iDestHeight, int iDestBufferWidth);
    int BlitScaledTo32(unsigned char* pBuffer, int iSrcX, int iSrcY, int iSrcWidth, int iSrcHeight, int iDestX, int iDestY, int iDestWidth, int iDestHeight, int iDestBufferWidth);

    int GetWidth() { return m_iWidth; }
    int GetHeight() { return m_iHeight; }

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(m_pBuffer, m_iWidth * m_iHeight);
    }

private:

    unsigned char* m_pBuffer;
    int             m_iWidth;
    int             m_iHeight;
};

#endif // CRYINCLUDE_CRYFONT_GLYPHBITMAP_H

// AMD AMDUtils code
// 
// Copyright(c) 2018 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once
#include "ImgLoader.h"

// Loads a JPEGs, PNGs, BMPs and any image the Windows Image Component can load.
// It even applies some alpha scaling to prevent cutouts to fade away when lower mips are used.

class WICLoader : public ImgLoader
{
public:
    ~WICLoader();
    bool Load(const char *pFilename, float cutOff, IMG_INFO *pInfo);
    // after calling Load, calls to CopyPixels return each time a lower mip level 
    void CopyPixels(void *pDest, uint32_t stride, uint32_t width, uint32_t height);
    void MipImage(uint32_t width, uint32_t height);
private:
    // scale alpha to prevent thinning when lower mips are used
    float GetAlphaCoverage(uint32_t width, uint32_t height, float scale, int cutoff);
    void ScaleAlpha(uint32_t width, uint32_t height, float scale);

    char *m_pData;

    float m_alphaTestCoverage;
    float m_cutOff;
};



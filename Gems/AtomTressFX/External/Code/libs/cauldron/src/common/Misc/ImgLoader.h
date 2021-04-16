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

#include <DXGIFormat.h>

struct IMG_INFO
{
    UINT32           width;
    UINT32           height;
    UINT32           depth;
    UINT32           arraySize;
    UINT32           mipMapCount;
    DXGI_FORMAT      format;
    UINT32           bitCount;
};

//Loads a Image file

class ImgLoader
{
public:
    virtual ~ImgLoader() {};
    virtual bool Load(const char *pFilename, float cutOff, IMG_INFO *pInfo) = 0;
    // after calling Load, calls to CopyPixels return each time a lower mip level 
    virtual void CopyPixels(void *pDest, uint32_t stride, uint32_t width, uint32_t height) = 0;
};


ImgLoader *GetImageLoader(const char *pFilename);



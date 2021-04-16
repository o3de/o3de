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

#include "stdafx.h"
#include "ImgLoader.h"
#include "DDSLoader.h"
#include "WICLoader.h"


ImgLoader *GetImageLoader(const char *pFilename)
{
    // get the last 4 char (assuming this is the file extension)
    size_t len = strlen(pFilename);
    char ext[5] = { 0 };
    for (int i = 0; i < 4; ++i)
    {
        ext[i] = tolower(pFilename[len - 4 + i]);
    }

    // check if the extension is known
    UINT32 ext4CC = *reinterpret_cast<const UINT32 *>(ext);
    if (ext4CC == 'sdd.')
    {
        return new DDSLoader();
    }
    else
    {
        return new WICLoader();
    }
}



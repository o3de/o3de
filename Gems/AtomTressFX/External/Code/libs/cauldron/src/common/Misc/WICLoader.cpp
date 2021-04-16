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
#include "WICLoader.h"
#include "wincodec.h"
#include "Misc/Misc.h"

static IWICImagingFactory *m_pWICFactory = NULL;

WICLoader::~WICLoader()
{
    free(m_pData);
}

bool WICLoader::Load(const char *pFilename, float cutOff, IMG_INFO *pInfo)
{
    HRESULT hr = S_OK;

    if (m_pWICFactory == NULL)
    {
        hr = CoInitialize(NULL);
        hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr,CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pWICFactory));
    }

    IWICStream* pWicStream;
    hr = m_pWICFactory->CreateStream(&pWicStream);

    wchar_t  uniName[1024];
    swprintf(uniName, 1024, L"%S", pFilename);
    hr = pWicStream->InitializeFromFilename(uniName, GENERIC_READ);
    //assert(hr == S_OK);
    if (hr != S_OK)
        return false;

    IWICBitmapDecoder *pBitmapDecoder;
    hr = m_pWICFactory->CreateDecoderFromStream(pWicStream, NULL, WICDecodeMetadataCacheOnDemand, &pBitmapDecoder);
    assert(hr == S_OK);

    IWICBitmapFrameDecode *pFrameDecode;
    hr = pBitmapDecoder->GetFrame(0, &pFrameDecode);
    assert(hr == S_OK);

    IWICFormatConverter *pIFormatConverter;
    hr = m_pWICFactory->CreateFormatConverter(&pIFormatConverter);
    assert(hr == S_OK);

    hr = pIFormatConverter->Initialize( pFrameDecode, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, NULL, 100.0, WICBitmapPaletteTypeCustom);
    assert(hr == S_OK);

    uint32_t width, height;
    pFrameDecode->GetSize(&width, &height);
    pFrameDecode->Release();

    int bufferSize = width * height * 4;
    m_pData = (char *)malloc(bufferSize);
    hr = pIFormatConverter->CopyPixels( NULL, width * 4, bufferSize, (BYTE*)m_pData);
    assert(hr == S_OK);

    // compute number of mips
    //
    uint32_t _width  = width;
    uint32_t _height = height;
    uint32_t mipCount = 0;
    for(;;)
    {        
        mipCount++;
        if (_width > 1) _width >>= 1;
        if (_height > 1) _height >>= 1;
        if (_width == 1 && _height == 1)
            break;
    }

    // fill img struct
    //
    pInfo->arraySize = 1;
    pInfo->width = width;
    pInfo->height = height;
    pInfo->depth = 1;
    pInfo->mipMapCount = mipCount;
    pInfo->bitCount = 32;
    pInfo->format = DXGI_FORMAT_R8G8B8A8_UNORM;

    // if there is a cut off, compute the alpha test coverage of the top mip
    // mip generation will try to match this value so objects dont get thinner
    // as they use lower mips
    m_cutOff = cutOff;
    if (m_cutOff < 1.0f)
        m_alphaTestCoverage = GetAlphaCoverage(width, height, 1.0f, (int)(255 * m_cutOff));
    else
        m_alphaTestCoverage = 1.0f;

    pIFormatConverter->Release();
    pBitmapDecoder->Release();
    pWicStream->Release();

    return true;
}

void WICLoader::CopyPixels(void *pDest, uint32_t stride, uint32_t bytesWidth, uint32_t height)
{
    for (uint32_t y = 0; y < height; y++)
    {
        memcpy((char*)pDest + y*stride, m_pData + y*bytesWidth, bytesWidth);
    }

    MipImage(bytesWidth / 4, height);
}

float WICLoader::GetAlphaCoverage(uint32_t width, uint32_t height, float scale, int cutoff)
{
    double val = 0;

    uint32_t *pImg = (uint32_t *)m_pData;

    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width; x++)
        {            
            uint8_t *pPixel = (uint8_t *)pImg++;

            int alpha = (int)(scale * (float)pPixel[3]);
            if (alpha > 255) alpha = 255;
            if (alpha <= cutoff)
                continue;

            val += alpha;
        }
    }

    return (float)(val / (height*width *255));
}

void WICLoader::ScaleAlpha(uint32_t width, uint32_t height, float scale)
{
    uint32_t *pImg = (uint32_t *)m_pData;

    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width; x++)
        {
            uint8_t *pPixel = (uint8_t *)pImg++;

            int alpha = (int)(scale * (float)pPixel[3]);
            if (alpha > 255) alpha = 255;

            pPixel[3] = alpha;
        }
    }
}

void WICLoader::MipImage(uint32_t width, uint32_t height)
{
    //compute mip so next call gets the lower mip    
    int offsetsX[] = { 0,1,0,1 };
    int offsetsY[] = { 0,0,1,1 };

    uint32_t *pImg = (uint32_t *)m_pData;

    #define GetByte(color, component) (((color) >> (8 * (component))) & 0xff)
    #define GetColor(ptr, x,y) (ptr[(x)+(y)*width])
    #define SetColor(ptr, x,y, col) ptr[(x)+(y)*width/2]=col;

    for (uint32_t y = 0; y < height; y+=2)
    {
        for (uint32_t x = 0; x < width; x+=2)
        {
            uint32_t ccc = 0;
            for (uint32_t c = 0; c < 4; c++)
            {
                uint32_t cc = 0;
                for (uint32_t i = 0; i < 4; i++)
                    cc += GetByte(GetColor(pImg, x + offsetsX[i], y + offsetsY[i]), 3-c);

                ccc = (ccc << 8) | (cc/4);
            }            
            SetColor(pImg, x / 2, y / 2, ccc);
        }
    }        


    // For cutouts we need we need to scale the alpha channel to match the coverage of the top MIP map
    // otherwise cutouts seem to get thinner when smaller mips are used
    // Credits: http://the-witness.net/news/2010/09/computing-alpha-mipmaps/
    //
    
    if (m_alphaTestCoverage < 1.0)
    {
        float ini = 0;
        float fin = 10;
        float mid;
        float alphaPercentage;
        int iter = 0;
        for(;iter<50;iter++)
        {
            mid = (ini + fin) / 2;
            alphaPercentage = GetAlphaCoverage(width / 2, height / 2, mid, (int)(m_cutOff * 255));

            if (fabs(alphaPercentage - m_alphaTestCoverage) < .001)
                break;

            if (alphaPercentage > m_alphaTestCoverage)
                fin = mid;
            if (alphaPercentage < m_alphaTestCoverage)
                ini = mid;
        }
        ScaleAlpha(width / 2, height / 2, mid);
        //Trace(format("(%4i x %4i), %f, %f, %i\n", width, height, alphaPercentage, 1.0f, 0));       
    }

}

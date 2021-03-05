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
#ifndef CRYINCLUDE_EDITOR_UTIL_IMAGEHISTOGRAM_H
#define CRYINCLUDE_EDITOR_UTIL_IMAGEHISTOGRAM_H
#pragma once

#include "Include/EditorCoreAPI.h"

class EDITOR_CORE_API CImageHistogram
{
public:
    static const int kNumChannels = 4;
    static const int kNumColorLevels = 256;

    enum EImageFormat
    {
        eImageFormat_8BPP,
        eImageFormat_24BPP_RGB,
        eImageFormat_24BPP_BGR,
        eImageFormat_32BPP_RGBA,
        eImageFormat_32BPP_BGRA,
        eImageFormat_32BPP_ARGB,
        eImageFormat_32BPP_ABGR
    };

    CImageHistogram();
    virtual ~CImageHistogram();

    // Description:
    //  Compute the histogram of an image
    // Arguments:
    //  pImageData - the image data
    //  aWidth - the width of the image in pixels
    //  aHeight - the height of the image in pixels
    //  aBitsPerPixel - the number of bits per pixel, currently supported: 8 (monochrome), 24 (RGB) and 32 (RGBA)
    void ComputeHistogram(BYTE* pImageData, unsigned int aWidth, unsigned int aHeight, EImageFormat aFormat = eImageFormat_32BPP_RGBA);
    void ClearHistogram();
    void CopyComputedDataFrom(CImageHistogram* histogram);

protected:
    void ComputeStatisticsForChannel(int aIndex);

public:
    unsigned int    m_count[kNumChannels][kNumColorLevels];
    unsigned int    m_lumCount[kNumColorLevels];
    unsigned int    m_maxCount[kNumChannels];
    unsigned int    m_maxLumCount;
    float           m_mean[kNumChannels], m_stdDev[kNumChannels], m_median[kNumChannels];
    float           m_meanAvg, m_stdDevAvg, m_medianAvg;
    EImageFormat    m_imageFormat;
};

#endif // CRYINCLUDE_EDITOR_UTIL_IMAGEHISTOGRAM_H

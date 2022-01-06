/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDefs.h"
#include "ImageHistogram.h"

CImageHistogram::CImageHistogram()
    : m_imageFormat(eImageFormat_32BPP_RGBA)
{
    ClearHistogram();
}

CImageHistogram::~CImageHistogram()
{
}

void CImageHistogram::ComputeHistogram(BYTE* pImageData, UINT aWidth, UINT aHeight, EImageFormat aFormat)
{
    ClearHistogram();
    UINT r, g, b, a;
    int lumIndex = 0;
    UINT pixelCount = aWidth * aHeight;

    m_imageFormat = aFormat;

    while (pixelCount--)
    {
        r = g = b = a = 0;

        switch (aFormat)
        {
        case eImageFormat_32BPP_RGBA:
        {
            r = *pImageData;
            g = *(pImageData + 1);
            b = *(pImageData + 2);
            a = *(pImageData + 3);
            pImageData += 4;
            break;
        }

        case eImageFormat_32BPP_BGRA:
        {
            b = *pImageData;
            g = *(pImageData + 1);
            r = *(pImageData + 2);
            a = *(pImageData + 3);
            pImageData += 4;
            break;
        }

        case eImageFormat_32BPP_ARGB:
        {
            a = *(pImageData);
            r = *(pImageData + 1);
            g = *(pImageData + 2);
            b = *(pImageData + 3);
            pImageData += 4;
            break;
        }

        case eImageFormat_32BPP_ABGR:
        {
            a = *(pImageData);
            b = *(pImageData + 1);
            g = *(pImageData + 2);
            r = *(pImageData + 3);
            pImageData += 4;
            break;
        }

        case eImageFormat_24BPP_RGB:
        {
            r = *pImageData;
            g = *(pImageData + 1);
            b = *(pImageData + 2);
            pImageData += 3;
            break;
        }

        case eImageFormat_24BPP_BGR:
        {
            r = *pImageData;
            g = *(pImageData + 1);
            b = *(pImageData + 2);
            pImageData += 3;
            break;
        }

        case eImageFormat_8BPP:
        {
            r = *pImageData;
            ++pImageData;
            break;
        }
        }

        ++m_count[0][r];
        ++m_count[1][g];
        ++m_count[2][b];
        ++m_count[3][a];

        lumIndex = (r + b + g) / 3;
        lumIndex = AZStd::clamp(lumIndex, 0, kNumColorLevels - 1);
        ++m_lumCount[lumIndex];

        if (m_maxCount[0] < m_count[0][r])
        {
            m_maxCount[0] = m_count[0][r];
        }

        if (m_maxCount[1] < m_count[1][g])
        {
            m_maxCount[1] = m_count[1][g];
        }

        if (m_maxCount[2] < m_count[2][b])
        {
            m_maxCount[2] = m_count[2][b];
        }

        if (m_maxCount[3] < m_count[3][a])
        {
            m_maxCount[3] = m_count[3][a];
        }

        if (m_maxLumCount < m_lumCount[lumIndex])
        {
            m_maxLumCount = m_lumCount[lumIndex];
        }
    }

    ComputeStatisticsForChannel(0);
    ComputeStatisticsForChannel(1);
    ComputeStatisticsForChannel(2);
    ComputeStatisticsForChannel(3);

    m_meanAvg = (m_mean[0] + m_mean[1] + m_mean[2]) / 3;
    m_stdDevAvg = (m_stdDev[0] + m_stdDev[1] + m_stdDev[2]) / 3;
    m_medianAvg = (m_median[0] + m_median[1] + m_median[2]) / 3;
}

void CImageHistogram::ClearHistogram()
{
    const int kSize = kNumColorLevels * sizeof(UINT);

    memset(m_count[0], 0, kSize);
    memset(m_count[1], 0, kSize);
    memset(m_count[2], 0, kSize);
    memset(m_count[3], 0, kSize);
    memset(m_lumCount, 0, kSize);
    m_maxCount[0] = 0;
    m_maxCount[1] = 0;
    m_maxCount[2] = 0;
    m_maxCount[3] = 0;
    m_maxLumCount = 0;
    memset(m_mean, 0, kNumChannels * sizeof(float));
    memset(m_stdDev, 0, kNumChannels * sizeof(float));
    memset(m_median, 0, kNumChannels * sizeof(float));
    m_meanAvg = m_stdDevAvg = m_medianAvg = 0;
}

void CImageHistogram::CopyComputedDataFrom(CImageHistogram* histogram)
{
    const int kSize = kNumColorLevels * sizeof(UINT);

    memcpy(m_count[0], histogram->m_count[0], kSize);
    memcpy(m_count[1], histogram->m_count[1], kSize);
    memcpy(m_count[2], histogram->m_count[2], kSize);
    memcpy(m_count[3], histogram->m_count[3], kSize);
    memcpy(m_lumCount, histogram->m_lumCount, kSize);
    memcpy(m_maxCount, histogram->m_maxCount, kNumChannels * sizeof(UINT));
    memcpy(m_mean, histogram->m_mean, kNumChannels * sizeof(float));
    memcpy(m_stdDev, histogram->m_stdDev, kNumChannels * sizeof(float));
    memcpy(m_median, histogram->m_median, kNumChannels * sizeof(float));
    m_maxLumCount = histogram->m_maxLumCount;
    m_meanAvg = histogram->m_meanAvg;
    m_stdDevAvg = histogram->m_stdDevAvg;
    m_medianAvg = histogram->m_medianAvg;
    m_imageFormat = histogram->m_imageFormat;
}

void CImageHistogram::ComputeStatisticsForChannel(int aIndex)
{
    int hits = 0;
    int total = 0;

    //
    // mean
    // std deviation
    //
    for (size_t i = 0; i < kNumColorLevels; ++i)
    {
        hits = m_count[aIndex][i];
        m_mean[aIndex] += i * hits;
        m_stdDev[aIndex] += i * i * hits;
        total += hits;
    }

    total = (total ? total : 1);
    m_mean[aIndex] = (float)m_mean[aIndex] / total;
    m_stdDev[aIndex] = m_stdDev[aIndex] / total - m_mean[aIndex] * m_mean[aIndex];
    m_stdDev[aIndex] = m_stdDev[aIndex] <= 0 ? 0 : sqrtf(m_stdDev[aIndex]);

    int halfTotal = total / 2;
    int median = 0, v = 0;

    // find median value
    for (; median < kNumColorLevels; ++median)
    {
        v += m_count[aIndex][median];

        if (v >= halfTotal)
        {
            break;
        }
    }

    m_median[aIndex] = static_cast<float>(median);
}

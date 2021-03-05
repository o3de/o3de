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

#ifndef CRYINCLUDE_CRYCOMMON_ICOLORGRADINGCONTROLLER_H
#define CRYINCLUDE_CRYCOMMON_ICOLORGRADINGCONTROLLER_H
#pragma once



struct SColorChartLayer
{
    int m_texID;
    float m_blendAmount;

    SColorChartLayer()
        : m_texID(-1)
        , m_blendAmount(-1)
    {
    }

    SColorChartLayer(int texID, float blendAmount)
        : m_texID(texID)
        , m_blendAmount(blendAmount)
    {
    }

    SColorChartLayer(const SColorChartLayer& rhs)
        : m_texID(rhs.m_texID)
        , m_blendAmount(rhs.m_blendAmount)
    {
    }
};


struct IColorGradingController
{
public:
    // <interfuscator:shuffle>
    virtual ~IColorGradingController(){}
    virtual int LoadColorChart(const char* pChartFilePath) const = 0;
    virtual int LoadDefaultColorChart() const = 0;
    virtual void UnloadColorChart(int texID) const = 0;

    virtual void SetLayers(const SColorChartLayer* pLayers, uint32 numLayers) = 0;
    // </interfuscator:shuffle>
};


#endif // CRYINCLUDE_CRYCOMMON_ICOLORGRADINGCONTROLLER_H

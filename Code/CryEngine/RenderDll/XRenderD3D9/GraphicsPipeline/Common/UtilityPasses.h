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

#pragma once

#include "FullscreenPass.h"


class CStretchRectPass
{
public:
    void Execute(CTexture* pSrcTex, CTexture* pDestTex);
    void Reset();

protected:
    CFullscreenPass  m_pass;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CGaussianBlurPass
{
public:
    CGaussianBlurPass()
        : m_scale(FLT_MIN)
        , m_distribution(FLT_MIN)
    {
    }
    void Execute(CTexture* pTex, CTexture* pTempTex, float scale, float distribution);
    void Reset();

protected:
    float GaussianDistribution1D(float x, float rho);
    void ComputeParams(int texWidth, int texHeight, int numSamples, float scale, float distribution);

protected:
    float            m_scale;
    float            m_distribution;
    Vec4             m_paramsH[16];
    Vec4             m_paramsV[16];
    Vec4             m_weights[16];

    CFullscreenPass  m_passH;
    CFullscreenPass  m_passV;
};

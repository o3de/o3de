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

// Description : Post processing RenderElement


#include "RenderDll_precompiled.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

CREPostProcess::CREPostProcess()
{
    mfSetType(eDATA_PostProcess);
    mfUpdateFlags(FCEF_TRANSFORM);
}

CREPostProcess::~CREPostProcess()
{
}

void CREPostProcess:: mfPrepare(bool bCheckOverflow)
{
    if (bCheckOverflow)
    {
        gRenDev->FX_CheckOverflow(0, 0, this);
    }
    gRenDev->m_RP.m_pRE = this;
    gRenDev->m_RP.m_RendNumIndices = 0;
    gRenDev->m_RP.m_RendNumVerts = 0;

    if (CRenderer::CV_r_PostProcessReset)
    {
        CRenderer::CV_r_PostProcessReset = 0;
        mfReset();
    }
}

void CREPostProcess::Reset(bool bOnSpecChange)
{
    if (PostEffectMgr())
    {
        PostEffectMgr()->Reset(bOnSpecChange);
    }
}

int CREPostProcess:: mfSetParameter(const char* pszParam, float fValue, bool bForceValue) const
{
    assert((pszParam) && "mfSetParameter: null parameter");

    CEffectParam* pParam = PostEffectMgr()->GetByName(pszParam);
    if (!pParam)
    {
        return 0;
    }

    pParam->SetParam(fValue, bForceValue);

    return 1;
}

void CREPostProcess:: mfGetParameter(const char* pszParam, float& fValue) const
{
    assert((pszParam) && "mfGetParameter: null parameter");

    CEffectParam* pParam = PostEffectMgr()->GetByName(pszParam);
    if (!pParam)
    {
        return;
    }

    fValue = pParam->GetParam();
}

int CREPostProcess:: mfSetParameterVec4(const char* pszParam, const Vec4& pValue, bool bForceValue) const
{
    assert((pszParam) && "mfSetParameter: null parameter");

    CEffectParam* pParam = PostEffectMgr()->GetByName(pszParam);
    if (!pParam)
    {
        return 0;
    }

    pParam->SetParamVec4(pValue, bForceValue);

    return 1;
}

void CREPostProcess:: mfGetParameterVec4(const char* pszParam, Vec4& pValue) const
{
    assert((pszParam) && "mfGetParameter: null parameter");

    CEffectParam* pParam = PostEffectMgr()->GetByName(pszParam);
    if (!pParam)
    {
        return;
    }

    pValue = pParam->GetParamVec4();
}

int CREPostProcess::mfSetParameterString(const char* pszParam, const char* pszArg) const
{
    assert((pszParam || pszArg) && "mfSetParameter: null parameter");

    CEffectParam* pParam = PostEffectMgr()->GetByName(pszParam);
    if (!pParam)
    {
        return 0;
    }

    pParam->SetParamString(pszArg);
    return 1;
}

void CREPostProcess::mfGetParameterString(const char* pszParam, const char*& pszArg) const
{
    assert((pszParam) && "mfGetParameter: null parameter");

    CEffectParam* pParam = PostEffectMgr()->GetByName(pszParam);
    if (!pParam)
    {
        return;
    }

    pszArg = pParam->GetParamString();
}

int32 CREPostProcess::mfGetPostEffectID(const char* pPostEffectName) const
{
    assert(pPostEffectName && "mfGetParameter: null parameter");

    return PostEffectMgr()->GetEffectID(pPostEffectName);
}

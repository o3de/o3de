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

#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_CHROMATICRING_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_CHROMATICRING_H
#pragma once

#include "OpticsElement.h"
#include "AbstractMeshElement.h"
#include "MeshUtil.h"

class CTexture;
class ChromaticRing
    : public COpticsElement
    , public AbstractMeshElement
{
private:
    bool m_bLockMovement : 1;

    _smart_ptr<CTexture> m_pSpectrumTex;
    bool m_bUseSpectrumTex : 1;

    int m_nPolyComplexity;
    int m_nColorComplexity;

    float m_fWidth;
    float m_fNoiseStrength;
    int m_nNoiseSeed;

    float m_fCompletionStart;
    float m_fCompletionEnd;
    float m_fCompletionFading;

protected:

#if defined(FLARES_SUPPORT_EDITING)
    void InitEditorParamGroups(AZStd::vector<FuncVariableGroup>& groups);
#endif
    void GenMesh()
    {
        ColorF c(1, 1, 1, 1);

        int polyComplexity(m_nPolyComplexity);
        if (CRenderer::CV_r_FlaresTessellationRatio < 1 && CRenderer::CV_r_FlaresTessellationRatio > 0)
        {
            polyComplexity = (int)((float)m_nPolyComplexity * CRenderer::CV_r_FlaresTessellationRatio);
        }

        MeshUtil::GenHoop(
            m_fSize, polyComplexity, m_fWidth, 2, c,
            m_fNoiseStrength * m_fSize, m_nNoiseSeed, m_fCompletionStart, m_fCompletionEnd, m_fCompletionFading,
            m_vertBuf, m_idxBuf);
    }
    void DrawMesh();
    void Invalidate()
    {
        m_meshDirty = true;
    }
    static float computeDynamicSize(const Vec3& vSrcProjPos, const float maxSize);

public:

    ChromaticRing(const char* name)
        : COpticsElement(name)
        , m_bUseSpectrumTex(false)
        , m_fWidth(0.5f)
        , m_nPolyComplexity(160)
        , m_nColorComplexity(2)
        , m_fNoiseStrength(0.0f)
        , m_fCompletionStart(90.f)
        , m_fCompletionEnd(270.f)
        , m_fCompletionFading(45.f)
    {
        SetSize(0.9f);
        SetAutoRotation(true);
        SetAspectRatioCorrection(false);

        m_meshDirty = true;
    }

    EFlareType GetType() { return eFT_ChromaticRing; }
    void Render(CShader* shader, Vec3 vSrcWorldPos, Vec3 vSrcProjPos, SAuxParams& aux);
    void Load(IXmlNode* pNode);

    void SetSize(float s)
    {
        COpticsElement::SetSize(s);
        m_meshDirty = true;
    }

    bool IsLockMovement() const { return m_bLockMovement; }
    void SetLockMovement(bool b) { m_bLockMovement = b; }

    int GetPolyComplexity() { return m_nPolyComplexity; }
    void SetPolyComplexity(int polyCplx)
    {
        if (polyCplx <= 0)
        {
            polyCplx = 1;
        }
        else if (polyCplx > 1024)
        {
            polyCplx = 1024;
        }
        m_nPolyComplexity = polyCplx;
        m_meshDirty = true;
    }

    int GetColorComplexity() { return m_nColorComplexity; }
    void SetColorComplexity(int clrCplx)
    {
        if (clrCplx <= 0)
        {
            clrCplx = 1;
        }
        m_nColorComplexity = clrCplx;
        m_meshDirty = true;
    }

    CTexture* GetSpectrumTex() { return m_pSpectrumTex; }
    void SetSpectrumTex(CTexture* tex)
    {
        m_pSpectrumTex = tex;
    }

    bool IsUsingSpectrumTex() { return m_bUseSpectrumTex; }
    void SetUsingSpectrumTex(bool b)
    {
        m_bUseSpectrumTex = b;
    }

    int GetNoiseSeed() { return m_nNoiseSeed; }
    void SetNoiseSeed(int seed)
    {
        m_nNoiseSeed = seed;
        m_meshDirty = true;
    }

    float GetWidth() { return m_fWidth; }
    void SetWidth(float w)
    {
        m_fWidth = w;
        m_meshDirty = true;
    }

    float GetNoiseStrength() { return m_fNoiseStrength; }
    void SetNoiseStrength(float noise)
    {
        m_fNoiseStrength = noise;
        m_meshDirty = true;
    }

    float GetCompletionFading() { return m_fCompletionFading; }
    void SetCompletionFading(float f)
    {
        m_fCompletionFading = f;
        m_meshDirty = true;
    }

    float GetCompletionSpanAngle() { return (m_fCompletionEnd - m_fCompletionStart); }
    void SetCompletionSpanAngle(float totalAngle)
    {
        float rotAngle = GetCompletionRotation();
        float halfTotalAngle = totalAngle / 2;
        m_fCompletionStart = rotAngle - halfTotalAngle;
        m_fCompletionEnd = rotAngle + halfTotalAngle;
        m_meshDirty = true;
    }

    float GetCompletionRotation() { return (m_fCompletionStart + m_fCompletionEnd) * 0.5f; }
    void SetCompletionRotation(float rot)
    {
        float oldRotAngle = GetCompletionRotation();
        float rotDiff = rot - oldRotAngle;
        m_fCompletionStart += rotDiff;
        m_fCompletionEnd += rotDiff;
        m_meshDirty = true;
    }

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this) + GetMeshDataSize());
    }
};

#endif // CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_CHROMATICRING_H


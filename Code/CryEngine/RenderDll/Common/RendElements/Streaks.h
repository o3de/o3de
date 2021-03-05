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

#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_STREAKS_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_STREAKS_H
#pragma once

#include "OpticsElement.h"
#include "AbstractMeshElement.h"
#include "MeshUtil.h"

class CTexture;
class Streaks
    : public COpticsElement
    , public AbstractMeshElement
{
private:
    _smart_ptr<CTexture> m_pSpectrumTex;
    bool m_bUseSpectrumTex;

    int m_nStreakCount;
    int m_nColorComplexity;

    // noise strengths
    float m_fSizeNoiseStrength;
    float m_fThicknessNoiseStrength;
    float m_fSpacingNoiseStrength;

    float m_fThickness;
    int m_nNoiseSeed;

protected:
    std::vector< std::vector<SVF_P3F_C4B_T2F> > m_separatedMeshList;
    std::vector< uint16 > m_separatedMeshIndices;

    // !Override
    virtual void GenMesh();
    virtual void ApplySingleMesh(int n);
    virtual void DrawMesh();
    void Invalidate()
    {
        m_meshDirty = true;
    }

public:
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // ctor
    Streaks (const char* name)
        : COpticsElement(name, 0.5f)
        , m_fThickness(0.3f)
        , m_nNoiseSeed(81)
        , m_fSizeNoiseStrength(0.8f)
        , m_fThicknessNoiseStrength(0.6f)
        , m_fSpacingNoiseStrength(0.2f)
        , m_bUseSpectrumTex(false)
    {
        m_vMovement.x = 1.f;
        m_vMovement.y = 1.f;
        m_Color.a = 1.f;

        SetAutoRotation(false);
        SetAspectRatioCorrection(true);
        SetColorComplexity(2);
        SetStreakCount(1);

        m_meshDirty = true;
    }

#if defined(FLARES_SUPPORT_EDITING)
    void InitEditorParamGroups(AZStd::vector<FuncVariableGroup>& groups);
#endif
    EFlareType GetType() { return eFT_Streaks; }
    void Render(CShader* shader, Vec3 vSrcWorldPos, Vec3 vSrcProjPos, SAuxParams& aux);
    void Load(IXmlNode* pNode);

    bool GetEnableSpectrumTex() const { return m_bUseSpectrumTex; }
    void SetEnableSpectrumTex(bool b) { m_bUseSpectrumTex = b; }

    CTexture* GetSpectrumTex() const { return m_pSpectrumTex; }
    void SetSpectrumTex(CTexture* tex)  { m_pSpectrumTex = tex; }

    int GetNoiseSeed() const { return m_nNoiseSeed; }
    void SetNoiseSeed(int seed)
    {
        m_nNoiseSeed = seed;
        m_meshDirty = true;
    }

    int GetStreakCount() const { return m_nStreakCount; }
    void SetStreakCount(int n)
    {
        m_nStreakCount = n;
        m_separatedMeshList.resize(n);
        m_meshDirty = true;
    }

    int GetColorComplexity() const { return m_nColorComplexity; }
    void SetColorComplexity(int n)
    {
        m_nColorComplexity = n;
        m_meshDirty = true;
    }

    float GetThickness() const { return m_fThickness; }
    void SetThickness(float f)
    {
        m_fThickness = f;
        m_meshDirty = true;
    }

    float GetThicknessNoise() const { return m_fThicknessNoiseStrength; }
    void SetThicknessNoise(float noise)
    {
        m_fThicknessNoiseStrength = noise;
        m_meshDirty = true;
    }

    float GetSizeNoise() const { return m_fSizeNoiseStrength; }
    void SetSizeNoise(float noise)
    {
        m_fSizeNoiseStrength = noise;
        m_meshDirty = true;
    }

    float GetSpacingNoise() const { return m_fSpacingNoiseStrength; }
    void SetSpacingNoise(float noise)
    {
        m_fSpacingNoiseStrength = noise;
        m_meshDirty = true;
    }

    void GetMemoryUsage(ICrySizer* pSizer) const;
};

#endif // CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_STREAKS_H

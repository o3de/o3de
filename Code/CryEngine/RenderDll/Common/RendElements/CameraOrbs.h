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

#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_CAMERAORBS_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_CAMERAORBS_H
#pragma once

#include "OpticsElement.h"
#include "AbstractMeshElement.h"

class CTexture;
class CameraOrbs
    : public COpticsElement
    , public AbstractMeshElement
{
private:
    _smart_ptr<CTexture> m_pOrbTex;
    _smart_ptr<CTexture> m_pLensTex;

    bool m_bUseLensTex : 1;
    bool m_bOrbDetailShading : 1;
    bool m_bLensDetailShading : 1;

    float m_fLensTexStrength;
    float m_fLensDetailShadingStrength;
    float m_fLensDetailBumpiness;

    bool m_bAdvancedShading : 1;
    ColorF m_cAmbientDiffuse;
    float m_fAbsorptance;
    float m_fTransparency;
    float m_fScatteringStrength;

    int m_nNumOrbs;
    float m_fIllumRadius;
    float m_fRotation;

    unsigned int m_iNoiseSeed;
    float m_fSizeNoise;
    float m_fBrightnessNoise;
    float m_fRotNoise;
    float m_fClrNoise;

    std::vector<SpritePoint> m_OrbsList;

    static const int MAX_ORBS_NUMBER = 10000;

protected:

#if defined(FLARES_SUPPORT_EDITING)
    void InitEditorParamGroups(AZStd::vector<FuncVariableGroup>& groups);
#endif
    void GenMesh();
    void DrawMesh(uint32 vOffset, uint32 iOffset);
    void Invalidate()
    {
        m_meshDirty = true;
    }

public:

    CameraOrbs (const char* name, const int numOrbs = 100)
        : COpticsElement(name, 0.19f)
        , m_fSizeNoise(0.8f)
        , m_fBrightnessNoise(0.4f)
        , m_fRotNoise(0.8f)
        , m_fClrNoise(0.5f)
        , m_fIllumRadius(1.f)
        , m_bUseLensTex(0)
        , m_bOrbDetailShading(0)
        , m_bLensDetailShading(0)
        , m_fLensTexStrength(1.f)
        , m_fLensDetailShadingStrength(0.157f)
        , m_fLensDetailBumpiness(0.073f)
        , m_bAdvancedShading(false)
        , m_cAmbientDiffuse(LensOpConst::_LO_DEF_CLR_BLK)
        , m_fAbsorptance(4.0f)
        , m_fTransparency(0.37f)
        , m_fScatteringStrength(1.0f)
        , m_iNoiseSeed(0)
    {
        m_Color.a = 1.f;
        SetPerspectiveFactor(0.f);
        SetRotation(0.7f);
        SetNumOrbs(numOrbs);
        m_meshDirty = true;
    }

protected:
    void ApplyOrbFlags(CShader* shader, bool detailShading) const;
    void ApplyLensDetailParams(CShader* shader, float texStength, float detailStrength, float bumpiness) const;
    void ApplyAdvancedShadingFlag(CShader* shader) const;
    void ApplyAdvancedShadingParams(CShader* shader, const ColorF& ambDiffuseRGBK, float absorptance, float transparency, float scattering) const;

public:

    void ScatterOrbs();

    int GetNumOrbs() const { return m_OrbsList.size(); }
    void SetNumOrbs(int n)
    {
        n = clamp_tpl<int>(n, 0, MAX_ORBS_NUMBER);
        if (n != m_nNumOrbs)
        {
            m_nNumOrbs = n;
            if (m_nNumOrbs > 0)
            {
                m_OrbsList.resize(m_nNumOrbs);
            }
            m_meshDirty = true;
        }
    }

    EFlareType GetType() { return eFT_CameraOrbs; }
    void PreRender([[maybe_unused]] CShader* shader, [[maybe_unused]] Vec3 vSrcWorldPos, [[maybe_unused]] Vec3 vSrcProjPos, [[maybe_unused]] SAuxParams& aux){}
    void Render(CShader* shader, Vec3 vSrcWorldPos, Vec3 vSrcProjPos, SAuxParams& aux);
    void PostRender([[maybe_unused]] CShader* shader, [[maybe_unused]] Vec3 vSrcWorldPos, [[maybe_unused]] Vec3 vSrcProjPos, [[maybe_unused]] SAuxParams& aux){}
    void Load(IXmlNode* pNode);

    CTexture* GetOrbTex();
    CTexture* GetLensTex();
    void SetOrbTex(CTexture* tex) { m_pOrbTex = tex; }
    void SetLensTex(CTexture* tex) { m_pLensTex = tex; }
    void SetUseLensTex(bool b) { m_bUseLensTex = b; }
    bool GetUseLensTex() { return m_bUseLensTex; }
    void SetEnableOrbDetailShading(bool b) { m_bOrbDetailShading = b; }
    bool GetEnableOrbDetailShading() { return m_bOrbDetailShading; }
    void SetEnableLensDetailShading(bool b) { m_bLensDetailShading = b; }
    bool GetEnableLensDetailShading() { return m_bLensDetailShading; }

    void SetSize(float s)
    {
        COpticsElement::SetSize(s);
        m_meshDirty = true;
    }

    float GetLensTexStrength() const { return m_fLensTexStrength; }
    void SetLensTexStrength(float strength) { m_fLensTexStrength = strength; }

    float GetLensDetailShadingStrength() const { return m_fLensDetailShadingStrength; }
    void SetLensDetailShadingStrength(float strength) { m_fLensDetailShadingStrength = strength; }

    float GetLensDetailBumpiness() const { return m_fLensDetailBumpiness; }
    void SetLensDetailBumpiness(float bumpiness) { m_fLensDetailBumpiness = bumpiness; }

    float GetRotation() { return m_fRotation; }
    void SetRotation(float rot)
    {
        m_fRotation = rot;
        m_meshDirty = true;
    }

    int GetNoiseSeed() const { return m_iNoiseSeed; }
    void SetNoiseSeed(int s)
    {
        m_iNoiseSeed = s;
        m_meshDirty = true;
    }

    float GetSizeNoise() { return m_fSizeNoise; }
    void SetSizeNoise(float s)
    {
        m_fSizeNoise = s;
        m_meshDirty = true;
    }

    float GetBrightnessNoise() { return m_fBrightnessNoise; }
    void SetBrightnessNoise(float b)
    {
        m_fBrightnessNoise = b;
        m_meshDirty = true;
    }

    float GetRotationNoise() { return m_fRotNoise; }
    void SetRotationNoise(float r)
    {
        m_fRotNoise = r;
        m_meshDirty = true;
    }

    float GetColorNoise() { return m_fClrNoise; }
    void SetColorNoise(float clrNoise)
    {
        m_fClrNoise = clrNoise;
        m_meshDirty = true;
    }

    float GetIllumRange() { return m_fIllumRadius; }
    void SetIllumRange(float range)
    {
        m_fIllumRadius = range;
    }

    bool GetEnableAdvancedShading() const { return m_bAdvancedShading; }
    void SetEnableAdvancdShading(bool b) { m_bAdvancedShading = b; }

    ColorF GetAmbientDiffuseRGBK() const { return m_cAmbientDiffuse; }
    void SetAmbientDiffuseRGBK(ColorF amb) { m_cAmbientDiffuse = amb; }

    float GetAbsorptance() const { return m_fAbsorptance; }
    void SetAbsorptance(float a) { m_fAbsorptance = a; }

    float GetTransparency() const { return m_fTransparency; }
    void SetTransparency(float t)  { m_fTransparency = t; }

    float GetScatteringStrength() const { return m_fScatteringStrength; }
    void SetScatteringStrength(float s) { m_fScatteringStrength = s; }

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this) + GetMeshDataSize());
    }
};

#endif // CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_CAMERAORBS_H

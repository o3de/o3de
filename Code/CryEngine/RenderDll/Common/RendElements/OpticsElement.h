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

#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_OPTICSELEMENT_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_OPTICSELEMENT_H
#pragma once

#include "CryString.h"
#include "Cry_Vector2.h"
#include "Cry_Vector3.h"
#include "Cry_Matrix33.h"
#include "Cry_Color.h"
#include "IFlares.h"

class CD3D9Renderer;
class CTexture;
class RootOpticsElement;

namespace LensOpConst
{
    static Vec4 _LO_DEF_VEC4(1.f, 1.f, 1.f, 0.f);
    static Vec2 _LO_DEF_VEC2(0.f, 0.f);
    static Vec2 _LO_DEF_VEC2_I(1.f, 1.f);
    static Vec2 _LO_DEF_ANGLE(-90.f, 90.f);
    static Matrix33 _LO_DEF_MX33(IDENTITY);
    static ColorF _LO_DEF_CLR(1.f, 1.f, 1.f, 0.2f);
    static ColorF _LO_DEF_CLR_BLK(0.f, 0.f, 0.f, 0.0f);
    static float _LO_MIN(1e-6f);
}

typedef MFPVariable<IOpticsElementBase> OpticsMFPVariable;
typedef void (IOpticsElementBase::* Optics_MFPtr)(void);

class COpticsElement
    : public IOpticsElementBase
{
public:
    struct SAuxParams
    {
        float perspectiveShortening;
        float linearDepth;
        float distance;
        float sensorVariationValue;
        float viewAngleFalloff;
        bool attachToSun;
        bool bMultiplyColor;
        bool bForceRender;
    };

private:
    Vec2 xform_scale;
    Vec2 xform_translate;
    float xform_rotation;

protected:
    COpticsElement* m_pParent;
    Vec2 m_globalMovement;
    Matrix33 m_globalTransform;
    ColorF m_globalColor;
    float m_globalFlareBrightness;
    float m_globalShaftBrightness;
    float m_globalSize;
    float m_globalPerpectiveFactor;
    float m_globalDistanceFadingFactor;
    float m_globalOrbitAngle;
    float m_globalSensorSizeFactor;
    float m_globalSensorBrightnessFactor;
    bool m_globalAutoRotation : 1;
    bool m_globalCorrectAspectRatio : 1;
    bool m_globalOcclusionBokeh : 1;

    string m_name;
    bool m_bEnabled : 1;

protected:

    Matrix33 m_mxTransform;
    float m_fSize;
    float m_fPerpectiveFactor;
    float m_fDistanceFadingFactor;
    ColorF m_Color;
    float m_fBrightness;
    Vec2 m_vMovement;
    float m_fOrbitAngle;
    float m_fSensorSizeFactor;
    float m_fSensorBrightnessFactor;
    Vec2 m_vDynamicsOffset;
    float m_fDynamicsRange;
    float m_fDynamicsFalloff;
    bool m_bAutoRotation : 1;
    bool m_bCorrectAspectRatio : 1;
    bool m_bOcclusionBokeh : 1;
    bool m_bDynamics : 1;
    bool m_bDynamicsInvert : 1;

#if defined(FLARES_SUPPORT_EDITING)
    AZStd::vector<FuncVariableGroup> m_paramGroups;
#endif
public:

    COpticsElement (const char* name, float size  = 0.3f, const float brightness = 1.0f, const ColorF& color = LensOpConst::_LO_DEF_CLR)
        : m_pParent(0)
        , m_bEnabled(true)
        , m_fPerpectiveFactor(0)
        , m_globalPerpectiveFactor(1)
        , m_fOrbitAngle(0)
        , m_bOcclusionBokeh(false)
        , m_fSensorSizeFactor(0)
        , m_fSensorBrightnessFactor(0)
        , m_fDistanceFadingFactor(1)
    {
        m_name = name;
        m_vMovement = Vec2(1.f, 1.f);
        m_fSize = size;
        m_Color = color;
        m_mxTransform = LensOpConst::_LO_DEF_MX33;

        m_globalMovement = LensOpConst::_LO_DEF_VEC2_I;
        m_globalTransform = Matrix33::CreateIdentity();
        m_globalColor.Set(1.f, 1.f, 1.f, 1.f);
        m_globalFlareBrightness = 1.f;
        m_globalShaftBrightness = 1.f;
        m_globalSize = 1;
        m_fBrightness = brightness;
        m_bAutoRotation = false;
        m_bCorrectAspectRatio = true;

        xform_scale.set(1.f, 1.f);
        xform_rotation = 0.f;
        xform_translate.set(0.f, 0.f);

        m_bDynamics = false;
        m_vDynamicsOffset.set(0.f, 0.f);
        m_fDynamicsRange = 1.f;
        m_bDynamicsInvert = false;
        m_fDynamicsFalloff = 1.f;
    }

    virtual ~COpticsElement()
    {
    }

    virtual void Load(IXmlNode* pNode);

    COpticsElement(const COpticsElement& copyFrom)
    {
        *this = copyFrom;
#if defined(FLARES_SUPPORT_EDITING)
        m_paramGroups.clear();
#endif
    }

    IOpticsElementBase* GetParent() const
    {
        return m_pParent;
    }

    RootOpticsElement* GetRoot();

    string GetName() const { return m_name; }
    void SetName(const char* newName)
    {
        m_name = newName;
    }
    bool IsEnabled() const { return m_bEnabled; }
    float GetSize() const { return m_fSize; }
    float GetPerspectiveFactor() const { return m_fPerpectiveFactor; }
    float GetDistanceFadingFactor() const { return m_fDistanceFadingFactor; }
    float GetBrightness() const { return m_fBrightness; }
    ColorF GetColor() const { return m_Color; }
    Vec2 GetMovement() const { return m_vMovement; }
    float GetOrbitAngle() const {return m_fOrbitAngle; }
    float GetSensorSizeFactor() const { return m_fSensorSizeFactor; }
    float GetSensorBrightnessFactor() const { return m_fSensorBrightnessFactor; }
    bool IsOccBokehEnabled() const { return m_bOcclusionBokeh; }
    bool HasAutoRotation() const { return m_bAutoRotation; }
    bool HasAspectRatioCorrection() const { return m_bCorrectAspectRatio; }

    void SetEnabled(bool b) override { m_bEnabled = b; }
    void SetSize(float s) override { m_fSize = s; }
    void SetPerspectiveFactor(float p) override { m_fPerpectiveFactor = p; }
    void SetDistanceFadingFactor(float p) override { m_fDistanceFadingFactor = p; }
    void SetBrightness(float b) override { m_fBrightness = b; }
    void SetColor(ColorF color) { m_Color = color; Invalidate(); }
    void SetMovement(Vec2 movement)
    {
        m_vMovement = movement;
        if (fabs(m_vMovement.x) < 0.0001f)
        {
            m_vMovement.x = 0.001f;
        }
        if (fabs(m_vMovement.y) < 0.0001f)
        {
            m_vMovement.y = 0.001f;
        }
    }
    void SetTransform(const Matrix33& xform) override { m_mxTransform = xform; }
    void SetOccBokehEnabled(bool b) override { m_bOcclusionBokeh = b; }
    void SetOrbitAngle(float orbitAngle) override { m_fOrbitAngle = orbitAngle; }
    void SetSensorSizeFactor(float sizeFactor) override { m_fSensorSizeFactor = sizeFactor; }
    void SetSensorBrightnessFactor(float brtFactor) override { m_fSensorBrightnessFactor = brtFactor; }
    void SetAutoRotation(bool b) override { m_bAutoRotation = b; }
    void SetAspectRatioCorrection(bool b) override { m_bCorrectAspectRatio = b; }

    void SetParent(COpticsElement* pParent)
    {
        m_pParent = pParent;
    }

    virtual void GetMemoryUsage(ICrySizer* pSizer) const;
    bool IsVisible() const
    {
        return m_globalColor.a > LensOpConst::_LO_MIN && m_globalFlareBrightness > LensOpConst::_LO_MIN;
    }

    virtual void Render([[maybe_unused]] SLensFlareRenderParam* pParam, [[maybe_unused]] const Vec3& vPos){assert(0); }

protected:
    void updateXformMatrix();
    virtual void Invalidate() {}

public:
    void SetScale(Vec2 scale)
    {
        xform_scale = scale;
        updateXformMatrix();
    }
    void SetRotation(float rot)
    {
        xform_rotation = rot;
        updateXformMatrix();
    }
    void SetTranslation(Vec2 xlation)
    {
        xform_translate = xlation;
        updateXformMatrix();
    }
    Vec2 GetScale() { return xform_scale; }
    Vec2 GetTranslation() { return xform_translate; }
    float GetRotation() { return xform_rotation; }

    void SetDynamicsEnabled(bool enable) { m_bDynamics = enable; }
    void SetDynamicsOffset(Vec2 offset) { m_vDynamicsOffset = offset; }
    void SetDynamicsRange(float range)  { m_fDynamicsRange = range; }
    void SetDynamicsInvert(bool invert) { m_bDynamicsInvert = invert; }
    void SetDynamicsFalloff(float falloff) { m_fDynamicsFalloff = falloff; }

    bool GetDynamicsEnabled() const { return m_bDynamics; }
    Vec2 GetDynamicsOffset() const { return m_vDynamicsOffset; }
    float GetDynamicsRange() const { return m_fDynamicsRange; }
    bool GetDynamicsInvert() const { return m_bDynamicsInvert; }
    float GetDynamicsFalloff() const { return m_fDynamicsFalloff; }

    virtual void AddElement([[maybe_unused]] IOpticsElementBase* pElement) {}
    virtual void InsertElement([[maybe_unused]] int nPos, [[maybe_unused]] IOpticsElementBase* pElement) {}
    virtual void Remove([[maybe_unused]] int i) {}
    virtual void RemoveAll() {}
    virtual int GetElementCount() const {return 0; }
    virtual IOpticsElementBase* GetElementAt([[maybe_unused]] int  i) const
    {
#ifndef RELEASE
        iLog->Log("ERROR");
        __debugbreak();
#endif
        return 0;
    }

#if defined(FLARES_SUPPORT_EDITING)
    virtual AZStd::vector<FuncVariableGroup> GetEditorParamGroups();
#endif

protected:

#if defined(FLARES_SUPPORT_EDITING)
    virtual void InitEditorParamGroups(AZStd::vector<FuncVariableGroup>& groups);
#endif

public:

    virtual void validateGlobalVars(SAuxParams& aux);

    float computeMovementLocationX(const Vec3& vSrcProjPos)
    {
        return (vSrcProjPos.x - 0.5f) * m_globalMovement.x + 0.5f;
    }

    float computeMovementLocationY(const Vec3& vSrcProjPos)
    {
        return (vSrcProjPos.y - 0.5f) * m_globalMovement.y + 0.5f;
    }

    static const Vec3 computeOrbitPos(const Vec3& vSrcProjPos, float orbitAngle);
    void ApplyVSParam_WPosAndSize(CShader* shader, const Vec3& wpos);

    void ApplyOcclusionPattern(CShader* shader);
    void ApplyGeneralFlags(CShader* shader);
    void ApplyOcclusionBokehFlag(CShader* shader);
    void ApplySpectrumTexFlag(CShader* shader, bool enabled);
    void ApplyExternTintAndBrightnessVS(CShader* shader, ColorF& cExTint, float fExBrt);
    void ApplyVSParam_Xform(CShader* shader, Matrix33& mx33);
    void ApplyVSParam_Dynamics(CShader* shader, const Vec3& projPos);
    void ApplyPSParam_LightProjPos(CShader* shader, const Vec3& lightProjPos);
    void ApplyVSParam_LightProjPos(CShader* shader, const Vec3& lightProjPos);

    void ApplyCommonVSParams(CShader* shader, const Vec3& wpos, const Vec3& lightProjPos)
    {
        //If aspect ratio correction is on (default) we want to adjust the global 
        //transform to re-scale the flare geometry to keep a constant size regardless of 
        //the window's aspect ratio.
        //This used to be handled in the shader with params passed from a method ApplyVSParams_ScreenWidthHeight.
        //But that only applied to ghost flares and why bother doing that per-vertex when we can do it once
        //per constant buffer.
        if (HasAspectRatioCorrection())
        {
            const float inverseAspectRatio = static_cast<float>(gEnv->pRenderer->GetHeight()) / static_cast<float>(gEnv->pRenderer->GetWidth());

            //Adjust the entire base to avoid warping when rotation is applied
            const Vec3 adjustedXBasis = m_globalTransform.GetRow(0) * inverseAspectRatio;
            m_globalTransform.SetRow(0, adjustedXBasis);
        }

        ApplyVSParam_WPosAndSize(shader, wpos);
        ApplyVSParam_Xform(shader, m_globalTransform);
        ApplyVSParam_Dynamics(shader, lightProjPos);
    }

    virtual EFlareType GetType() { return eFT__Base__; }
    virtual bool IsGroup() const { return false; }

    virtual void Render(CShader* shader, Vec3 vSrcWorldPos, Vec3 vSrcProjPos, SAuxParams& aux) = 0;
};

#endif // CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_OPTICSELEMENT_H

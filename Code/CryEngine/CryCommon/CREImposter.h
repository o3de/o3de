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

#include <IRenderer.h>
#include "Cry_Camera.h"

//================================================================================

struct SDynTexture2;
struct SDynTexture;
class IDynTexture;
class CameraViewParameters;

struct IImposterRenderElement
{
    virtual void GetMemoryUsage(ICrySizer* pSizer) const = 0;
    virtual void mfPrepare(bool bCheckOverflow) = 0;
    virtual bool mfDraw(CShader* ef, SShaderPass* sl) = 0;
    virtual const SMinMaxBox& mfGetWorldSpaceBounds() = 0;

    virtual bool IsSplit() = 0;
    virtual bool IsScreenImposter() = 0;

    virtual float GetRadiusX() = 0;
    virtual float GetRadiusY() = 0;
    virtual Vec3* GetQuadCorners() = 0;
    virtual Vec3 GetNearPoint() = 0;
    virtual Vec3 GetFarPoint() = 0;
    virtual float GetErrorToleranceCosAngle() = 0;
    virtual uint32 GetState() = 0;
    virtual int GetAlphaRef() = 0;
    virtual ColorF GetColorHelper() = 0;
    virtual Vec3 GetLastSunDirection() = 0;
    virtual uint8 GetLastBestEdge() = 0;
    virtual float GetNear() = 0;
    virtual float GetFar() = 0;
    virtual float GetTransparency() = 0;
    virtual Vec3 GetPosition();
    virtual int GetLogResolutionX() = 0;
    virtual int GetLogResolutionY() = 0;
    virtual CameraViewParameters& GetLastViewParameters() = 0;
    virtual IDynTexture* GetTexture() = 0;
    virtual IDynTexture* GetScreenTexture() = 0;
    virtual IDynTexture* GetFrontTexture() = 0;
    virtual IDynTexture* GetDepthTexture() = 0;
    virtual const SMinMaxBox& GetWorldSpaceBounds() = 0;

    virtual void SetBBox(const Vec3& min, const Vec3& max) = 0;
    virtual void SetScreenImposterState(bool state) = 0;
    virtual void SetState(uint32 state) = 0;
    virtual void SetAlphaRef(uint32 ref) = 0;
    virtual void SetPosition(Vec3 pos) = 0;
    virtual void SetFrameResetValue(int frameResetValue) = 0;
    virtual void SetTexture(IDynTexture* texture) = 0;
    virtual void SetScreenTexture(IDynTexture* texture) = 0;
    virtual void SetFrontTexture(IDynTexture* texture) = 0;
    virtual void SetDepthTexture(IDynTexture* texture) = 0;
};

class CREImposter
    : public CRendElementBase
{
    friend class CRECloud;
    static IDynTexture* m_pScreenTexture;

    CameraViewParameters m_LastViewParameters;
    bool m_bScreenImposter;
    bool m_bSplit;
    float m_fRadiusX;
    float m_fRadiusY;
    Vec3 m_vQuadCorners[4];                             // in world space, relative to m_vPos, in clockwise order, can be rotated
    Vec3 m_vNearPoint;
    Vec3 m_vFarPoint;
    int m_nLogResolutionX;
    int m_nLogResolutionY;
    IDynTexture* m_pTexture;
    IDynTexture* m_pFrontTexture;
    IDynTexture* m_pTextureDepth;
    float m_fErrorToleranceCosAngle;            // cosine of m_fErrorToleranceAngle used to check if IsImposterValid
    SMinMaxBox m_WorldSpaceBV;
    uint32 m_State;
    int m_AlphaRef;
    float m_fCurTransparency;
    ColorF m_ColorHelper;
    Vec3 m_vPos;
    Vec3 m_vLastSunDir;
    uint8 m_nLastBestEdge;                              // 0..11 this edge is favored to not jitter between different edges
    float m_fNear;
    float m_fFar;

    bool IsImposterValid(const CameraViewParameters& viewParameters, float fRadiusX, float fRadiusY, float fCamRadiusX, float fCamRadiusY,
        const int iRequiredLogResX, const int iRequiredLogResY, const uint32 dwBestEdge);

    bool Display(bool bDisplayFrontOfSplit);

public:
    int m_nFrameReset;
    int m_FrameUpdate;
    float m_fTimeUpdate;

    static int m_MemUpdated;
    static int m_MemPostponed;
    static int m_PrevMemUpdated;
    static int m_PrevMemPostponed;

    CREImposter()
        : CRendElementBase()
        , m_pTexture(NULL)
        , m_pFrontTexture(NULL)
        , m_pTextureDepth(NULL)
        , m_bSplit(false)
        , m_fRadiusX(0)
        , m_fRadiusY(0)
        , m_fErrorToleranceCosAngle(cos(DEG2RAD(0.25f)))
        , m_bScreenImposter(false)
        , m_State(GS_DEPTHWRITE)
        , m_AlphaRef(-1)
        , m_fCurTransparency(1.0f)
        , m_FrameUpdate(0)
        , m_nFrameReset(0)
        , m_fTimeUpdate(0)
        , m_vLastSunDir(0, 0, 0)
        , m_nLogResolutionX(0)
        , m_nLogResolutionY(0)
        , m_nLastBestEdge(0)
    {
        mfSetType(eDATA_Imposter);
        mfUpdateFlags(FCEF_TRANSFORM);
        m_ColorHelper = Col_White;
    }
    virtual ~CREImposter()
    {
        ReleaseResources();
    }

    bool UpdateImposter();
    void ReleaseResources();

    bool PrepareForUpdate();

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }
    virtual void mfPrepare(bool bCheckOverflow);
    virtual bool mfDraw(CShader* ef, SShaderPass* sl);
    const SMinMaxBox& mfGetWorldSpaceBounds() { return m_WorldSpaceBV; }

    virtual bool IsSplit() { return m_bSplit; }
    virtual bool IsScreenImposter()  { return m_bScreenImposter; }

    virtual float GetRadiusX()  { return m_fRadiusX; }
    virtual float GetRadiusY()  { return m_fRadiusY; }
    virtual Vec3* GetQuadCorners()  { return &m_vQuadCorners[0]; }
    virtual Vec3 GetNearPoint()  { return m_vNearPoint; }
    virtual Vec3 GetFarPoint()  { return m_vFarPoint; }
    virtual float GetErrorToleranceCosAngle()  { return m_fErrorToleranceCosAngle; }
    virtual uint32 GetState()  { return m_State; }
    virtual int GetAlphaRef() { return m_AlphaRef; }
    virtual ColorF GetColorHelper() { return m_ColorHelper; }
    virtual Vec3 GetLastSunDirection() { return m_vLastSunDir; }
    virtual uint8 GetLastBestEdge() { return m_nLastBestEdge; }
    virtual float GetNear() { return m_fNear; }
    virtual float GetFar() { return m_fFar; }
    virtual float GetTransparency() { return m_fCurTransparency; }
    virtual Vec3 GetPosition();
    virtual int GetLogResolutionX() { return m_nLogResolutionX; }
    virtual int GetLogResolutionY() { return m_nLogResolutionY; }
    virtual CameraViewParameters& GetLastViewParameters() { return m_LastViewParameters; }
    virtual IDynTexture** GetTexture() { return &m_pTexture; }
    virtual IDynTexture** GetScreenTexture() { return &m_pScreenTexture; }
    virtual IDynTexture** GetFrontTexture() { return &m_pFrontTexture; }
    virtual IDynTexture** GetDepthTexture() { return &m_pTextureDepth; }
    virtual const SMinMaxBox& GetWorldSpaceBounds() { return m_WorldSpaceBV; }
    virtual int GetFrameReset() { return m_nFrameReset; }
    virtual void SetBBox(const Vec3& min, const Vec3& max) { m_WorldSpaceBV.SetMin(min); m_WorldSpaceBV.SetMax(max); }
    virtual void SetScreenImposterState(bool state) { m_bScreenImposter = state; }
    virtual void SetState(uint32 state) { m_State = state; }
    virtual void SetAlphaRef(uint32 ref) { m_AlphaRef = ref; }
    virtual void SetPosition(Vec3 pos) { m_vPos = pos; }
    virtual void SetFrameResetValue(int frameResetValue) { m_nFrameReset = frameResetValue; }
};

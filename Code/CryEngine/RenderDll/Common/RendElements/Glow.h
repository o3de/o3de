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

#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_GLOW_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_GLOW_H
#pragma once

#include "OpticsElement.h"
#include "AbstractMeshElement.h"
#include "MeshUtil.h"

class Glow
    : public COpticsElement
    , public AbstractMeshElement
{
private:
    static float compositionBufRatio;

protected:
    float m_fFocusFactor;
    float m_fPolyonFactor;
    float m_fGamma;

protected:
    void ApplyDistributionParamsPS(CShader* shader);
    virtual void GenMesh();
    void DrawMesh();
    void Invalidate()
    {
        m_meshDirty = true;
    }

public:
    Glow(const char* name)
        : COpticsElement(name)
        , m_fFocusFactor(0.3f)
        , m_fPolyonFactor(32.f)
        , m_fGamma(1)
    {
        m_Color.a = 1.f; //Max alpha so that the ghost can be seen when it's created
    }


#if defined(FLARES_SUPPORT_EDITING)
    void InitEditorParamGroups(AZStd::vector<FuncVariableGroup>& groups);
#endif

    EFlareType GetType() { return eFT_Glow; }
    void Render(CShader* shader, Vec3 vSrcWorldPos, Vec3 vSrcProjPos, SAuxParams& aux);
    void Load(IXmlNode* pNode);

    void SetColor(ColorF t)
    {
        COpticsElement::SetColor(t);
        m_meshDirty = true;
    }
    float GetFocusFactor() const {return m_fFocusFactor; }
    void SetFocusFactor(float f)
    {
        m_fFocusFactor = f;
    }
    int GetPolygonFactor() const { return (int)m_fPolyonFactor; }
    void SetPolygonFactor(int f)
    {
        if (f < 0)
        {
            f = 0;
        }
        else if (f > 128)
        {
            f = 128;
        }
        if (m_fPolyonFactor != f)
        {
            m_fPolyonFactor = (float)f;
            m_meshDirty = true;
        }
    }
    float GetGamma() const { return m_fGamma; }
    void SetGamma(float gamma)
    {
        m_fGamma = (float)gamma;
    }

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this) + GetMeshDataSize());
    }
};

#endif // CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_GLOW_H

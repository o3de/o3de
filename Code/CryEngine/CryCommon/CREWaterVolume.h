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
#ifndef _CREWATERVOLUME_
#define _CREWATERVOLUME_

#pragma once


#include "VertexFormats.h"

class CREWaterVolume
    : public CRendElementBase
{
public:
    CREWaterVolume();

    virtual ~CREWaterVolume();
    virtual void mfPrepare(bool bCheckOverflow);
    virtual bool mfDraw(CShader* ef, SShaderPass* sfm);
    virtual void mfGetPlane(Plane_tpl<f32>& pl);
    virtual void mfCenter(Vec3& vCenter, CRenderObject* pObj);

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }

public:
    struct SParams
    {
        SParams()
            : m_pVertices(0)
            , m_pIndices(0)
            , m_numVertices(0)
            , m_numIndices(0)
            , m_center(0, 0, 0)
            , m_WSBBox(Vec3(-1, -1, -1), Vec3(1, 1, 1))
            , m_fogPlane(Vec3(0, 0, 1), 0)
            , m_fogDensity(0.1f)
            , m_fogColor(0.2f, 0.5f, 0.7f)
            , m_fogColorAffectedBySun(true)
            , m_fogShadowing(0.5f)
            , m_caustics(true)
            , m_causticIntensity(1.0f)
            , m_causticTiling(1.0f)
            , m_causticHeight(0.9f)
            , m_viewerInsideVolume(false)
            , m_viewerCloseToWaterPlane(false)
            , m_viewerCloseToWaterVolume(false)
        {
        }

        const SVF_P3F_C4B_T2F* m_pVertices;
        const uint16* m_pIndices;

        size_t m_numVertices;
        size_t m_numIndices;

        Vec3 m_center;
        AABB m_WSBBox;

        Plane_tpl<f32> m_fogPlane;
        float m_fogDensity;
        Vec3 m_fogColor;
        bool m_fogColorAffectedBySun;
        float m_fogShadowing;

        bool m_caustics;
        float m_causticIntensity;
        float m_causticTiling;
        float m_causticHeight;

        bool m_viewerInsideVolume;
        bool m_viewerCloseToWaterPlane;
        bool m_viewerCloseToWaterVolume;
    };

    struct SOceanParams
    {
        SOceanParams()
            : m_fogColor(0.2f, 0.5f, 0.7f)
            , m_fogColorShallow(0.2f, 0.5f, 0.7f)
            , m_fogDensity(0.2f)
        {
        }

        Vec3 m_fogColor;
        Vec3 m_fogColorShallow;
        float m_fogDensity;
    };

public:
    const SParams* m_pParams;
    const SOceanParams* m_pOceanParams;
    bool m_drawWaterSurface;
    bool m_drawFastPath;
};


#endif // #ifndef _CREWATERVOLUME_

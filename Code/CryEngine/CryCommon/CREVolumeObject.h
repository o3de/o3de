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
#ifndef _CREVOLUMEOBJECT_
#define _CREVOLUMEOBJECT_

#pragma once

#include "VertexFormats.h"


struct IVolumeObjectRenderNode;

struct IVolumeTexture
{
public:
    virtual ~IVolumeTexture() {}
    virtual void Release() = 0;
    virtual bool Create(unsigned int width, unsigned int height, unsigned int depth, unsigned char* pData) = 0;
    virtual bool Update(unsigned int width, unsigned int height, unsigned int depth, const unsigned char* pData) = 0;
    virtual int GetTexID() const = 0;
    virtual uint32 GetWidth() const = 0;
    virtual uint32 GetHeight() const = 0;
    virtual uint32 GetDepth() const = 0;
    virtual ITexture* GetTexture() const  = 0;
};

class CREVolumeObject
    : public CRendElementBase
{
public:
    CREVolumeObject();

    virtual ~CREVolumeObject();
    virtual void mfPrepare(bool bCheckOverflow);
    virtual bool mfDraw(CShader* ef, SShaderPass* sfm);

    virtual IVolumeTexture* CreateVolumeTexture() const;

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }

    Vec3 m_center;
    Matrix34 m_matInv;
    Vec3 m_eyePosInWS;
    Vec3 m_eyePosInOS;
    Plane m_volumeTraceStartPlane;
    AABB m_renderBoundsOS;
    bool m_viewerInsideVolume;
    bool m_nearPlaneIntersectsVolume;
    float m_alpha;
    float m_scale;

    IVolumeTexture* m_pDensVol;
    IVolumeTexture* m_pShadVol;
    _smart_ptr<IRenderMesh> m_pHullMesh;
};

#endif // #ifndef _CREVOLUMEOBJECT_
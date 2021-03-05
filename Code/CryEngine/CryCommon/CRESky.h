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

#ifndef __CRESKY_H__
#define __CRESKY_H__

//=============================================================

#include "VertexFormats.h"
#include "../RenderDll/Common/Shaders/Vertex.h"

struct SSkyLightRenderParams;

class CRESky
    : public CRendElementBase
{
    friend class CRender3D;

public:

    float m_fTerrainWaterLevel;
    float m_fSkyBoxStretching;
    float m_fAlpha;
    int m_nSphereListId;

public:
    CRESky();
    virtual ~CRESky();
    virtual void mfPrepare(bool bCheckOverflow);
    virtual bool mfDraw(CShader* ef, SShaderPass* sfm);

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }

    AZ::Vertex::Format GetVertexFormat() const override;
    bool GetGeometryInfo(SGeometryInfo& streams) override;

private:
    AZ::Vertex::Format m_skyVertexFormat;
};

class CREHDRSky
    : public CRendElementBase
{
public:
    CREHDRSky();
    virtual ~CREHDRSky();
    virtual void mfPrepare(bool bCheckOverflow);
    virtual bool mfDraw(CShader* ef, SShaderPass* sfm);

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }

    void GenerateSkyDomeTextures(int32 width, int32 height);

    virtual AZ::Vertex::Format GetVertexFormat() const override;
    virtual bool GetGeometryInfo(SGeometryInfo& streams) override;

public:
    const SSkyLightRenderParams* m_pRenderParams;
    int m_moonTexId;
    class CTexture* m_pSkyDomeTextureMie;
    class CTexture* m_pSkyDomeTextureRayleigh;

    static void SetCommonMoonParams(CShader* ef, bool bUseMoon = false);

private:
    void Init();

private:
    int m_skyDomeTextureLastTimeStamp;
    int m_frameReset;
    class CStars* m_pStars;
    AZ::Vertex::Format m_hdrSkyVertexFormat;
};


#endif  // __CRESKY_H__

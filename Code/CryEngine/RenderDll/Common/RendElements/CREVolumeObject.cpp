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

#include "RenderDll_precompiled.h"

#include "CREVolumeObject.h"
#include <IEntityRenderState.h> // <> required for Interfuscator
#include <AzCore/std/containers/vector.h>

#if !defined(NULL_RENDERER)
#include "../../XRenderD3D9/DriverD3D.h"
#endif

//////////////////////////////////////////////////////////////////////////
//

class CVolumeTexture
    : public IVolumeTexture
{
public:
    void Release() override;
    bool Create(unsigned int width, unsigned int height, unsigned int depth, unsigned char* pData) override;
    bool Update(unsigned int width, unsigned int height, unsigned int depth, const unsigned char* pData) override;
    int GetTexID() const override;
    uint32 GetWidth() const override { return m_width; }
    uint32 GetHeight() const override { return m_height; }
    uint32 GetDepth() const override { return m_depth; }
    CTexture* GetTexture() const override { return m_pTex; }
    CVolumeTexture();
    ~CVolumeTexture();

private:
    unsigned int m_width;
    unsigned int m_height;
    unsigned int m_depth;

    static const size_t StagingBufferFrameCount = 2;
    AZStd::vector<uint8_t> m_StagingData;
    uint8_t m_FrameIndex;

    inline uint8_t* GetCurrentStagingData()
    {
        const size_t sliceSize = m_StagingData.size() / StagingBufferFrameCount;
        return &m_StagingData[m_FrameIndex * sliceSize];
    }

    CTexture* m_pTex;
};


CVolumeTexture::CVolumeTexture()
    : m_width(0)
    , m_height(0)
    , m_depth(0)
    , m_pTex(0)
    , m_FrameIndex(0)
{
}


CVolumeTexture::~CVolumeTexture()
{
    if (m_pTex)
    {
        gRenDev->RemoveTexture(m_pTex->GetTextureID());
    }
}


void CVolumeTexture::Release()
{
    delete this;
}


bool CVolumeTexture::Create(unsigned int width, unsigned int height, unsigned int depth, unsigned char* pData)
{
    assert(!m_pTex);
    if (!m_pTex)
    {
        char name[128];
        name[sizeof(name) - 1] = '\0';
        azsnprintf(name, sizeof(name) - 1, "$VolObj_%d", gRenDev->m_TexGenID++);

        const uint32_t totalByteCount = width * height * depth;
        m_StagingData.resize(totalByteCount * StagingBufferFrameCount);

        if (pData)
        {
            uint8_t* currentStagingData = GetCurrentStagingData();
            memcpy(currentStagingData, pData, totalByteCount);
            pData = currentStagingData;
        }

        int flags(FT_DONT_STREAM);
        m_pTex = CTexture::Create3DTexture(name, width, height, depth, 1, flags, pData, eTF_A8, eTF_A8);

        m_width = width;
        m_height = height;
        m_depth = depth;
    }
    return m_pTex != 0;
}


bool CVolumeTexture::Update([[maybe_unused]] unsigned int width, [[maybe_unused]] unsigned int height, [[maybe_unused]] unsigned int depth, [[maybe_unused]] const unsigned char* pData)
{
    if (!CTexture::IsTextureExist(m_pTex))
    {
        return false;
    }

    m_FrameIndex ^= m_FrameIndex;
    uint8_t* stagingData = GetCurrentStagingData();
    
#if !defined(NULL_RENDERER)
    unsigned int cpyWidth = min(width, m_width);
    unsigned int cpyHeight = min(height, m_height);
    unsigned int cpyDepth = min(depth, m_depth);
    memcpy(stagingData, pData, cpyWidth * cpyHeight * cpyDepth);

    m_pTex->UpdateTextureRegion(stagingData, 0, 0, 0, cpyWidth, cpyHeight, cpyDepth, m_pTex->GetDstFormat());
#endif

    return true;
}


int CVolumeTexture::GetTexID() const
{
    return m_pTex ? m_pTex->GetTextureID() : 0;
}


//////////////////////////////////////////////////////////////////////////
//

CREVolumeObject::CREVolumeObject()
    : CRendElementBase()
    , m_center(0, 0, 0)
    , m_matInv()
    , m_eyePosInWS(0, 0, 0)
    , m_eyePosInOS(0, 0, 0)
    , m_volumeTraceStartPlane(Vec3(0, 0, 1), 0)
    , m_renderBoundsOS(Vec3(-1, -1, -1), Vec3(1, 1, 1))
    , m_viewerInsideVolume(false)
    , m_nearPlaneIntersectsVolume(false)
    , m_alpha(1)
    , m_scale(1)
    , m_pDensVol(0)
    , m_pShadVol(0)
    , m_pHullMesh(0)
{
    mfSetType(eDATA_VolumeObject);
    mfUpdateFlags(FCEF_TRANSFORM);
    m_matInv.SetIdentity();
}


CREVolumeObject::~CREVolumeObject()
{
}


void CREVolumeObject::mfPrepare(bool bCheckOverflow)
{
    if (bCheckOverflow)
    {
        gRenDev->FX_CheckOverflow(0, 0, this);
    }
    gRenDev->m_RP.m_pRE = this;
    gRenDev->m_RP.m_RendNumIndices = 0;
    gRenDev->m_RP.m_RendNumVerts = 0;
    gRenDev->m_RP.m_CurVFormat = eVF_P3F;
}


IVolumeTexture* CREVolumeObject::CreateVolumeTexture() const
{
    return new CVolumeTexture();
}

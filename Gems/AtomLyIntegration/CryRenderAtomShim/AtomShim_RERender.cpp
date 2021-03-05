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

#include "CryRenderOther_precompiled.h"
#include "AtomShim_Renderer.h"
#include "I3DEngine.h"

//=======================================================================

bool CRESky::mfDraw([[maybe_unused]] CShader* ef, [[maybe_unused]] SShaderPass* sfm)
{
    return true;
}

bool CREHDRSky::mfDraw([[maybe_unused]] CShader* ef, [[maybe_unused]] SShaderPass* sfm)
{
    return true;
}

bool CREFogVolume::mfDraw([[maybe_unused]] CShader* ef, [[maybe_unused]] SShaderPass* sfm)
{
    return true;
}

bool CREWaterVolume::mfDraw([[maybe_unused]] CShader* ef, [[maybe_unused]] SShaderPass* sfm)
{
    return true;
}

void CREWaterOcean::FrameUpdate()
{
}

void CREWaterOcean::Create([[maybe_unused]] uint32 nVerticesCount, [[maybe_unused]] SVF_P3F_C4B_T2F* pVertices, [[maybe_unused]] uint32 nIndicesCount, [[maybe_unused]] const void* pIndices, [[maybe_unused]] uint32 nIndexSizeof)
{
}

void CREWaterOcean::ReleaseOcean()
{
}

bool CREWaterOcean::mfDraw([[maybe_unused]] CShader* ef, [[maybe_unused]] SShaderPass* sfm)
{
    return true;
}

CREOcclusionQuery::~CREOcclusionQuery()
{
    mfReset();
}

void CREOcclusionQuery::mfReset()
{
    m_nOcclusionID = 0;
}

uint32 CREOcclusionQuery::m_nQueriesPerFrameCounter = 0;
uint32 CREOcclusionQuery::m_nReadResultNowCounter = 0;
uint32 CREOcclusionQuery::m_nReadResultTryCounter = 0;

bool CREOcclusionQuery::mfDraw([[maybe_unused]] CShader* ef, [[maybe_unused]] SShaderPass* sfm)
{
    return true;
}
bool CREOcclusionQuery::mfReadResult_Now(void)
{
    return true;
}
bool CREOcclusionQuery::mfReadResult_Try([[maybe_unused]] uint32 nDefaultNumSamples)
{
    return true;
}
bool CREOcclusionQuery::RT_ReadResult_Try([[maybe_unused]] uint32 nDefaultNumSamples)
{
    return true;
}

bool CREMeshImpl::mfPreDraw([[maybe_unused]] SShaderPass* sl)
{
    return true;
}

bool CREMeshImpl::mfDraw([[maybe_unused]] CShader* ef, [[maybe_unused]] SShaderPass* sl)
{
    return true;
}

bool CREHDRProcess::mfDraw([[maybe_unused]] CShader* ef, [[maybe_unused]] SShaderPass* sfm)
{
    return true;
}

bool CREDeferredShading::mfDraw([[maybe_unused]] CShader* ef, [[maybe_unused]] SShaderPass* sfm)
{
    return true;
}

bool CREBeam::mfDraw([[maybe_unused]] CShader* ef, [[maybe_unused]] SShaderPass* sl)
{
    return true;
}

bool CREImposter::mfDraw([[maybe_unused]] CShader* ef, [[maybe_unused]] SShaderPass* pPass)
{
    return true;
}

bool CRECloud::mfDraw([[maybe_unused]] CShader* ef, [[maybe_unused]] SShaderPass* pPass)
{
    return true;
}

bool CRECloud::UpdateImposter([[maybe_unused]] CRenderObject* pObj)
{
    return true;
}

bool CRECloud::GenerateCloudImposter([[maybe_unused]] CShader* pShader, [[maybe_unused]] CShaderResources* pRes, [[maybe_unused]] CRenderObject* pObject)
{
    return true;
}

bool CREImposter::UpdateImposter()
{
    return true;
}

bool CREVolumeObject::mfDraw([[maybe_unused]] CShader* ef, [[maybe_unused]] SShaderPass* sfm)
{
    return true;
}

#if !defined(EXCLUDE_DOCUMENTATION_PURPOSE)
bool CREPrismObject::mfDraw([[maybe_unused]] CShader* ef, [[maybe_unused]] SShaderPass* sfm)
{
    return true;
}
#endif // EXCLUDE_DOCUMENTATION_PURPOSE

bool CREGameEffect::mfDraw([[maybe_unused]] CShader* ef, [[maybe_unused]] SShaderPass* sfm)
{
    return true;
}

void CRELensOptics::ClearResources()
{
}

#if defined(USE_GEOM_CACHES)
bool CREGeomCache::mfDraw([[maybe_unused]] CShader* pShader, [[maybe_unused]] SShaderPass* pShaderPass)
{
    return true;
}
#endif

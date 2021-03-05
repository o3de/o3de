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

// Description : draw, create decals on the world


#include "Cry3DEngine_precompiled.h"

#include "DecalManager.h"
#include "3dEngine.h"
#include "ObjMan.h"

#include <AzFramework/Terrain/TerrainDataRequestBus.h>

IGeometry* CDecal::s_pSphere = 0;

void CDecal::ResetStaticData()
{
    SAFE_RELEASE(s_pSphere);
}

int CDecal::Update(bool& active, const float fFrameTime)
{
    // process life time and disable decal when needed
    m_fLifeTime -= fFrameTime;

    if (m_fLifeTime < 0)
    {
        active = 0;
        FreeRenderData();
    }
    else if (m_ownerInfo.pRenderNode && m_ownerInfo.pRenderNode->m_nInternalFlags & IRenderNode::UPDATE_DECALS)
    {
        active = false;
        return 1;
    }
    return 0;
}

Vec3 CDecal::GetWorldPosition()
{
    Vec3 vPos = m_vPos;

    if (m_ownerInfo.pRenderNode)
    {
        if (m_eDecalType == eDecalType_OS_SimpleQuad || m_eDecalType == eDecalType_OS_OwnersVerticesUsed)
        {
            assert(m_ownerInfo.pRenderNode);
            if (m_ownerInfo.pRenderNode)
            {
                Matrix34A objMat;
                if (IStatObj* pEntObject = m_ownerInfo.GetOwner(objMat))
                {
                    vPos = objMat.TransformPoint(vPos);
                }
            }
        }
    }

    return vPos;
}

void CDecal::Render(const float fCurTime, int nAfterWater, float fDistanceFading, float fDistance, const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter)
{
    FUNCTION_PROFILER_3DENGINE;

    if (!m_pMaterial || !m_pMaterial->GetShaderItem().m_pShader || m_pMaterial->GetShaderItem().m_pShader->GetShaderType() != eST_General)
    {
        return; // shader not supported for decals
    }
    // Get decal alpha from life time
    float fAlpha = m_fLifeTime * 2;

    if (fAlpha > 1.f)
    {
        fAlpha = 1.f;
    }
    else if (fAlpha < 0)
    {
        return;
    }

    fAlpha *= fDistanceFading;

    float fSizeK;
    if (m_fGrowTime)
    {
        fSizeK = min(1.f, sqrt_tpl((fCurTime - m_fLifeBeginTime) / m_fGrowTime));
    }
    else
    {
        fSizeK = 1.f;
    }

    float fSizeAlphaK;
    if (m_fGrowTimeAlpha)
    {
        fSizeAlphaK = min(1.f, sqrt_tpl((fCurTime - m_fLifeBeginTime) / m_fGrowTimeAlpha));
    }
    else
    {
        fSizeAlphaK = 1.f;
    }

    if (m_bDeferred)
    {
        SDeferredDecal newItem;
        newItem.fAlpha = fAlpha;
        newItem.pMaterial = m_pMaterial;
        newItem.nSortOrder = m_sortPrio;
        newItem.nFlags = 0;

        Vec3 vRight, vUp, vNorm;
        Matrix34A objMat;

        if (IStatObj* pEntObject = m_ownerInfo.GetOwner(objMat))
        {
            vRight = objMat.TransformVector(m_vRight * m_fSize);
            vUp    = objMat.TransformVector(m_vUp * m_fSize);
            vNorm  = objMat.TransformVector((Vec3(m_vRight).Cross(m_vUp)) * m_fSize);
        }
        else
        {
            vRight = (m_vRight * m_fSize);
            vUp    = (m_vUp * m_fSize);
            vNorm  = ((Vec3(m_vRight).Cross(m_vUp)) * m_fSize);
        }

        Matrix33 matRotation;
        matRotation.SetColumn(0, vRight);
        matRotation.SetColumn(1, vUp);
        matRotation.SetColumn(2, vNorm * GetFloatCVar(e_DecalsDefferedDynamicDepthScale));
        newItem.projMatrix.SetRotation33(matRotation);
        newItem.projMatrix.SetTranslation(m_vWSPos + vNorm * .1f * m_fWSSize);

        if (m_fGrowTimeAlpha)
        {
            newItem.fGrowAlphaRef = max(.02f, 1.f - fSizeAlphaK);
        }
        else
        {
            newItem.fGrowAlphaRef = 0;
        }

        GetRenderer()->EF_AddDeferredDecal(newItem);
        return;
    }

    switch (m_eDecalType)
    {
    case eDecalType_WS_Merged:
    case eDecalType_OS_OwnersVerticesUsed:
    {
        // check if owner mesh was deleted
        if (m_pRenderMesh && (m_pRenderMesh->GetVertexContainer() == m_pRenderMesh) && m_pRenderMesh->GetVerticesCount() < 3)
        {
            FreeRenderData();
        }

        if (!m_pRenderMesh)
        {
            break;
        }

        // setup transformation
        CRenderObject* pObj = GetRenderer()->EF_GetObject_Temp(passInfo.ThreadID());
        if (!pObj)
        {
            return;
        }
        pObj->m_fSort = 0;
        pObj->m_RState = 0;

        Matrix34A objMat;
        if (m_ownerInfo.pRenderNode && !m_ownerInfo.GetOwner(objMat))
        {
            assert(0);
            return;
        }
        else
        if (!m_ownerInfo.pRenderNode)
        {
            objMat.SetIdentity();
            if (m_eDecalType == eDecalType_WS_Merged)
            {
                objMat.SetTranslation(m_vPos);
            }
        }

        pObj->m_II.m_Matrix = objMat;

        pObj->m_nSort = m_sortPrio;

        // somehow it's need's to be twice bigger to be same as simple decals
        float fSize2 = m_fSize * fSizeK * 2.f;    ///m_ownerInfo.pRenderNode->GetScale();
        if (fSize2 < 0.0001f)
        {
            return;
        }

        // setup texgen
        // S component
        float correctScale(-1);
        m_arrBigDecalRMCustomData[0] = correctScale * m_vUp.x / fSize2;
        m_arrBigDecalRMCustomData[1] = correctScale * m_vUp.y / fSize2;
        m_arrBigDecalRMCustomData[2] = correctScale * m_vUp.z / fSize2;

        Vec3 vPosDecS = m_vPos;
        if (m_eDecalType == eDecalType_WS_Merged)
        {
            vPosDecS.zero();
        }

        float D0 =
            m_arrBigDecalRMCustomData[0] * vPosDecS.x +
            m_arrBigDecalRMCustomData[1] * vPosDecS.y +
            m_arrBigDecalRMCustomData[2] * vPosDecS.z;

        m_arrBigDecalRMCustomData[3] = -D0 + 0.5f;

        // T component
        m_arrBigDecalRMCustomData[4] = m_vRight.x / fSize2;
        m_arrBigDecalRMCustomData[5] = m_vRight.y / fSize2;
        m_arrBigDecalRMCustomData[6] = m_vRight.z / fSize2;

        float D1 =
            m_arrBigDecalRMCustomData[4] * vPosDecS.x +
            m_arrBigDecalRMCustomData[5] * vPosDecS.y +
            m_arrBigDecalRMCustomData[6] * vPosDecS.z;

        m_arrBigDecalRMCustomData[7] = -D1 + 0.5f;

        // pass attenuation info
        m_arrBigDecalRMCustomData[8] = vPosDecS.x;
        m_arrBigDecalRMCustomData[9] = vPosDecS.y;
        m_arrBigDecalRMCustomData[10] = vPosDecS.z;
        m_arrBigDecalRMCustomData[11] = m_fSize;

        // N component
        Vec3 vNormal(Vec3(correctScale* m_vUp).Cross(m_vRight).GetNormalized());
        m_arrBigDecalRMCustomData[12] = vNormal.x * (m_fSize / m_fWSSize);
        m_arrBigDecalRMCustomData[13] = vNormal.y * (m_fSize / m_fWSSize);
        m_arrBigDecalRMCustomData[14] = vNormal.z * (m_fSize / m_fWSSize);
        m_arrBigDecalRMCustomData[15] = 0;

        CStatObj* pBody = NULL;
        bool bUseBending = GetCVars()->e_VegetationBending != 0;

        // draw complex decal using new indices and original object vertices
        pObj->m_fAlpha = fAlpha;
        pObj->m_ObjFlags |= FOB_DECAL | FOB_DECAL_TEXGEN_2D;
        pObj->m_nTextureID = -1;
        //pObj->m_nTextureID1 = -1;
        pObj->m_II.m_AmbColor = m_vAmbient;
        m_pRenderMesh->SetREUserData(m_arrBigDecalRMCustomData, 0, fAlpha);
        m_pRenderMesh->AddRenderElements(m_pMaterial, pObj, passInfo, EFSLIST_GENERAL, nAfterWater);
    }
    break;

    case eDecalType_OS_SimpleQuad:
    {
        assert(m_ownerInfo.pRenderNode);
        if (!m_ownerInfo.pRenderNode)
        {
            break;
        }

        // transform decal in software from owner space into world space and render as quad
        Matrix34A objMat;
        IStatObj* pEntObject = m_ownerInfo.GetOwner(objMat);
        if (!pEntObject)
        {
            break;
        }

        Vec3 vPos    = objMat.TransformPoint(m_vPos);
        Vec3 vRight = objMat.TransformVector(m_vRight * m_fSize);
        Vec3 vUp    = objMat.TransformVector(m_vUp * m_fSize);
        UCol uCol;

        uCol.dcolor = 0xffffffff;
        uCol.bcolor[3] = fastround_positive(fAlpha * 255);

        GetObjManager()->AddDecalToRenderer(fDistance, m_pMaterial, m_sortPrio, vRight * fSizeK, vUp * fSizeK, uCol,
            OS_ALPHA_BLEND, m_vAmbient, vPos, nAfterWater, passInfo, rendItemSorter);
    }
    break;

    case eDecalType_WS_SimpleQuad:
    {       // draw small world space decal untransformed
        UCol uCol;
        uCol.dcolor = 0;
        uCol.bcolor[3] = fastround_positive(fAlpha * 255);

        GetObjManager()->AddDecalToRenderer(fDistance, m_pMaterial, m_sortPrio, m_vRight * m_fSize * fSizeK,
            m_vUp * m_fSize * fSizeK, uCol, OS_ALPHA_BLEND, m_vAmbient, m_vPos, nAfterWater, passInfo,
            rendItemSorter);
    }
    break;

    case eDecalType_WS_OnTheGround:
    {
        RenderBigDecalOnTerrain(fAlpha, fSizeK, passInfo);
    }
    break;
    }
}

void CDecal::FreeRenderData()
{
    // delete render mesh
    m_pRenderMesh = NULL;

    m_ownerInfo.pRenderNode = 0;
}

void CDecal::RenderBigDecalOnTerrain(float fAlpha, float fScale, const SRenderingPassInfo& passInfo)
{
    float fRadius = m_fSize * fScale;

    // check terrain bounds
    if (m_vPos.x < -fRadius || m_vPos.y < -fRadius)
    {
        return;
    }
    auto terrain = AzFramework::Terrain::TerrainDataRequestBus::FindFirstHandler();
    if (!terrain)
    {
        return;
    }

    const AZ::Aabb terrainAabb = terrain->GetTerrainAabb();
    const float terrainSizeX = terrainAabb.GetXExtent();
    const float terrainSizeY = terrainAabb.GetYExtent();

    if (m_vPos.x >= terrainSizeX + fRadius || m_vPos.y >= terrainSizeY + fRadius)
    {
        return;
    }

    const AZ::Vector2 terrainGridResolution = terrain->GetTerrainGridResolution();
    const int nUsintSize = static_cast<int>(AZ::GetMax(terrainGridResolution.GetX(), terrainGridResolution.GetY()));
    fRadius += nUsintSize;

    const float terrainHeight = terrain->GetHeightFromFloats(m_vPos.x, m_vPos.y, AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP);
    if (fabs(m_vPos.z - terrainHeight) > fRadius)
    {
        return; // too far from ground surface
    }
    // setup texgen
    float fSize = m_fSize * fScale;
    if (fSize < 0.05f)
    {
        return;
    }

    // m_vUp and m_vRight are the scaled binormal and tangent
    // The shader projects the vertex pass position onto these to calculate UVs
    // However binormal and tangent are only half the height and width of the decal
    // So we need to double them
    Vec3 uvUp = m_vUp * 2.0f;
    Vec3 uvRight = m_vRight * 2.0f;

    // Let T denote the tangent, B the binormal and P the vertex position in decal space
    // The shader calculates UVs by projecting vertex position onto the tangent and binormal:
    // U = dot( T, P )
    // V = dot( B, P )
    // UVs should range 0...1, so normalize by the length of the tangent and binormal:
    // U = dot( normalize(T), P / length(T) )
    // V = dot( normalize(B), P / length(B) )
    // This is equivalent to:
    // U = dot( T, P ) / ( length(T) * length(T) )
    // V = dot( B, P ) / ( length(B) * length(B) )
    // The length squared can be folded into the tangent and binormal:
    // U = dot( T / lengthSq(T), P )
    // V = dot( B / lengthSq(B), P )
    // Hence:
    uvUp /= uvUp.GetLengthSquared();
    uvRight /= uvRight.GetLengthSquared();

    // S component
    float correctScale(-1);
    m_arrBigDecalRMCustomData[0] = correctScale * uvUp.x / fSize;
    m_arrBigDecalRMCustomData[1] = correctScale * uvUp.y / fSize;
    m_arrBigDecalRMCustomData[2] = correctScale * uvUp.z / fSize;

    // T component
    m_arrBigDecalRMCustomData[4] = uvRight.x / fSize;
    m_arrBigDecalRMCustomData[5] = uvRight.y / fSize;
    m_arrBigDecalRMCustomData[6] = uvRight.z / fSize;

    // UV centering happens in the shader
    // See shader function _TCModifyDecal
    m_arrBigDecalRMCustomData[3] = 0.0f;
    m_arrBigDecalRMCustomData[7] = 0.0f;

    // pass attenuation info
    m_arrBigDecalRMCustomData[8] = 0;
    m_arrBigDecalRMCustomData[9] = 0;
    m_arrBigDecalRMCustomData[10] = 0;
    m_arrBigDecalRMCustomData[11] = fSize * 2.0f;

    Vec3 vNormal(Vec3(correctScale* m_vUp).Cross(m_vRight).GetNormalized());
    m_arrBigDecalRMCustomData[12] = vNormal.x;
    m_arrBigDecalRMCustomData[13] = vNormal.y;
    m_arrBigDecalRMCustomData[14] = vNormal.z;
    m_arrBigDecalRMCustomData[15] = 0;

    CRenderObject* pObj = GetIdentityCRenderObject(passInfo.ThreadID());
    if (!pObj)
    {
        return;
    }

    pObj->m_II.m_Matrix.SetTranslation(m_vPos);

    pObj->m_fAlpha = fAlpha;
    pObj->m_ObjFlags |= FOB_DECAL | FOB_DECAL_TEXGEN_2D;
    pObj->m_nTextureID = -1;
    //pObj->m_nTextureID1 = -1;
    pObj->m_II.m_AmbColor = m_vAmbient;

    pObj->m_nSort = m_sortPrio;

    Plane planes[4];
    planes[0].SetPlane(m_vRight, m_vRight * m_fSize + m_vPos);
    planes[1].SetPlane(-m_vRight, -m_vRight * m_fSize + m_vPos);
    planes[2].SetPlane(m_vUp,    m_vUp * m_fSize    + m_vPos);
    planes[3].SetPlane(-m_vUp,   -m_vUp * m_fSize    + m_vPos);
}

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

#include "Cry3DEngine_precompiled.h"
#include "DistanceCloudRenderNode.h"
#include "VisAreas.h"
#include "ObjMan.h"
#include "MatMan.h"
#include "Environment/OceanEnvironmentBus.h"

CDistanceCloudRenderNode::CDistanceCloudRenderNode()
    :   m_pos(0, 0, 0)
    , m_sizeX(1)
    , m_sizeY(1)
    , m_rotationZ(0)
    , m_pMaterial(NULL)
{
}


CDistanceCloudRenderNode::~CDistanceCloudRenderNode()
{
    Get3DEngine()->FreeRenderNodeState(this);
}


SDistanceCloudProperties CDistanceCloudRenderNode::GetProperties() const
{
    SDistanceCloudProperties properties;

    properties.m_sizeX = m_sizeX;
    properties.m_sizeY = m_sizeY;
    properties.m_rotationZ = m_rotationZ;
    properties.m_pos = m_pos;
    properties.m_pMaterialName = 0; // query materialID instead!

    return properties;
}


void CDistanceCloudRenderNode::SetProperties(const SDistanceCloudProperties& properties)
{
    // register material
    m_pMaterial = GetMatMan()->LoadMaterial(properties.m_pMaterialName, false);

    // copy distance cloud properties
    m_sizeX = properties.m_sizeX;
    m_sizeY = properties.m_sizeY;
    m_rotationZ = properties.m_rotationZ;
    m_pos = properties.m_pos;
}


void CDistanceCloudRenderNode::SetMatrix(const Matrix34& mat)
{
    Get3DEngine()->UnRegisterEntityAsJob(this);

    m_pos = mat.GetTranslation();

    m_WSBBox.SetTransformedAABB(mat, AABB(-Vec3(1, 1, 1e-4f), Vec3(1, 1, 1e-4f)));

    Get3DEngine()->RegisterEntity(this);
}


const char* CDistanceCloudRenderNode::GetEntityClassName() const
{
    return "DistanceCloud";
}


const char* CDistanceCloudRenderNode::GetName() const
{
    return "DistanceCloud";
}

static inline uint16 HalfFlip(uint16 h)
{
    uint16 mask = -int16(h >> 15) | 0x8000;
    return h ^ mask;
}


void CDistanceCloudRenderNode::Render(const SRendParams& rParam, const SRenderingPassInfo& passInfo)
{
    FUNCTION_PROFILER_3DENGINE;

    _smart_ptr<IMaterial> pMaterial(GetMaterial());

    if (!passInfo.RenderClouds() || !pMaterial)
    {
        return; // false;
    }
    CRenderObject* pOb(gEnv->pRenderer->EF_GetObject_Temp(passInfo.ThreadID()));
    if (!pOb)
    {
        return; // false;
    }
    const CCamera& cam(passInfo.GetCamera());
    float zDist = cam.GetPosition().z - m_pos.z;
    if (cam.GetViewdir().z < 0)
    {
        zDist = -zDist;
    }
    pOb->m_nSort = HalfFlip(CryConvertFloatToHalf(zDist));
    //pOb->m_II.m_Matrix.SetIdentity();

    // fill general vertex data
    f32 sinZ(0), cosZ(1);
    sincos_tpl(DEG2RAD(m_rotationZ), &sinZ, &cosZ);
    Vec3 right(m_sizeX * cosZ, m_sizeY * sinZ, 0);
    Vec3 up(-m_sizeX * sinZ, m_sizeY * cosZ, 0);

    SVF_P3F_C4B_T2F pVerts[4];
    pVerts[0].xyz = (-right - up) + m_pos;
    pVerts[0].st = Vec2(0, 1);
    pVerts[0].color.dcolor = ~0;

    pVerts[1].xyz = (right - up) + m_pos;
    pVerts[1].st = Vec2(1, 1);
    pVerts[1].color.dcolor = ~0;

    pVerts[2].xyz = (right + up) + m_pos;
    pVerts[2].st = Vec2(1, 0);
    pVerts[2].color.dcolor = ~0;

    pVerts[3].xyz = (-right + up) + m_pos;
    pVerts[3].st = Vec2(0, 0);
    pVerts[3].color.dcolor = ~0;

    // prepare tangent space (tangent, bitangent) and fill it in
    Vec3 rightUnit(cosZ, sinZ, 0);
    Vec3 upUnit(-sinZ, cosZ, 0);

    SPipTangents pTangents[4];

    pTangents[0] = SPipTangents(rightUnit, -upUnit, 1);
    pTangents[1] = pTangents[0];
    pTangents[2] = pTangents[0];
    pTangents[3] = pTangents[0];

    // prepare indices
    uint16 pIndices[6];
    pIndices[0] = 0;
    pIndices[1] = 1;
    pIndices[2] = 2;

    pIndices[3] = 0;
    pIndices[4] = 2;
    pIndices[5] = 3;

    int afterWater(GetObjManager()->IsAfterWater(m_pos, passInfo));
    GetRenderer()->EF_AddPolygonToScene(pMaterial->GetShaderItem(), 4, pVerts, pTangents, pOb, passInfo, pIndices, 6, afterWater, SRendItemSorter(rParam.rendItemSorter));

    //  return true;
}


void CDistanceCloudRenderNode::SetMaterial(_smart_ptr<IMaterial> pMat)
{
    m_pMaterial = pMat;
}


void CDistanceCloudRenderNode::Precache()
{
}


void CDistanceCloudRenderNode::GetMemoryUsage(ICrySizer* pSizer) const
{
    SIZER_COMPONENT_NAME(pSizer, "DistanceCloudNode");
    pSizer->AddObject(this, sizeof(*this));
}

void CDistanceCloudRenderNode::OffsetPosition(const Vec3& delta)
{
    if (m_pRNTmpData)
    {
        m_pRNTmpData->OffsetPosition(delta);
    }
    m_pos += delta;
    m_WSBBox.Move(delta);
}

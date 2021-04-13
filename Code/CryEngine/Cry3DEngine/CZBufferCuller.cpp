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

// Description : Occlusion buffer


#include "Cry3DEngine_precompiled.h"
#include "CZBufferCuller.h"

SHWOccZBuffer HWZBuffer;

void CZBufferCuller::BeginFrame(const SRenderingPassInfo& passInfo)
{
    if (!GetCVars()->e_CoverageBuffer)
    {
        return;
    }
    const CCamera& rCam = passInfo.GetCamera();
    m_AccurateTest  =   GetCVars()->e_CoverageBufferAccurateOBBTest;

    m_Treshold          =   GetCVars()->e_CoverageBufferTolerance;


    FUNCTION_PROFILER_3DENGINE;
    if (GetCVars()->e_CoverageBufferDebugFreeze || GetCVars()->e_CameraFreeze)
    {
        return;
    }

    m_ObjectsTested =
        m_ObjectsTestedAndRejected =    0;
    //to enable statistics
    m_Camera        =   rCam;
    m_Position  =   rCam.GetPosition();
    uint32 oldSizeX = m_SizeX;

    const uint32 sizeX = min(max(1, GetCVars()->e_CoverageBufferResolution), 1024);
    const uint32 sizeY = sizeX;

    m_SizeX                 =   sizeX;
    m_SizeY                 = sizeY;
    m_fSizeX                =   static_cast<f32>(sizeX);
    m_fSizeY                =   static_cast<f32>(sizeY);
    m_fSizeZ                =   static_cast<f32>(TZB_MAXDEPTH);


    if (oldSizeX != sizeX)
    {
        CryModuleMemalignFree(m_ZBuffer);
        //64-byte buffer to avoid memory page issues when vector loading
        m_ZBuffer = (TZBZexel*)CryModuleMemalign((sizeof(TZBZexel) * sizeX * sizeY) + 64, 128);
    }

    m_MatViewProj.Transpose();

    m_RotationSafe  =   GetCVars()->e_CoverageBufferRotationSafeCheck;

    m_DebugFreez        =   GetCVars()->e_CoverageBufferDebugFreeze != 0;
}

void CZBufferCuller::ReloadBuffer(const uint32 BufferID)
{
    if (m_DebugFreez)
    {
        return;
    }
    m_Bias              =   BufferID == 0 ? static_cast<int32>(GetCVars()->e_CoverageBufferBias) : 0;
}


CZBufferCuller::CZBufferCuller()
    : m_OutdoorVisible(1)
    , m_MatViewProj(IDENTITY)
{
    m_SizeX =
        m_SizeY = min(max(1, GetCVars()->e_CoverageBufferResolution), 1024);
    m_ZBuffer = (TZBZexel*)CryModuleMemalign((sizeof(TZBZexel) * m_SizeX * m_SizeY) + 64, 128);

    m_ObjectsTested =
        m_ObjectsTestedAndRejected  =   0;
}

bool CZBufferCuller::IsBoxVisible(const AABB& objBox, [[maybe_unused]] uint32* const __restrict pResDest)
{
    FUNCTION_PROFILER_3DENGINE;
    m_ObjectsTested++;
    Vec4 Verts[8] =
    {
        m_MatViewProj* Vec4(objBox.min.x, objBox.min.y, objBox.min.z, 1.f),//0
        m_MatViewProj * Vec4(objBox.min.x, objBox.max.y, objBox.min.z, 1.f),//1
        m_MatViewProj * Vec4(objBox.max.x, objBox.min.y, objBox.min.z, 1.f),//2
        m_MatViewProj * Vec4(objBox.max.x, objBox.max.y, objBox.min.z, 1.f),//3
        m_MatViewProj * Vec4(objBox.min.x, objBox.min.y, objBox.max.z, 1.f),//4
        m_MatViewProj * Vec4(objBox.min.x, objBox.max.y, objBox.max.z, 1.f),//5
        m_MatViewProj * Vec4(objBox.max.x, objBox.min.y, objBox.max.z, 1.f),//6
        m_MatViewProj * Vec4(objBox.max.x, objBox.max.y, objBox.max.z, 1.f)//7
    };
    bool CutNearPlane = Verts[0].w <= 0.f;
    CutNearPlane    |=  Verts[1].w <= 0.f;
    CutNearPlane    |=  Verts[2].w <= 0.f;
    CutNearPlane    |=  Verts[3].w <= 0.f;
    CutNearPlane    |=  Verts[4].w <= 0.f;
    CutNearPlane    |=  Verts[5].w <= 0.f;
    CutNearPlane    |=  Verts[6].w <= 0.f;
    CutNearPlane    |=  Verts[7].w <= 0.f;
    if (CutNearPlane)
    {
        return true;
    }

    IF (m_RotationSafe == 1, 0)
    {
        return Rasterize<1>(Verts, 8);
    }
    IF (m_RotationSafe == 2, 1)
    {
        return Rasterize<2>(Verts, 8);
    }
    return Rasterize<0>(Verts, 8);

    ++m_ObjectsTestedAndRejected;

    return false;
}

static int sh   =   8;
void CZBufferCuller::DrawDebug(int32 nStep)
{ // project buffer to the screen
    nStep   %=  32;
    if (!nStep)
    {
        return;
    }

    const CCamera& rCam = GetCamera();
    float farPlane = rCam.GetFarPlane();
    float nearPlane = rCam.GetNearPlane();

    float a = farPlane / (farPlane - nearPlane);
    float b = farPlane * nearPlane / (nearPlane - farPlane);

    const float scale = 10.0f;

    TransformationMatrices backupSceneMatrices;

    m_pRenderer->Set2DMode(m_SizeX, m_SizeY, backupSceneMatrices);
    SAuxGeomRenderFlags Flags   =   e_Def3DPublicRenderflags;
    Flags.SetDepthWriteFlag(e_DepthWriteOff);
    Flags.SetAlphaBlendMode(e_AlphaBlended);
    m_pRenderer->GetIRenderAuxGeom()->SetRenderFlags(Flags);
    Vec3 vSize(.4f, .4f, .4f);
    if (nStep == 1)
    {
        vSize = Vec3(.5f, .5f, .5f);
    }
    for (uint32 y = 0; y < m_SizeY; y += nStep)
    {
        for (uint32 x = 0; x < m_SizeX; x += nStep)
        {
            Vec3 vPos((float)x, (float)(m_SizeY - y - 1), 0);
            vPos += Vec3(0.5f, -0.5f, 0);
            const uint32 Value  =   m_ZBuffer[x + y * m_SizeX];
            //Value>>=sh;

            float w = Value / (65535.0f);

            float z = b / (w - a);

            uint32 ValueC =  255u - min(255u, (uint32)(z * scale));
            ColorB col;

            col = ColorB(ValueC, ValueC, ValueC, 200);


            if (Value != 0xffff)
            {
                //ColorB col((Value&31)<<3,((Value>>5)&31)<<3,((Value>>10)&63)<<2,200);
                GetRenderer()->GetIRenderAuxGeom()->DrawAABB(AABB(vPos - vSize, vPos + vSize), nStep <= 2, col, eBBD_Faceted);
            }
        }
    }
    //m_pRenderer->GetIRenderAuxGeom()->Flush();
    m_pRenderer->Unset2DMode(backupSceneMatrices);
}

void CZBufferCuller::GetMemoryUsage(ICrySizer* pSizer) const
{
    SIZER_COMPONENT_NAME(pSizer, "CoverageBuffer");
    pSizer->AddObject(m_ZBuffer, sizeof(TZBZexel) * m_SizeX * m_SizeY);
}
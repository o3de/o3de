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

// Description : implementation of ambient occlusion related features.


#include "RenderDll_precompiled.h"
#include "DriverD3D.h"
#include "I3DEngine.h"
#include "D3DPostProcess.h"
#include "../Common/Textures/TextureHelpers.h"


#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define D3DAMBIENTOCCLUSION_CPP_SECTION_1 1
#define D3DAMBIENTOCCLUSION_CPP_SECTION_2 2
#endif

#ifdef USE_NV_API
    #include <nvapi.h>
#endif

// TODO: Unify with other deferred primitive implementation
const t_arrDeferredMeshVertBuff& CD3D9Renderer::GetDeferredUnitBoxVertexBuffer() const
{
    return m_arrDeferredVerts;
}

const t_arrDeferredMeshIndBuff& CD3D9Renderer::GetDeferredUnitBoxIndexBuffer() const
{
    return m_arrDeferredInds;
}

void CD3D9Renderer::CreateDeferredUnitBox(t_arrDeferredMeshIndBuff& indBuff, t_arrDeferredMeshVertBuff& vertBuff)
{
    SVF_P3F_C4B_T2F vert;
    Vec3 vNDC;

    indBuff.clear();
    indBuff.reserve(36);

    vertBuff.clear();
    vertBuff.reserve(8);

    //Create frustum
    for (int i = 0; i < 8; i++)
    {
        //Generate screen space frustum (CCW faces)
        vNDC = Vec3((i == 0 || i == 1 || i == 4 || i == 5) ? 0.0f : 1.0f,
                (i == 0 || i == 3 || i == 4 || i == 7) ? 0.0f : 1.0f,
                (i == 0 || i == 1 || i == 2 || i == 3) ? 0.0f : 1.0f
                );
        vert.xyz = vNDC;
        vert.st = Vec2(0.0f, 0.0f);
        vert.color.dcolor = -1;
        vertBuff.push_back(vert);
    }

    //CCW faces
    uint16 nFaces[6][4] = {
        {0, 1, 2, 3},
        {4, 7, 6, 5},
        {0, 3, 7, 4},
        {1, 5, 6, 2},
        {0, 4, 5, 1},
        {3, 2, 6, 7}
    };

    //init indices for triangles drawing
    for (int i = 0; i < 6; i++)
    {
        indBuff.push_back((uint16)  nFaces[i][0]);
        indBuff.push_back((uint16)  nFaces[i][1]);
        indBuff.push_back((uint16)  nFaces[i][2]);

        indBuff.push_back((uint16)  nFaces[i][0]);
        indBuff.push_back((uint16)  nFaces[i][2]);
        indBuff.push_back((uint16)  nFaces[i][3]);
    }
}

void CD3D9Renderer::SetDepthBoundTest(float fMin, float fMax, bool bEnable)
{
    if (!m_bDeviceSupports_NVDBT)
    {
        return;
    }

    m_bDepthBoundsEnabled = bEnable;
    if (bEnable)
    {
        m_fDepthBoundsMin = fMin;
        m_fDepthBoundsMax = fMax;
#if defined(OPENGL) && !DXGL_FULL_EMULATION
        DXGLSetDepthBoundsTest(true, fMin, fMax);
#elif defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DAMBIENTOCCLUSION_CPP_SECTION_1
    #include AZ_RESTRICTED_FILE(D3DAmbientOcclusion_cpp)
#elif defined (USE_NV_API) //transparent execution without NVDB
        NvAPI_Status status = NvAPI_D3D11_SetDepthBoundsTest(&GetDevice(), bEnable, fMin, fMax);
        assert(status == NVAPI_OK);
#endif
    }
    else // disable depth bound test
    {
        m_fDepthBoundsMin = 0;
        m_fDepthBoundsMax = 1.0f;
#if defined(OPENGL) && !DXGL_FULL_EMULATION
        DXGLSetDepthBoundsTest(false, 0.0f, 1.0f);
#elif defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DAMBIENTOCCLUSION_CPP_SECTION_2
    #include AZ_RESTRICTED_FILE(D3DAmbientOcclusion_cpp)
#elif defined (USE_NV_API)
        NvAPI_Status status = NvAPI_D3D11_SetDepthBoundsTest(&GetDevice(), bEnable, fMin, fMax);
        assert(status == NVAPI_OK);
#endif
    }
}

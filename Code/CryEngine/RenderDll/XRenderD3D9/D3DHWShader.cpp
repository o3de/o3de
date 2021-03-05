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
#include "DriverD3D.h"
#include "I3DEngine.h"
#include "CryCrc32.h"
#include "../Common/Shaders/RemoteCompiler.h"
#include "../Common/PostProcess/PostEffects.h"
#include "D3DPostProcess.h"
#include "../Common/Textures/TextureHelpers.h"
#include "../Common/Textures/TextureManager.h"
#include "../Common/Include_HLSL_CPP_Shared.h"
#include "../Common/TypedConstantBuffer.h"
#include "GraphicsPipeline/FurBendData.h"
#include "GraphicsPipeline/FurPasses.h"
#include <AzFramework/Terrain/TerrainDataRequestBus.h>

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define D3DHWSHADER_CPP_SECTION_1 1
#define D3DHWSHADER_CPP_SECTION_2 2
#endif

#if defined(FEATURE_SVO_GI)
#include "D3D_SVO.h"
#endif
#include "../Cry3DEngine/Environment/OceanEnvironmentBus.h"

CHWShader_D3D::SHWSInstance *CHWShader_D3D::s_pCurInstGS; bool CHWShader_D3D::s_bFirstGS = true;
CHWShader_D3D::SHWSInstance *CHWShader_D3D::s_pCurInstHS; bool CHWShader_D3D::s_bFirstHS = true;
CHWShader_D3D::SHWSInstance *CHWShader_D3D::s_pCurInstDS; bool CHWShader_D3D::s_bFirstDS = true;
CHWShader_D3D::SHWSInstance *CHWShader_D3D::s_pCurInstCS; bool CHWShader_D3D::s_bFirstCS = true;
CHWShader_D3D::SHWSInstance *CHWShader_D3D::s_pCurInstVS; bool CHWShader_D3D::s_bFirstVS = true;
CHWShader_D3D::SHWSInstance *CHWShader_D3D::s_pCurInstPS; bool CHWShader_D3D::s_bFirstPS = true;

#if !defined(_RELEASE)
AZStd::unordered_set<uint32_t, AZStd::hash<uint32_t>, AZStd::equal_to<uint32_t>, AZ::StdLegacyAllocator> CHWShader_D3D::s_ErrorsLogged;
#endif

int CHWShader_D3D::s_nActivationFailMask = 0;

AZStd::vector<SShaderTechniqueStat, AZ::StdLegacyAllocator> g_SelectedTechs;

bool CHWShader_D3D::s_bInitShaders = true;

int CHWShader_D3D::s_nResetDeviceFrame = -1;
int CHWShader_D3D::s_nInstFrame = -1;

SD3DShader* CHWShader::s_pCurPS;
SD3DShader* CHWShader::s_pCurVS;
SD3DShader* CHWShader::s_pCurGS;
SD3DShader* CHWShader::s_pCurDS;
SD3DShader* CHWShader::s_pCurHS;
SD3DShader* CHWShader::s_pCurCS;

FXShaderCache CHWShader::m_ShaderCache;
FXShaderCacheNames CHWShader::m_ShaderCacheList;

// REFACTOR NOTE:
//  Everything in the block should be pulled into its own file once stablized back to mainline.
#pragma region ShaderConstants

CHWShader_D3D::SCGTextures CHWShader_D3D::s_PF_Textures;     // Per-frame textures
CHWShader_D3D::SCGSamplers CHWShader_D3D::s_PF_Samplers;     // Per-frame samplers

namespace
{
    alloc_info_struct* GetFreeChunk(int bytes_count, int nBufSize, PodArray<alloc_info_struct>& alloc_info, const char* szSource)
    {
        int best_i = -1;
        int min_size = 10000000;

        // find best chunk
        for (int i = 0; i < alloc_info.Count(); i++)
        {
            if (!alloc_info[i].busy)
            {
                if (alloc_info[i].bytes_num >= bytes_count)
                {
                    if (alloc_info[i].bytes_num < min_size)
                    {
                        best_i = i;
                        min_size = alloc_info[i].bytes_num;
                    }
                }
            }
        }

        if (best_i >= 0)
        { // use best free chunk
            alloc_info[best_i].busy = true;
            alloc_info[best_i].szSource = szSource;

            int bytes_free = alloc_info[best_i].bytes_num - bytes_count;
            if (bytes_free > 0)
            {
                // modify reused shunk
                alloc_info[best_i].bytes_num = bytes_count;

                // insert another free shunk
                alloc_info_struct new_chunk;
                new_chunk.bytes_num = bytes_free;
                new_chunk.ptr = alloc_info[best_i].ptr + alloc_info[best_i].bytes_num;
                new_chunk.busy = false;

                if (best_i < alloc_info.Count() - 1) // if not last
                {
                    alloc_info.InsertBefore(new_chunk, best_i + 1);
                }
                else
                {
                    alloc_info.Add(new_chunk);
                }
            }

            return &alloc_info[best_i];
        }

        int res_ptr = 0;

        int piplevel = alloc_info.Count() ? (alloc_info.Last().ptr - alloc_info[0].ptr) + alloc_info.Last().bytes_num : 0;
        if (piplevel + bytes_count >= nBufSize)
        {
            return NULL;
        }
        else
        {
            res_ptr = piplevel;
        }

        // register new chunk
        alloc_info_struct ai;
        ai.ptr = res_ptr;
        ai.szSource = szSource;
        ai.bytes_num = bytes_count;
        ai.busy = true;
        alloc_info.Add(ai);

        return &alloc_info[alloc_info.Count() - 1];
    }

    bool ReleaseChunk(int p, PodArray<alloc_info_struct>& alloc_info)
    {
        for (int i = 0; i < alloc_info.Count(); i++)
        {
            if (alloc_info[i].ptr == p)
            {
                alloc_info[i].busy = false;

                // delete info about last unused chunks
                while (alloc_info.Count() && alloc_info.Last().busy == false)
                {
                    alloc_info.Delete(alloc_info.Count() - 1);
                }

                // merge unused chunks
                for (int s = 0; s < alloc_info.Count() - 1; s++)
                {
                    assert(alloc_info[s].ptr < alloc_info[s + 1].ptr);

                    if (alloc_info[s].busy == false)
                    {
                        if (alloc_info[s + 1].busy == false)
                        {
                            alloc_info[s].bytes_num += alloc_info[s + 1].bytes_num;
                            alloc_info.Delete(s + 1);
                            s--;
                        }
                    }
                }

                return true;
            }
        }

        return false;
    }
}

DynArray<SCGParamPool> CGParamManager::s_Pools;
AZStd::vector<SCGParamsGroup, AZ::StdLegacyAllocator> CGParamManager::s_Groups;
AZStd::vector<uint32, AZ::StdLegacyAllocator> CGParamManager::s_FreeGroups;

SCGParamPool::SCGParamPool(int nEntries)
    : m_Params(new SCGParam[nEntries], nEntries)
{
}

SCGParamPool::~SCGParamPool()
{
    delete[] m_Params.begin();
}

SCGParamsGroup SCGParamPool::Alloc(int nEntries)
{
    SCGParamsGroup Group;

    alloc_info_struct* pAI = GetFreeChunk(nEntries, m_Params.size(), m_alloc_info, "CGParam");
    if (pAI)
    {
        Group.nParams = nEntries;
        Group.pParams = &m_Params[pAI->ptr];
    }

    return Group;
}

bool SCGParamPool::Free(SCGParamsGroup& Group)
{
    bool bRes = ReleaseChunk((int)(Group.pParams - m_Params.begin()), m_alloc_info);
    return bRes;
}

int CGParamManager::GetParametersGroup(SParamsGroup& InGr, int nId)
{
    std::vector<SCGParam>& InParams = nId > 1 ? InGr.Params_Inst : InGr.Params[nId];
    int32 i;
    int nParams = InParams.size();

    int nGroupSize = s_Groups.size();
    for (i = 0; i < nGroupSize; i++)
    {
        SCGParamsGroup& Gr = s_Groups[i];
        if (Gr.nParams != nParams)
        {
            continue;
        }
        int j;
        for (j = 0; j < nParams; j++)
        {
            if (InParams[j] != Gr.pParams[j])
            {
                break;
            }
        }
        if (j == nParams)
        {
            Gr.nRefCounter++;
            return i;
        }
    }

    SCGParamsGroup Group;
    SCGParamPool* pPool = NULL;
    for (i = 0; i < s_Pools.size(); i++)
    {
        pPool = &s_Pools[i];
        Group = pPool->Alloc(nParams);
        if (Group.nParams)
        {
            break;
        }
    }
    if (!Group.pParams)
    {
        pPool = NewPool(PARAMS_POOL_SIZE);
        Group = pPool->Alloc(nParams);
    }
    assert(Group.pParams);
    if (!Group.pParams)
    {
        return 0;
    }
    Group.nPool = i;
    uint32 n = s_Groups.size();
    if (s_FreeGroups.size())
    {
        n = s_FreeGroups.back();
        s_FreeGroups.pop_back();
        s_Groups[n] = Group;
    }
    else
    {
        s_Groups.push_back(Group);
    }

    for (i = 0; i < nParams; i++)
    {
        s_Groups[n].pParams[i] = InParams[i];
    }

    return n;
}

bool CGParamManager::FreeParametersGroup(int nIDGroup)
{
    if (nIDGroup < 0 || nIDGroup >= (int)s_Groups.size())
    {
        return false;
    }
    SCGParamsGroup& Group = s_Groups[nIDGroup];
    Group.nRefCounter--;
    if (Group.nRefCounter)
    {
        return true;
    }
    if (Group.nPool < 0 || Group.nPool >= s_Pools.size())
    {
        return false;
    }
    SCGParamPool& Pool = s_Pools[Group.nPool];
    if (!Pool.Free(Group))
    {
        return false;
    }
    for (int i = 0; i < Group.nParams; i++)
    {
        Group.pParams[i].m_Name.reset();
        SAFE_DELETE(Group.pParams[i].m_pData);
    }

    Group.nParams = 0;
    Group.nPool = 0;
    Group.pParams = 0;

    s_FreeGroups.push_back(nIDGroup);

    return true;
}

void CGParamManager::Init()
{
    s_FreeGroups.reserve(128); // Based on spear
    s_Groups.reserve(2048);
}
void CGParamManager::Shutdown()
{
    s_FreeGroups.clear();
    s_Pools.clear();
    s_Groups.clear();
}

SCGParamPool* CGParamManager::NewPool(int nEntries)
{
    return new(s_Pools.grow_raw())SCGParamPool(nEntries);
}


DEFINE_ALIGNED_DATA(UFloat4, s_ConstantScratchBuffer[48], 16);

namespace
{
    static inline void TransposeAndStore(UFloat4* sData, const Matrix44A& mMatrix)
    {
#    if defined(_CPU_SSE)
        __m128 row0 = _mm_load_ps(&mMatrix.m00);
        __m128 row1 = _mm_load_ps(&mMatrix.m10);
        __m128 row2 = _mm_load_ps(&mMatrix.m20);
        __m128 row3 = _mm_load_ps(&mMatrix.m30);
        _MM_TRANSPOSE4_PS(row0, row1, row2, row3);
        _mm_store_ps(&sData[0].f[0], row0);
        _mm_store_ps(&sData[1].f[0], row1);
        _mm_store_ps(&sData[2].f[0], row2);
        _mm_store_ps(&sData[3].f[0], row3);
#   else
        *alias_cast<Matrix44A*>(&sData[0]) = mMatrix.GetTransposed();
#   endif
    }

    static inline void Store(UFloat4* sData, const Matrix44A& mMatrix)
    {
#    if defined(_CPU_SSE)
        __m128 row0 = _mm_load_ps(&mMatrix.m00);
        _mm_store_ps(&sData[0].f[0], row0);
        __m128 row1 = _mm_load_ps(&mMatrix.m10);
        _mm_store_ps(&sData[1].f[0], row1);
        __m128 row2 = _mm_load_ps(&mMatrix.m20);
        _mm_store_ps(&sData[2].f[0], row2);
        __m128 row3 = _mm_load_ps(&mMatrix.m30);
        _mm_store_ps(&sData[3].f[0], row3);
#   else
        *alias_cast<Matrix44A*>(&sData[0]) = mMatrix;
#   endif
    }

    static inline void Store(UFloat4* sData, const Matrix34A& mMatrix)
    {
#    if defined(_CPU_SSE)
        __m128 row0 = _mm_load_ps(&mMatrix.m00);
        _mm_store_ps(&sData[0].f[0], row0);
        __m128 row1 = _mm_load_ps(&mMatrix.m10);
        _mm_store_ps(&sData[1].f[0], row1);
        __m128 row2 = _mm_load_ps(&mMatrix.m20);
        _mm_store_ps(&sData[2].f[0], row2);
        _mm_store_ps(&sData[3].f[0], _mm_setr_ps(0, 0, 0, 1));
#   else
        *alias_cast<Matrix44A*>(&sData[0]) = mMatrix;
#   endif
    }

#if defined(_CPU_SSE)
    // Matrix multiplication using SSE instructions set
    // IMPORTANT NOTE: much faster if matrices m1 and product are 16 bytes aligned
    inline void multMatrixf_Transp2_SSE(float* product, const float* m1, const float* m2)
    {
        __m128 x0 = _mm_load_ss(m2);
        __m128 x1 = _mm_load_ps(m1);
        x0 = _mm_shuffle_ps(x0, x0, 0);
        __m128 x2 = _mm_load_ss(&m2[4]);
        x0 = _mm_mul_ps(x0, x1);
        x2 = _mm_shuffle_ps(x2, x2, 0);
        __m128 x3 = _mm_load_ps(&m1[4]);
        __m128 x4 = _mm_load_ss(&m2[8]);
        x2 = _mm_mul_ps(x2, x3);
        x4 = _mm_shuffle_ps(x4, x4, 0);
        x0 = _mm_add_ps(x0, x2);
        x2 = _mm_load_ps(&m1[8]);
        x4 = _mm_mul_ps(x4, x2);
        __m128 x6 = _mm_load_ps(&m1[12]);
        x0 = _mm_add_ps(x0, x4);
        _mm_store_ps(product, x0);
        x0 = _mm_load_ss(&m2[1]);
        x4 = _mm_load_ss(&m2[5]);
        x0 = _mm_shuffle_ps(x0, x0, 0);
        x4 = _mm_shuffle_ps(x4, x4, 0);
        x0 = _mm_mul_ps(x0, x1);
        x4 = _mm_mul_ps(x4, x3);
        __m128 x5 = _mm_load_ss(&m2[9]);
        x0 = _mm_add_ps(x0, x4);
        x5 = _mm_shuffle_ps(x5, x5, 0);
        x5 = _mm_mul_ps(x5, x2);
        x0 = _mm_add_ps(x0, x5);
        _mm_store_ps(&product[4], x0);
        x0 = _mm_load_ss(&m2[2]);
        x4 = _mm_load_ss(&m2[6]);
        x0 = _mm_shuffle_ps(x0, x0, 0);
        x4 = _mm_shuffle_ps(x4, x4, 0);
        x0 = _mm_mul_ps(x0, x1);
        x4 = _mm_mul_ps(x4, x3);
        x5 = _mm_load_ss(&m2[10]);
        x0 = _mm_add_ps(x0, x4);
        x5 = _mm_shuffle_ps(x5, x5, 0);
        x5 = _mm_mul_ps(x5, x2);
        x0 = _mm_add_ps(x0, x5);
        _mm_store_ps(&product[8], x0);
        x0 = _mm_load_ss(&m2[3]);
        x4 = _mm_load_ss(&m2[7]);
        x0 = _mm_shuffle_ps(x0, x0, 0);
        x4 = _mm_shuffle_ps(x4, x4, 0);
        x0 = _mm_mul_ps(x0, x1);
        x4 = _mm_mul_ps(x4, x3);
        x1 = _mm_load_ss(&m2[11]);
        x0 = _mm_add_ps(x0, x4);
        x1 = _mm_shuffle_ps(x1, x1, 0);
        x1 = _mm_mul_ps(x1, x2);
        x1 = _mm_add_ps(x1, x6);
        x0 = _mm_add_ps(x0, x1);
        _mm_store_ps(&product[12], x0);
    }
#endif

    inline void multMatrixf_Transp2(float* product, const float* m1, const float* m2)
    {
        float temp[16];

#define A(row, col)  m1[(col << 2) + row]
#define B(row, col)  m2[(col << 2) + row]
#define P(row, col)  temp[(col << 2) + row]

        int i;
        for (i = 0; i < 4; i++)
        {
            float ai0 = A(i, 0), ai1 = A(i, 1), ai2 = A(i, 2), ai3 = A(i, 3);
            P(i, 0) = ai0 * B(0, 0) + ai1 * B(0, 1) + ai2 * B(0, 2);
            P(i, 1) = ai0 * B(1, 0) + ai1 * B(1, 1) + ai2 * B(1, 2);
            P(i, 2) = ai0 * B(2, 0) + ai1 * B(2, 1) + ai2 * B(2, 2);
            P(i, 3) = ai0 * B(3, 0) + ai1 * B(3, 1) + ai2 * B(3, 2) + ai3;
        }

        cryMemcpy(product, temp, sizeof(temp));

#undef A
#undef B
#undef P
    }


    inline void mathMatrixMultiply_Transp2(float* pOut, const float* pM1, const float* pM2, [[maybe_unused]] int OptFlags)
    {
#if defined(_CPU_SSE)
        multMatrixf_Transp2_SSE(pOut, pM1, pM2);
#else
        multMatrixf_Transp2(pOut, pM1, pM2);
#endif
    }

    NO_INLINE void sIdentityLine(UFloat4* sData)
    {
        sData[0].f[0] = sData[0].f[1] = sData[0].f[2] = 0.f;
        sData[0].f[3] = 1.0f;
    }
    NO_INLINE void sOneLine(UFloat4* sData)
    {
        sData[0].f[0] = sData[0].f[1] = sData[0].f[2] = 1.f;
        sData[0].f[3] = 1.0f;
    }
    NO_INLINE void sZeroLine(UFloat4* sData)
    {
        sData[0].f[0] = sData[0].f[1] = sData[0].f[2] = 0.f;
        sData[0].f[3] = 0.0f;
    }

    NO_INLINE IRenderElement* sGetContainerRE0(IRenderElement* pRE)
    {
        assert(pRE);    // someone assigned wrong shader - function should not be called then

        if (pRE->mfGetType() == eDATA_Mesh && ((CREMeshImpl*)pRE)->m_pRenderMesh->_GetVertexContainer())
        {
            assert(((CREMeshImpl*)pRE)->m_pRenderMesh->_GetVertexContainer()->m_Chunks.size() >= 1);
            return ((CREMeshImpl*)pRE)->m_pRenderMesh->_GetVertexContainer()->m_Chunks[0].pRE;
        }

        return pRE;
    }

    NO_INLINE void sGetTerrainBase(UFloat4* sData, CD3D9Renderer* r)
    {
        if (!r->m_RP.m_pRE)
        {
            sZeroLine(sData);
            return;
        }
        // use render element from vertex container render mesh if available
        IRenderElement* pRE = sGetContainerRE0(r->m_RP.m_pRE);

        if (pRE && pRE->GetCustomData())
        {
            float* pData;

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
            // render to texture uses custom data as a double buffer to prevent flicker
            // in the terrain texture base
            if (r->m_pRT->GetThreadList() == 0)
#else
            if (SRendItem::m_RecurseLevel[r->m_RP.m_nProcessThreadID] <= 0)
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
            {
                pData = (float*)pRE->GetCustomData();
            }
            else
            {
                pData = (float*)pRE->GetCustomData() + 4;
            }

            sData[0].f[0] = pData[2];
            sData[0].f[1] = pData[0];
            sData[0].f[2] = pData[1];
        }
        else
        {
            sZeroLine(sData);
        }
    }

    NO_INLINE void sGetTerrainLayerGen(UFloat4* sData, CD3D9Renderer* r)
    {
        IRenderElement* pRE = r->m_RP.m_pRE;
        if (pRE && pRE->GetCustomData())
        {
            float* pData = (float*)pRE->GetCustomData();
            memcpy(sData, pData, sizeof(float) * 16);
        }
        else
        {
            memset(sData, 0, sizeof(float) * 16);
        }
    }

    void sGetTexMatrix(UFloat4* sData, CD3D9Renderer* r, const SCGParam* ParamBind)
    {
        Matrix44& result = *((Matrix44*)sData);

        SHRenderTarget* renderTarget = (SHRenderTarget*)(UINT_PTR)ParamBind->m_nID;
        if (!renderTarget)
        {
            result.SetIdentity();
            return;
        }

        SEnvTexture* pEnvTex = renderTarget->GetEnv2D();
        if (!pEnvTex || !pEnvTex->m_pTex)
        {
            result.SetIdentity();
            return;
        }

        if ((renderTarget->m_eUpdateType != eRTUpdate_WaterReflect))
        {
            result = r->m_RP.m_pCurObject->m_II.m_Matrix * pEnvTex->m_Matrix;
        }
        else
        {
            result = pEnvTex->m_Matrix;
        }
        result.Transpose();
    }



    NO_INLINE void sGetScreenSize(UFloat4* sData, CD3D9Renderer* r)
    {
        int iTempX, iTempY, iWidth, iHeight;
        r->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);
#if defined(WIN32) || defined(WIN64)
        float w = (float)(iWidth > 1 ? iWidth : 1);
        float h = (float)(iHeight > 1 ? iHeight : 1);
#else
        float w = (float)iWidth;
        float h = (float)iHeight;
#endif
        sData[0].f[0] = w;
        sData[0].f[1] = h;
        sData[0].f[2] = 0.5f / (w / r->m_RP.m_CurDownscaleFactor.x);
        sData[0].f[3] = 0.5f / (h / r->m_RP.m_CurDownscaleFactor.y);
    }

    NO_INLINE void sGetIrregKernel(UFloat4* sData, CD3D9Renderer* r)
    {
        int nSamplesNum = 1;
        switch (r->m_RP.m_nShaderQuality)
        {
        case eSQ_Low:
            nSamplesNum = 4;
            break;
        case eSQ_Medium:
            nSamplesNum = 8;
            break;
        case eSQ_High:
            nSamplesNum = 16;
            break;
        case eSQ_VeryHigh:
            nSamplesNum = 16;
            break;
        default:
            assert(0);
        }

        CShadowUtils::GetIrregKernel((float(*)[4]) & sData[0], nSamplesNum);
    }

    enum class FrameType
    {
        Current,
        Previous
    };

    NO_INLINE void sGetBendInfo(CRenderObject* const __restrict renderObject, FrameType frameType, UFloat4* sData, [[maybe_unused]] CD3D9Renderer* r)
    {
        Vec4 bendingInfoResult(0.0f);
        SRenderObjData* objectData = renderObject->GetObjData();
        SBending* bending = nullptr;
        float time = 0.0f;

        if (objectData)
        {
            if (frameType == FrameType::Current)
            {
                bending = objectData->m_pBending;
                time = CRenderer::GetRealTime();
            }
            else
            {
                bending = objectData->m_BendingPrev;
                time = CRenderer::GetRealTime() - CRenderer::GetElapsedTime();
            }
        }

        // Set values to zero if no bending found - eg. trees created as geom entity and not vegetation,
        // these are still rendered with bending/detailbending enabled in shader
        // (very ineffective but they should not appear in real levels)
        if (bending)
        {
            bendingInfoResult = bending->GetShaderConstants(time);
        }

        *(alias_cast<Vec4*>(&sData[0])) = bendingInfoResult;
    }

    NO_INLINE Vec4 sGetVolumetricFogParams(CD3D9Renderer* r)
    {
        Vec4 pFogParams;

        I3DEngine* pEng = gEnv->p3DEngine;
        assert(pEng);

        Vec3 globalDensityParams(0, 1, 1);
        pEng->GetGlobalParameter(E3DPARAM_VOLFOG_GLOBAL_DENSITY, globalDensityParams);

        float globalDensity = globalDensityParams.x;
        if (!gRenDev->IsHDRModeEnabled())
        {
            globalDensity *= globalDensityParams.y;
        }

        Vec3 volFogHeightDensity(0, 1, 0);
        pEng->GetGlobalParameter(E3DPARAM_VOLFOG_HEIGHT_DENSITY, volFogHeightDensity);
        volFogHeightDensity.y = clamp_tpl(volFogHeightDensity.y, 1e-5f, 1.0f);

        Vec3 volFogHeightDensity2(4000.0f, 0, 0);
        pEng->GetGlobalParameter(E3DPARAM_VOLFOG_HEIGHT_DENSITY2, volFogHeightDensity2);
        volFogHeightDensity2.y = clamp_tpl(volFogHeightDensity2.y, 1e-5f, 1.0f);
        volFogHeightDensity2.x = volFogHeightDensity2.x < volFogHeightDensity.x + 1.0f ? volFogHeightDensity.x + 1.0f : volFogHeightDensity2.x;

        const float ha = volFogHeightDensity.x;
        const float hb = volFogHeightDensity2.x;

        const float da = volFogHeightDensity.y;
        const float db = volFogHeightDensity2.y;

        const float ga = logf(da);
        const float gb = logf(db);

        const float c = (gb - ga) / (hb - ha);
        const float o = ga - c * ha;

        const float viewerHeight = r->GetViewParameters().vOrigin.z;
        const float co = clamp_tpl(c * viewerHeight + o, -50.0f, 50.0f); // Avoiding FPEs at extreme ranges

        globalDensity *= 0.01f; // multiply by 1/100 to scale value editor value back to a reasonable range

        pFogParams.x = c;
        pFogParams.y = 1.44269502f * globalDensity * expf(co); // log2(e) = 1.44269502
        pFogParams.z = globalDensity;
        pFogParams.w = 1.0f - clamp_tpl(globalDensityParams.z, 0.0f, 1.0f);

        return pFogParams;
    }

    NO_INLINE Vec4 sGetVolumetricFogRampParams()
    {
        I3DEngine* pEng = gEnv->p3DEngine;

        Vec3 vfRampParams(0, 100.0f, 0);
        pEng->GetGlobalParameter(E3DPARAM_VOLFOG_RAMP, vfRampParams);

        vfRampParams.x = vfRampParams.x < 0 ? 0 : vfRampParams.x; // start
        vfRampParams.y = vfRampParams.y < vfRampParams.x + 0.1f ? vfRampParams.x + 0.1f : vfRampParams.y; // end
        vfRampParams.z = clamp_tpl(vfRampParams.z, 0.0f, 1.0f); // influence

        float invRampDist = 1.0f / (vfRampParams.y - vfRampParams.x);
        return Vec4(invRampDist, -vfRampParams.x * invRampDist, vfRampParams.z, -vfRampParams.z + 1.0f);
    }

    NO_INLINE void sGetFogColorGradientConstants(Vec4& fogColGradColBase, Vec4& fogColGradColDelta)
    {
        I3DEngine* pEng = gEnv->p3DEngine;

        Vec3 colBase = pEng->GetFogColor();
        fogColGradColBase = Vec4(colBase, 0);

        Vec3 colTop(colBase);
        pEng->GetGlobalParameter(E3DPARAM_FOG_COLOR2, colTop);
        fogColGradColDelta = Vec4(colTop - colBase, 0);
    }

    NO_INLINE Vec4 sGetFogColorGradientParams()
    {
        I3DEngine* pEng = gEnv->p3DEngine;

        Vec3 volFogHeightDensity(0, 1, 0);
        pEng->GetGlobalParameter(E3DPARAM_VOLFOG_HEIGHT_DENSITY, volFogHeightDensity);

        Vec3 volFogHeightDensity2(4000.0f, 0, 0);
        pEng->GetGlobalParameter(E3DPARAM_VOLFOG_HEIGHT_DENSITY2, volFogHeightDensity2);
        volFogHeightDensity2.x = volFogHeightDensity2.x < volFogHeightDensity.x + 1.0f ? volFogHeightDensity.x + 1.0f : volFogHeightDensity2.x;

        Vec3 gradientCtrlParams(0, 0.75f, 0.5f);
        pEng->GetGlobalParameter(E3DPARAM_VOLFOG_GRADIENT_CTRL, gradientCtrlParams);

        const float colorHeightOffset = clamp_tpl(gradientCtrlParams.x, -1.0f, 1.0f);
        const float radialSize = -expf((1.0f - clamp_tpl(gradientCtrlParams.y, 0.0f, 1.0f)) * 14.0f) * 1.44269502f; // log2(e) = 1.44269502;
        const float radialLobe = 1.0f / clamp_tpl(gradientCtrlParams.z, 1.0f / 21.0f, 1.0f) - 1.0f;

        const float invDist = 1.0f / (volFogHeightDensity2.x - volFogHeightDensity.x);
        return Vec4(invDist, -volFogHeightDensity.x * invDist - colorHeightOffset, radialSize, radialLobe);
    }

    NO_INLINE Vec4 sGetFogColorGradientRadial(CD3D9Renderer* r)
    {
        I3DEngine* pEng = gEnv->p3DEngine;

        Vec3 fogColorRadial(0, 0, 0);
        pEng->GetGlobalParameter(E3DPARAM_FOG_RADIAL_COLOR, fogColorRadial);

        const CameraViewParameters& rc = r->GetViewParameters();
        const float invFarDist = 1.0f / rc.fFar;

        return Vec4(fogColorRadial, invFarDist);
    }

    NO_INLINE Vec4 sGetVolumetricFogSamplingParams(CD3D9Renderer* r)
    {
        const CameraViewParameters& rc = r->GetViewParameters();

        Vec3 volFogCtrlParams(0.0f, 0.0f, 0.0f);
        gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLFOG2_CTRL_PARAMS, volFogCtrlParams);
        const float raymarchStart = rc.fNear;
        const float raymarchDistance = (volFogCtrlParams.x > raymarchStart) ? (volFogCtrlParams.x - raymarchStart) : 0.0001f;

        Vec4 params;
        params.x = raymarchStart;
        params.y = 1.0f / raymarchDistance;
        params.z = static_cast<f32>(CTexture::s_ptexVolumetricFog ? CTexture::s_ptexVolumetricFog->GetDepth() : 0.0f);
        params.w = params.z > 0.0f ? (1.0f / params.z) : 0.0f;
        return params;
    }

    NO_INLINE Vec4 sGetVolumetricFogDistributionParams(CD3D9Renderer* r)
    {
        const CameraViewParameters& rc = r->GetViewParameters();

        Vec3 volFogCtrlParams(0.0f, 0.0f, 0.0f);
        gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLFOG2_CTRL_PARAMS, volFogCtrlParams);
        const float raymarchStart = rc.fNear;
        const float raymarchDistance = (volFogCtrlParams.x > raymarchStart) ? (volFogCtrlParams.x - raymarchStart) : 0.0001f;

        Vec4 params;
        params.x = raymarchStart;
        params.y = raymarchDistance;
        float d = static_cast<f32>(CTexture::s_ptexVolumetricFog ? CTexture::s_ptexVolumetricFog->GetDepth() : 0.0f);
        params.z = d > 1.0f ? (1.0f / (d - 1.0f)) : 0.0f;
        params.w = 0.0f;
        return params;
    }

    NO_INLINE Vec4 sGetVolumetricFogScatteringParams(CD3D9Renderer* r)
    {
        const CameraViewParameters& rc = r->GetViewParameters();

        Vec3 volFogScatterParams(0.0f, 0.0f, 0.0f);
        gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLFOG2_SCATTERING_PARAMS, volFogScatterParams);

        Vec3 anisotropy(0.0f, 0.0f, 0.0f);
        gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLFOG2_HEIGHT_DENSITY, anisotropy);

        float k = anisotropy.z;
        bool bNegative = k < 0.0f ? true : false;
        k = (abs(k) > 0.99999f) ? (bNegative ? -0.99999f : 0.99999f) : k;

        Vec4 params;
        params.x = volFogScatterParams.x;
        params.y = (volFogScatterParams.y < 0.0001f) ? 0.0001f : volFogScatterParams.y;// it ensures extinction is more than zero.
        params.z = k;
        params.w = 1.0f - k * k;
        return params;
    }

    NO_INLINE Vec4 sGetVolumetricFogScatteringBlendParams([[maybe_unused]] CD3D9Renderer* r)
    {
        I3DEngine* pEng = gEnv->p3DEngine;

        Vec3 volFogCtrlParams(0.0f, 0.0f, 0.0f);
        pEng->GetGlobalParameter(E3DPARAM_VOLFOG2_CTRL_PARAMS, volFogCtrlParams);

        Vec4 params;
        params.x = volFogCtrlParams.y;// blend factor of two radial lobes
        params.y = volFogCtrlParams.z;// blend mode of two radial lobes
        params.z = 0.0f;
        params.w = 0.0f;
        return params;
    }

    NO_INLINE Vec4 sGetVolumetricFogScatteringColor([[maybe_unused]] CD3D9Renderer* r)
    {
        I3DEngine* pEng = gEnv->p3DEngine;

        Vec3 fogAlbedo(0.0f, 0.0f, 0.0f);
        pEng->GetGlobalParameter(E3DPARAM_VOLFOG2_COLOR1, fogAlbedo);
        Vec3 sunColor = pEng->GetSunColor();
        sunColor = sunColor.CompMul(fogAlbedo);

        Vec3 anisotropy(0.0f, 0.0f, 0.0f);
        pEng->GetGlobalParameter(E3DPARAM_VOLFOG2_HEIGHT_DENSITY2, anisotropy);

        float k = anisotropy.z;
        bool bNegative = k < 0.0f ? true : false;
        k = (abs(k) > 0.99999f) ? (bNegative ? -0.99999f : 0.99999f) : k;

        return Vec4(sunColor, k);
    }

    NO_INLINE Vec4 sGetVolumetricFogScatteringSecondaryColor([[maybe_unused]] CD3D9Renderer* r)
    {
        I3DEngine* pEng = gEnv->p3DEngine;

        Vec3 fogAlbedo(0.0f, 0.0f, 0.0f);
        pEng->GetGlobalParameter(E3DPARAM_VOLFOG2_COLOR2, fogAlbedo);
        Vec3 sunColor = pEng->GetSunColor();
        sunColor = sunColor.CompMul(fogAlbedo);

        Vec3 anisotropy(0.0f, 0.0f, 0.0f);
        pEng->GetGlobalParameter(E3DPARAM_VOLFOG2_HEIGHT_DENSITY2, anisotropy);

        float k = anisotropy.z;
        bool bNegative = k < 0.0f ? true : false;
        k = (abs(k) > 0.99999f) ? (bNegative ? -0.99999f : 0.99999f) : k;

        return Vec4(sunColor, 1.0f - k * k);
    }

    NO_INLINE Vec4 sGetVolumetricFogHeightDensityParams(CD3D9Renderer* r)
    {
        Vec4 pFogParams;

        I3DEngine* pEng = gEnv->p3DEngine;
        assert(pEng);

        Vec3 globalDensityParams(0, 1, 1);
        pEng->GetGlobalParameter(E3DPARAM_VOLFOG2_GLOBAL_DENSITY, globalDensityParams);

        float globalDensity = globalDensityParams.x;
        const float clampTransmittance = globalDensityParams.y > 0.9999999f ? 1.0f : globalDensityParams.y;
        const float visibility = globalDensityParams.z;

        Vec3 volFogHeightDensity(0, 1, 0);
        pEng->GetGlobalParameter(E3DPARAM_VOLFOG2_HEIGHT_DENSITY, volFogHeightDensity);
        volFogHeightDensity.y = clamp_tpl(volFogHeightDensity.y, 1e-5f, 1.0f);

        Vec3 volFogHeightDensity2(4000.0f, 0, 0);
        pEng->GetGlobalParameter(E3DPARAM_VOLFOG2_HEIGHT_DENSITY2, volFogHeightDensity2);
        volFogHeightDensity2.y = clamp_tpl(volFogHeightDensity2.y, 1e-5f, 1.0f);
        volFogHeightDensity2.x = volFogHeightDensity2.x < volFogHeightDensity.x + 1.0f ? volFogHeightDensity.x + 1.0f : volFogHeightDensity2.x;

        const float ha = volFogHeightDensity.x;
        const float hb = volFogHeightDensity2.x;

        const float db = volFogHeightDensity2.y;
        const float da = abs(db - volFogHeightDensity.y) < 0.00001f ? volFogHeightDensity.y + 0.00001f : volFogHeightDensity.y;

        const float ga = logf(da);
        const float gb = logf(db);

        const float c = (gb - ga) / (hb - ha);
        const float o = ga - c * ha;

        const float viewerHeight = r->GetViewParameters().vOrigin.z;
        const float co = clamp_tpl(c * viewerHeight + o, -50.0f, 50.0f); // Avoiding FPEs at extreme ranges

        globalDensity *= 0.01f; // multiply by 1/100 to scale value editor value back to a reasonable range

        pFogParams.x = c;
        pFogParams.y = 1.44269502f * globalDensity * expf(co); // log2(e) = 1.44269502
        pFogParams.z = visibility;
        pFogParams.w = 1.0f - clamp_tpl(clampTransmittance, 0.0f, 1.0f);

        return pFogParams;
    }

    NO_INLINE Vec4 sGetVolumetricFogHeightDensityRampParams([[maybe_unused]] CD3D9Renderer* r)
    {
        I3DEngine* pEng = gEnv->p3DEngine;

        Vec3 vfRampParams(0, 100.0f, 0);
        pEng->GetGlobalParameter(E3DPARAM_VOLFOG2_RAMP, vfRampParams);

        vfRampParams.x = vfRampParams.x < 0 ? 0 : vfRampParams.x; // start
        vfRampParams.y = vfRampParams.y < vfRampParams.x + 0.1f ? vfRampParams.x + 0.1f : vfRampParams.y; // end

        float t0 = 1.0f / (vfRampParams.y - vfRampParams.x);
        float t1 = vfRampParams.x * t0;

        return Vec4(vfRampParams.x, vfRampParams.y, t0, t1);
    }

    NO_INLINE Vec4 sGetVolumetricFogDistanceParams(CD3D9Renderer* rndr)
    {
        const CameraViewParameters& rc = rndr->GetViewParameters();
        float l, r, b, t, Ndist, Fdist;
        rc.GetFrustumParams(&l, &r, &b, &t, &Ndist, &Fdist);

        Vec3 volFogCtrlParams(0.0f, 0.0f, 0.0f);
        gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLFOG2_CTRL_PARAMS, volFogCtrlParams);
        const float raymarchStart = rc.fNear;
        const float raymarchEnd = (volFogCtrlParams.x > raymarchStart) ? volFogCtrlParams.x : raymarchStart + 0.0001f;

        float l2 = l * l;
        float t2 = t * t;
        float n2 = Ndist * Ndist;
        Vec4 params;
        params.x = raymarchEnd * (Ndist / sqrt(l2 + t2 + n2));
        params.y = raymarchEnd * (Ndist / sqrt(t2 + n2));
        params.z = raymarchEnd * (Ndist / sqrt(l2 + n2));
        params.w = raymarchEnd;

        return params;
    }

    void sGetMotionBlurData(UFloat4* sData, CD3D9Renderer* r, [[maybe_unused]] const CRenderObject::SInstanceInfo& instInfo, SRenderPipeline& rRP)
    {
        CRenderObject* pObj = r->m_RP.m_pCurObject;

        Matrix44A mObjPrev;
        if ((rRP.m_FlagsPerFlush & RBSI_CUSTOM_PREVMATRIX) == 0)
        {
            CMotionBlur::GetPrevObjToWorldMat(pObj, mObjPrev);
        }
        else
        {
            mObjPrev = *rRP.m_pPrevMatrix;
        }

        float* pData = mObjPrev.GetData();
#if defined(_CPU_SSE) && !defined(_DEBUG)
        sData[0].m128 = _mm_load_ps(&pData[0]);
        sData[1].m128 = _mm_load_ps(&pData[4]);
        sData[2].m128 = _mm_load_ps(&pData[8]);
#else
        sData[0].f[0] = pData[0];
        sData[0].f[1] = pData[1];
        sData[0].f[2] = pData[2];
        sData[0].f[3] = pData[3];
        sData[1].f[0] = pData[4];
        sData[1].f[1] = pData[5];
        sData[1].f[2] = pData[6];
        sData[1].f[3] = pData[7];
        sData[2].f[0] = pData[8];
        sData[2].f[1] = pData[9];
        sData[2].f[2] = pData[10];
        sData[2].f[3] = pData[11];
#endif
    }

    void sGetPrevObjWorldData(UFloat4* sData, SRenderPipeline& rRP)
    {
        CRenderObject* pObj = rRP.m_pCurObject;

        Matrix44A mObjPrev;
        if ((rRP.m_FlagsPerFlush & RBSI_CUSTOM_PREVMATRIX) == 0)
        {
            FurBendData::Get().GetPrevObjToWorldMat(*pObj, mObjPrev);
        }
        else
        {
            mObjPrev = *rRP.m_pPrevMatrix;
        }

        float* pData = mObjPrev.GetData();
#if defined(_CPU_SSE) && !defined(_DEBUG)
        sData[0].m128 = _mm_load_ps(&pData[0]);
        sData[1].m128 = _mm_load_ps(&pData[4]);
        sData[2].m128 = _mm_load_ps(&pData[8]);
#else
        sData[0].f[0] = pData[0];
        sData[0].f[1] = pData[1];
        sData[0].f[2] = pData[2];
        sData[0].f[3] = pData[3];
        sData[1].f[0] = pData[4];
        sData[1].f[1] = pData[5];
        sData[1].f[2] = pData[6];
        sData[1].f[3] = pData[7];
        sData[2].f[0] = pData[8];
        sData[2].f[1] = pData[9];
        sData[2].f[2] = pData[10];
        sData[2].f[3] = pData[11];
#endif
    }

    NO_INLINE void sVisionParams(UFloat4* sData)
    {
        CRenderObject* pObj = gRenDev->m_RP.m_pCurObject;
        SRenderObjData* pOD = pObj->GetObjData();
        if (pOD)
        {
            float fRecip = (1.0f / 255.0f);

            uint32 nParams(pOD->m_nHUDSilhouetteParams);

            sData[0].f[0] = float((nParams & 0xff000000) >> 24) * fRecip;
            sData[0].f[1] = float((nParams & 0x00ff0000) >> 16) * fRecip;
            sData[0].f[2] = float((nParams & 0x0000ff00) >> 8) * fRecip;
            sData[0].f[3] = float((nParams & 0x000000ff)) * fRecip;

            if IsCVarConstAccess(constexpr) (CRenderer::CV_r_customvisions == 2)
            {
                sData[0].f[3] = gEnv->pTimer->GetCurrTime() + ((float)(2 * pObj->m_Id) / 32768.0f);
            }
        }
        else
        {
            sData[0].f[0] = 0;
            sData[0].f[1] = 0;
            sData[0].f[2] = 0;
            sData[0].f[3] = 0;
        }
    }

    NO_INLINE void sFromObjSB(UFloat4* sData)
    {
        CRenderObject* pObj = gRenDev->m_RP.m_pCurObject;
        if (pObj && (pObj->m_nTextureID > 0))
        {
            CTexture* pTex = CTexture::GetByID(pObj->m_nTextureID);

            // SB == Scale & Bias
            _MS_ALIGN(16) ColorF B = pTex->GetMinColor();
            _MS_ALIGN(16) ColorF S = pTex->GetMaxColor() - pTex->GetMinColor();

            sData[0] = (const UFloat4&)B;
            sData[1] = (const UFloat4&)S;
        }
        else
        {
            // SB == Scale & Bias
            _MS_ALIGN(16) ColorF B = ColorF(0.0f);
            _MS_ALIGN(16) ColorF S = ColorF(1.0f);

            sData[0] = (const UFloat4&)B;
            sData[1] = (const UFloat4&)S;
        }
    }

    NO_INLINE void sVisionMtlParams(UFloat4* sData)
    {
        const Vec3& vCameraPos = gRenDev->GetViewParameters().vOrigin;
        Vec3 vObjectPos = gRenDev->m_RP.m_pCurObject->GetTranslation();
        if (gRenDev->m_RP.m_pCurObject->m_ObjFlags & FOB_NEAREST)
        {
            vObjectPos += vCameraPos; // Nearest objects are rendered in camera space, so convert to world space
        }
        const float fRecipThermalViewDist = 1.0f;

        CShaderResources* pRes = gRenDev->m_RP.m_pShaderResources;
        float heatAmount = 0.0f;

        sData[0].f[0] = heatAmount;
        sData[0].f[0] *= 1.0f - min(1.0f, vObjectPos.GetSquaredDistance(vCameraPos) * fRecipThermalViewDist);

        if (gRenDev->m_RP.m_nPassGroupID == EFSLIST_TRANSP && gRenDev->m_RP.m_pCurObject->m_ObjFlags & FOB_REQUIRES_RESOLVE)
        {
            static void* pPrevNode = NULL;
            static int prevFrame = 0;
            static float fScale = 1.0f;

            sData[0].f[0] *= fScale;

            //Cache parameters for use in the next call, to provide a consistent value for an individual character
            pPrevNode = gRenDev->m_RP.m_pCurObject->m_pRenderNode;
            prevFrame = gRenDev->GetFrameID(true);
        }

        sData[0].f[1] = sData[0].f[2] = 0.0f;
    }

    NO_INLINE void sTexelsPerMeterInfo(UFloat4 *sData, uint32 texIdx)
    {
        sData[0].f[0] = sData[0].f[1] = sData[0].f[2] = sData[0].f[3] = 0;
        CShaderResources*   pRes = gRenDev->m_RP.m_pShaderResources;
        SEfResTexture*      pTextureRes = pRes ? pRes->GetTextureResource(texIdx) : nullptr;

        if (pTextureRes && pTextureRes->m_Sampler.m_pTex)
        {
            CTexture*   pTexture = pTextureRes->m_Sampler.m_pTex;

            int texWidth(pTexture->GetWidth());
            int texHeight(pTexture->GetHeight());
            float ratio = 0.5f / CRenderer::CV_r_TexelsPerMeter;
            sData[0].f[0] = (float)texWidth * ratio;
            sData[0].f[1] = (float)texHeight * ratio;
        }
    }

    inline void sAppendClipSpaceAdaptation([[maybe_unused]] Matrix44A* __restrict pTransform)
    {
#if defined(OPENGL) && CRY_OPENGL_MODIFY_PROJECTIONS
#if CRY_OPENGL_FLIP_Y
        (*pTransform)(1, 0) = -(*pTransform)(1, 0);
        (*pTransform)(1, 1) = -(*pTransform)(1, 1);
        (*pTransform)(1, 2) = -(*pTransform)(1, 2);
        (*pTransform)(1, 3) = -(*pTransform)(1, 3);
#endif //CRY_OPENGL_FLIP_Y
        (*pTransform)(2, 0) = +2.0f * (*pTransform)(2, 0) - (*pTransform)(3, 0);
        (*pTransform)(2, 1) = +2.0f * (*pTransform)(2, 1) - (*pTransform)(3, 1);
        (*pTransform)(2, 2) = +2.0f * (*pTransform)(2, 2) - (*pTransform)(3, 2);
        (*pTransform)(2, 3) = +2.0f * (*pTransform)(2, 3) - (*pTransform)(3, 3);
#endif //defined(OPENGL) && CRY_OPENGL_MODIFY_PROJECTIONS
    }


    NO_INLINE void sOceanMat(UFloat4* sData)
    {
        const CameraViewParameters& cam(gRenDev->GetViewParameters());

        Matrix44A viewMat;
        viewMat.m00 = cam.vX.x;
        viewMat.m01 = cam.vY.x;
        viewMat.m02 = cam.vZ.x;
        viewMat.m03 = 0;
        viewMat.m10 = cam.vX.y;
        viewMat.m11 = cam.vY.y;
        viewMat.m12 = cam.vZ.y;
        viewMat.m13 = 0;
        viewMat.m20 = cam.vX.z;
        viewMat.m21 = cam.vY.z;
        viewMat.m22 = cam.vZ.z;
        viewMat.m23 = 0;
        viewMat.m30 = 0;
        viewMat.m31 = 0;
        viewMat.m32 = 0;
        viewMat.m33 = 1;
        Matrix44A* pMat = alias_cast<Matrix44A*>(&sData[0]);
        *pMat = viewMat * gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_matProj;
        *pMat = pMat->GetTransposed();
        sAppendClipSpaceAdaptation(pMat);
    }

    // Filling texture dimensions and inverse dimensions to CB
    NO_INLINE void sResInfo(UFloat4 *sData, int texIdx)  // EFTT_DIFFUSE, EFTT_GLOSS, ... etc (but NOT EFTT_BUMP!)
    {
        sIdentityLine(sData);
        CShaderResources *pRes = gRenDev->m_RP.m_pShaderResources;
        if (!pRes)
            return;

        SEfResTexture*  pTextureRes = pRes->GetTextureResource(texIdx);
        if (!pTextureRes)
            return;

        ITexture*   pTexture(pTextureRes->m_Sampler.m_pTex);
        if (pTexture)
        {
            int texWidth(pTexture->GetWidth());
            int texHeight(pTexture->GetHeight());
            sData[0].f[0] = (float)texWidth;
            sData[0].f[1] = (float)texHeight;
            if (texWidth && texHeight)
            {
                sData[0].f[2] = 1.0f / (float)texWidth;
                sData[0].f[3] = 1.0f / (float)texHeight;
            }
        }
    }

    NO_INLINE void sTexelDensityParam(UFloat4 *sData, uint32 texIdx)
    {
        sIdentityLine(sData);

        CShaderResources *pRes = gRenDev->m_RP.m_pShaderResources;
        if (!pRes)
            return;

        SEfResTexture*  pTextureRes = pRes->GetTextureResource(texIdx);
        if (pTextureRes)
        {
            CRenderChunk*   pRenderChunk = NULL;
            int texWidth = 512;
            int texHeight = 512;
            int mipLevel = 0;

            IRenderElement *pRE = gRenDev->m_RP.m_pRE;

            if (pRE)
            {
                pRenderChunk = pRE->mfGetMatInfo();
            }

            CRenderObject *pCurObject = gRenDev->m_RP.m_pCurObject;

            if (pRenderChunk && pCurObject)
            {
                float weight = 1.0f;

                if (pRenderChunk->m_texelAreaDensity > 0.0f)
                {
                    float scale = 1.0f;

                    IRenderNode *pRenderNode = (IRenderNode *)pCurObject->m_pRenderNode;

                    float distance = pCurObject->m_fDistance * TANGENT30_2 / scale;
                    int screenHeight = gRenDev->GetHeight();

                    weight = pRenderChunk->m_texelAreaDensity * distance * distance * texWidth * texHeight * 
                        pTextureRes->GetTiling(0) *pTextureRes->GetTiling(1) / (screenHeight * screenHeight);
                }

                mipLevel = fastround_positive(0.5f * logf(max(weight, 1.0f)) / LN2);
            }

            texWidth /= (1 << mipLevel);
            texHeight /= (1 << mipLevel);

            if (texWidth == 0)
                texWidth = 1;
            if (texHeight == 0)
                texHeight = 1;

            sData[0].f[0] = (float)texWidth;
            sData[0].f[1] = (float)texHeight;
            sData[0].f[2] = 1.0f / (float)texWidth;
            sData[0].f[3] = 1.0f / (float)texHeight;
        }
    }

    NO_INLINE void sTexelDensityColor(UFloat4 *sData, uint32 texIdx)
    {
        sOneLine(sData);

        CShaderResources *pRes = gRenDev->m_RP.m_pShaderResources;
        if (!pRes)
            return;

        SEfResTexture*  pTextureRes = pRes->GetTextureResource(texIdx);
        if (pTextureRes)
        {
            if (CRenderer::CV_e_DebugTexelDensity == 2 || gcpRendD3D->CV_e_DebugTexelDensity == 4)
            {
                CRenderChunk *pRenderChunk = NULL;
                int texWidth = 512;
                int texHeight = 512;
                int mipLevel = 0;

                IRenderElement *pRE = gRenDev->m_RP.m_pRE;

                if (pRE)
                {
                    pRenderChunk = pRE->mfGetMatInfo();
                }

                CRenderObject *pCurObject = gRenDev->m_RP.m_pCurObject;

                if (pRenderChunk && pCurObject)
                {
                    float weight = 1.0f;

                    if (pRenderChunk->m_texelAreaDensity > 0.0f)
                    {
                        float scale = 1.0f;

                        IRenderNode *pRenderNode = (IRenderNode *)pCurObject->m_pRenderNode;

                        float distance = pCurObject->m_fDistance * TANGENT30_2 / scale;
                        int screenHeight = gRenDev->GetHeight();

                        weight = pRenderChunk->m_texelAreaDensity * distance * distance * texWidth * texHeight * 
                            pTextureRes->GetTiling(0) * pTextureRes->GetTiling(1) / (screenHeight * screenHeight);
                    }

                    mipLevel = fastround_positive(0.5f * logf(max(weight, 1.0f)) / LN2);
                }

                switch (mipLevel)
                {
                case 0:
                    sData[0].f[0] = 1.0f; sData[0].f[1] = 1.0f; sData[0].f[2] = 1.0f; break;
                case 1:
                    sData[0].f[0] = 0.0f; sData[0].f[1] = 0.0f; sData[0].f[2] = 1.0f; break;
                case 2:
                    sData[0].f[0] = 0.0f; sData[0].f[1] = 1.0f; sData[0].f[2] = 0.0f; break;
                case 3:
                    sData[0].f[0] = 0.0f; sData[0].f[1] = 1.0f; sData[0].f[2] = 1.0f; break;
                case 4:
                    sData[0].f[0] = 1.0f; sData[0].f[1] = 0.0f; sData[0].f[2] = 0.0f; break;
                case 5:
                    sData[0].f[0] = 1.0f; sData[0].f[1] = 0.0f; sData[0].f[2] = 1.0f; break;
                default:
                    sData[0].f[0] = 1.0f; sData[0].f[1] = 1.0f; sData[0].f[2] = 0.0f; break;
                }
            }
            else
            {
                sData[0].f[0] = 1.0f; sData[0].f[1] = 1.0f; sData[0].f[2] = 1.0f;
            }
        }
    }

    NO_INLINE void sNumInstructions(UFloat4* sData)
    {
        sData[0].f[0] = gRenDev->m_RP.m_NumShaderInstructions / CRenderer::CV_r_measureoverdrawscale / 256.0f;
    }

    NO_INLINE void sAmbient(UFloat4* sData, SRenderPipeline& rRP, const CRenderObject::SInstanceInfo& instInfo)
    {
        sData[0].f[0] = instInfo.m_AmbColor[0];
        sData[0].f[1] = instInfo.m_AmbColor[1];
        sData[0].f[2] = instInfo.m_AmbColor[2];
        sData[0].f[3] = instInfo.m_AmbColor[3];

        if (CShaderResources* pRes = rRP.m_pShaderResources)
        {
            if (pRes->m_ResFlags & MTL_FLAG_ADDITIVE)
            {
                sData[0].f[0] *= rRP.m_fCurOpacity;
                sData[0].f[1] *= rRP.m_fCurOpacity;
                sData[0].f[2] *= rRP.m_fCurOpacity;
            }
        }
    }
    NO_INLINE void sAmbientOpacity(CRenderObject* const renderObject, CD3D9Renderer* renderer, CShaderResources* shaderResources, UFloat4* sData, const CRenderObject::SInstanceInfo& instInfo)
    {
        PerFrameParameters& RESTRICT_REFERENCE PF = renderer->m_cEF.m_PF;
        SRenderPipeline& RESTRICT_REFERENCE rRP = renderer->m_RP;

        float opacity = shaderResources ? shaderResources->GetStrengthValue(EFTT_OPACITY) : 1.0f;
        float opal = opacity * renderObject->m_fAlpha;
        float s0 = instInfo.m_AmbColor[0];
        float s1 = instInfo.m_AmbColor[1];
        float s2 = instInfo.m_AmbColor[2];
        float s3 = opal;// object opacity

        if (shaderResources)
        {
            if (rRP.m_nShaderQuality == eSQ_Low)
            {
                ColorF diffuseColor = shaderResources->GetColorValue(EFTT_DIFFUSE);
                s0 *= diffuseColor.r;
                s1 *= diffuseColor.g;
                s2 *= diffuseColor.b;
            }
            ColorF emissiveColor = shaderResources->GetColorValue(EFTT_EMITTANCE);
            s0 += emissiveColor.r;
            s1 += emissiveColor.g;
            s2 += emissiveColor.b;

            if (shaderResources->m_ResFlags & MTL_FLAG_ADDITIVE)
            {
                s0 *= opacity;
                s1 *= opacity;
                s2 *= opacity;
            }
        }

        sData[0].f[0] = s0;
        sData[0].f[1] = s1;
        sData[0].f[2] = s2;
        sData[0].f[3] = s3;
    }

    NO_INLINE void sObjectAmbColComp(UFloat4* sData, const CRenderObject::SInstanceInfo& instInfo, const float fRenderQuality)
    {
        CD3D9Renderer* const __restrict r = gcpRendD3D;
        SRenderPipeline& RESTRICT_REFERENCE rRP = r->m_RP;
        CRenderObject* pObj = rRP.m_pCurObject;
        CShaderResources* shaderResources = rRP.m_pShaderResources;
        sData[0].f[0] = instInfo.m_AmbColor[3];
        sData[0].f[1] = /*instInfo.m_AmbColor[3] * */ rRP.m_fCurOpacity * pObj->m_fAlpha;


        sData[0].f[2] = 0.f;
        sData[0].f[3] = fRenderQuality * (1.0f / 65535.0f);
    }

    NO_INLINE void sMotionBlurInfo([[maybe_unused]] UFloat4* sData, [[maybe_unused]] SRenderPipeline& rRP)
    {
#ifdef PARTICLE_MOTION_BLUR
        SRenderObjData* pOD = rRP.m_pCurObject->GetObjData();
        if (pOD && pOD->m_pParticleParams)
        {
            if (pOD->m_pParticleParams->eFacing().Camera)
            {
                sData[0].f[0] = pOD->m_pParticleParams->fMotionBlurCamStretchScale;
                sData[0].f[1] = pOD->m_pParticleParams->fMotionBlurStretchScale;
            }
            else
            {
                sData[0].f[0] = infosData[0].f1] = 0;
            }
            sData[0].f[2] = pOD->m_pParticleParams->fMotionBlurScale * 0.2f;
        }
#endif
    }

    NO_INLINE void sWrinklesMask(UFloat4* sData, SRenderPipeline& rRP, uint index)
    {
        static uint8 WrinkleMask[3] = { ECGP_PI_WrinklesMask0, ECGP_PI_WrinklesMask1, ECGP_PI_WrinklesMask2 };

        SRenderObjData* pOD = rRP.m_pCurObject->GetObjData();
        if (pOD)
        {
            if (pOD->m_pShaderParams)
            {
                if (SShaderParam::GetValue(WrinkleMask[index], const_cast<AZStd::vector<SShaderParam>*>(pOD->m_pShaderParams), &sData[0].f[0], 4) == false)
                {
                    sData[0].f[0] = 0.f;
                    sData[0].f[1] = 0.f;
                    sData[0].f[2] = 0.f;
                    sData[0].f[3] = 0.f;
                }
            }
            else
            {
                sData[0].f[0] = 0.f;
                sData[0].f[1] = 0.f;
                sData[0].f[2] = 0.f;
                sData[0].f[3] = 0.f;
            }
        }
    }

     NO_INLINE void sAlphaTest(UFloat4* sData, const float dissolveRef)
    {
        SRenderPipeline& RESTRICT_REFERENCE rRP = gRenDev->m_RP;

        sData[0].f[0] = dissolveRef * (1.0f / 255.0f);
        sData[0].f[1] = (rRP.m_pCurObject->m_ObjFlags & FOB_DISSOLVE_OUT) ? 1.0f : -1.0f;
        sData[0].f[2] = 0.0f;
        sData[0].f[3] = 0.0f;
    }

    NO_INLINE void sParticleEmissiveColor(UFloat4* data, SRenderPipeline& renderPipeline)
    {
        if (renderPipeline.m_pShaderResources)
        {
            ColorF finalEmittance = renderPipeline.m_pShaderResources->GetFinalEmittance();
            data[0].f[0] = finalEmittance.r;
            data[0].f[1] = finalEmittance.g;
            data[0].f[2] = finalEmittance.b;
            data[0].f[3] = 0.0f;
        }
        else
        {
            sZeroLine(data);
        }
    }

    NO_INLINE void sAvgFogVolumeContrib(UFloat4* sData)
    {
        CD3D9Renderer* const __restrict r = gcpRendD3D;
        SRenderPipeline& RESTRICT_REFERENCE rRP = r->m_RP;
        CRenderObject* pObj = rRP.m_pCurObject;
        SRenderObjData* pOD = r->FX_GetObjData(pObj, rRP.m_nProcessThreadID);
        if (!pOD || pOD->m_FogVolumeContribIdx[rRP.m_nProcessThreadID] == (uint16)-1)
        {
            sData[0].f[0] = sData[0].f[1] = sData[0].f[2] = 0.0f;
            sData[0].f[3] = 1.0f;
            return;
        }
        SFogVolumeData fogVolData;
        ColorF& contrib = fogVolData.fogColor;
        r->GetFogVolumeContribution(pOD->m_FogVolumeContribIdx[rRP.m_nProcessThreadID], fogVolData);
        // Pre-multiply alpha (saves 1 instruction in pixel shader)
        sData[0].f[0] = contrib.r * (1 - contrib.a);
        sData[0].f[1] = contrib.g * (1 - contrib.a);
        sData[0].f[2] = contrib.b * (1 - contrib.a);
        sData[0].f[3] = contrib.a;
        // Pass min & max of the aabb and cvar value.
        sData[1].f[0] = fogVolData.avgAABBox.min.x;
        sData[1].f[1] = fogVolData.avgAABBox.min.y;
        sData[1].f[2] = fogVolData.avgAABBox.min.z;
        static ICVar* pCVarFogVolumeShadingQuality = gEnv->pConsole->GetCVar("e_FogVolumeShadingQuality");

        sData[1].f[3] = (pCVarFogVolumeShadingQuality->GetIVal() && fogVolData.avgAABBox.GetRadius() > 0.001f) ? 1.0f : 0.0f;

        sData[2].f[0] = fogVolData.avgAABBox.max.x;
        sData[2].f[1] = fogVolData.avgAABBox.max.y;
        sData[2].f[2] = fogVolData.avgAABBox.max.z;
        sData[2].f[3] = (fogVolData.avgAABBox.GetRadius() > 0.001f) ? 1.0f : 0.0f;

        sData[3].f[0] = fogVolData.m_heightFallOffBasePoint.x;
        sData[3].f[1] = fogVolData.m_heightFallOffBasePoint.y;
        sData[3].f[2] = fogVolData.m_heightFallOffBasePoint.z;
        sData[3].f[3] = fogVolData.m_densityOffset;
        
        sData[4].f[0] = fogVolData.m_heightFallOffDirScaled.x;
        sData[4].f[1] = fogVolData.m_heightFallOffDirScaled.y;
        sData[4].f[2] = fogVolData.m_heightFallOffDirScaled.z;
        sData[4].f[3] = fogVolData.m_globalDensity;

        sData[5].f[0] = Overlap::Point_AABB(r->GetViewParameters().vOrigin, fogVolData.avgAABBox);

    }

    NO_INLINE void sDLightsInfo(UFloat4* sData)
    {
        COMPILE_TIME_ASSERT(sizeof(s_ConstantScratchBuffer) >= (sizeof(SLightVolume::SLightData) * LIGHTVOLUME_MAXLIGHTS));
        PROFILE_FRAME(DLightsInfo_UpdateCB);

        CD3D9Renderer* const __restrict r = gcpRendD3D;
        SRenderPipeline& RESTRICT_REFERENCE rRP = r->m_RP;
        SRenderObjData* pOD = r->FX_GetObjData(rRP.m_pCurObject, rRP.m_nProcessThreadID);
        int nVols = 0;
        if (pOD->m_LightVolumeId > 0)
        {
            assert((uint32)pOD->m_LightVolumeId - 1 < r->m_nNumVols);
            SLightVolume::LightDataVector& lvData = r->m_pLightVols[pOD->m_LightVolumeId - 1].pData;
            nVols = lvData.size();
            assert(nVols <= LIGHTVOLUME_MAXLIGHTS);
            if (nVols)
            {
                memcpy(&sData[0], &lvData[0], sizeof(SLightVolume::SLightData) * nVols);
            }
        }
        memset((char*)sData + sizeof(SLightVolume::SLightData) * nVols, 0, sizeof(SLightVolume::SLightData) * (LIGHTVOLUME_MAXLIGHTS - nVols));
    }

    NO_INLINE void sGetTempData(UFloat4* sData, CD3D9Renderer* r, const SCGParam* ParamBind)
    {
        sData[0].f[0] = r->m_cEF.m_TempVecs[ParamBind->m_nID].x;
        sData[0].f[1] = r->m_cEF.m_TempVecs[ParamBind->m_nID].y;
        sData[0].f[2] = r->m_cEF.m_TempVecs[ParamBind->m_nID].z;
        sData[0].f[3] = r->m_cEF.m_TempVecs[ParamBind->m_nID].w;
    }

    NO_INLINE void sRTRect(UFloat4* sData, CD3D9Renderer* r)
    {
        sData[0].f[0] = r->m_cEF.m_RTRect.x;
        sData[0].f[1] = r->m_cEF.m_RTRect.y;
        sData[0].f[2] = r->m_cEF.m_RTRect.z;
        sData[0].f[3] = r->m_cEF.m_RTRect.w;
    }

    bool sCanSet(const STexSamplerRT* pSM, CTexture* pTex)
    {
        assert(pTex);
        if (!pTex)
        {
            return false;
        }
        if (!pSM->m_bGlobal)
        {
            return true;
        }
        CD3D9Renderer* pRD = gcpRendD3D;
        if (pRD->m_pNewTarget[0] && pRD->m_pNewTarget[0]->m_pTex == pTex)
        {
            return false;
        }
        if (pRD->m_pNewTarget[1] && pRD->m_pNewTarget[1]->m_pTex == pTex)
        {
            return false;
        }
        return true;
    }

    void LogParameter(
        EHWShaderClass shaderClass,
        const SCGParam* parameter,
        AZ::u32 componentIndex = 0)
    {
#ifdef DO_RENDERLOG
        static const char* s_shaderClassNames[] = { "VS", "PS", "GS", "CS", "DS", "HS" };
        static const char* s_componentNames[] = { "x", "y", "z", "w" };

        CD3D9Renderer* const __restrict r = gcpRendD3D;
        SRenderPipeline& RESTRICT_REFERENCE rRP = r->m_RP;
        if (CRenderer::CV_r_log >= 3)
        {
            ECGParam parameterType = ECGParam((parameter->m_eCGParamType >> (componentIndex << 3)) & 0xff);
            if (parameter->m_Flags & PF_SINGLE_COMP)
            {
                r->Logv(
                    SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID],
                    " Set %s parameter '%s:%s' (%d vectors, reg: %d)\n",
                    s_shaderClassNames[shaderClass],
                    parameter->m_Name.c_str(),
                    r->m_cEF.mfGetShaderParamName(parameterType),
                    parameter->m_RegisterCount,
                    parameter->m_RegisterOffset);
            }
            else
            {
                r->Logv(
                    SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID],
                    " Set %s parameter '%s:%s' (%d vectors, reg: %d)\n",
                    s_shaderClassNames[shaderClass],
                    parameter->m_Name.c_str(),
                    r->m_cEF.mfGetShaderParamName(parameterType),
                    parameter->m_RegisterCount,
                    parameter->m_RegisterOffset,
                    s_componentNames[componentIndex]);
            }
        }
#endif
    }

    void UpdateConstants(
        EHWShaderClass shaderClass,
        [[maybe_unused]] EConstantBufferShaderSlot shaderSlot,
        const SCGParam* parameters,
        AZ::u32 parameterCount,
        void* outputData)
    {
        DETAILED_PROFILE_MARKER("mfSetParameters");
        PROFILE_FRAME(Shader_SetParams);

        if (!parameters)
        {
            return;
        }

        CD3D9Renderer* r = gcpRendD3D;
        SRenderPipeline& rRP = r->m_RP;
        CRenderObject* renderObject = rRP.m_pCurObject;
        CShaderResources* shaderResources = rRP.m_pShaderResources;

        for (AZ::u32 parameterIdx = 0; parameterIdx < parameterCount; parameterIdx++)
        {
            const SCGParam* parameter = &parameters[parameterIdx];
            AZ::u32 registerCount = parameter->m_RegisterCount;
            AZ::u32 registerOffset = parameter->m_RegisterOffset;
            AZ::u32 parameterTypeFlags = (uint32)parameter->m_eCGParamType;
            AZ::u32 parameterType = parameterTypeFlags & 0xFF;

            UFloat4* result = s_ConstantScratchBuffer;

            for (AZ::u32 componentIndex = 0; componentIndex < 4; componentIndex++)
            {
                LogParameter(shaderClass, parameter, componentIndex);

                switch (parameterType)
                {
                case ECGP_PB_ScreenSize:
                    sGetScreenSize(result, r);
                    break;

                case ECGP_Matr_PB_TerrainBase:
                    sGetTerrainBase(result, r);
                    break;
                case ECGP_Matr_PB_TerrainLayerGen:
                    sGetTerrainLayerGen(result, r);
                    break;

                case ECGP_PB_BlendTerrainColInfo:
                {
                    SRenderObjData* objectData = rRP.m_pCurObject->GetObjData();
                    if (objectData)
                    {
                        float* pObjTmpVars = objectData->m_fTempVars;

                        result[0].f[0] = pObjTmpVars[3];     // fTexOffsetX
                        result[0].f[1] = pObjTmpVars[4];     // fTexOffsetY
                        result[0].f[2] = pObjTmpVars[5];     // fTexScale
                        result[0].f[3] = pObjTmpVars[8];     // Obj view distance
                    }
                }
                break;

                case ECGP_Matr_PB_Temp4_0:
                case ECGP_Matr_PB_Temp4_1:
                case ECGP_Matr_PB_Temp4_2:
                case ECGP_Matr_PB_Temp4_3:
                    memcpy(result, &r->m_TempMatrices[parameter->m_eCGParamType - ECGP_Matr_PB_Temp4_0][parameter->m_nID], sizeof(Matrix44));
                    break;

                case ECGP_PB_FromRE:
                {
                    IRenderElement* pRE = rRP.m_pRE;
                    if (!pRE || !pRE->GetCustomData())
                    {
                        result[0].f[componentIndex] = 0;
                    }
                    else
                    {
                        result[0].f[componentIndex] = reinterpret_cast<float*>(pRE->GetCustomData())[(parameter->m_nID >> (componentIndex * 8)) & 0xff];
                    }
                    break;
                }
                case ECGP_PB_FromObjSB:
                    sFromObjSB(result);
                    break;

                case ECGP_PB_GmemStencilValue:
                {
                    if (gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
                    {
                        uint32 stencilRef = CRenderer::CV_r_VisAreaClipLightsPerPixel ? 0 : (rRP.m_RIs[0][0]->nStencRef | BIT_STENCIL_INSIDE_CLIPVOLUME);
                        // Here we check if an object can receive decals.
                        bool bObjectAcceptsDecals = !(rRP.m_pCurObject->m_NoDecalReceiver);
                        if (bObjectAcceptsDecals)
                        {
                             stencilRef |= (!(rRP.m_pCurObject->m_ObjFlags & FOB_DYNAMIC_OBJECT) || CRenderer::CV_r_deferredDecalsOnDynamicObjects ? BIT_STENCIL_RESERVED : 0);
                        }
                        result[0].f[0] = azlossy_caster(stencilRef);
                        result[0].f[1] = 0.0f;
                        result[0].f[2] = 0.0f;
                        result[0].f[3] = 0.0f;
                    }
                    else
                    {
                        CRY_ASSERT_MESSAGE(false, "Warning: Trying to use GMEM Stencil attribute in a shader but GMEM is disabled. Value will not be set.");
                    }
                    break;
                }

                case ECGP_PB_TempData:
                    sGetTempData(result, r, parameter);
                    break;

                case ECGP_Matr_PB_UnProjMatrix:
                {
                    Matrix44A* pMat = alias_cast<Matrix44A*>(&result[0]);
                    *pMat = rRP.m_TI[rRP.m_nProcessThreadID].m_matView * rRP.m_TI[rRP.m_nProcessThreadID].m_matProj;
                    *pMat = pMat->GetInverted();
                    *pMat = pMat->GetTransposed();
                    break;
                }

                case ECGP_PB_DLightsInfo:
                    sDLightsInfo(result);
                    break;

                case ECGP_PB_ObjVal:
                {
                    SRenderObjData* objectData = rRP.m_pCurObject->GetObjData();
                    if (objectData)
                    {
                        result[0].f[componentIndex] =
                            reinterpret_cast<float*>(objectData->m_fTempVars)[(parameter->m_nID >> (componentIndex * 8)) & 0xff];
                    }
                }
                break;
                case ECGP_PB_IrregKernel:
                    sGetIrregKernel(result, r);
                    break;
                case ECGP_PB_TFactor:
                    result[0].f[0] = r->m_RP.m_CurGlobalColor[0];
                    result[0].f[1] = r->m_RP.m_CurGlobalColor[1];
                    result[0].f[2] = r->m_RP.m_CurGlobalColor[2];
                    result[0].f[3] = r->m_RP.m_CurGlobalColor[3];
                    break;
                case ECGP_PB_RTRect:
                    sRTRect(result, r);
                    break;
                case ECGP_PB_Scalar:
                    assert(parameter->m_pData);
                    if (parameter->m_pData)
                    {
                        result[0].f[componentIndex] = parameter->m_pData->d.fData[componentIndex];
                    }
                    break;

                case ECGP_PB_ClipVolumeParams:
                    result[0].f[0] = rRP.m_pCurObject->m_nClipVolumeStencilRef + 1.0f;
                    result[0].f[1] = result[0].f[2] = result[0].f[3] = 0;
                    break;

                /// Used by Sketch.cfx
                case ECGP_PB_ResInfoDiffuse:
                    sResInfo(result, EFTT_DIFFUSE);
                    break;
                case ECGP_PB_TexelDensityParam:
                    sTexelDensityParam(result, EFTT_DIFFUSE);
                    break;
                case ECGP_PB_TexelDensityColor:
                    sTexelDensityColor(result, EFTT_DIFFUSE);
                    break;
                case ECGP_PB_TexelsPerMeterInfo:
                    sTexelsPerMeterInfo(result, EFTT_DIFFUSE);
                    break;
                ///!

                case ECGP_PB_VisionMtlParams:
                    sVisionMtlParams(result);
                    break;

                /// Water.cfx / WaterVolume.cfx
                case ECGP_PB_WaterRipplesLookupParams:
                    if (CPostEffectsMgr* pPostEffectsMgr = PostEffectMgr())
                    {
                        if (CWaterRipples* pWaterRipplesTech = (CWaterRipples*)pPostEffectsMgr->GetEffect(ePFX_WaterRipples))
                        {
                            result[0].f[0] = pWaterRipplesTech->GetLookupParams().x;
                            result[0].f[1] = pWaterRipplesTech->GetLookupParams().y;
                            result[0].f[2] = pWaterRipplesTech->GetLookupParams().z;
                            result[0].f[3] = pWaterRipplesTech->GetLookupParams().w;
                        }
                    }
                    break;
                ///!

                case ECGP_PB_SkinningExtraWeights:
                {
                    if (rRP.m_pRE->mfGetType() == eDATA_Mesh && ((CREMeshImpl*)rRP.m_pRE)->m_pRenderMesh->m_extraBonesBuffer.m_numElements > 0)
                    {
                        result[0].f[0] = 1.0f;
                        result[0].f[1] = 1.0f;
                        result[0].f[2] = 1.0f;
                        result[0].f[3] = 1.0f;
                    }
                    else
                    {
                        result[0].f[0] = 0.0f;
                        result[0].f[1] = 0.0f;
                        result[0].f[2] = 0.0f;
                        result[0].f[3] = 0.0f;
                    }
                    break;
                }

                case 0:
                    break;

                default:
                    assert(0);
                    break;
                }
                if (parameter->m_Flags & PF_SINGLE_COMP)
                {
                    break;
                }

                parameterTypeFlags >>= 8;
            }

            AzRHI::SIMDCopy(&reinterpret_cast<UFloat4*>(outputData)[registerOffset], s_ConstantScratchBuffer, registerCount);
        }
    }
}

void CHWShader_D3D::UpdatePerInstanceConstants(
    EHWShaderClass shaderClass,
    const SCGParam* parameters,
    AZ::u32 parameterCount,
    void* outputData)
{
    DETAILED_PROFILE_MARKER("UpdatePerInstanceConstants");

    CD3D9Renderer* const __restrict r = gcpRendD3D;
    SRenderPipeline& RESTRICT_REFERENCE rRP = r->m_RP;
    CRenderObject* const __restrict pObj = rRP.m_pCurObject;
    CShaderResources* shaderResources = rRP.m_pShaderResources;

    UFloat4* result = s_ConstantScratchBuffer;
    AZ_Assert(pObj, "Trying to set PI parameters with NULL object");

    // precache int to float conversions for some parameters
    float objDissolveRef = (float)(pObj->m_DissolveRef);
    float objRenderQuality = (float)(pObj->m_nRenderQuality);
    const CRenderObject::SInstanceInfo& instanceInfo = pObj->m_II;

    for (AZ::u32 parameterIdx = 0; parameterIdx < parameterCount; parameterIdx++)
    {
        const SCGParam* parameter = &parameters[parameterIdx];

        // Not activated yet for this shader
        if (parameter->m_BindingSlot < 0)
        {
            continue;
        }

        assert(parameter->m_Flags & PF_SINGLE_COMP);
        LogParameter(shaderClass, parameter);

        switch (parameter->m_eCGParamType)
        {
        case ECGP_SI_AmbientOpacity:
            sAmbientOpacity(pObj, r, shaderResources, result, instanceInfo);
            break;
        case ECGP_SI_BendInfo:
            sGetBendInfo(pObj, FrameType::Current, result, r);
            break;
        case ECGP_SI_PrevBendInfo:
            sGetBendInfo(pObj, FrameType::Previous, result, r);
            break;
        case ECGP_SI_ObjectAmbColComp:
            sObjectAmbColComp(result, instanceInfo, objRenderQuality);
            break;
        case ECGP_SI_AlphaTest:
            sAlphaTest(result, objDissolveRef);
            break;

        case ECGP_Matr_PI_Obj_T:
        {
            Store(result, instanceInfo.m_Matrix);
        }
        break;
        case ECGP_Matr_PI_ViewProj:
        {
            mathMatrixMultiply_Transp2(&result[4].f[0], r->m_ViewProjMatrix.GetData(), instanceInfo.m_Matrix.GetData(), g_CpuFlags);
            TransposeAndStore(result, *alias_cast<Matrix44A*>(&result[4]));
            sAppendClipSpaceAdaptation(alias_cast<Matrix44A*>(&result[0]));
        }
        break;
        case ECGP_PI_Ambient:
            sAmbient(result, rRP, instanceInfo);
            break;
        case ECGP_PI_MotionBlurInfo:
            sMotionBlurInfo(result, rRP);
            break;

        case ECGP_PI_ParticleEmissiveColor:
            sParticleEmissiveColor(result, rRP);
            break;

        case ECGP_PI_WrinklesMask0:
            sWrinklesMask(result, rRP, 0);
            break;
        case ECGP_PI_WrinklesMask1:
            sWrinklesMask(result, rRP, 1);
            break;
        case ECGP_PI_WrinklesMask2:
            sWrinklesMask(result, rRP, 2);
            break;

        case ECGP_PI_AvgFogVolumeContrib:
            sAvgFogVolumeContrib(result);
            break;
        // Remove ECGP_Matr_PI_Composite after we refactor Set2DMode and m_matView and m_matProj
        // For now ECGP_Matr_PI_Composite is not used in 3D object rendering shaders.
        case ECGP_Matr_PI_Composite:
        {
            const Matrix44A viewProj = rRP.m_TI[rRP.m_nProcessThreadID].m_matView * rRP.m_TI[rRP.m_nProcessThreadID].m_matProj;
            TransposeAndStore(result, viewProj);
        }
        break;
        case ECGP_PI_MotionBlurData:
            sGetMotionBlurData(result, r, instanceInfo, rRP);

            break;
        case ECGP_PI_PrevObjWorldMatrix:
            sGetPrevObjWorldData(result, rRP);
            break;
        case ECGP_Matr_PI_TexMatrix:
            sGetTexMatrix(result, r, parameter);
            break;
        case ECGP_Matr_PI_TCGMatrix:
        {
            SEfResTexture* pRT = rRP.m_ShaderTexResources[parameter->m_nID];
            if (pRT && pRT->m_Ext.m_pTexModifier)
            {
                Store(result, pRT->m_Ext.m_pTexModifier->m_TexGenMatrix);
            }
            else
            {
                Store(result, r->m_IdentityMatrix);
            }
        }
        break;
        case ECGP_PI_OSCameraPos:
        {
            Matrix44A* pMat1 = alias_cast<Matrix44A*>(&result[4]);
            Matrix44A* pMat2 = alias_cast<Matrix44A*>(&result[0]);
            *pMat1 = instanceInfo.m_Matrix.GetTransposed();
            *pMat2 = fabs(pMat1->Determinant()) > 1e-6 ? (*pMat1).GetInverted() : Matrix44(IDENTITY);

            // Respect Camera-Space rendering
            Vec3 cameraPosObjectSpace;
            Vec3 cameraPos = (rRP.m_pCurObject->m_ObjFlags & FOB_NEAREST) ? ZERO : r->GetViewParameters().vOrigin;
            TransformPosition(cameraPosObjectSpace, cameraPos, *pMat2);

            result[0].f[0] = cameraPosObjectSpace.x;
            result[0].f[1] = cameraPosObjectSpace.y;
            result[0].f[2] = cameraPosObjectSpace.z;
            result[0].f[3] = 1.f;
        }
        break;
        case ECGP_PI_VisionParams:
            sVisionParams(result);
            break;
        case ECGP_PI_NumInstructions:
            sNumInstructions(result);
            break;
        case ECGP_Matr_PI_OceanMat:
            sOceanMat(result);
            break;

        case ECGP_PI_FurLODInfo:
        {
            // FurLODInfo contains LOD values for the current object to adjust fur rendering:
            //   x - Current object's first LOD distance
            //   y - Current object's max view distance
            // Presently, this is used to control self-shadowing as a function of distance and LOD.
            if (rRP.m_pCurObject)
            {
                if (IRenderNode* pRenderNode = rRP.m_pCurObject->m_pRenderNode)
                {
                    /// Fix it! should not access the memory pointed by IRenderNode in render thread.
                    // Scale number of shell passes by object's distance to camera and LOD ratio
                    float lodRatio = pRenderNode->GetLodRatioNormalized();
                    if (lodRatio > 0.0f)
                    {
                        static ICVar* pTargetSize = gEnv->pConsole->GetCVar("e_LodFaceAreaTargetSize");
                        if (pTargetSize)
                        {
                            lodRatio *= pTargetSize->GetFVal();
                        }
                    }

                    float maxDistance = CD3D9Renderer::CV_r_FurMaxViewDist * pRenderNode->GetViewDistanceMultiplier();
                    float lodDistance = AZ::GetClamp(pRenderNode->GetFirstLodDistance() / lodRatio, 0.0f, maxDistance);

                    result[0].f[0] = lodDistance;
                    result[0].f[1] = maxDistance;
                }
            }
            break;
        }

        case ECGP_PI_FurParams:
        {
            // FurParams contains common information for fur rendering:
            //   x, y, z - wind direction and strength in world space, for wind bending of fur
            //   w - distance of current shell between base and outermost shell, in the range [0, 1]
            AABB instanceBboxWorld(AABB::RESET);
            Vec3 vWindWorld(ZERO);
            if (rRP.m_pRE)
            {
                rRP.m_pRE->mfGetBBox(instanceBboxWorld.min, instanceBboxWorld.max);
                instanceBboxWorld.min = instanceInfo.m_Matrix * instanceBboxWorld.min;
                instanceBboxWorld.max = instanceInfo.m_Matrix * instanceBboxWorld.max;
            }
            if (instanceBboxWorld.IsReset())
            {
                vWindWorld = gEnv->p3DEngine->GetGlobalWind(false);
            }
            else
            {
                vWindWorld = gEnv->p3DEngine->GetWind(instanceBboxWorld, false);
            }

            result[0].f[0] = vWindWorld.x;
            result[0].f[1] = vWindWorld.y;
            result[0].f[2] = vWindWorld.z;
            result[0].f[3] = FurPasses::GetInstance().GetFurShellPassPercent();
            break;
        }

        default:
            assert(0);
            break;
        }

        const AZ::u32 registerCount = parameter->m_RegisterCount;
        const AZ::u32 registerOffset = parameter->m_RegisterOffset;
        AzRHI::SIMDCopy(&reinterpret_cast<UFloat4*>(outputData)[registerOffset], result, registerCount);
    }
}

void CHWShader_D3D::UpdatePerInstanceConstantBuffer()
{
    if (!m_pCurInst)
    {
        return;
    }
    SHWSInstance* pInst = m_pCurInst;
    if (pInst->m_nParams[1] >= 0)
    {
        SCGParamsGroup& Group = CGParamManager::s_Groups[pInst->m_nParams[1]];

        void* mappedData = AzRHI::ConstantBufferCache::GetInstance().MapConstantBuffer(
            m_eSHClass, eConstantBufferShaderSlot_PerInstanceLegacy, pInst->m_nMaxVecs[1]);
        UpdatePerInstanceConstants(m_eSHClass, Group.pParams, Group.nParams, mappedData);
    }
}

void CHWShader_D3D::UpdatePerBatchConstantBuffer()
{
    if (!m_pCurInst)
    {
        return;
    }
    SHWSInstance* pInst = m_pCurInst;
    if (pInst->m_nParams[0] >= 0)
    {
        SCGParamsGroup& Group = CGParamManager::s_Groups[pInst->m_nParams[0]];

        void* mappedData = AzRHI::ConstantBufferCache::GetInstance().MapConstantBuffer(
            m_eSHClass, eConstantBufferShaderSlot_PerBatch, pInst->m_nMaxVecs[0]);
        UpdateConstants(m_eSHClass, eConstantBufferShaderSlot_PerBatch, Group.pParams, Group.nParams, mappedData);
    }
}

void CHWShader_D3D::UpdatePerViewConstantBuffer()
{
    CD3D9Renderer* rd = gcpRendD3D;
    rd->GetGraphicsPipeline().UpdatePerViewConstantBuffer();
    rd->GetGraphicsPipeline().BindPerViewConstantBuffer();
}

void CD3D9Renderer::UpdatePerFrameParameters()
{
    // Per frame - hardcoded/fast - update of commonly used data - feel free to improve this
    int nThreadID = m_RP.m_nFillThreadID;
    uint32 nFrameID = gRenDev->m_RP.m_TI[nThreadID].m_nFrameUpdateID;
    PerFrameParameters& PF = gRenDev->m_RP.m_TI[nThreadID].m_perFrameParameters;
    if (PF.m_FrameID == nFrameID || SRendItem::m_RecurseLevel[nThreadID] > 0)
    {
        return;
    }

    PF.m_FrameID = nFrameID;

    I3DEngine* p3DEngine = gEnv->p3DEngine;
    if (p3DEngine == nullptr)
    {
        return;
    }

    PF.m_WaterLevel = Vec3(OceanToggle::IsActive() ? OceanRequest::GetOceanLevel() : p3DEngine->GetWaterLevel());

    {
        // Caustics are done with projection from sun - ence they update too fast with regular
        // sun direction. Use a smooth sun direction update instead to workaround this
        Vec3 realtimeSunDirNormalized = p3DEngine->GetRealtimeSunDirNormalized();

        const float snapshot = 0.98f;
        float dotProduct = fabs(PF.m_CausticsSunDirection.Dot(realtimeSunDirNormalized));
        if (dotProduct < snapshot)
        {
            PF.m_CausticsSunDirection = realtimeSunDirNormalized;
        }

        PF.m_CausticsSunDirection += (realtimeSunDirNormalized - PF.m_CausticsSunDirection) * 0.005f * gEnv->pTimer->GetFrameTime();
        PF.m_CausticsSunDirection.Normalize();
    }

    {
        Vec4 hdrSetupParams[5];
        p3DEngine->GetHDRSetupParams(hdrSetupParams);
        // Film curve setup
        PF.m_HDRParams = Vec4(
            hdrSetupParams[0].x * 6.2f,
            hdrSetupParams[0].y * 0.5f,
            hdrSetupParams[0].z * 0.06f,
            1.0f);
    }

    {
        Vec3 multiplier;
        p3DEngine->GetGlobalParameter(E3DPARAM_SUN_SPECULAR_MULTIPLIER, multiplier);
        PF.m_SunSpecularMultiplier = multiplier.x;
    }

    // Set energy indicator representing the sun intensity compared to noon.
    Vec3    dayNightIndicators;
    p3DEngine->GetGlobalParameter(E3DPARAM_DAY_NIGHT_INDICATOR, dayNightIndicators);
    PF.m_MidDayIndicator = dayNightIndicators.y;

    p3DEngine->GetGlobalParameter(E3DPARAM_CLOUDSHADING_SUNCOLOR, PF.m_CloudShadingColorSun);
    p3DEngine->GetGlobalParameter(E3DPARAM_CLOUDSHADING_SKYCOLOR, PF.m_CloudShadingColorSky);

    {
        //Prevent division by Zero if there's no terrain system.
        AZ::Aabb terrainAabb = AZ::Aabb::CreateFromMinMax(AZ::Vector3::CreateZero(), AZ::Vector3::CreateOne());
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(terrainAabb, &AzFramework::Terrain::TerrainDataRequests::GetTerrainAabb);
        const float heightMapSizeX = terrainAabb.GetXExtent();
        const float heightMapSizeY = terrainAabb.GetYExtent();

        Vec3 cloudShadowOffset = m_cloudShadowSpeed * gEnv->pTimer->GetCurrTime();
        cloudShadowOffset.x -= (int)cloudShadowOffset.x;
        cloudShadowOffset.y -= (int)cloudShadowOffset.y;
        PF.m_CloudShadowAnimParams = Vec4(m_cloudShadowTiling / heightMapSizeX, -m_cloudShadowTiling / heightMapSizeY, cloudShadowOffset.x, -cloudShadowOffset.y);
        PF.m_CloudShadowParams = Vec4(0, 0, m_cloudShadowInvert ? 1.0f : 0.0f, m_cloudShadowBrightness);
    }

    {
        float* projMatrix = (float*)&gcpRendD3D->m_RP.m_TI[nThreadID].m_matProj;
        float scalingFactor = clamp_tpl(CRenderer::CV_r_ZFightingDepthScale, 0.1f, 1.0f);

        PF.m_DecalZFightingRemedy.x = scalingFactor; // scaling factor to pull decal in front
        PF.m_DecalZFightingRemedy.y = (float)((1.0f - scalingFactor) * projMatrix[4 * 3 + 2]); // correction factor for homogeneous z after scaling is applied to xyzw { = ( 1 - v[0] ) * zMappingRageBias }
        PF.m_DecalZFightingRemedy.z = clamp_tpl(CRenderer::CV_r_ZFightingExtrude, 0.0f, 1.0f);
    }

    PF.m_VolumetricFogParams = sGetVolumetricFogParams(gcpRendD3D);
    PF.m_VolumetricFogRampParams = sGetVolumetricFogRampParams();

    sGetFogColorGradientConstants(PF.m_VolumetricFogColorGradientBase, PF.m_VolumetricFogColorGradientDelta);
    PF.m_VolumetricFogColorGradientParams = sGetFogColorGradientParams();
    PF.m_VolumetricFogColorGradientRadial = sGetFogColorGradientRadial(gcpRendD3D);
    PF.m_VolumetricFogSamplingParams = sGetVolumetricFogSamplingParams(gcpRendD3D);
    PF.m_VolumetricFogDistributionParams = sGetVolumetricFogDistributionParams(gcpRendD3D);
    PF.m_VolumetricFogScatteringParams = sGetVolumetricFogScatteringParams(gcpRendD3D);
    PF.m_VolumetricFogScatteringBlendParams = sGetVolumetricFogScatteringBlendParams(gcpRendD3D);
    PF.m_VolumetricFogScatteringColor = sGetVolumetricFogScatteringColor(gcpRendD3D);
    PF.m_VolumetricFogScatteringSecondaryColor = sGetVolumetricFogScatteringSecondaryColor(gcpRendD3D);
    PF.m_VolumetricFogHeightDensityParams = sGetVolumetricFogHeightDensityParams(gcpRendD3D);
    PF.m_VolumetricFogHeightDensityRampParams = sGetVolumetricFogHeightDensityRampParams(gcpRendD3D);
    PF.m_VolumetricFogDistanceParams = sGetVolumetricFogDistanceParams(gcpRendD3D);
}

void CD3D9Renderer::ForceUpdateGlobalShaderParameters()
{
    UpdatePerFrameParameters();

    gRenDev->m_pRT->EnqueueRenderCommand([this]()
    {
        FX_PreRender(1);
        CHWShader_D3D::UpdatePerFrameResourceGroup();
    });
}

void CHWShader_D3D::UpdatePerFrameResourceGroup()
{
    static std::vector<SCGTexture> s_textures;
    static std::vector<STexSamplerRT> s_samplers;
    s_textures.assign(s_PF_Textures.begin(), s_PF_Textures.end());
    s_samplers.assign(s_PF_Samplers.begin(), s_PF_Samplers.end());
    mfSetTextures(s_textures, eHWSC_Pixel);
    mfSetSamplers_Old(s_samplers, eHWSC_Pixel);

    const PerFrameParameters& PF = gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_perFrameParameters;
    CD3D9Renderer* const __restrict rd = gcpRendD3D;
    CDeviceManager& deviceManager = rd->m_DevMan;
    rd->GetGraphicsPipeline().UpdatePerFrameConstantBuffer(PF);
    rd->GetGraphicsPipeline().BindPerFrameConstantBuffer();
}

void CHWShader_D3D::UpdatePerMaterialConstantBuffer()
{
    DETAILED_PROFILE_MARKER("UpdatePerMaterialConstantBuffer");
    CD3D9Renderer* const __restrict rd = gcpRendD3D;
    CShaderResources* const __restrict shaderResources = rd->m_RP.m_pShaderResources;
    CDeviceManager& deviceManager = rd->m_DevMan;
    if (shaderResources)
    {
        AzRHI::ConstantBuffer* constantBuffer = shaderResources->GetConstantBuffer();
        deviceManager.BindConstantBuffer(eHWSC_Vertex, constantBuffer, eConstantBufferShaderSlot_PerMaterial);
        deviceManager.BindConstantBuffer(eHWSC_Pixel, constantBuffer, eConstantBufferShaderSlot_PerMaterial);
        
        if (s_pCurInstDS)
        {
            deviceManager.BindConstantBuffer(eHWSC_Domain, constantBuffer, eConstantBufferShaderSlot_PerMaterial);
        }
        if (s_pCurInstHS)
        {
            deviceManager.BindConstantBuffer(eHWSC_Hull, constantBuffer, eConstantBufferShaderSlot_PerMaterial);
        }
        if (s_pCurInstGS)
        {
            deviceManager.BindConstantBuffer(eHWSC_Geometry, constantBuffer, eConstantBufferShaderSlot_PerMaterial);
        }
        if (s_pCurInstCS)
        {
            deviceManager.BindConstantBuffer(eHWSC_Compute, constantBuffer, eConstantBufferShaderSlot_PerMaterial);
        }
    }
}

void CHWShader_D3D::mfCommitParamsGlobal()
{
    DETAILED_PROFILE_MARKER("mfCommitParamsGlobal");
    PROFILE_FRAME(CommitGlobalShaderParams);

    CD3D9Renderer* const __restrict rd = gcpRendD3D;
    SRenderPipeline& RESTRICT_REFERENCE rRP = rd->m_RP;
    if (rRP.m_PersFlags2 & (RBPF2_COMMIT_PF | RBPF2_COMMIT_CM))
    {
        rRP.m_PersFlags2 &= ~(RBPF2_COMMIT_PF | RBPF2_COMMIT_CM);
        UpdatePerViewConstantBuffer();
    }
}

void CHWShader_D3D::mfSetGlobalParams()
{
    AZ_TRACE_METHOD();
    DETAILED_PROFILE_MARKER("mfSetGlobalParams");
#ifdef DO_RENDERLOG
    if (CRenderer::CV_r_log >= 3)
    {
        gRenDev->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "--- Set global shader constants...\n");
    }
#endif

    Vec4 v;
    CD3D9Renderer* r = gcpRendD3D;

    r->m_RP.m_PersFlags2 |= RBPF2_COMMIT_PF | RBPF2_COMMIT_CM;
    r->m_RP.m_nCommitFlags |= FC_GLOBAL_PARAMS;
}

void CHWShader_D3D::mfSetCameraParams()
{
    DETAILED_PROFILE_MARKER("mfSetCameraParams");
#ifdef DO_RENDERLOG
    if (CRenderer::CV_r_log >= 3)
    {
        gRenDev->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "--- Set camera shader constants...\n");
    }
#endif
    gRenDev->m_RP.m_PersFlags2 |= RBPF2_COMMIT_PF | RBPF2_COMMIT_CM;
    gRenDev->m_RP.m_nCommitFlags |= FC_GLOBAL_PARAMS;
}

#if !defined(_RELEASE)
void CHWShader_D3D::LogSamplerTextureMismatch(const CTexture* pTex, const STexSamplerRT* pSampler, EHWShaderClass shaderClass, const char* pMaterialName)
{
    if (pTex && pSampler)
    {
        const CHWShader_D3D::SHWSInstance* pInst = 0;
        switch (shaderClass)
        {
        case eHWSC_Vertex:
            pInst = s_pCurInstVS;
            break;
        case eHWSC_Pixel:
            pInst = s_pCurInstPS;
            break;
        case eHWSC_Geometry:
            pInst = s_pCurInstGS;
            break;
        case eHWSC_Compute:
            pInst = s_pCurInstCS;
            break;
        case eHWSC_Domain:
            pInst = s_pCurInstDS;
            break;
        case eHWSC_Hull:
            pInst = s_pCurInstHS;
            break;
        default:
            break;
        }

        const char* pSamplerName = "unknown";
        if (pInst)
        {
            const uint32 slot = pSampler->m_nTextureSlot;
            for (size_t idx = 0; idx < pInst->m_pBindVars.size(); ++idx)
            {
                if (slot == (pInst->m_pBindVars[idx].m_RegisterOffset & 0xF) && (pInst->m_pBindVars[idx].m_RegisterOffset & SHADER_BIND_SAMPLER) != 0)
                {
                    pSamplerName = pInst->m_pBindVars[idx].m_Name.c_str();
                    break;
                }
            }
        }

        const char* pShaderName = gcpRendD3D->m_RP.m_pShader ? gcpRendD3D->m_RP.m_pShader->GetName() : "NULL";
        const char* pTechName = gcpRendD3D->m_RP.m_pCurTechnique ? gcpRendD3D->m_RP.m_pCurTechnique->m_NameStr.c_str() : "NULL";
        const char* pSamplerTypeName = CTexture::NameForTextureType((ETEX_Type)pSampler->m_eTexType);
        const char* pTexName = pTex->GetName();
        const char* pTexTypeName = pTex->GetTypeName();
        const char* pTexSurrogateMsg = pTex->IsNoTexture() ? " (texture doesn't exist!)" : "";

        // Do not keep re-logging the same error every frame, in editor this will pop-up an error dialog (every frame), rendering it unusable (also can't save map)
        const char* pMaterialNameNotNull = pMaterialName ? pMaterialName : "none";
        CCrc32 crc;
        crc.Add(pShaderName);
        crc.Add(pTechName);
        crc.Add(pSamplerName);
        crc.Add(pSamplerTypeName);
        crc.Add(pTexName);
        crc.Add(pTexTypeName);
        crc.Add(pTexSurrogateMsg);
        crc.Add(pMaterialNameNotNull);
        const bool bShouldLog = s_ErrorsLogged.insert(crc.Get()).second;
        if (!bShouldLog)
        {
            return;
        }

        if (!pTex->GetIsTextureMissing())
        {
            CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_ERROR_DBGBRK, "!Mismatch between texture and sampler type detected! ...\n"
                "- Shader \"%s\" with technique \"%s\"\n"
                "- Sampler \"%s\" is of type \"%s\"\n"
                "- Texture \"%s\" is of type \"%s\"%s\n"
                "- Material is \"%s\"",
                pShaderName, pTechName,
                pSamplerName, pSamplerTypeName,
                pTexName, pTexTypeName, pTexSurrogateMsg,
                pMaterialName ? pMaterialName : "none");
        }
    }
}
#endif

static EEfResTextures sSlots[] =
{
    EFTT_UNKNOWN,

    EFTT_DIFFUSE,
    EFTT_NORMALS,
    EFTT_HEIGHT,
    EFTT_SPECULAR,
    EFTT_ENV,
    EFTT_SUBSURFACE,
    EFTT_SMOOTHNESS,
    EFTT_DECAL_OVERLAY,
    EFTT_CUSTOM,
    EFTT_CUSTOM_SECONDARY,
    EFTT_OPACITY,
    EFTT_DETAIL_OVERLAY,
    EFTT_EMITTANCE,
    EFTT_OCCLUSION,
    EFTT_SPECULAR_2,
};

bool CHWShader_D3D::mfSetSamplers(const std::vector<SCGSampler>& Samplers, EHWShaderClass eSHClass)
{
    DETAILED_PROFILE_MARKER("mfSetSamplers");
    FUNCTION_PROFILER_RENDER_FLAT
        //PROFILE_FRAME(Shader_SetShaderSamplers);
        const uint32 nSize = Samplers.size();
    if (!nSize)
    {
        return true;
    }
    CD3D9Renderer* __restrict rd = gcpRendD3D;
    CShaderResources* __restrict pSR = rd->m_RP.m_pShaderResources;

    uint32 i;
    const SCGSampler* pSamp = &Samplers[0];
    for (i = 0; i < nSize; i++)
    {
        const SCGSampler* pSM = pSamp++;
        int nSUnit = pSM->m_BindingSlot;
        int nTState = pSM->m_nStateHandle;

        switch (pSM->m_eCGSamplerType)
        {
        case ECGS_Unknown:
            break;

        case ECGS_Shadow0:
        case ECGS_Shadow1:
        case ECGS_Shadow2:
        case ECGS_Shadow3:
        case ECGS_Shadow4:
        case ECGS_Shadow5:
        case ECGS_Shadow6:
        case ECGS_Shadow7:
        {
            const int nShadowMapNum = pSM->m_eCGSamplerType - ECGS_Shadow0;
            //force  MinFilter = Linear; MagFilter = Linear; for HW_PCF_FILTERING
            STexState TS;

            TS.m_pDeviceState = NULL;
            TS.SetClampMode(TADDR_CLAMP, TADDR_CLAMP, TADDR_CLAMP);

            SShaderProfile* pSP = &gRenDev->m_cEF.m_ShaderProfiles[eST_Shadow];
            const int  nShadowQuality = (int)pSP->GetShaderQuality();
            const bool bShadowsVeryHigh = nShadowQuality == eSQ_VeryHigh;
            const bool bForwardShadows = (rd->m_RP.m_pShader->m_Flags2 & EF2_ALPHABLENDSHADOWS) != 0;
            const bool bParticleShadow = (gRenDev->m_RP.m_FlagsShader_RT & g_HWSR_MaskBit[HWSR_PARTICLE_SHADOW]) != 0;
            const bool bPCFShadow = (gRenDev->m_RP.m_FlagsShader_RT & g_HWSR_MaskBit[HWSR_HW_PCF_COMPARE]) != 0;

            if ((!bShadowsVeryHigh || (nShadowMapNum != 0) || bForwardShadows || bParticleShadow) && bPCFShadow)
            {
                // non texture array case vs. texture array case
                TS.SetComparisonFilter(true);
                TS.SetFilterMode(FILTER_LINEAR);
            }
            else
            {
                TS.SetComparisonFilter(false);
                TS.SetFilterMode(FILTER_POINT);
            }

            const int texState = CTexture::GetTexState(TS);
            CTexture::SetSamplerState(texState, nSUnit, eSHClass);
        }
        break;

        case ECGS_TrilinearClamp:
        {
            const static int nTStateTrilinearClamp = CTexture::GetTexState(STexState(FILTER_TRILINEAR, true));
            CTexture::SetSamplerState(nTStateTrilinearClamp, nSUnit, eSHClass);
        }
        break;
        case ECGS_MatAnisoHighWrap:
        {
            CTexture::SetSamplerState(gcpRendD3D->m_nMaterialAnisoHighSampler, nSUnit, eSHClass);
        }
        break;
        case ECGS_MatAnisoLowWrap:
        {
            CTexture::SetSamplerState(gcpRendD3D->m_nMaterialAnisoLowSampler, nSUnit, eSHClass);
        }
        break;
        case ECGS_MatTrilinearWrap:
        {
            const static int nTStateTrilinearWrap = CTexture::GetTexState(STexState(FILTER_TRILINEAR, false));
            CTexture::SetSamplerState(nTStateTrilinearWrap, nSUnit, eSHClass);
        }
        break;
        case ECGS_MatBilinearWrap:
        {
            const static int nTStateBilinearWrap = CTexture::GetTexState(STexState(FILTER_BILINEAR, false));
            CTexture::SetSamplerState(nTStateBilinearWrap, nSUnit, eSHClass);
        }
        break;
        case ECGS_MatTrilinearClamp:
        {
            const static int nTStateTrilinearClamp = CTexture::GetTexState(STexState(FILTER_TRILINEAR, true));
            CTexture::SetSamplerState(nTStateTrilinearClamp, nSUnit, eSHClass);
        }
        break;
        case ECGS_MatBilinearClamp:
        {
            const static int nTStateBilinearClamp = CTexture::GetTexState(STexState(FILTER_BILINEAR, true));
            CTexture::SetSamplerState(nTStateBilinearClamp, nSUnit, eSHClass);
        }
        break;
        case ECGS_MatAnisoHighBorder:
        {
            CTexture::SetSamplerState(gcpRendD3D->m_nMaterialAnisoSamplerBorder, nSUnit, eSHClass);
        }
        break;
        case ECGS_MatTrilinearBorder:
        {
            const static int nTStateTrilinearBorder = CTexture::GetTexState(STexState(FILTER_TRILINEAR, TADDR_BORDER, TADDR_BORDER, TADDR_BORDER, 0x0));
            CTexture::SetSamplerState(nTStateTrilinearBorder, nSUnit, eSHClass);
        }
        break;

        default:
            assert(0);
            break;
        }
    }

    return true;
}

//------------------------------------------------------------------------------
// This is the final texture prep and bind point to the shader, using the 
// function call 'CTexture::ApplyTexture'.
//
// This method runs over the list of parsed textures and bind them to the HW stage
// Materials textures are handled the same and if do not exist they use a default.
// The rest of the textures (engine / per frame ..) are specifically handled.
// [Shader System] - this method should go data driven and have the same handling per texture.
//
// Observations:
// 1. The binding indices here are determined by ECGTexture while textures contexts
//      are derived by EEfResTextures - they do NOT exactly match!  
//      It seems that the order can be switched without side effect - TEST!
//------------------------------------------------------------------------------
bool CHWShader_D3D::mfSetTextures(const std::vector<SCGTexture>& Textures, EHWShaderClass eSHClass)
{
    DETAILED_PROFILE_MARKER("mfSetTextures");
    FUNCTION_PROFILER_RENDER_FLAT

        const uint32 nSize = Textures.size();
    if (!nSize)
    {
        return true;
    }
    CD3D9Renderer* __restrict rd = gcpRendD3D;
    CShaderResources* __restrict pSR = rd->m_RP.m_pShaderResources;

    uint32 i;
    const SCGTexture* pTexBind = &Textures[0];
    for (i = 0; i < nSize; i++)
    {
        int nTUnit = pTexBind->m_BindingSlot;

        // Get appropriate view for the texture to bind (can be SRGB, MipLevels etc.)
        SResourceView::KeyType nResViewKey = SResourceView::DefaultView;
        if (pTexBind->m_bSRGBLookup)
        {
            nResViewKey = SResourceView::DefaultViewSRGB;
        }

        // This case handles texture names parsed from the shader that are not contextually predefined 
        // [Shader System] - can be used for the per material stage once the texture Id is not 
        // contextual anymore, hence not hard coded.
        if (pTexBind->m_eCGTextureType == ECGT_Unknown)
        {
            CTexture* pTexture = pTexBind->GetTexture();
            if (pTexture)
            {
                pTexture->ApplyTexture(nTUnit, eSHClass, nResViewKey);
            }

            pTexBind++;
            continue;
        }

        CTexture* pT = nullptr;
        switch (pTexBind->m_eCGTextureType)
        {
        case ECGT_MatSlot_Diffuse:
        case ECGT_MatSlot_Normals:
        case ECGT_MatSlot_Specular:
        case ECGT_MatSlot_Height:
        case ECGT_MatSlot_SubSurface:
        case ECGT_MatSlot_Smoothness:
        case ECGT_MatSlot_DecalOverlay:
        case ECGT_MatSlot_Custom:
        case ECGT_MatSlot_CustomSecondary:
        case ECGT_MatSlot_Env:
        case ECGT_MatSlot_Opacity:
        case ECGT_MatSlot_Detail:
        case ECGT_MatSlot_Emittance:
        case ECGT_MatSlot_Occlusion:
        case ECGT_MatSlot_Specular2:
        {
            uint32          texSlot = EEfResTextures(pTexBind->m_eCGTextureType - 1);
            SEfResTexture*  pTextureRes = pSR ? pSR->GetTextureResource(texSlot) : nullptr;
            CTexture*       pTex = pTextureRes ? 
                pTextureRes->m_Sampler.m_pTex : 
                TextureHelpers::LookupTexDefault((EEfResTextures)texSlot);

            if (pTex)
            {
                pTex->ApplyTexture(texSlot, eSHClass, nResViewKey);
            }
        }
        break;

        case ECGT_Shadow0:
        case ECGT_Shadow1:
        case ECGT_Shadow2:
        case ECGT_Shadow3:
        case ECGT_Shadow4:
        case ECGT_Shadow5:
        case ECGT_Shadow6:
        case ECGT_Shadow7:
        {
            const int nShadowMapNum = pTexBind->m_eCGTextureType - ECGT_Shadow0;
            const int nCustomID = rd->m_RP.m_ShadowCustomTexBind[nShadowMapNum];
            if (nCustomID < 0)
            {
                break;
            }

            CTexture* tex = nCustomID ? CTexture::GetByID(nCustomID) : CTexture::s_ptexRT_ShadowStub;
            tex->ApplyTexture(nTUnit, eSHClass, nResViewKey);
        }
        break;

        case ECGT_ShadowMask:
        {
            CTexture* tex = CTexture::IsTextureExist(CTexture::s_ptexShadowMask)
                ? CTexture::s_ptexShadowMask
                : CTextureManager::Instance()->GetBlackTexture();
            tex->ApplyTexture(nTUnit, eSHClass, nResViewKey);
        }
        break;

        case ECGT_ZTarget:
        {
            CTexture* tex = CTexture::s_ptexZTarget;
            tex->ApplyTexture(nTUnit, eSHClass, nResViewKey);
        }
        break;
        case ECGT_ZTargetMS:
        {
            CTexture* tex = CTexture::s_ptexZTarget;
            tex->ApplyTexture(nTUnit, eSHClass, SResourceView::DefaultViewMS);
        }
        break;
        case ECGT_ZTargetScaled:
        {
            CTexture* tex = CTexture::s_ptexZTargetScaled;
            tex->ApplyTexture(nTUnit, eSHClass, nResViewKey);
        }
        break;
        case ECGT_ShadowMaskZTarget:
        {
            // Returns FurZTarget if fur rendering is present in frame, otherwise ZTarget is returned
            CTexture* tex = CTexture::s_ptexZTarget;
            if (FurPasses::GetInstance().IsRenderingFur())
            {
                tex = CTexture::s_ptexFurZTarget;
            }
            tex->ApplyTexture(nTUnit, eSHClass, nResViewKey);
        }
        break;

        case ECGT_SceneNormalsBent:
        {
            CTexture* tex = CTexture::s_ptexSceneNormalsBent;
            tex->ApplyTexture(nTUnit, eSHClass, nResViewKey);
        }
        break;

        case ECGT_SceneNormals:
        {
            CTexture* tex = CTexture::s_ptexSceneNormalsMap;
            tex->ApplyTexture(nTUnit, eSHClass, nResViewKey);
        }
        break;
        case ECGT_SceneDiffuse:
        {
            CTexture* tex = CTexture::s_ptexSceneDiffuse;
            tex->ApplyTexture(nTUnit, eSHClass, nResViewKey);
        }
        break;
        case ECGT_SceneSpecular:
        {
            CTexture* tex = CTexture::s_ptexSceneSpecular;
            tex->ApplyTexture(nTUnit, eSHClass, nResViewKey);
        }
        break;
        case ECGT_SceneDiffuseAcc:
        {
            const uint32 nLightsCount = CDeferredShading::Instance().GetLightsCount();
            CTexture* tex = nLightsCount ? CTexture::s_ptexSceneDiffuseAccMap : CTextureManager::Instance()->GetBlackTexture();
            tex->ApplyTexture(nTUnit, eSHClass, nResViewKey);
        }
        break;
        case ECGT_SceneSpecularAcc:
        {
            const uint32 nLightsCount = CDeferredShading::Instance().GetLightsCount();
            CTexture* tex = nLightsCount ? CTexture::s_ptexSceneSpecularAccMap : CTextureManager::Instance()->GetBlackTexture();
            tex->ApplyTexture(nTUnit, eSHClass, nResViewKey);
        }
        break;

        case ECGT_SceneNormalsMapMS:
        {
            CTexture* tex = CTexture::s_ptexSceneNormalsMapMS;
            tex->ApplyTexture(nTUnit, eSHClass, SResourceView::DefaultViewMS);
        }
        break;
        case ECGT_SceneDiffuseAccMS:
        {
            const uint32 nLightsCount = CDeferredShading::Instance().GetLightsCount();
            CTexture* tex = nLightsCount ? CTexture::s_ptexSceneDiffuseAccMapMS : CTextureManager::Instance()->GetBlackTexture();
            tex->ApplyTexture(nTUnit, eSHClass, SResourceView::DefaultViewMS);
        }
        break;
        case ECGT_SceneSpecularAccMS:
        {
            const uint32 nLightsCount = CDeferredShading::Instance().GetLightsCount();
            CTexture* tex = nLightsCount ? CTexture::s_ptexSceneSpecularAccMapMS : CTextureManager::Instance()->GetBlackTexture();
            tex->ApplyTexture(nTUnit, eSHClass, SResourceView::DefaultViewMS);
        }
        break;

        case ECGT_VolumetricClipVolumeStencil:
        {
            CTexture* tex = CTexture::s_ptexVolumetricClipVolumeStencil;
            tex->ApplyTexture(nTUnit, eSHClass, nResViewKey);
        }
        break;

        case ECGT_VolumetricFog:
        {
            CTexture* tex = CTexture::s_ptexVolumetricFog;
            tex->ApplyTexture(nTUnit, eSHClass, nResViewKey);
        }
        break;

        case ECGT_VolumetricFogGlobalEnvProbe0:
        {
            CTexture* tex = rd->GetVolumetricFog().GetGlobalEnvProbeTex0();
            tex = (tex != NULL) ? tex : CTextureManager::Instance()->GetNoTextureCM();
            tex->ApplyTexture(nTUnit, eSHClass, nResViewKey);
        }
        break;

        case ECGT_VolumetricFogGlobalEnvProbe1:
        {
            CTexture* tex = rd->GetVolumetricFog().GetGlobalEnvProbeTex1();
            tex = (tex != NULL) ? tex : CTextureManager::Instance()->GetNoTextureCM();
            tex->ApplyTexture(nTUnit, eSHClass, nResViewKey);
        }
        break;

        default:
            assert(0);
            break;
        }

        pTexBind++;
    }

    return true;
}

bool CHWShader_D3D::mfSetSamplers_Old(const std::vector<STexSamplerRT>& Samplers, EHWShaderClass eSHClass)
{
    DETAILED_PROFILE_MARKER("mfSetSamplers_Old");
    FUNCTION_PROFILER_RENDER_FLAT

    const uint32 nSize = Samplers.size();
    if (!nSize)
    {
        return true;
    }

    CD3D9Renderer* __restrict       rd = gcpRendD3D;
    CShaderResources* __restrict    pSR = rd->m_RP.m_pShaderResources;

    uint32 i;
    const STexSamplerRT* pSamp = &Samplers[0];

    // Loop counter increments moved to resolve an issue where the compiler introduced
    // load hit stores by storing the counters as the last instruction in the loop then
    // immediately reloading and incrementing them after the branch back to the top
    for (i = 0; i < nSize; )
    {
        CTexture*   tx = pSamp->m_pTex;
        assert(tx);
        if (!tx)
        {
            ++pSamp;
            ++i;
            continue;
        }

        int nTexMaterialSlot = EFTT_UNKNOWN;
        const STexSamplerRT* pSM = pSamp++;
        int nSUnit = pSM->m_nSamplerSlot;
        int nTUnit = pSM->m_nTextureSlot;
        assert(nTUnit >= 0);
        int nTState = pSM->m_nTexState;
        const ETEX_Type smpTexType = (ETEX_Type)pSM->m_eTexType;

        ++i;

        if (tx >= &(*CTexture::s_ShaderTemplates)[0] && tx <= &(*CTexture::s_ShaderTemplates)[EFTT_MAX - 1])
        {
            nTexMaterialSlot = (int)(tx - &(*CTexture::s_ShaderTemplates)[0]);

            SEfResTexture*      pTextureRes = pSR ? pSR->GetTextureResource(nTexMaterialSlot) : nullptr;
            if (!pTextureRes)
            {
                tx = TextureHelpers::LookupTexDefault((EEfResTextures)nTexMaterialSlot);
            }
#if defined(CONSOLE_CONST_CVAR_MODE)
            else if constexpr (CD3D9Renderer::CV_r_TexturesDebugBandwidth > 0)
#else
            else if (rd->CV_r_TexturesDebugBandwidth > 0)
#endif
            {
                tx = CTextureManager::Instance()->GetDefaultTexture("Gray");
            }
            else
            {
                pSM = &pTextureRes->m_Sampler;
                tx = pSM->m_pTex;

                if (nTState < 0 || !CTexture::s_TexStates[nTState].m_bActive)
                {
                    nTState = pSM->m_nTexState; // Use material texture state
                }
            }
        }

        IF(pSM && pSM->m_pAnimInfo, 0)
        {
            STexSamplerRT* pRT = (STexSamplerRT*)pSM;
            pRT->Update();
            tx = pRT->m_pTex;
        }

        IF(!tx || tx->GetCustomID() <= 0 && smpTexType != tx->GetTexType(), 0)
        {
#if !defined(_RELEASE)
            string matName = "unknown";

            if (pSR->m_szMaterialName)
            {
                matName = pSR->m_szMaterialName;
            }

            CRenderObject* pObj = gcpRendD3D->m_RP.m_pCurObject;

            if (pObj && pObj->m_pCurrMaterial)
            {
                matName.Format("%s/%s", pObj->m_pCurrMaterial->GetName(), pSR->m_szMaterialName ? pSR->m_szMaterialName : "unknown");
            }

            if (tx && !tx->IsNoTexture())
            {
                LogSamplerTextureMismatch(tx, pSamp - 1, eSHClass, pSR && (nTexMaterialSlot >= 0 && nTexMaterialSlot < EFTT_UNKNOWN) ? matName : "none");
            }
#endif
            tx = CTexture::s_pTexNULL;
        }

        {
            int nCustomID = tx->GetCustomID();
            if (nCustomID <= 0)
            {
                if (nTState >= 0 && nTState < CTexture::s_TexStates.size())
                {
                    if (tx->UseDecalBorderCol())
                    {
                        STexState TS = CTexture::s_TexStates[nTState];
                        //TS.SetFilterMode(...); // already set up
                        TS.SetClampMode(TADDR_CLAMP, TADDR_CLAMP, TADDR_CLAMP);
                        nTState = CTexture::GetTexState(TS);
                    }

                    if (CRenderer::CV_r_texNoAnisoAlphaTest && (rd->m_RP.m_FlagsShader_RT & g_HWSR_MaskBit[HWSR_ALPHATEST]))
                    {
                    
                        STexState TS = CTexture::s_TexStates[nTState];
                        if (TS.m_nAnisotropy > 1)
                        {
                            TS.m_nAnisotropy = 1;
                            TS.SetFilterMode(FILTER_TRILINEAR);
                            nTState = CTexture::GetTexState(TS);
                        }
                    }
                }

                tx->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit, SResourceView::DefaultView, eSHClass);
            }
            else 
            {
                // Allow render elements to set their own samplers
                IRenderElement* pRE = rd->m_RP.m_pRE;
                if (pRE && pRE->mfSetSampler(nCustomID, nTUnit, nTState, nTexMaterialSlot, nSUnit))
                {
                    continue;
                }

                switch (nCustomID)
                {
                case TO_FROMRE0:
                case TO_FROMRE1:
                {
                    if (rd->m_RP.m_pRE)
                    {
                        nCustomID = rd->m_RP.m_pRE->GetCustomTexBind(nCustomID - TO_FROMRE0);
                    }
                    else
                    {
                        nCustomID = rd->m_RP.m_RECustomTexBind[nCustomID - TO_FROMRE0];
                    }
                    if (nCustomID < 0)
                    {
                        break;
                    }

                    CTexture* pTex = CTexture::GetByID(nCustomID);
                    pTex->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit);
                }
                break;

                case TO_FROMRE0_FROM_CONTAINER:
                case TO_FROMRE1_FROM_CONTAINER:
                {
                    // take render element from vertex container render mesh if available
                    IRenderElement* _pRE = sGetContainerRE0(rd->m_RP.m_pRE);
                    if (_pRE)
                    {
                        nCustomID = _pRE->GetCustomTexBind(nCustomID - TO_FROMRE0_FROM_CONTAINER);
                    }
                    else
                    {
                        nCustomID = rd->m_RP.m_RECustomTexBind[nCustomID - TO_FROMRE0_FROM_CONTAINER];
                    }
                    if (nCustomID < 0)
                    {
                        break;
                    }
                    CTexture::ApplyForID(nTUnit, nCustomID, nTState, nSUnit);
                }
                break;

                case TO_ZTARGET_MS:
                {
                    CTexture* pTex = CTexture::s_ptexZTarget;
                    assert(pTex);
                    if (pTex)
                    {
                        pTex->Apply(nTUnit, nTState, nTexMaterialSlot, nTUnit, SResourceView::DefaultViewMS);
                    }
                }
                break;

                case TO_SCENE_NORMALMAP_MS:
                case TO_SCENE_NORMALMAP:
                {
                    CTexture* pTex = CTexture::s_ptexSceneNormalsMap;
                    if (sCanSet(pSM, pTex))
                    {
                        if (nCustomID != TO_SCENE_NORMALMAP_MS)
                        {
                            pTex->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit, SResourceView::DefaultView);
                        }
                        else
                        {
                            pTex->Apply(nTUnit, nTState, nTexMaterialSlot, nTUnit, SResourceView::DefaultViewMS);
                        }
                    }
                }
                break;

                case TO_SHADOWID0:
                case TO_SHADOWID1:
                case TO_SHADOWID2:
                case TO_SHADOWID3:
                case TO_SHADOWID4:
                case TO_SHADOWID5:
                case TO_SHADOWID6:
                case TO_SHADOWID7:
                {
                    const int nShadowMapNum = nCustomID - TO_SHADOWID0;
                    nCustomID = rd->m_RP.m_ShadowCustomTexBind[nShadowMapNum];

                    if (nCustomID < 0)
                    {
                        break;
                    }

                    if (nTState >= 0 && nTState < CTexture::s_TexStates.size())
                    {
                        //force  MinFilter = Linear; MagFilter = Linear; for HW_PCF_FILTERING
                        STexState TS = CTexture::s_TexStates[nTState];
                        TS.m_pDeviceState = NULL;
                        TS.SetClampMode(TADDR_CLAMP, TADDR_CLAMP, TADDR_CLAMP);

                        const bool bComparisonSampling = rd->m_RP.m_ShadowCustomComparisonSampling[nShadowMapNum];
                        if (bComparisonSampling)
                        {
                            TS.SetFilterMode(FILTER_LINEAR);
                            TS.SetComparisonFilter(true);
                        }
                        else
                        {
                            TS.SetFilterMode(FILTER_POINT);
                            TS.SetComparisonFilter(false);
                        }

                        nTState = CTexture::GetTexState(TS);
                    }

                    CTexture* tex = CTexture::GetByID(nCustomID);
                    tex->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit, SResourceView::DefaultView, eSHClass);
                }
                break;


                case TO_SHADOWMASK:
                {
                    CTexture* pTex = CTexture::IsTextureExist(CTexture::s_ptexShadowMask)
                        ? CTexture::s_ptexShadowMask
                        : CTextureManager::Instance()->GetBlackTexture();

                    pTex->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit);
                }
                break;

                case TO_SCENE_DIFFUSE_ACC_MS:
                case TO_SCENE_DIFFUSE_ACC:
                {
                    const uint32 nLightsCount = CDeferredShading::Instance().GetLightsCount();
                    CTexture* pTex = nLightsCount ? CTexture::s_ptexCurrentSceneDiffuseAccMap : CTextureManager::Instance()->GetBlackTexture();
                    if (sCanSet(pSM, pTex))
                    {
                        if (!(nLightsCount && nCustomID == TO_SCENE_DIFFUSE_ACC_MS))
                        {
                            pTex->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit, SResourceView::DefaultView);
                        }
                        else
                        {
                            pTex->Apply(nTUnit, nTState, nTexMaterialSlot, nTUnit, SResourceView::DefaultViewMS);
                        }
                    }
                }
                break;

                case TO_SCENE_SPECULAR_ACC_MS:
                case TO_SCENE_SPECULAR_ACC:
                {
                    const uint32 nLightsCount = CDeferredShading::Instance().GetLightsCount();
                    CTexture* pTex = nLightsCount ? CTexture::s_ptexSceneSpecularAccMap : CTextureManager::Instance()->GetBlackTexture();
                    if (sCanSet(pSM, pTex))
                    {
                        if (!(nLightsCount && nCustomID == TO_SCENE_SPECULAR_ACC_MS))
                        {
                            pTex->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit, SResourceView::DefaultView);
                        }
                        else
                        {
                            pTex->Apply(nTUnit, nTState, nTexMaterialSlot, nTUnit, SResourceView::DefaultViewMS);
                        }
                    }
                }
                break;

                case TO_SCENE_TARGET:
                {
                    CTexture* tex = CTexture::s_ptexCurrSceneTarget;
                    if (!tex)
                    {
                        tex = CTextureManager::Instance()->GetWhiteTexture();
                    }

                    tex->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit);
                }
                break;

                case TO_DOWNSCALED_ZTARGET_FOR_AO:
                {
                    assert(CTexture::s_ptexZTargetScaled);
                    if (CTexture::s_ptexZTargetScaled)
                    {
                        CTexture::s_ptexZTargetScaled->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit);
                    }
                }
                break;

                case TO_QUARTER_ZTARGET_FOR_AO:
                {
                    assert(CTexture::s_ptexZTargetScaled2);
                    if (CTexture::s_ptexZTargetScaled2)
                    {
                        CTexture::s_ptexZTargetScaled2->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit);
                    }
                }
                break;

                case TO_FROMOBJ:
                {
                    CTexture* pTex = CTextureManager::Instance()->GetBlackTexture();
                    if (rd->m_RP.m_pCurObject)
                    {
                        nCustomID = rd->m_RP.m_pCurObject->m_nTextureID;
                        if (nCustomID > 0)
                        {
                            pTex = CTexture::GetByID(nCustomID);
                        }
                    }
                    pTex->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit);
                }
                break;

                case TO_FROMOBJ_CM:
                {
                    SEfResTexture*  overloadTexture = nullptr;
                    CTexture*       pTex = CTextureManager::Instance()->GetNoTextureCM();
                    if (rd->m_RP.m_pCurObject)
                    {
                        nCustomID = rd->m_RP.m_pCurObject->m_nTextureID;
                        if (nCustomID > 0)
                        {
                            pTex = CTexture::GetByID(nCustomID);
                        }
                        else if ((nTUnit < EFTT_MAX) && pSR && (overloadTexture = pSR->GetTextureResource(EFTT_ENV)))
                        {
                            // Perhaps user wanted a specific cubemap instead?
                            // This should still be allowed even if the sampler is "TO_FROMOBJ_CM" as the
                            // end user can still select specific cubemaps from the material editor.
                            CTexture* tex = overloadTexture->m_Sampler.m_pTex;
                            if (tex)
                            {
                                tex->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit);
                            }
                            nCustomID = -1;
                        }
                        else if (nCustomID == 0)
                        {
                            pTex = CTextureManager::Instance()->GetNoTextureCM();
                        }
                    }
                    if (nCustomID >= 0)
                    {
                        pTex->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit);
                    }
                }
                break;

                case TO_RT_2D:
                {
                    SHRenderTarget* pRT = pSM->m_pTarget ? pSM->m_pTarget : pSamp->m_pTarget;
                    SEnvTexture* pEnvTex = pRT->GetEnv2D();
                    //assert(pEnvTex->m_pTex);
                    if (pEnvTex && pEnvTex->m_pTex)
                    {
                        pEnvTex->m_pTex->Apply(nTUnit, nTState);
                    }
                    else
                    {
                        CTextureManager::Instance()->GetWhiteTexture()->Apply(nTUnit, nTState);
                    }
                }
                break;

                case TO_WATEROCEANMAP:
                    CTexture::s_ptexWaterOcean->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit, SResourceView::DefaultView, eSHClass);
                    break;

                case TO_WATERVOLUMEREFLMAP:
                {
#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
                    const uint32 nCurrWaterVolID = gRenDev->GetCameraFrameID() % 2;
#else
                    const uint32 nCurrWaterVolID = gRenDev->GetFrameID(false) % 2;
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
                    CTexture* pTex = CTexture::s_ptexWaterVolumeRefl[nCurrWaterVolID] ? CTexture::s_ptexWaterVolumeRefl[nCurrWaterVolID] : CTextureManager::Instance()->GetBlackTexture();
                    pTex->Apply(nTUnit, CTexture::GetTexState(STexState(FILTER_ANISO16X, true)), nTexMaterialSlot, nSUnit, SResourceView::DefaultView, eSHClass);
                }
                break;

                case TO_WATERVOLUMEREFLMAPPREV:
                {
#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
                    const uint32 nPrevWaterVolID = (gRenDev->GetCameraFrameID() + 1) % 2;
#else
                    const uint32 nPrevWaterVolID = (gRenDev->GetFrameID(false) + 1) % 2;
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED

                    CTexture* pTex = CTexture::s_ptexWaterVolumeRefl[nPrevWaterVolID] ? CTexture::s_ptexWaterVolumeRefl[nPrevWaterVolID] : CTextureManager::Instance()->GetBlackTexture();
                    pTex->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit, SResourceView::DefaultView, eSHClass);
                }
                break;

                case TO_WATERVOLUMECAUSTICSMAP:
                {
#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
                    const uint32 nCurrWaterVolID = gRenDev->GetCameraFrameID() % 2;
#else
                    const uint32 nCurrWaterVolID = gRenDev->GetFrameID(false) % 2;
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
                    CTexture* pTex = CTexture::s_ptexWaterCaustics[nCurrWaterVolID];
                    pTex->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit, SResourceView::DefaultView, eSHClass);
                }
                break;

                case TO_WATERVOLUMECAUSTICSMAPTEMP:
                {
#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
                    const uint32 nPrevWaterVolID = (gRenDev->GetCameraFrameID() + 1) % 2;
#else
                    const uint32 nPrevWaterVolID = (gRenDev->GetFrameID(false) + 1) % 2;
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
                    CTexture* pTex = CTexture::s_ptexWaterCaustics[nPrevWaterVolID];
                    pTex->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit, SResourceView::DefaultView, eSHClass);
                }
                break;

                case TO_WATERVOLUMEMAP:
                {
                    if (CTexture::s_ptexWaterVolumeDDN)
                    {
                        CEffectParam* pParam = PostEffectMgr()->GetByName("WaterVolume_Amount");
                        assert(pParam && "Parameter doesn't exist");

                        // Activate puddle generation
                        if (pParam)
                        {
                            pParam->SetParam(1.0f);
                        }

                        CTexture::s_ptexWaterVolumeDDN->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit, SResourceView::DefaultView, eSHClass);
                    }
                    else
                    {
                        CTextureManager::Instance()->GetDefaultTexture("FlatBump")->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit, SResourceView::DefaultView, eSHClass);
                    }
                }
                break;

                case TO_WATERRIPPLESMAP:
                {
                    if (CTexture::s_ptexWaterRipplesDDN)
                    {
                        CTexture::s_ptexWaterRipplesDDN->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit, SResourceView::DefaultView, eSHClass);
                    }
                    else
                    {
                        CTextureManager::Instance()->GetWhiteTexture()->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit, SResourceView::DefaultView, eSHClass);
                    }
                }
                break;

                case TO_BACKBUFFERSCALED_D2:
                case TO_BACKBUFFERSCALED_D4:
                case TO_BACKBUFFERSCALED_D8:
                {
                    const uint32 nTargetID = nCustomID - TO_BACKBUFFERSCALED_D2;
                    CTexture* pTex = CTexture::s_ptexBackBufferScaled[nTargetID] ? CTexture::s_ptexBackBufferScaled[nTargetID] : CTextureManager::Instance()->GetBlackTexture();
                    pTex->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit, SResourceView::DefaultView, eSHClass);
                }
                break;

                case TO_CLOUDS_LM:
                {
                    const bool setupCloudShadows = rd->m_bShadowsEnabled && rd->m_bCloudShadowsEnabled;
                    if (setupCloudShadows)
                    {
                        // cloud shadow map
                        CTexture* pCloudShadowTex(rd->GetCloudShadowTextureId() > 0 ? CTexture::GetByID(rd->GetCloudShadowTextureId()) : CTextureManager::Instance()->GetWhiteTexture());
                        assert(pCloudShadowTex);

                        STexState pTexStateLinearClamp;
                        pTexStateLinearClamp.SetFilterMode(FILTER_LINEAR);
                        pTexStateLinearClamp.SetClampMode(false, false, false);
                        int nTexStateLinearClampID = CTexture::GetTexState(pTexStateLinearClamp);

                        pCloudShadowTex->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit, SResourceView::DefaultView, eSHClass);
                    }
                    else
                    {
                        CTextureManager::Instance()->GetWhiteTexture()->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit, SResourceView::DefaultView, eSHClass);
                    }

                    break;
                }

                case TO_MIPCOLORS_DIFFUSE:
                    CTextureManager::Instance()->GetWhiteTexture()->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit);
                    break;

                case TO_BACKBUFFERMAP:
                {
                    CTexture* pBackBufferTex = CTexture::s_ptexBackBuffer;
                    if (pBackBufferTex)
                    {
                        pBackBufferTex->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit);
                    }
                }
                break;

                case TO_HDR_MEASURED_LUMINANCE:
                {
                    CTexture::s_ptexHDRMeasuredLuminance[gRenDev->RT_GetCurrGpuID()]->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit);
                }
                break;

                case TO_VOLOBJ_DENSITY:
                case TO_VOLOBJ_SHADOW:
                {
                    bool texBound(false);
                    IRenderElement* _pRE(rd->m_RP.m_pRE);
                    if (_pRE && _pRE->mfGetType() == eDATA_VolumeObject)
                    {
                        CREVolumeObject* pVolObj((CREVolumeObject*)_pRE);
                        int texId(0);
                        if (pVolObj)
                        {
                            switch (nCustomID)
                            {
                            case TO_VOLOBJ_DENSITY:
                                if (pVolObj->m_pDensVol)
                                {
                                    texId = pVolObj->m_pDensVol->GetTexID();
                                }
                                break;
                            case TO_VOLOBJ_SHADOW:
                                if (pVolObj->m_pShadVol)
                                {
                                    texId = pVolObj->m_pShadVol->GetTexID();
                                }
                                break;
                            default:
                                assert(0);
                                break;
                            }
                        }
                        CTexture* pTex(texId > 0 ? CTexture::GetByID(texId) : 0);
                        if (pTex)
                        {
                            pTex->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit);
                            texBound = true;
                        }
                    }
                    if (!texBound)
                    {
                        CTextureManager::Instance()->GetWhiteTexture()->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit);
                    }
                    break;
                }

                case TO_COLORCHART:
                {
                    CColorGradingControllerD3D* pCtrl = gcpRendD3D->m_pColorGradingControllerD3D;
                    if (pCtrl)
                    {
                        CTexture* pTex = pCtrl->GetColorChart();
                        if (pTex)
                        {
                            const static int texStateID = CTexture::GetTexState(STexState(FILTER_LINEAR, true));
                            pTex->Apply(nTUnit, texStateID);
                            break;
                        }
                    }
                    CRenderer::CV_r_colorgrading_charts = 0;
                    CTextureManager::Instance()->GetWhiteTexture()->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit);
                    break;
                }

                case TO_SKYDOME_MIE:
                case TO_SKYDOME_RAYLEIGH:
                {
                    IRenderElement* _pRE = rd->m_RP.m_pRE;
                    if (_pRE && _pRE->mfGetType() == eDATA_HDRSky)
                    {
                        CTexture* pTex = nCustomID == TO_SKYDOME_MIE ? ((CREHDRSky*)_pRE)->m_pSkyDomeTextureMie : ((CREHDRSky*)_pRE)->m_pSkyDomeTextureRayleigh;
                        if (pTex)
                        {
                            pTex->Apply(nTUnit, -1, nTexMaterialSlot, nSUnit);
                            break;
                        }
                    }
                    CTextureManager::Instance()->GetBlackTexture()->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit);
                    break;
                }

                case TO_SKYDOME_MOON:
                {
                    IRenderElement* _pRE = rd->m_RP.m_pRE;
                    if (_pRE && _pRE->mfGetType() == eDATA_HDRSky)
                    {
                        CREHDRSky* pHDRSky = (CREHDRSky*)_pRE;
                        CTexture* pMoonTex(pHDRSky->m_moonTexId > 0 ? CTexture::GetByID(pHDRSky->m_moonTexId) : 0);
                        if (pMoonTex)
                        {
                            const static int texStateID = CTexture::GetTexState(STexState(FILTER_BILINEAR, TADDR_BORDER, TADDR_BORDER, TADDR_BORDER, 0));
                            pMoonTex->Apply(nTUnit, texStateID, nTexMaterialSlot, nSUnit);
                            break;
                        }
                    }
                    CTextureManager::Instance()->GetBlackTexture()->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit);
                    break;
                }

                case TO_VOLFOGSHADOW_BUF:
                {
#if defined(VOLUMETRIC_FOG_SHADOWS)
                    const bool enabled = gRenDev->m_bVolFogShadowsEnabled;
                    assert(enabled);
                    CTexture* pTex = enabled ? CTexture::s_ptexVolFogShadowBuf[0] : CTextureManager::Instance()->GetWhiteTexture();
                    pTex->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit);
                    break;
#else
                    assert(0);
                    break;
#endif
                }

                case TO_DEFAULT_ENVIRONMENT_PROBE:
                {
                    // The environment probe entity render object requires that a texture that is bound to a sampler via shader declaration be overloaded in code.
                    // This is not supported by default, and changing the behavior generically breaks other systems that depend on this behavior.
                    // For the default environment probe texture declaration, we need to check if someone has tried to bind a new texture to the shader
                    // (which would happen either via C++ code or by overloading the environment map slot on the envcube.mtl material)
                    SEfResTexture*      pTextureRes = pSR ? pSR->GetTextureResource(EFTT_ENV) : nullptr;

                    if ((nSUnit == nTUnit) && (nTUnit < EFTT_MAX) && 
                        pTextureRes && pTextureRes->m_Sampler.m_pTex &&
                        pTextureRes->m_Sampler.m_pTex->GetDevTexture())
                    {
                        SEfResTexture*  overloadTexture = pTextureRes;
                        overloadTexture->m_Sampler.m_pTex->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit);
                    }
                    else 
                    {
                        CTexture* defaultEnvironmentProbe = CTextureManager::Instance()->GetDefaultTexture("DefaultProbeCM");
                        if (defaultEnvironmentProbe)
                        {
                            defaultEnvironmentProbe->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit);
                        }
                    }
                }
                break;

                default:
                {
#if defined(FEATURE_SVO_GI)
                    if (CSvoRenderer::SetSamplers(nCustomID, eSHClass, nTUnit, nTState, nTexMaterialSlot, nSUnit))
                    {
                        break;
                    }
#endif
                    tx->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit);
                }
                break;
                }
            }
        }
    }

    return true;
}

//------------------------------------------------------------------------------
// Going over the samplers and making sure that dependent slots exist and
// represent the same texture (normals and smoothness/
// [Shader System] - TO DO: 
// 1. Seems like dependency on second smoothness is missing
// 2. Move this to be data driven based on flagged slots
//------------------------------------------------------------------------------
bool CHWShader_D3D::mfUpdateSamplers(CShader* shader)
{
    DETAILED_PROFILE_MARKER("mfUpdateSamplers");
    FUNCTION_PROFILER_RENDER_FLAT
    if (!m_pCurInst)
    {
        return false;
    }

    SHWSInstance*       pInst = m_pCurInst;
    CShaderResources*   pSRes = gRenDev->m_RP.m_pShaderResources;
    if (!pInst->m_pSamplers.size() || !pSRes)
    {
        return true;
    }

    CD3D9Renderer*                      rd = gcpRendD3D;
    SRenderPipeline& RESTRICT_REFERENCE rRP = rd->m_RP;
    SThreadInfo& RESTRICT_REFERENCE     rTI = rRP.m_TI[rRP.m_nProcessThreadID];
    STexSamplerRT*                      pSamp = &pInst->m_pSamplers[0];

    const uint32                        samplerCount = pInst->m_pSamplers.size();
    bool                                bDiffuseSlotUpdated = false;

    if (pSRes)
    {
        AZ::u32                             updateCount = 0;
        AZStd::map<uint16, SEfResTexture*>  UpdatedTMap;

        for (AZ::u32 i = 0; i < samplerCount; i++, pSamp++)
        {
            CTexture* tx = pSamp->m_pTex;
            if (!tx)
            {
                continue;
            }

            //  [Shader System TO DO] - replace with proper data driven code reflected from the shaders 
            if (tx >= &(*CTexture::s_ShaderTemplates)[0] && tx <= &(*CTexture::s_ShaderTemplates)[EFTT_MAX - 1])
            {
                int             nSlot = (int)(tx - &(*CTexture::s_ShaderTemplates)[0]);
                int16           replacementSlot = -1;
                SEfResTexture*  pTextureRes = pSRes->GetTextureResource(nSlot);

                if (pTextureRes)
                {
                    // Default inserting and indexing - because we force some slots (Normal, Diffuse..), this 
                    // operation might be done twice (same data). 
                    UpdatedTMap[nSlot] = pTextureRes;   

                    //----------------------------------------------------------
                    // Force adding samplers / textures if they indirectly assumed to be used 
                    //----------------------------------------------------------
                    //  [Shader System TO DO] - replace with data driven reflection (i.e. a texture should be 
                    // able to specify that it is driven by another texture and not hard code it)
                    if (nSlot == EFTT_HEIGHT || nSlot == EFTT_SMOOTHNESS)
                    {
                        replacementSlot = EFTT_NORMALS;
                    }
                    else if (nSlot == EFTT_DIFFUSE)
                    {   // marked as updated - no need to look for replacement 
                        bDiffuseSlotUpdated = true;
                    }

                    // Force uploading the diffuse when the normal already exist (Really?)
                    if ((rTI.m_PersFlags & RBPF_ZPASS) && (nSlot == EFTT_NORMALS) && !bDiffuseSlotUpdated)
                    {
                        replacementSlot = EFTT_DIFFUSE;
                    }
 
                    // Using the following block we can now drive forced slots to be data driven!
                    if (replacementSlot != -1)
                    {
                        SEfResTexture*  replacementTexRes = pSRes->GetTextureResource(replacementSlot);
                        if (replacementTexRes)
                        {
                            UpdatedTMap[replacementSlot] = replacementTexRes;   // inserting and indexing
                        }
/*                      [Shader System] - this is a good warning, however it will repeat every frame and drop fps.
                        else
                        {
                            AZ_Warning("ShadersSystem", false, "CHWShader_D3D::mfUpdateSamplers - [%s] using texture slot %d without existing forced texture %d",
                                pSRes->m_szMaterialName, nSlot, replacementSlot);
                        }
*/
                    }

                }
            }
        }

        // Next run over all existing textures and explore if they have dynamic modulators 
        // which will force shader resource constants update.
        bool    bNeedsConstantUpdate = false;
        for (auto iter = UpdatedTMap.begin(); iter != UpdatedTMap.end(); ++iter)
        {
            SEfResTexture*  pTexture = iter->second;

            pTexture->Update(iter->first);
            bNeedsConstantUpdate |= pTexture->IsNeedTexTransform();
        }

        // Rebuild shader resources - there was at least one transform modulator request
        if (bNeedsConstantUpdate)
        {
            pSRes->Rebuild(shader);
        }
    }

    return true;
}


bool CHWShader_D3D::mfAddGlobalTexture(SCGTexture& Texture)
{
    DETAILED_PROFILE_MARKER("mfAddGlobalTexture");
    uint32 i;
    if (!Texture.m_bGlobal)
    {
        return false;
    }
    for (i = 0; i < s_PF_Textures.size(); i++)
    {
        SCGTexture* pP = &s_PF_Textures[i];
        if (pP->m_pTexture == Texture.m_pTexture)
        {
            break;
        }
    }
    if (i == s_PF_Textures.size())
    {
        s_PF_Textures.push_back(Texture);
        return true;
    }
    return false;
}


bool CHWShader_D3D::mfAddGlobalSampler(STexSamplerRT& Sampler)
{
    DETAILED_PROFILE_MARKER("mfAddGlobalSampler");
    uint32 i;
    if (!Sampler.m_bGlobal)
    {
        return false;
    }
    for (i = 0; i < s_PF_Samplers.size(); i++)
    {
        STexSamplerRT* pP = &s_PF_Samplers[i];
        if (pP->m_pTex == Sampler.m_pTex)
        {
            break;
        }
    }
    if (i == s_PF_Samplers.size())
    {
        s_PF_Samplers.push_back(Sampler);
        assert(s_PF_Samplers.size() <= MAX_PF_SAMPLERS);
        return true;
    }
    return false;
}


Vec4 CHWShader_D3D::GetVolumetricFogParams()
{
    DETAILED_PROFILE_MARKER("GetVolumetricFogParams");
    return sGetVolumetricFogParams(gcpRendD3D);
}

Vec4 CHWShader_D3D::GetVolumetricFogRampParams()
{
    DETAILED_PROFILE_MARKER("GetVolumetricFogRampParams");
    return sGetVolumetricFogRampParams();
}

void CHWShader_D3D::GetFogColorGradientConstants(Vec4& fogColGradColBase, Vec4& fogColGradColDelta)
{
    DETAILED_PROFILE_MARKER("GetFogColorGradientConstants");
    sGetFogColorGradientConstants(fogColGradColBase, fogColGradColDelta);
};

Vec4 CHWShader_D3D::GetFogColorGradientRadial()
{
    DETAILED_PROFILE_MARKER("GetFogColorGradientRadial");
    return sGetFogColorGradientRadial(gcpRendD3D);
}

Vec4 SBending::GetShaderConstants(float realTime) const
{
    Vec4 result(ZERO);
    if ((m_vBending.x * m_vBending.x + m_vBending.y * m_vBending.y) > 0.0f)
    {
        const Vec2& vBending = m_vBending;
        Vec2 vAddBending(ZERO);

        if (m_Waves[0].m_Amp)
        {
            // Fast version of CShaderMan::EvalWaveForm (for bending)
            const SWaveForm2& RESTRICT_REFERENCE wave0 = m_Waves[0];
            const SWaveForm2& RESTRICT_REFERENCE wave1 = m_Waves[1];
            const float* const __restrict pSinTable = gcpRendD3D->m_RP.m_tSinTable;

            int val0 = (int)((realTime * wave0.m_Freq + wave0.m_Phase) * (float)SRenderPipeline::sSinTableCount);
            int val1 = (int)((realTime * wave1.m_Freq + wave1.m_Phase) * (float)SRenderPipeline::sSinTableCount);

            float sinVal0 = pSinTable[val0 & (SRenderPipeline::sSinTableCount - 1)];
            float sinVal1 = pSinTable[val1 & (SRenderPipeline::sSinTableCount - 1)];
            vAddBending.x = wave0.m_Amp * sinVal0 + wave0.m_Level;
            vAddBending.y = wave1.m_Amp * sinVal1 + wave1.m_Level;
        }

        result.x = vAddBending.x * 50.f + vBending.x;
        result.y = vAddBending.y * 50.f + vBending.y;
        result.z = vBending.GetLength() * 2.f;
        result *= m_fMainBendingScale;
        result.w = (vAddBending + vBending).GetLength() * 0.3f;
    }

    return result;
}

void SBending::GetShaderConstantsStatic([[maybe_unused]] float realTime, Vec4* pBendInfo) const
{
    pBendInfo[0] = Vec4(ZERO);
    pBendInfo[0].x = m_Waves[0].m_Freq;
    pBendInfo[0].y = m_Waves[0].m_Amp;
    pBendInfo[0].z = m_Waves[1].m_Freq;
    pBendInfo[0].w = m_Waves[1].m_Amp;
    pBendInfo[1].x = m_vBending.x;
    pBendInfo[1].y = m_vBending.y;
    pBendInfo[1].z = m_vBending.GetLength();
    pBendInfo[1].w = m_fMainBendingScale;
}

#pragma endregion everything in this block is related to shader parameters

int SD3DShader::Release(EHWShaderClass eSHClass, int nSize)
{
    m_nRef--;
    if (m_nRef)
    {
        return m_nRef;
    }
    void* pHandle = m_pHandle;
    delete this;
    if (!pHandle)
    {
        return 0;
    }
    if (eSHClass == eHWSC_Pixel)
    {
        CHWShader_D3D::s_nDevicePSDataSize -= nSize;
    }
    else
    {
        CHWShader_D3D::s_nDeviceVSDataSize -= nSize;
    }

    if (eSHClass == eHWSC_Pixel)
    {
        return ((ID3D11PixelShader*)pHandle)->Release();
    }
    else
    if (eSHClass == eHWSC_Vertex)
    {
        return ((ID3D11VertexShader*)pHandle)->Release();
    }
    else
    if (eSHClass == eHWSC_Geometry)
    {
        return ((ID3D11GeometryShader*)pHandle)->Release();
    }
    else
    if (eSHClass == eHWSC_Hull)
    {
        return ((ID3D11HullShader*)pHandle)->Release();
    }
    else
    if (eSHClass == eHWSC_Compute)
    {
        return ((ID3D11ComputeShader*)pHandle)->Release();
    }
    else
    if (eSHClass == eHWSC_Domain)
    {
        return ((ID3D11DomainShader*)pHandle)->Release();
    }
    else
    {
        assert(0);
        return 0;
    }
}

void CHWShader_D3D::SHWSInstance::Release(SShaderDevCache* pCache, [[maybe_unused]] bool bReleaseData)
{
    if (m_nParams[0] >= 0)
    {
        CGParamManager::FreeParametersGroup(m_nParams[0]);
    }
    if (m_nParams[1] >= 0)
    {
        CGParamManager::FreeParametersGroup(m_nParams[1]);
    }
    if (m_nParams_Inst >= 0)
    {
        CGParamManager::FreeParametersGroup(m_nParams_Inst);
    }

    int nCount = -1;
    if (m_Handle.m_pShader)
    {
        if (m_eClass == eHWSC_Pixel)
        {
            SD3DShader* pPS = m_Handle.m_pShader;
            if (pPS)
            {
                nCount = m_Handle.Release(m_eClass, m_nDataSize);
                if (!nCount && CHWShader::s_pCurPS == pPS)
                {
                    CHWShader::s_pCurPS = NULL;
                }
            }
        }
        else
        if (m_eClass == eHWSC_Vertex)
        {
            SD3DShader* pVS = m_Handle.m_pShader;
            if (pVS)
            {
                nCount = m_Handle.Release(m_eClass, m_nDataSize);
                if (!nCount && CHWShader::s_pCurVS == pVS)
                {
                    CHWShader::s_pCurVS = NULL;
                }
            }
        }
        else
        if (m_eClass == eHWSC_Geometry)
        {
            SD3DShader* pGS = m_Handle.m_pShader;
            if (pGS)
            {
                nCount = m_Handle.Release(m_eClass, m_nDataSize);
                if (!nCount && CHWShader::s_pCurGS == pGS)
                {
                    CHWShader::s_pCurGS = NULL;
                }
            }
        }
        else
        if (m_eClass == eHWSC_Hull)
        {
            SD3DShader* pHS = m_Handle.m_pShader;
            if (pHS)
            {
                nCount = m_Handle.Release(m_eClass, m_nDataSize);
                if (!nCount && CHWShader::s_pCurHS == pHS)
                {
                    CHWShader::s_pCurHS = NULL;
                }
            }
        }
        else
        if (m_eClass == eHWSC_Compute)
        {
            SD3DShader* pCS = m_Handle.m_pShader;
            if (pCS)
            {
                nCount = m_Handle.Release(m_eClass, m_nDataSize);
                if (!nCount && CHWShader::s_pCurCS == pCS)
                {
                    CHWShader::s_pCurCS = NULL;
                }
            }
        }
        else
        if (m_eClass == eHWSC_Domain)
        {
            SD3DShader* pDS = m_Handle.m_pShader;
            if (pDS)
            {
                nCount = m_Handle.Release(m_eClass, m_nDataSize);
                if (!nCount && CHWShader::s_pCurDS == pDS)
                {
                    CHWShader::s_pCurDS = NULL;
                }
            }
        }
    }

    if (m_pShaderData)
    {
        delete[] (char*)m_pShaderData;
        m_pShaderData = NULL;
    }

    if (!nCount && pCache && !pCache->m_DeviceShaders.empty())
    {
        pCache->m_DeviceShaders.erase(m_DeviceObjectID);
    }
    m_Handle.m_pShader = NULL;
}

void CHWShader_D3D::SHWSInstance::GetInstancingAttribInfo(uint8 Attributes[32], int32& nUsedAttr, int& nInstAttrMask)
{
    Attributes[0] = (byte)m_nInstMatrixID;
    for (int32 i = 1; i < nUsedAttr; ++i)
    {
        Attributes[i] = Attributes[0] + i;
    }

    nInstAttrMask = 0x7 << m_nInstMatrixID;
    if (m_nParams_Inst >= 0)
    {
        SCGParamsGroup& Group = CGParamManager::s_Groups[m_nParams_Inst];
        uint32 nSize = Group.nParams;
        for (uint32 j = 0; j < nSize; ++j)
        {
            SCGParam* pr = &Group.pParams[j];
            for (uint32 na = 0; na < (uint32)pr->m_RegisterCount; ++na)
            {
                Attributes[nUsedAttr + na] = pr->m_RegisterOffset + na;
                nInstAttrMask |= 1 << Attributes[nUsedAttr + na];
            }
            nUsedAttr += pr->m_RegisterCount;
        }
    }
}


#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
void CHWShader_D3D::UpdateSamplerEngineTextures()
{
    // get all pixel shaders and update all the sampler textures that point to engine render targets
    CCryNameTSCRC className = CHWShader::mfGetClassName(eHWSC_Pixel);
    SResourceContainer* pRL = CBaseResource::GetResourcesForClass(className);
    if (!pRL)
    {
        return;
    }

    for (auto iter : pRL->m_RMap)
    {
        CHWShader_D3D* shader = static_cast<CHWShader_D3D*>(iter.second);
        if (!shader)
        {
            continue;
        }

        for (CHWShader_D3D::SHWSInstance* shaderInstance : shader->m_Insts)
        {
            if (!shaderInstance || shaderInstance->m_bDeleted || shaderInstance->m_pSamplers.empty())
            {
                continue;
            }

            for (STexSamplerRT& sampler : shaderInstance->m_pSamplers)
            {
                CTexture* texture = sampler.m_pTex;
                if (!texture || !(texture->GetFlags() & FT_USAGE_RENDERTARGET))
                {
                    continue;
                }

                const char* name = texture->GetName();
                if (name == nullptr || name[0] != '$')
                {
                    continue;
                }

                CTexture* engineTexture = CTextureManager::Instance()->GetEngineTexture(CCryNameTSCRC(sampler.m_nCrc));
                if (engineTexture && sampler.m_pTex != engineTexture)
                {
                    sampler.m_pTex->Release();
                    sampler.m_pTex = engineTexture;

                    // don't add a reference to texture we can't Release()
                    if (!(engineTexture->GetFlags() & FT_DONT_RELEASE))
                    {
                        engineTexture->AddRef();
                    }
                }
            }
        }
    }
}
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED

void CHWShader_D3D::ShutDown()
{
    CCryNameTSCRC Name;
    SResourceContainer* pRL;

    AzRHI::ConstantBufferCache::GetInstance().Reset();

    uint32 numResourceLeaks = 0;

    // First make sure all HW and FX shaders are released
    Name = CHWShader::mfGetClassName(eHWSC_Vertex);
    pRL = CBaseResource::GetResourcesForClass(Name);
    if (pRL)
    {
        ResourcesMapItor itor;
        for (itor = pRL->m_RMap.begin(); itor != pRL->m_RMap.end(); itor++)
        {
            CHWShader* vsh = (CHWShader*)itor->second;
            if (vsh)
            {
                ++numResourceLeaks;
            }
        }
        if (!pRL->m_RMap.size())
        {
            pRL->m_RList.clear();
            pRL->m_AvailableIDs.clear();
        }
    }

    Name = CHWShader::mfGetClassName(eHWSC_Pixel);
    pRL = CBaseResource::GetResourcesForClass(Name);
    if (pRL)
    {
        ResourcesMapItor itor;
        for (itor = pRL->m_RMap.begin(); itor != pRL->m_RMap.end(); itor++)
        {
            CHWShader* psh = (CHWShader*)itor->second;
            if (psh)
            {
                ++numResourceLeaks;
            }
        }
        if (!pRL->m_RMap.size())
        {
            pRL->m_RList.clear();
            pRL->m_AvailableIDs.clear();
        }
    }

    Name = CShader::mfGetClassName();
    pRL = CBaseResource::GetResourcesForClass(Name);
    if (pRL)
    {
        ResourcesMapItor itor;
        for (itor = pRL->m_RMap.begin(); itor != pRL->m_RMap.end(); itor++)
        {
            CShader* sh = (CShader*)itor->second;
            if (!sh->m_DerivedShaders)
            {
                numResourceLeaks++;
            }
        }
        if (!pRL->m_RMap.size())
        {
            pRL->m_RList.clear();
            pRL->m_AvailableIDs.clear();
        }
    }

    if (numResourceLeaks > 0)
    {
        iLog->LogWarning("Detected shader resource leaks on shutdown");
    }

    stl::free_container(s_PF_Samplers);

    gRenDev->m_cEF.m_Bin.mfReleaseFXParams();

    while (!m_ShaderCache.empty())
    {
        SShaderCache* pC = m_ShaderCache.begin()->second;
        SAFE_RELEASE(pC);
    }
    m_ShaderCacheList.clear();
    g_SelectedTechs.clear();
#if !defined(_RELEASE)
    s_ErrorsLogged.clear();
#endif
    CGParamManager::Shutdown();
}

CHWShader* CHWShader::mfForName(const char* name, const char* nameSource, uint32 CRC32, const char* szEntryFunc, EHWShaderClass eClass, TArray<uint32>& SHData, FXShaderToken* pTable, uint32 dwType, CShader* pFX, uint64 nMaskGen, uint64 nMaskGenFX)
{
    //  LOADING_TIME_PROFILE_SECTION(iSystem);
    if (!name || !name[0])
    {
        return NULL;
    }

    CHWShader_D3D* pSH = NULL;
    stack_string strName = name;
    CCryNameTSCRC className = mfGetClassName(eClass);
    stack_string AddStr;

    if (nMaskGen)
    {
        strName += AddStr.Format("(GL_%llx)", nMaskGen);
    }

    if (pFX->m_maskGenStatic)
    {
        strName += AddStr.Format("(ST_%llx)", pFX->m_maskGenStatic);
    }

    strName += AddStr.Format( GetShaderLanguageResourceName() );

    CCryNameTSCRC Name = strName.c_str();
    CBaseResource* pBR = CBaseResource::GetResource(className, Name, false);
    if (!pBR)
    {
        pSH = new CHWShader_D3D;
        pSH->m_Name = strName.c_str();
        pSH->m_NameSourceFX = nameSource;
        pSH->Register(className, Name);
        pSH->m_EntryFunc = szEntryFunc;
        pSH->mfFree(CRC32);

        // do we want to use lookup table for faster searching of shaders
        if (CRenderer::CV_r_shadersuseinstancelookuptable)
        {
            pSH->m_bUseLookUpTable = true;
        }
    }
    else
    {
        pSH = (CHWShader_D3D*)pBR;
        pSH->AddRef();
        if (pSH->m_CRC32 == CRC32)
        {
            if (pTable && CRenderer::CV_r_shadersAllowCompilation)
            {
                FXShaderToken* pMap = pTable;
                TArray<uint32>* pData = &SHData;
                pSH->mfGetCacheTokenMap(pMap, pData, pSH->m_nMaskGenShader);
            }
            return pSH;
        }
        pSH->mfFree(CRC32);
        pSH->m_CRC32 = CRC32;
    }

    if (CParserBin::m_bEditable)
    {
        if (pTable)
        {
            pSH->m_TokenTable = *pTable;
        }
        pSH->m_TokenData = SHData;
    }

    pSH->m_dwShaderType = dwType;
    pSH->m_eSHClass = eClass;
    pSH->m_nMaskGenShader = nMaskGen;
    pSH->m_nMaskGenFX = nMaskGenFX;
    pSH->m_maskGenStatic = pFX->m_maskGenStatic;
    pSH->m_CRC32 = CRC32;

    pSH->mfConstructFX(pTable, &SHData);

    return pSH;
}


void CHWShader_D3D::SetTokenFlags(uint32 nToken)
{
    switch (nToken)
    {
    case eT__LT_LIGHTS:
        m_Flags |= HWSG_SUPPORTS_LIGHTING;
        break;
    case eT__LT_0_TYPE:
    case eT__LT_1_TYPE:
    case eT__LT_2_TYPE:
    case eT__LT_3_TYPE:
        m_Flags |= HWSG_SUPPORTS_MULTILIGHTS;
        break;
    case eT__TT_TEXCOORD_MATRIX:
    case eT__TT_TEXCOORD_GEN_OBJECT_LINEAR_DIFFUSE:
    case eT__TT_TEXCOORD_GEN_OBJECT_LINEAR_EMITTANCE:
    case eT__TT_TEXCOORD_GEN_OBJECT_LINEAR_EMITTANCE_MULT:
    case eT__TT_TEXCOORD_GEN_OBJECT_LINEAR_DETAIL:
    case eT__TT_TEXCOORD_GEN_OBJECT_LINEAR_CUSTOM:
        m_Flags |= HWSG_SUPPORTS_MODIF;
        break;
    case eT__VT_TYPE:
        m_Flags |= HWSG_SUPPORTS_VMODIF;
        break;
    case eT__FT_TEXTURE:
        m_Flags |= HWSG_FP_EMULATION;
        break;
    }
}

uint64 CHWShader_D3D::CheckToken(uint32 nToken)
{
    uint64 nMask = 0;
    SShaderGen* pGen = gRenDev->m_cEF.m_pGlobalExt;
    uint32 i;
    for (i = 0; i < pGen->m_BitMask.Num(); i++)
    {
        SShaderGenBit* bit = pGen->m_BitMask[i];
        if (!bit)
        {
            continue;
        }

        if (bit->m_dwToken == nToken)
        {
            nMask |= bit->m_Mask;
            break;
        }
    }
    if (!nMask)
    {
        SetTokenFlags(nToken);
    }

    return nMask;
}

uint64 CHWShader_D3D::CheckIfExpr_r(uint32* pTokens, uint32& nCur, uint32 nSize)
{
    uint64 nMask = 0;

    while (nCur < nSize)
    {
        int nRecurs = 0;
        uint32 nToken = pTokens[nCur++];
        if (nToken == eT_br_rnd_1) // check for '('
        {
            uint32 tmpBuf[64];
            int n = 0;
            int nD = 0;
            while (true)
            {
                nToken = pTokens[nCur];
                if (nToken == eT_br_rnd_1) // check for '('
                {
                    n++;
                }
                else
                if (nToken == eT_br_rnd_2) // check for ')'
                {
                    if (!n)
                    {
                        tmpBuf[nD] = 0;
                        nCur++;
                        break;
                    }
                    n--;
                }
                else
                if (nToken == 0)
                {
                    return nMask;
                }
                tmpBuf[nD++] = nToken;
                nCur++;
            }
            if (nD)
            {
                uint32 nC = 0;
                nMask |= CheckIfExpr_r(tmpBuf, nC, nSize);
            }
        }
        else
        {
            bool bNeg = false;
            if (nToken == eT_excl)
            {
                bNeg = true;
                nToken = pTokens[nCur++];
            }
            nMask |= CheckToken(nToken);
        }
        nToken = pTokens[nCur];
        if (nToken == eT_or)
        {
            nCur++;
            assert (pTokens[nCur] == eT_or);
            if (pTokens[nCur] == eT_or)
            {
                nCur++;
            }
        }
        else
        if (nToken == eT_and)
        {
            nCur++;
            assert (pTokens[nCur] == eT_and);
            if (pTokens[nCur] == eT_and)
            {
                nCur++;
            }
        }
        else
        {
            break;
        }
    }
    return nMask;
}

void CHWShader_D3D::mfConstructFX_Mask_RT([[maybe_unused]] FXShaderToken* Table, TArray<uint32>* pSHData)
{
    assert(gRenDev->m_cEF.m_pGlobalExt);
    m_nMaskAnd_RT = 0;
    m_nMaskOr_RT = 0;
    if (!gRenDev->m_cEF.m_pGlobalExt)
    {
        return;
    }
    SShaderGen* pGen = gRenDev->m_cEF.m_pGlobalExt;
    
    // Construct mask of all mask bits that are usable for this shader from precache entries. This mask is then ANDed 
    // with the property defines used in the shader, in other words, permutation flags prep for shader fetch will 
    // be AND with these masks so that only acceptable / used permutations are being fetched.
    // See Runtime.ext file for the flags bits themselves.
    uint64 allowedBits = 0;
    if (m_dwShaderType)
    {
        for (uint32 i = 0; i < pGen->m_BitMask.Num(); i++)
        {
            SShaderGenBit* bit = pGen->m_BitMask[i];
            if (!bit)
            {
                continue;
            }
            if (bit->m_Flags & SHGF_RUNTIME)
            {
                allowedBits |= bit->m_Mask;
                continue;
            }
            
            uint32 j;
            if (bit->m_PrecacheNames.size())
            {
                for (j = 0; j < bit->m_PrecacheNames.size(); j++)
                {
                    if (m_dwShaderType == bit->m_PrecacheNames[j])
                    {                    
                        AZ_Error("Shaders", ((allowedBits & bit->m_Mask) == 0), "Two shader properties in this shader technique have the same mask which is bad. Look for mask 0x%x in Runtime.ext", bit->m_Mask);
                        allowedBits |= bit->m_Mask;
                        break;
                    }
                }
            }
        }
    }
    else
    {
        allowedBits = 0xFFFFFFFFFFFFFFFF;
    }
    
    AZ_Assert(!pSHData->empty(), "Shader data is empty");
    uint32* pTokens = &(*pSHData)[0];
    uint32 nSize = pSHData->size();
    uint32 nCur = 0;
    while (nCur < nSize)
    {
        uint32 nTok = CParserBin::NextToken(pTokens, nCur, nSize - 1);
        if (!nTok)
        {
            continue;
        }
        if (nTok >= eT_if && nTok <= eT_elif)
        {
            m_nMaskAnd_RT |= CheckIfExpr_r(pTokens, nCur, nSize) & allowedBits;
        }
        else
        {
            SetTokenFlags(nTok);
        }
    }

    mfSetDefaultRT(m_nMaskAnd_RT, m_nMaskOr_RT);
}

void CHWShader_D3D::mfConstructFX(FXShaderToken* Table, TArray<uint32>* pSHData)
{
    if (!_strnicmp(m_EntryFunc.c_str(), "Sync_", 5))
    {
        m_Flags |= HWSG_SYNC;
    }

    if (!pSHData->empty())
    {
        mfConstructFX_Mask_RT(Table, pSHData);
    }
    else
    {
        m_nMaskAnd_RT = -1;
        m_nMaskOr_RT = 0;
    }

    if (Table && CRenderer::CV_r_shadersAllowCompilation)
    {
        FXShaderToken* pMap = Table;
        TArray<uint32>* pData = pSHData;
        mfGetCacheTokenMap(pMap, pData, m_nMaskGenShader); // Store tokens
    }
}

bool CHWShader_D3D::mfPrecache(SShaderCombination& cmb, bool bForce, bool bFallback, bool bCompressedOnly, CShader* pSH, CShaderResources* pRes)
{
    assert(gRenDev->m_pRT->IsRenderThread());

    bool bRes = true;

    if (!CRenderer::CV_r_shadersAllowCompilation && !bForce)
    {
        return bRes;
    }

    uint64 AndRTMask = 0;
    uint64 OrRTMask = 0;
    mfSetDefaultRT(AndRTMask, OrRTMask);
    SShaderCombIdent Ident;
    Ident.m_RTMask = cmb.m_RTMask & AndRTMask | OrRTMask;
    Ident.m_pipelineState.opaque = cmb.m_pipelineState.opaque;
    Ident.m_MDVMask = cmb.m_MDVMask;
    if (m_eSHClass == eHWSC_Pixel)
    {
        Ident.m_MDVMask = CParserBin::m_nPlatform;
    }
    if (m_Flags & HWSG_SUPPORTS_MULTILIGHTS)
    {
        Ident.m_LightMask = 1;
    }
    Ident.m_GLMask = m_nMaskGenShader;
    Ident.m_STMask = m_maskGenStatic;
    uint32 nFlags = HWSF_PRECACHE;
    if (m_eSHClass == eHWSC_Pixel && pRes)
    {
        SHWSInstance* pInst = mfGetInstance(pSH, Ident, HWSF_PRECACHE_INST);
        pInst->m_bFallback = bFallback;
        int nResult = mfCheckActivation(pSH, pInst, HWSF_PRECACHE);
        if (!nResult)
        {
            return bRes;
        }
        mfUpdateSamplers(pSH);
        pInst->m_fLastAccess = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_RealTime;
        Ident.m_MDMask = gRenDev->m_RP.m_FlagsShader_MD & ~HWMD_TEXCOORD_FLAG_MASK;
    }
    if (m_eSHClass == eHWSC_Pixel && gRenDev->m_RP.m_pShaderResources)
    {
        Ident.m_MDMask &= ~HWMD_TEXCOORD_FLAG_MASK;
    }

    if (Ident.m_MDMask || bForce)
    {
        SHWSInstance* pInst = mfGetInstance(pSH, Ident, HWSF_PRECACHE_INST);
        pInst->m_bFallback = bFallback;
        pInst->m_fLastAccess = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_RealTime;
        mfActivate(pSH, nFlags, NULL, NULL, bCompressedOnly);
    }

    return bRes;
}

void CHWShader_D3D::mfReset([[maybe_unused]] uint32 CRC32)
{
    DETAILED_PROFILE_MARKER("mfReset");
    for (uint32 i = 0; i < m_Insts.size(); i++)
    {
        m_pCurInst = m_Insts[i];
        assert(m_pCurInst);
        PREFAST_ASSUME(m_pCurInst);
        if (!m_pCurInst->m_bDeleted)
        {
            m_pCurInst->Release(m_pDevCache);
        }

        delete m_pCurInst;
    }
    m_pCurInst = NULL;
    m_Insts.clear();
    m_LookupMap.clear();

    mfCloseCacheFile();
}

CHWShader_D3D::~CHWShader_D3D()
{
    mfFree(0);
}

void CHWShader_D3D::mfInit()
{
    CGParamManager::Init();
}

ED3DShError CHWShader_D3D::mfFallBack(SHWSInstance*& pInst, int nStatus)
{
    // No fallback for:
    //  - ShadowGen pass
    //  - Z-prepass
    //  - Shadow-pass
    if (CParserBin::m_nPlatform & (SF_D3D11 | SF_ORBIS | SF_DURANGO | SF_JASPER | SF_GL4 | SF_GLES3 | SF_METAL))
    {
        //assert(gRenDev->m_cEF.m_nCombinationsProcess >= 0);
        return ED3DShError_CompilingError;
    }
    if (
        m_eSHClass == eHWSC_Geometry || m_eSHClass == eHWSC_Domain || m_eSHClass == eHWSC_Hull ||
        (gRenDev->m_RP.m_nBatchFilter & FB_Z) || (gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_SHADOWGEN) || gRenDev->m_RP.m_nPassGroupID == EFSLIST_SHADOW_PASS)
    {
        return ED3DShError_CompilingError;
    }
    if (gRenDev->m_RP.m_pShader)
    {
        if (gRenDev->m_RP.m_pShader->GetShaderType() == eST_HDR || gRenDev->m_RP.m_pShader->GetShaderType() == eST_PostProcess || gRenDev->m_RP.m_pShader->GetShaderType() == eST_Water || gRenDev->m_RP.m_pShader->GetShaderType() == eST_Shadow || gRenDev->m_RP.m_pShader->GetShaderType() == eST_Shadow)
        {
            return ED3DShError_CompilingError;
        }
    }
    // Skip rendering if async compiling Cvar is 2
    if (CRenderer::CV_r_shadersasynccompiling == 2)
    {
        return ED3DShError_CompilingError;
    }

    CShader* pSH = CShaderMan::s_ShaderFallback;
    int nTech = 0;
    if (nStatus == -1)
    {
        pInst->m_Handle.m_bStatus = 1;
        nTech = 1;
    }
    else
    {
        nTech = 0;
        assert(nStatus == 0);
    }
    assert(pSH);
    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_logShaders)
    {
        char nameSrc[256];
        mfGetDstFileName(pInst, this, nameSrc, 256, 3);
        gcpRendD3D->LogShv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "Async %d: using Fallback tech '%s' instead of 0x%x '%s' shader\n", gRenDev->GetFrameID(false), pSH->m_HWTechniques[nTech]->m_NameStr.c_str(), pInst, nameSrc);
    }
    // Fallback
    if (pSH)
    {
        if (gRenDev->m_RP.m_CurState & GS_DEPTHFUNC_EQUAL)
        {
            int nState = gRenDev->m_RP.m_CurState & ~GS_DEPTHFUNC_EQUAL;
            nState |= GS_DEPTHWRITE;
            gRenDev->FX_SetState(nState);
        }
        CHWShader_D3D* pHWSH;
        if (m_eSHClass == eHWSC_Vertex)
        {
            pHWSH = (CHWShader_D3D*)pSH->m_HWTechniques[nTech]->m_Passes[0].m_VShader;
#ifdef DO_RENDERLOG
            if (CRenderer::CV_r_log >= 3)
            {
                gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "---- Fallback FX VShader \"%s\"\n", pHWSH->GetName());
            }
#endif
        }
        else
        {
            pHWSH = (CHWShader_D3D*)pSH->m_HWTechniques[nTech]->m_Passes[0].m_PShader;
#ifdef DO_RENDERLOG
            if (CRenderer::CV_r_log >= 3)
            {
                gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "---- Fallback FX PShader \"%s\"\n", pHWSH->GetName());
            }
#endif
        }

        if (!pHWSH->m_Insts.size())
        {
            SShaderCombination cmb;
            pHWSH->mfPrecache(cmb, true, true, false, gRenDev->m_RP.m_pShader, gRenDev->m_RP.m_pShaderResources);
        }
        if (pHWSH->m_Insts.size())
        {
            SHWSInstance* pInstF = pHWSH->m_Insts[0];
            if (!pInstF->m_Handle.m_pShader || !pInstF->m_Handle.m_pShader->m_pHandle)
            {
                return ED3DShError_CompilingError;
            }
            pInst = pInstF;
            m_pCurInst = pInstF;
            pInstF->m_bFallback = true;
        }
        else
        {
            return ED3DShError_CompilingError;
        }
    }
    //if (nStatus == 0)
    //  return ED3DShError_Compiling;
    return ED3DShError_Ok;
}

ED3DShError CHWShader_D3D::mfIsValid_Int(SHWSInstance*& pInst, bool bFinalise)
{
    //if (_stricmp(m_EntryFunc.c_str(), "FPPS") && _stricmp(m_EntryFunc.c_str(), "FPVS") && _stricmp(m_EntryFunc.c_str(), "AuxGeomPS") && _stricmp(m_EntryFunc.c_str(), "AuxGeomVS"))
    //  return mfFallBack(pInst, 0);

    if (pInst->m_Handle.m_bStatus == 1)
    {
        return mfFallBack(pInst, -1);
    }
    if (pInst->m_Handle.m_bStatus == 2)
    {
        return ED3DShError_Fake;
    }
    if (pInst->m_Handle.m_pShader == NULL)
    {
        if (pInst->m_bAsyncActivating)
        {
            return mfFallBack(pInst, 0);
        }

        if (!bFinalise || !pInst->m_pAsync)
        {
            return ED3DShError_NotCompiled;
        }

        int nStatus = 0;
        if (!pInst->m_bAsyncActivating)
        {
            nStatus = mfAsyncCompileReady(pInst);
            if (nStatus == 1)
            {
                if (gcpRendD3D->m_cEF.m_nCombinationsProcess <= 0 || gcpRendD3D->m_cEF.m_bActivatePhase)
                {
                    assert(pInst->m_Handle.m_pShader != NULL);
                }
                return ED3DShError_Ok;
            }
        }
        return mfFallBack(pInst, nStatus);
    }
    return ED3DShError_Ok;
}

struct InstContainerByHash
{
    bool operator () (const CHWShader_D3D::SHWSInstance* left, const CHWShader_D3D::SHWSInstance* right) const
    {
        return left->m_Ident.m_nHash < right->m_Ident.m_nHash;
    }
    bool operator () (const uint32 left, const CHWShader_D3D::SHWSInstance* right) const
    {
        return left < right->m_Ident.m_nHash;
    }
    bool operator () (const CHWShader_D3D::SHWSInstance* left, uint32 right) const
    {
        return left->m_Ident.m_nHash < right;
    }
};


CHWShader_D3D::SHWSInstance* CHWShader_D3D::mfGetInstance([[maybe_unused]] CShader* pSH, int nHashInstance, [[maybe_unused]] uint64 GLMask)
{
    DETAILED_PROFILE_MARKER("mfGetInstance");
    FUNCTION_PROFILER_RENDER_FLAT
    InstContainer* pInstCont = &m_Insts;
    if (m_bUseLookUpTable)
    {
        assert(nHashInstance < pInstCont->size());
        SHWSInstance* pInst = (*pInstCont)[nHashInstance];
        return pInst;
    }
    InstContainerIt it = std::lower_bound(pInstCont->begin(), pInstCont->end(), nHashInstance, InstContainerByHash());
    assert (it != pInstCont->end() && nHashInstance == (*it)->m_Ident.m_nHash);

    return (*it);
}

CHWShader_D3D::SHWSInstance* CHWShader_D3D::mfGetInstance(CShader* pSH, SShaderCombIdent& Ident, uint32 nFlags)
{
    DETAILED_PROFILE_MARKER("mfGetInstance");
    FUNCTION_PROFILER_RENDER_FLAT
    SHWSInstance* cgi = m_pCurInst;
    if (cgi && !cgi->m_bFallback)
    {
        assert(cgi->m_eClass < eHWSC_Num);

        const SShaderCombIdent& other = cgi->m_Ident;
        // other will have been through PostCreate, and so won't have the platform mask set anymore
        if ((Ident.m_MDVMask & ~SF_PLATFORM) == other.m_MDVMask && Ident.m_RTMask == other.m_RTMask && Ident.m_GLMask == other.m_GLMask && Ident.m_FastCompare1 == other.m_FastCompare1 && Ident.m_pipelineState.opaque == other.m_pipelineState.opaque && Ident.m_STMask == other.m_STMask)
        {
            return cgi;
        }
    }
    InstContainerByHash findByHash;
    InstContainer* pInstCont = &m_Insts;
    THWInstanceLookupMap* pInstMap = &m_LookupMap;
    uint32 identHash = Ident.PostCreate();
    if (m_bUseLookUpTable)
    {
        uint64 uiKey = Ident.m_RTMask + Ident.m_GLMask + Ident.m_LightMask + Ident.m_MDMask + Ident.m_MDVMask + Ident.m_pipelineState.opaque + Ident.m_STMask;

        std::pair<THWInstanceLookupMap::iterator, THWInstanceLookupMap::iterator> itp = pInstMap->equal_range(uiKey);
        for (THWInstanceLookupMap::iterator it = itp.first; it != itp.second; ++it)
        {
            // use index redirection
            uint32 uiIndex = it->second;
            cgi = (*pInstCont)[uiIndex];
            if (cgi->m_Ident.m_nHash == identHash)
            {
                m_pCurInst = cgi;
                return cgi;
            }
        }
        cgi = new SHWSInstance;
        cgi->m_nContIndex = pInstCont->size();
        cgi->m_vertexFormat = pSH->m_vertexFormat;
        cgi->m_nCache = -1;
        s_nInstFrame++;
        cgi->m_Ident = Ident;
        cgi->m_eClass = m_eSHClass;
        pInstCont->push_back(cgi);
        uint32 uiIndex = pInstCont->size() - 1;
        if (nFlags & HWSF_FAKE)
        {
            cgi->m_Handle.SetFake();
            //mfSetHWStartProfile(nFlags);
        }

        // only store index to object instead of pointer itself
        // else we have lots of issues with the internal resize
        // functionality of the vector itself (does some strange
        // allocation stuff once above 20 000 members)
        pInstMap->insert(std::pair<uint64, uint32>(uiKey, uiIndex));
    }
    else
    {
        cgi = 0;

        //int nFree = -1;

        // Find first matching shader RT bit flag combination (CRC hash identification)
        InstContainerIt it = std::lower_bound(pInstCont->begin(), pInstCont->end(), identHash, findByHash);
        if (it != pInstCont->end() && identHash == (*it)->m_Ident.m_nHash)
        {
#ifdef _RELEASE
            cgi = *it; // release - return the first matching shader permutation
#else

            // If not release, run over all matching shaders permutations and look for matching CRC hash
            while (it != pInstCont->end() && identHash == (*it)->m_Ident.m_nHash)
            {
                const SShaderCombIdent& other = (*it)->m_Ident;
                if ((Ident.m_MDVMask & ~SF_PLATFORM) == other.m_MDVMask && Ident.m_RTMask == other.m_RTMask
                    && Ident.m_GLMask == other.m_GLMask && Ident.m_FastCompare1 == other.m_FastCompare1
                    && Ident.m_pipelineState.opaque == other.m_pipelineState.opaque && Ident.m_STMask == other.m_STMask)
                {
                    cgi = *it;
                    break;
                }

                // Matching CRC hash was found, but the shader permutation bits do not match - this is a CRC
                // wrongly matched due to small chance of having same CRC over different bits
                iLog->Log("Error: ShaderIdent hash value not unique - matching two different shader permutations with same CRC!");

                // Move to the next iterator, hoping to have a match with the right permutation
                ++it;
            }


            // No matching permutation was found:
            // If a matching CRC hash was found, set the iterator to the last matching CRC on the list, 
            // otherwise set it to the last iterator.
            if (cgi == 0)
            {
                --it;
            }
#endif
        }

        // Either the CRC was not found, or no match permutation was found - in either case create
        // a new entry and insert it to the table.
        if (cgi == 0)
        {
            cgi = new SHWSInstance;
            cgi->m_nContIndex = pInstCont->size();
            cgi->m_vertexFormat = pSH->m_vertexFormat;
            cgi->m_nCache = -1;
            s_nInstFrame++;
            cgi->m_Ident = Ident;
            cgi->m_eClass = m_eSHClass;
            size_t i = std::distance(pInstCont->begin(), it);
            pInstCont->insert(it, cgi);
            if (nFlags & HWSF_FAKE)
            {
                cgi->m_Handle.SetFake();
                //mfSetHWStartProfile(nFlags);
            }
        }
    }
    m_pCurInst = cgi;
    return cgi;
}

//=================================================================================

void CHWShader_D3D::mfSetForOverdraw(SHWSInstance* pInst, uint32 nFlags, uint64& RTMask)
{
    if (mfIsValid(pInst, false) ==  ED3DShError_NotCompiled)
    {
        mfActivate(gRenDev->m_RP.m_pShader, nFlags);
    }
    RTMask |= g_HWSR_MaskBit[HWSR_DEBUG0] | g_HWSR_MaskBit[HWSR_DEBUG1] | g_HWSR_MaskBit[HWSR_DEBUG2] | g_HWSR_MaskBit[HWSR_DEBUG3];
    RTMask &= m_nMaskAnd_RT;
    RTMask |= m_nMaskOr_RT;
    CD3D9Renderer* rd = gcpRendD3D;
    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_measureoverdraw == 1 && m_eSHClass == eHWSC_Pixel)
    {
        rd->m_RP.m_NumShaderInstructions = pInst->m_nInstructions;
    }
    else
    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_measureoverdraw == 3 && m_eSHClass == eHWSC_Vertex)
    {
        rd->m_RP.m_NumShaderInstructions = pInst->m_nInstructions;
    }
    else
    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_measureoverdraw == 2 || CRenderer::CV_r_measureoverdraw == 4)
    {
        rd->m_RP.m_NumShaderInstructions = 30;
    }
}


void CHWShader_D3D::ModifyLTMask(uint32& nMask)
{
    if (nMask)
    {
        if (!(m_Flags & (HWSG_SUPPORTS_MULTILIGHTS | HWSG_SUPPORTS_LIGHTING | HWSG_FP_EMULATION)))
        {
            nMask = 0;
        }
        else
        if (!(m_Flags & HWSG_SUPPORTS_MULTILIGHTS) && (m_Flags & HWSG_SUPPORTS_LIGHTING))
        {
            int nLightType = (nMask >> SLMF_LTYPE_SHIFT) & SLMF_TYPE_MASK;
            if (nLightType != SLMF_PROJECTED)
            {
                nMask = 1;
            }
        }
    }
}

bool CHWShader_D3D::mfSetVS(int nFlags)
{
    DETAILED_PROFILE_MARKER("mfSetVS");

    CD3D9Renderer* rd = gcpRendD3D;
    SRenderPipeline& RESTRICT_REFERENCE rRP = rd->m_RP;
    SThreadInfo& RESTRICT_REFERENCE rTI = rRP.m_TI[rRP.m_nProcessThreadID];

    SShaderCombIdent Ident;
    Ident.m_LightMask = rRP.m_FlagsShader_LT;
    Ident.m_RTMask = rRP.m_FlagsShader_RT & m_nMaskAnd_RT | m_nMaskOr_RT;
    Ident.m_MDMask = rRP.m_FlagsShader_MD;
    Ident.m_MDVMask = rRP.m_FlagsShader_MDV | CParserBin::m_nPlatform;
    Ident.m_GLMask = m_nMaskGenShader;
    Ident.m_STMask = m_maskGenStatic;

    ModifyLTMask(Ident.m_LightMask);

    SHWSInstance* pInst = mfGetInstance(rRP.m_pShader, Ident, nFlags);

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_measureoverdraw == 3)
    {
        mfSetForOverdraw(pInst, nFlags, Ident.m_RTMask);
        pInst = mfGetInstance(rRP.m_pShader, Ident, nFlags);
    }

    pInst->m_fLastAccess = rTI.m_RealTime;

    if (!mfCheckActivation(rRP.m_pShader, pInst, nFlags))
    {
        s_pCurInstVS = NULL;
        s_nActivationFailMask |= (1 << eHWSC_Vertex);
        return false;
    }

#ifdef DO_RENDERLOG
    if (CRenderer::CV_r_log >= 3)
    {
        rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "--- Set FX VShader \"%s\" (%d instr), LTMask: 0x%x, GLMask: 0x%llx, RTMask: 0x%llx, MDMask: 0x%x, MDVMask: 0x%x, PSS: 0x%llx, STMask: 0x%llx\n", GetName(), pInst->m_nInstructions, Ident.m_LightMask, Ident.m_GLMask, Ident.m_RTMask, Ident.m_MDMask, Ident.m_MDVMask, Ident.m_pipelineState.opaque, Ident.m_STMask);
    }
#endif
    if (m_nFrame != rTI.m_nFrameUpdateID)
    {
        m_nFrame = rTI.m_nFrameUpdateID;
#ifndef _RELEASE
        rRP.m_PS[rRP.m_nProcessThreadID].m_NumVShaders++;
        if (pInst->m_nInstructions > rRP.m_PS[rRP.m_nProcessThreadID].m_NumVSInstructions)
        {
            rRP.m_PS[rRP.m_nProcessThreadID].m_NumVSInstructions = pInst->m_nInstructions;
            rRP.m_PS[rRP.m_nProcessThreadID].m_pMaxVShader = this;
            rRP.m_PS[rRP.m_nProcessThreadID].m_pMaxVSInstance = pInst;
        }
#endif
    }
    if (!(nFlags & HWSF_PRECACHE))
    {
        if (s_pCurVS != pInst->m_Handle.m_pShader)
        {
            s_pCurVS = pInst->m_Handle.m_pShader;
#ifndef _RELEASE
            rRP.m_PS[rRP.m_nProcessThreadID].m_NumVShadChanges++;
#endif
            mfBind();
        }
        s_pCurInstVS = pInst;
        rRP.m_FlagsStreams_Decl = pInst->m_VStreamMask_Decl;
        rRP.m_FlagsStreams_Stream = pInst->m_VStreamMask_Stream;
        // Make sure we don't use any texture attributes except baseTC in instancing case
        if (nFlags & HWSF_INSTANCED)
        {
            rRP.m_FlagsStreams_Decl &= ~(VSM_MORPHBUDDY);
            rRP.m_FlagsStreams_Stream &= ~(VSM_MORPHBUDDY);
        }

        UpdatePerBatchConstantBuffer();
    }
    if (nFlags & HWSF_SETTEXTURES)
    {
        mfSetSamplers(pInst->m_pSamplers, m_eSHClass);
    }

    s_nActivationFailMask &= ~(1 << eHWSC_Vertex);

    return true;
}

bool CHWShader_D3D::mfSetPS(int nFlags)
{
    DETAILED_PROFILE_MARKER("mfSetPS");

    CD3D9Renderer* rd = gcpRendD3D;
    SRenderPipeline& RESTRICT_REFERENCE rRP = rd->m_RP;
    SThreadInfo& RESTRICT_REFERENCE rTI = rRP.m_TI[rRP.m_nProcessThreadID];

    SShaderCombIdent Ident;
    Ident.m_LightMask = rRP.m_FlagsShader_LT;
    Ident.m_RTMask = rRP.m_FlagsShader_RT & m_nMaskAnd_RT | m_nMaskOr_RT;
    Ident.m_MDMask = rRP.m_FlagsShader_MD & ~HWMD_TEXCOORD_FLAG_MASK;
    Ident.m_MDVMask = CParserBin::m_nPlatform;
    Ident.m_GLMask = m_nMaskGenShader;
    Ident.m_STMask = m_maskGenStatic;

    ModifyLTMask(Ident.m_LightMask);

    SHWSInstance* pInst = mfGetInstance(rRP.m_pShader, Ident, nFlags);

    // Update texture modificator flags based on active samplers state
    if (nFlags & HWSF_SETTEXTURES)
    {
        int nResult = mfCheckActivation(rRP.m_pShader, pInst, nFlags);
        if (!nResult)
        {
            CHWShader_D3D::s_pCurInstPS = NULL;
            s_nActivationFailMask |= (1 << eHWSC_Pixel);
            return false;
        }
        mfUpdateSamplers(rRP.m_pShader);
        if ((rRP.m_FlagsShader_MD ^ Ident.m_MDMask) & ~HWMD_TEXCOORD_FLAG_MASK)
        {
            pInst->m_fLastAccess = rTI.m_RealTime;
            if (rd->m_nFrameSwapID != pInst->m_nUsedFrame)
            {
                pInst->m_nUsedFrame = rd->m_nFrameSwapID;
                pInst->m_nUsed++;
            }
            Ident.m_MDMask = rRP.m_FlagsShader_MD & ~HWMD_TEXCOORD_FLAG_MASK;
            Ident.m_MDVMask = CParserBin::m_nPlatform;
            pInst = mfGetInstance(rRP.m_pShader, Ident, nFlags);
        }
    }
    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_measureoverdraw > 0 && CRenderer::CV_r_measureoverdraw < 5)
    {
        mfSetForOverdraw(pInst, nFlags, Ident.m_RTMask);
        pInst = mfGetInstance(rRP.m_pShader, Ident, nFlags);
    }
    pInst->m_fLastAccess = rTI.m_RealTime;

    if (!mfCheckActivation(rRP.m_pShader, pInst, nFlags))
    {
        s_pCurInstPS = NULL;
        s_nActivationFailMask |= (1 << eHWSC_Pixel);
        return false;
    }

#ifdef DO_RENDERLOG
    if (CRenderer::CV_r_log >= 3)
    {
        rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "--- Set FX PShader \"%s\" (%d instr) LTMask: 0x%x, GLMask: 0x%llx, RTMask: 0x%llx, MDMask: 0x%x, MDVMask: 0x%x, PSS: 0x%llx, STMask: 0x%llx\n", GetName(), pInst->m_nInstructions, Ident.m_LightMask, Ident.m_GLMask, Ident.m_RTMask, Ident.m_MDMask, Ident.m_MDVMask & 0x0fffffff, Ident.m_pipelineState.opaque, Ident.m_STMask);
    }
#endif

    if (m_nFrame != rTI.m_nFrameUpdateID)
    {
        m_nFrame = rTI.m_nFrameUpdateID;
#ifndef _RELEASE
        rRP.m_PS[rRP.m_nProcessThreadID].m_NumPShaders++;
        if (pInst->m_nInstructions > rRP.m_PS[rRP.m_nProcessThreadID].m_NumPSInstructions)
        {
            rRP.m_PS[rRP.m_nProcessThreadID].m_NumPSInstructions = pInst->m_nInstructions;
            rRP.m_PS[rRP.m_nProcessThreadID].m_pMaxPShader = this;
            rRP.m_PS[rRP.m_nProcessThreadID].m_pMaxPSInstance = pInst;
        }
#endif
    }
    if (!(nFlags & HWSF_PRECACHE))
    {
        if (s_pCurPS != pInst->m_Handle.m_pShader)
        {
            s_pCurPS = pInst->m_Handle.m_pShader;
#ifndef _RELEASE
            rRP.m_PS[rRP.m_nProcessThreadID].m_NumPShadChanges++;
#endif
            mfBind();
        }
        s_pCurInstPS = pInst;
        UpdatePerBatchConstantBuffer();
        if (nFlags & HWSF_SETTEXTURES)
        {
            mfSetSamplers(pInst->m_pSamplers, m_eSHClass);
        }
    }

    s_nActivationFailMask &= ~(1 << eHWSC_Pixel);

    return true;
}

bool CHWShader_D3D::mfSetGS(int nFlags)
{
    DETAILED_PROFILE_MARKER("mfSetGS");

    CD3D9Renderer* rd = gcpRendD3D;
    SRenderPipeline& RESTRICT_REFERENCE rRP = rd->m_RP;
    SThreadInfo& RESTRICT_REFERENCE rTI = rRP.m_TI[rRP.m_nProcessThreadID];

    SShaderCombIdent Ident;
    Ident.m_LightMask = rRP.m_FlagsShader_LT;
    Ident.m_RTMask = rRP.m_FlagsShader_RT & m_nMaskAnd_RT | m_nMaskOr_RT;
    Ident.m_MDMask = rRP.m_FlagsShader_MD;
    Ident.m_MDVMask = rRP.m_FlagsShader_MDV | CParserBin::m_nPlatform;
    Ident.m_GLMask = m_nMaskGenShader;
    Ident.m_STMask = m_maskGenStatic;

    ModifyLTMask(Ident.m_LightMask);

    SHWSInstance* pInst = mfGetInstance(rRP.m_pShader, Ident, nFlags);
    pInst->m_fLastAccess = rTI.m_RealTime;

    if (!mfCheckActivation(rRP.m_pShader, pInst, nFlags))
    {
        s_pCurInstGS = NULL;
        s_nActivationFailMask |= (1 << eHWSC_Geometry);
        return false;
    }

#ifdef DO_RENDERLOG
    if (CRenderer::CV_r_log >= 3)
    {
        rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "--- Set FX GShader \"%s\" (%d instr), LTMask: 0x%x, GLMask: 0x%llx, RTMask: 0x%llx, MDMask: 0x%x, MDVMask: 0x%x, PSS: 0x%llx, STMask: 0x%llx\n", GetName(), pInst->m_nInstructions, Ident.m_LightMask, Ident.m_GLMask, Ident.m_RTMask, Ident.m_MDMask, Ident.m_MDVMask, Ident.m_pipelineState.opaque, Ident.m_STMask);
    }
#endif

    rRP.m_PersFlags2 |= s_bFirstGS * (RBPF2_COMMIT_PF | RBPF2_COMMIT_CM);
    rRP.m_nCommitFlags |= s_bFirstGS * (FC_GLOBAL_PARAMS);

    s_bFirstGS = false;
    s_pCurInstGS = pInst;
    if (!(nFlags & HWSF_PRECACHE))
    {
        mfBindGS(pInst->m_Handle.m_pShader, pInst->m_Handle.m_pShader->m_pHandle);

        UpdatePerBatchConstantBuffer();

        if (nFlags & HWSF_SETTEXTURES)
        {
            mfSetSamplers(pInst->m_pSamplers, m_eSHClass);
        }
    }

    s_nActivationFailMask &= ~(1 << eHWSC_Geometry);

    return true;
}

bool CHWShader_D3D::mfSetHS(int nFlags)
{
    DETAILED_PROFILE_MARKER("mfSetHS");

    CD3D9Renderer* rd = gcpRendD3D;
    SRenderPipeline& RESTRICT_REFERENCE rRP = rd->m_RP;
    SThreadInfo& RESTRICT_REFERENCE rTI = rRP.m_TI[rRP.m_nProcessThreadID];

    SShaderCombIdent Ident;
    Ident.m_LightMask = rRP.m_FlagsShader_LT;
    Ident.m_RTMask = rRP.m_FlagsShader_RT & m_nMaskAnd_RT | m_nMaskOr_RT;
    Ident.m_MDMask = rRP.m_FlagsShader_MD;
    Ident.m_MDVMask = rRP.m_FlagsShader_MDV | CParserBin::m_nPlatform;
    Ident.m_GLMask = m_nMaskGenShader;
    Ident.m_STMask = m_maskGenStatic;
    ModifyLTMask(Ident.m_LightMask);

    SHWSInstance* pInst = mfGetInstance(rRP.m_pShader, Ident, nFlags);
    pInst->m_fLastAccess = rTI.m_RealTime;

    if (!mfCheckActivation(rRP.m_pShader, pInst, nFlags))
    {
        s_pCurInstHS = NULL;
        s_nActivationFailMask |= (1 << eHWSC_Hull);
        return false;
    }

#ifdef DO_RENDERLOG
    if (CRenderer::CV_r_log >= 3)
    {
        rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "--- Set FX HShader \"%s\" (%d instr), LTMask: 0x%x, GLMask: 0x%llx, RTMask: 0x%llx, MDMask: 0x%x, MDVMask: 0x%x, PSS: 0x%llx, STMask: 0x%llx\n", GetName(), pInst->m_nInstructions, Ident.m_LightMask, Ident.m_GLMask, Ident.m_RTMask, Ident.m_MDMask, Ident.m_MDVMask, Ident.m_pipelineState.opaque, Ident.m_STMask);
    }
#endif

    rRP.m_PersFlags2 |= s_bFirstHS * (RBPF2_COMMIT_PF | RBPF2_COMMIT_CM);
    rRP.m_nCommitFlags |= s_bFirstHS * (FC_GLOBAL_PARAMS);

    s_bFirstHS = false;
    s_pCurInstHS = pInst;
    if (!(nFlags & HWSF_PRECACHE))
    {
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DHWSHADER_CPP_SECTION_1
    #include AZ_RESTRICTED_FILE(D3DHWShader_cpp)
#endif

        mfBindHS(pInst->m_Handle.m_pShader, pInst->m_Handle.m_pShader->m_pHandle);

        UpdatePerBatchConstantBuffer();
    }

    s_nActivationFailMask &= ~(1 << eHWSC_Hull);

    return true;
}

bool CHWShader_D3D::mfSetDS(int nFlags)
{
    DETAILED_PROFILE_MARKER("mfSetDS");

    CD3D9Renderer* rd = gcpRendD3D;
    SRenderPipeline& RESTRICT_REFERENCE rRP = rd->m_RP;
    SThreadInfo& RESTRICT_REFERENCE rTI = rRP.m_TI[rRP.m_nProcessThreadID];

    SShaderCombIdent Ident;
    Ident.m_LightMask = rRP.m_FlagsShader_LT;
    Ident.m_RTMask = rRP.m_FlagsShader_RT & m_nMaskAnd_RT | m_nMaskOr_RT;
    Ident.m_MDMask = rRP.m_FlagsShader_MD;
    Ident.m_MDVMask = rRP.m_FlagsShader_MDV | CParserBin::m_nPlatform;
    Ident.m_GLMask = m_nMaskGenShader;
    Ident.m_STMask = m_maskGenStatic;

    ModifyLTMask(Ident.m_LightMask);

    SHWSInstance* pInst = mfGetInstance(rRP.m_pShader, Ident, nFlags);
    pInst->m_fLastAccess = rTI.m_RealTime;

    if (!mfCheckActivation(rRP.m_pShader, pInst, nFlags))
    {
        s_pCurInstDS = NULL;
        s_nActivationFailMask |= (1 << eHWSC_Domain);
        return false;
    }

#ifdef DO_RENDERLOG
    if (CRenderer::CV_r_log >= 3)
    {
        rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "--- Set FX CShader \"%s\" (%d instr), LTMask: 0x%x, GLMask: 0x%llx, RTMask: 0x%llx, MDMask: 0x%x, MDVMask: 0x%x, PSS: 0x%llx, STMask: 0x%llx\n", GetName(), pInst->m_nInstructions, Ident.m_LightMask, Ident.m_GLMask, Ident.m_RTMask, Ident.m_MDMask, Ident.m_MDVMask, Ident.m_pipelineState.opaque, Ident.m_STMask);
    }
#endif

    rRP.m_PersFlags2 |= s_bFirstDS * (RBPF2_COMMIT_PF | RBPF2_COMMIT_CM);
    rRP.m_nCommitFlags |= s_bFirstDS * (FC_GLOBAL_PARAMS);

    s_bFirstDS = false;
    s_pCurInstDS = pInst;
    if (!(nFlags & HWSF_PRECACHE))
    {
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DHWSHADER_CPP_SECTION_2
    #include AZ_RESTRICTED_FILE(D3DHWShader_cpp)
#endif

        mfBindDS(pInst->m_Handle.m_pShader, pInst->m_Handle.m_pShader->m_pHandle);

        UpdatePerBatchConstantBuffer();
    }

    if (nFlags & HWSF_SETTEXTURES)
    {
        mfSetSamplers(pInst->m_pSamplers, m_eSHClass);
    }

    s_nActivationFailMask &= ~(1 << eHWSC_Domain);

    return true;
}

bool CHWShader_D3D::mfSetCS(int nFlags)
{
    DETAILED_PROFILE_MARKER("mfSetCS");

    CD3D9Renderer* rd = gcpRendD3D;
    SRenderPipeline& RESTRICT_REFERENCE rRP = rd->m_RP;
    SThreadInfo& RESTRICT_REFERENCE rTI = rRP.m_TI[rRP.m_nProcessThreadID];

    SShaderCombIdent Ident;
    Ident.m_LightMask = rRP.m_FlagsShader_LT;
    Ident.m_RTMask = rRP.m_FlagsShader_RT & m_nMaskAnd_RT | m_nMaskOr_RT;
    Ident.m_MDMask = rRP.m_FlagsShader_MD;
    Ident.m_MDVMask = rRP.m_FlagsShader_MDV | CParserBin::m_nPlatform;
    Ident.m_GLMask = m_nMaskGenShader;
    Ident.m_STMask = m_maskGenStatic;

    if (Ident.m_LightMask)
    {
        if (!(m_Flags & (HWSG_SUPPORTS_MULTILIGHTS | HWSG_SUPPORTS_LIGHTING | HWSG_FP_EMULATION)))
        {
            Ident.m_LightMask = 0;
        }
        else
        if (!(m_Flags & HWSG_SUPPORTS_MULTILIGHTS) && (m_Flags & HWSG_SUPPORTS_LIGHTING))
        {
            int nLightType = (Ident.m_LightMask >> SLMF_LTYPE_SHIFT) & SLMF_TYPE_MASK;
            if (nLightType != SLMF_PROJECTED)
            {
                Ident.m_LightMask = 1;
            }
        }
    }

    SHWSInstance* pInst = mfGetInstance(rRP.m_pShader, Ident, nFlags);
    pInst->m_fLastAccess = rTI.m_RealTime;

    if (!mfCheckActivation(rRP.m_pShader, pInst, nFlags))
    {
        s_pCurInstCS = NULL;
        s_nActivationFailMask |= (1 << eHWSC_Compute);
        return false;
    }

#ifdef DO_RENDERLOG
    if (CRenderer::CV_r_log >= 3)
    {
        rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "--- Set FX CShader \"%s\" (%d instr), LTMask: 0x%x, GLMask: 0x%llx, RTMask: 0x%llx, MDMask: 0x%x, MDVMask: 0x%x, STMask: 0x%llx\n", GetName(), pInst->m_nInstructions, Ident.m_LightMask, Ident.m_GLMask, Ident.m_RTMask, Ident.m_MDMask, Ident.m_MDVMask, Ident.m_STMask);
    }
#endif

    s_pCurInstCS = pInst;
    if (!(nFlags & HWSF_PRECACHE))
    {
        mfBindCS(pInst->m_Handle.m_pShader, pInst->m_Handle.m_pShader->m_pHandle);

        UpdatePerBatchConstantBuffer();
    }

    if (nFlags & HWSF_SETTEXTURES)
    {
        mfSetSamplers(pInst->m_pSamplers, m_eSHClass);
    }

    s_nActivationFailMask = 0; // Reset entire mask since CS does not need any other shader stages

    return true;
}

void CHWShader_D3D::mfUpdatePreprocessFlags(SShaderTechnique* pTech)
{
    DETAILED_PROFILE_MARKER("mfUpdatePreprocessFlags");
    uint32 nFlags = 0;

    for (uint32 i = 0; i < (uint32)m_Insts.size(); i++)
    {
        SHWSInstance* pInst = m_Insts[i];
        if (pInst->m_pSamplers.size())
        {
            for (uint32 j = 0; j < (uint32)pInst->m_pSamplers.size(); j++)
            {
                STexSamplerRT* pSamp = &pInst->m_pSamplers[j];
                if (pSamp && pSamp->m_pTarget)
                {
                    SHRenderTarget* pTarg = pSamp->m_pTarget;
                    if (pTarg->m_eOrder == eRO_PreProcess)
                    {
                        nFlags |= pTarg->m_nProcessFlags;
                    }
                    if (pTech)
                    {
                        uint32 n = 0;
                        for (n = 0; n < pTech->m_RTargets.Num(); n++)
                        {
                            if (pTarg == pTech->m_RTargets[n])
                            {
                                break;
                            }
                        }
                        if (n == pTech->m_RTargets.Num())
                        {
                            pTech->m_RTargets.AddElem(pTarg);
                        }
                    }
                }
            }
        }
    }
    if (pTech)
    {
        pTech->m_RTargets.Shrink();
        pTech->m_nPreprocessFlags |= nFlags;
    }
}

AZ::u32 CHWShader_D3D::SHWSInstance::GenerateVertexDeclarationCacheKey(const AZ::Vertex::Format& vertexFormat)
{

    // We cannot naively use the AZ::Vertex::Format CRC to cache the results of CreateInputLayout.
    // CreateInputLayout compiles a fetch shader to associate the vertex format with the individual vertex shader instance.
    // If the vertex shader does not reference one of the input semantics, then the fetch shader will not
    AZ::u32 fetchShaderKey = ( m_uniqueNameCRC ^ vertexFormat.GetEnum() );

    return fetchShaderKey;
}


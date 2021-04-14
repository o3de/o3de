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
#include "UnalignedBlit.h"
#include <CryPath.h>
#include <Pak/CryPakUtils.h>

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define D3DHWSHADERCOMPILING_CPP_SECTION_1 1
#define D3DHWSHADERCOMPILING_CPP_SECTION_2 2
#define D3DHWSHADERCOMPILING_CPP_SECTION_3 3
#define D3DHWSHADERCOMPILING_CPP_SECTION_4 4
#define D3DHWSHADERCOMPILING_CPP_SECTION_5 5
#define D3DHWSHADERCOMPILING_CPP_SECTION_6 6
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DHWSHADERCOMPILING_CPP_SECTION_1
    #include AZ_RESTRICTED_FILE(D3DHWShaderCompiling_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif !defined(OPENGL)
    #include <D3D11Shader.h>
    #include <D3DCompiler.h>
#endif

#ifdef CRY_USE_METAL
#import <Foundation/Foundation.h>
#endif
#include "../Common/Shaders/RemoteCompiler.h"
#include "../Common/RenderCapabilities.h"

#include <AzFramework/IO/FileOperations.h>

static CryCriticalSection g_cAILock;
static SShaderAsyncInfo g_PendingList;
static SShaderAsyncInfo g_PendingListT;
#ifdef SHADER_ASYNC_COMPILATION
static SShaderAsyncInfo g_BuildList;
#endif

SShaderAsyncInfo& SShaderAsyncInfo::PendingList() { return g_PendingList; }
SShaderAsyncInfo& SShaderAsyncInfo::PendingListT() { return g_PendingListT; }
#ifdef SHADER_ASYNC_COMPILATION
SShaderAsyncInfo& CAsyncShaderTask::BuildList() { return g_BuildList; }
#endif

CryEvent SShaderAsyncInfo::s_RequestEv;

int CHWShader_D3D::s_nDevicePSDataSize;
int CHWShader_D3D::s_nDeviceVSDataSize;

class CSpinLock
{
public:
    CSpinLock()
    {
#if defined (WIN32) || defined(LINUX) || defined(APPLE) || defined(IMPLEMENT_SPIN_LOCK_WITH_INTERLOCKED_COMPARE_EXCHANGE)
        while (CryInterlockedCompareExchange(&s_locked, 1L, 0L) == 1L)
        {
            Sleep(0);
        }
#endif
    }

    ~CSpinLock()
    {
#if defined (WIN32)
        InterlockedExchange(&s_locked, 0L);
#elif defined(LINUX) || defined(APPLE) || defined(IMPLEMENT_SPIN_LOCK_WITH_INTERLOCKED_COMPARE_EXCHANGE)
        while (CryInterlockedCompareExchange(&s_locked, 0L, 1L) == 0L)
        {
            ;
        }
#endif
    }

private:
    static volatile LONG s_locked;
};

volatile LONG CSpinLock::s_locked = 0L;

volatile int SShaderAsyncInfo::s_nPendingAsyncShaders = 0;

//------------------------------------------------------------------------------
// This is the single method that is used to map a sampler and its HW slot.
// It is called when parsing a shader / loading from cache.
//------------------------------------------------------------------------------
bool CHWShader_D3D::mfAddFXSampler(SHWSInstance* pInst, SShaderFXParams& FXParams, SFXSampler* pr, const char* ParamName, SCGBind* pBind, CShader* ef, EHWShaderClass eSHClass)
{
    assert(pBind);
    if (!pBind)
    {
        return false;
    }

    std::vector<SCGSampler>* pParams;
    pParams = &pInst->m_Samplers;
    uint32 nOffs = pParams->size();
    bool bRes = gRenDev->m_cEF.mfParseFXSampler(FXParams, pr, ParamName, ef, pBind->m_RegisterCount, pParams, eSHClass);
    if (pParams->size() > nOffs)
    {
        for (uint32 i = 0; i < pParams->size() - nOffs; i++)
        {
            SCGSampler& p = (*pParams)[nOffs + i];
            p.m_RegisterOffset = pBind->m_RegisterOffset + i;
            p.m_BindingSlot = pBind->m_BindingSlot;
            p.m_Name = pBind->m_Name;
        }
    }
    // Parameter without semantic
    return bRes;
}

//------------------------------------------------------------------------------
// The main texture binding method.
// This is the single method that is used to map a texture and its HW slot.
// It is called when going over a shader / loading from cache.
//------------------------------------------------------------------------------
bool CHWShader_D3D::mfAddFXTexture(
    SHWSInstance* pInst, SShaderFXParams& FXParams, SFXTexture* pr, const char* ParamName, 
    SCGBind* pBind, CShader* ef, EHWShaderClass eSHClass)
{
    assert(pBind);
    if (!pBind)
    {
        return false;
    }

    std::vector<SCGTexture>* pParams;
    pParams = &pInst->m_Textures;
    uint32 nOffs = pParams->size();
    bool bRes = gRenDev->m_cEF.mfParseFXTexture(FXParams, pr, ParamName, ef, pBind->m_RegisterCount, pParams, eSHClass);
    
    if (pParams->size() > nOffs)
    {   // If the texture was added
        for (uint32 i = 0; i < pParams->size() - nOffs; i++)
        {
            SCGTexture& p = (*pParams)[nOffs + i];
            p.m_RegisterOffset = pBind->m_RegisterOffset + i;   // Offset by one and set it for this texture
            p.m_BindingSlot = pBind->m_BindingSlot;             // Set the passed binding slot 
            p.m_Name = pBind->m_Name;
        }
    }
    // Parameter without semantic
    return bRes;
}

//------------------------------------------------------------------------------
// This is the single method that is used to map constant parameter and its HW slot.
// It is called when parsing a shader / loading from cache.
//------------------------------------------------------------------------------
void CHWShader_D3D::mfAddFXParameter( 
    [[maybe_unused]] SHWSInstance* pInst, SParamsGroup& OutParams, SShaderFXParams& FXParams, 
    SFXParam* pr, const char* ParamName, SCGBind* pBind, CShader* ef, bool bInstParam, EHWShaderClass eSHClass)
{
    SCGParam CGpr;

    assert(pBind);
    if (!pBind)
    {
        return;
    }

    int nComps = 0;
    int nParams = pBind->m_RegisterCount;
    if (!pr->m_Semantic.empty())
    {
        nComps = pr->m_ComponentCount;
    }
    else
    {
        for (int i = 0; i < pr->m_ComponentCount; i++)
        {
            CryFixedStringT<128> cur;
            pr->GetParamComp(i, cur);
            if (cur.empty())
            {
                break;
            }
            nComps++;
        }
    }
    // Process parameters only with semantics
    if (nComps && nParams)
    {
        std::vector<SCGParam>* pParams;
        if (bInstParam)
        {
            pParams = &OutParams.Params_Inst;
        }
        else
        {
            pParams = &OutParams.Params[0];
        }
        uint32 nOffs = pParams->size();
        [[maybe_unused]] bool bRes = gRenDev->m_cEF.mfParseFXParameter(FXParams, pr, ParamName, ef, bInstParam, pBind->m_RegisterCount, pParams, eSHClass, false);
        AZ_Assert(bRes, "Error: CHWShader_D3D::mfAddFXParameter - bRes is false");

        if (pParams->size() > nOffs)
        {
            for (uint32 i = 0; i < pParams->size() - nOffs; i++)
            {
                //assert(pBind->m_nComponents == 1);
                SCGParam& p = (*pParams)[nOffs + i];
                p.m_RegisterOffset = pBind->m_RegisterOffset + i;
                p.m_BindingSlot = pBind->m_BindingSlot;
            }
        }
    }
    // Parameter without semantic
}

struct SAliasSampler
{
    STexSamplerRT Sampler;
    string NameTex;
};

bool CHWShader_D3D::mfAddFXParameter(SHWSInstance* pInst, SParamsGroup& OutParams, SShaderFXParams& FXParams, const char* param, SCGBind* bn, bool bInstParam, EHWShaderClass eSHClass, CShader* pFXShader)
{
    if (bn->m_RegisterOffset & SHADER_BIND_TEXTURE)
    {
        SFXTexture* pr = gRenDev->m_cEF.mfGetFXTexture(FXParams.m_FXTextures, param);
        if (pr)
        {
            if (bn->m_RegisterCount < 0)
            {
                bn->m_RegisterCount = pr->m_nArray;
            }
            return true;
        }
    }
    else
    if (bn->m_RegisterOffset & SHADER_BIND_SAMPLER)
    {
        SFXSampler* pr = gRenDev->m_cEF.mfGetFXSampler(FXParams.m_FXSamplers, param);
        if (pr)
        {
            if (bn->m_RegisterCount < 0)
            {
                bn->m_RegisterCount = pr->m_nArray;
            }
            return true;
        }
    }
    else
    {
        SFXParam* pr = gRenDev->m_cEF.mfGetFXParameter(FXParams.m_FXParams, param);
        if (pr)
        {
            if (bn->m_RegisterCount < 0)
            {
                bn->m_RegisterCount = pr->m_RegisterCount;
            }
            mfAddFXParameter(pInst, OutParams, FXParams, pr, param, bn, pFXShader, bInstParam, eSHClass);
            return true;
        }
    }
    return false;
}

//==================================================================================================================

int CGBindCallback(const VOID* arg1, const VOID* arg2)
{
    SCGBind* pi1 = (SCGBind*)arg1;
    SCGBind* pi2 = (SCGBind*)arg2;

    if (pi1->m_RegisterOffset < pi2->m_RegisterOffset)
    {
        return -1;
    }
    if (pi1->m_RegisterOffset > pi2->m_RegisterOffset)
    {
        return 1;
    }

    return 0;
}

const char* ReflectedConstantBufferNames[eConstantBufferShaderSlot_ReflectedCount] = {"PER_BATCH", "PER_INSTANCE", "PER_MATERIAL"};

void CHWShader_D3D::mfCreateBinds(SHWSInstance* pInst, void* pConstantTable, [[maybe_unused]] byte* pShader, [[maybe_unused]] int nSize)
{
    uint32 i;
    ID3D11ShaderReflection* pShaderReflection = (ID3D11ShaderReflection*)pConstantTable;
    D3D11_SHADER_DESC Desc;
    pShaderReflection->GetDesc(&Desc);
    ID3D11ShaderReflectionConstantBuffer* pCB = NULL;
    for (uint32 n = 0; n < Desc.ConstantBuffers; n++)
    {
        pCB = pShaderReflection->GetConstantBufferByIndex(n);
        D3D11_SHADER_BUFFER_DESC SBDesc;
        pCB->GetDesc(&SBDesc);
        if (SBDesc.Type == D3D11_CT_RESOURCE_BIND_INFO)
        {
            continue;
        }
        int nCB;
        if (!strcmp("$Globals", SBDesc.Name))
        {
            nCB = eConstantBufferShaderSlot_PerBatch;
        }
        else
        {
            for (nCB = 0; nCB < eConstantBufferShaderSlot_ReflectedCount; nCB++)
            {
                if (!strcmp(ReflectedConstantBufferNames[nCB], SBDesc.Name))
                {
                    break;
                }
            }
        }
        if (nCB >= eConstantBufferShaderSlot_ReflectedCount)  // Allow having custom cbuffers in shaders
        {
            continue;
        }
        for (i = 0; i < SBDesc.Variables; i++)
        {
            ID3D11ShaderReflectionVariable* pCV = pCB->GetVariableByIndex(i);
            ID3D11ShaderReflectionType* pVT = pCV->GetType();
            D3D11_SHADER_VARIABLE_DESC CDesc;
            D3D11_SHADER_TYPE_DESC CTDesc;
            pVT->GetDesc(&CTDesc);
            pCV->GetDesc(&CDesc);
            if (!(CDesc.uFlags & D3D10_SVF_USED))
            {
                continue;
            }
            if (CTDesc.Class == D3D10_SVC_VECTOR || CTDesc.Class == D3D10_SVC_SCALAR || CTDesc.Class == D3D10_SVC_MATRIX_COLUMNS || CTDesc.Class == D3D10_SVC_MATRIX_ROWS)
            {
                SCGBind cgp;
                assert(!(CDesc.StartOffset & 0xf));
                //assert(!(CDesc.Size & 0xf));
                int nReg = CDesc.StartOffset >> 4;
                cgp.m_RegisterOffset = nReg; //<<2;
                cgp.m_BindingSlot = nCB;
                cgp.m_RegisterCount = (CDesc.Size + 15) >> 4;
                cgp.m_Name = CDesc.Name;
                cgp.m_Flags = CParserBin::GetCRC32(CDesc.Name);
                pInst->m_pBindVars.push_back(cgp);
            }
            else
            {
                assert(false);
            }
        }
    }

    D3D11_SHADER_INPUT_BIND_DESC IBDesc;
    for (i = 0; i < Desc.BoundResources; i++)
    {
        ZeroStruct(IBDesc);
        pShaderReflection->GetResourceBindingDesc(i, &IBDesc);
        SCGBind cgp;
        if (IBDesc.Type == D3D10_SIT_TEXTURE)
        {
            cgp.m_RegisterOffset = IBDesc.BindPoint | SHADER_BIND_TEXTURE;
        }
        else
        if (IBDesc.Type == D3D10_SIT_SAMPLER)
        {
            cgp.m_RegisterOffset = IBDesc.BindPoint | SHADER_BIND_SAMPLER;
        }
        else
        {
            continue;
        }

        if (IBDesc.Dimension == D3D10_SRV_DIMENSION_BUFFER)
        {
            continue;
        }

        cgp.m_BindingSlot = IBDesc.BindPoint;
        cgp.m_RegisterCount = IBDesc.BindCount;
        cgp.m_Name = IBDesc.Name;
        cgp.m_Flags = CParserBin::GetCRC32(IBDesc.Name);
        pInst->m_pBindVars.push_back(cgp);
    }
}

void CHWShader_D3D::mfGatherFXParameters(SHWSInstance* pInst, std::vector<SCGBind>* InstBindVars, CHWShader_D3D* pSH, int nFlags, CShader* pFXShader)
{
    //  LOADING_TIME_PROFILE_SECTION(iSystem);

    std::vector<SCGBind>& parameters = pInst->m_pBindVars;

    uint32 i, j;
    SAliasSampler samps[MAX_TMU];
    int nMaxSampler = -1;
    SParamsGroup Group;
    SShaderFXParams& FXParams = gRenDev->m_cEF.m_Bin.mfGetFXParams(pInst->m_bFallback ? CShaderMan::s_ShaderFallback : pFXShader);
    if (parameters.size())
    {
        std::list<uint32> skipped;

        for (i = 0; i < parameters.size(); i++)
        {
            SCGBind* bn = &parameters[i];
            const char* param = bn->m_Name.c_str();
            if (!strncmp(param, "_g_", 3))
            {
                continue;
            }
            bool bRes = mfAddFXParameter(pInst, Group, FXParams, param, bn, false, pSH->m_eSHClass, pFXShader);
            if (!bRes && (bn->m_RegisterOffset & SHADER_BIND_TEXTURE)) // try to find old samplers (Without semantics)
            {
                skipped.push_back(i);
            }
        }

        bool setSamplers[256] = { false };
        bool setTextures[256] = { false };

        for (i = 0; i < pInst->m_Samplers.size(); i++)
        {
            setSamplers[pInst->m_Samplers[i].m_RegisterOffset & 0xff] = true;
        }
        for (i = 0; i < pInst->m_Textures.size(); i++)
        {
            setTextures[pInst->m_Textures[i].m_RegisterOffset & 0xff] = true;
        }

        while (skipped.size() > 0)
        {
            i = skipped.front();
            skipped.pop_front();

            SCGBind *bn = &parameters[i];
            const char *param = bn->m_Name.c_str();

            {
                for (j = 0; j < (uint32)FXParams.m_FXSamplersOld.size(); j++)
                {
                    STexSamplerFX* sm = &FXParams.m_FXSamplersOld[j];
                    if (!azstricmp(sm->m_szName.c_str(), param))
                    {
                        int nSampler = bn->m_RegisterOffset & 0x7f;
                        if (nSampler < MAX_TMU)
                        {
                            nMaxSampler = max(nSampler, nMaxSampler);
                            samps[nSampler].Sampler = STexSamplerRT(*sm);
                            samps[nSampler].NameTex = sm->m_szTexture;
                            samps[nSampler].Sampler.m_nSamplerSlot = (int8)(bn->m_BindingSlot & 0xff);
                            samps[nSampler].Sampler.m_nTextureSlot = nSampler;

                            for (int k = 0; k < parameters.size(); k++)
                            {
                                if (parameters[k].m_RegisterOffset & SHADER_BIND_TEXTURE)
                                {
                                    if (!azstricmp(parameters[k].m_Name.c_str(), param))
                                    {
                                        samps[nSampler].Sampler.m_nTextureSlot = (int8)parameters[k].m_BindingSlot;
                                    }
                                }
                            }

                            for (int k = 0; k < parameters.size(); k++)
                            {
                                if (parameters[k].m_RegisterOffset & SHADER_BIND_SAMPLER)
                                {
                                    if (!azstricmp(parameters[k].m_Name.c_str(), param))
                                    {
                                        samps[nSampler].Sampler.m_nSamplerSlot = (int8)parameters[k].m_BindingSlot;
                                    }
                                }
                            }


                            // Texture slot occupied, search an alternative
                            if (setSamplers[samps[nSampler].Sampler.m_nSamplerSlot])
                            {
                                uint32 f = 0; while (setSamplers[f]) ++f;
                                samps[nSampler].Sampler.m_nSamplerSlot = f;
                                assert(f < 16);
                            }

                            // Sampler slot occupied, search an alternative
                            if (setTextures[samps[nSampler].Sampler.m_nTextureSlot])
                            {
                                uint32 f = 0; while (setTextures[f]) ++f;
                                samps[nSampler].Sampler.m_nTextureSlot = f;
                                assert(f < 256);
                            }

                            setTextures[samps[nSampler].Sampler.m_nTextureSlot] = true;
                            setSamplers[samps[nSampler].Sampler.m_nSamplerSlot] = true;

                            break;
                        }
                    }
                }
                if (j == FXParams.m_FXSamplersOld.size())
                {
                    for (j = 0; j < (uint32)FXParams.m_FXSamplersOld.size(); j++)
                    {
                        STexSamplerFX* sm = &FXParams.m_FXSamplersOld[j];
                        const char* src = sm->m_szName.c_str();
                        char name[128];
                        int n = 0;
                        while (src[n])
                        {
                            if (src[n] <= 0x20 || src[n] == '[')
                            {
                                break;
                            }
                            name[n] = src[n];
                            n++;
                        }
                        name[n] = 0;
                        if (!azstricmp(name, param))
                        {
                            int nSampler = bn->m_RegisterOffset & 0x7f;
                            if (nSampler < MAX_TMU)
                            {
                                samps[nSampler].Sampler = STexSamplerRT(*sm);
                                samps[nSampler].NameTex = sm->m_szTexture;
                                samps[nSampler].Sampler.m_nSamplerSlot = (int8)(bn->m_BindingSlot & 0xff);
                                samps[nSampler].Sampler.m_nTextureSlot = nSampler;

                                for (int k = 0; k < parameters.size(); k++)
                                {
                                    if (parameters[k].m_RegisterOffset & SHADER_BIND_TEXTURE)
                                    {
                                        if (!azstricmp(parameters[k].m_Name.c_str(), param))
                                        {
                                            samps[nSampler].Sampler.m_nTextureSlot = (int8)parameters[k].m_BindingSlot;
                                        }
                                    }
                                }

                                for (int k = 0; k < parameters.size(); k++)
                                {
                                    if (parameters[k].m_RegisterOffset & SHADER_BIND_SAMPLER)
                                    {
                                        if (!azstricmp(parameters[k].m_Name.c_str(), param))
                                        {
                                            samps[nSampler].Sampler.m_nSamplerSlot = (int8)parameters[k].m_BindingSlot;
                                        }
                                    }
                                }

                                // Texture slot occupied, search an alternative
                                if (setSamplers[samps[nSampler].Sampler.m_nSamplerSlot])
                                {
                                    uint32 f = 0; while (setSamplers[f]) ++f;
                                    samps[nSampler].Sampler.m_nSamplerSlot = f;
                                    assert(f < 16);
                                }

                                // Sampler slot occupied, search an alternative
                                if (setTextures[samps[nSampler].Sampler.m_nTextureSlot])
                                {
                                    uint32 f = 0; while (setTextures[f]) ++f;
                                    samps[nSampler].Sampler.m_nTextureSlot = f;
                                    assert(f < 256);
                                }

                                setTextures[samps[nSampler].Sampler.m_nTextureSlot] = true;
                                setSamplers[samps[nSampler].Sampler.m_nSamplerSlot] = true;

                                for (int nS = 0; nS < bn->m_RegisterCount; nS++)
                                {
                                    nMaxSampler = max(nSampler + nS, nMaxSampler);
                                    samps[nSampler + nS].Sampler = samps[nSampler].Sampler;
                                    samps[nSampler + nS].NameTex = sm->m_szTexture;
                                }
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    if (nFlags != 1)
    {
        for (i = 0; (int)i <= nMaxSampler; i++)
        {
            STexSamplerRT& smp = samps[i].Sampler;
            smp.m_pTex = gRenDev->m_cEF.mfParseFXTechnique_LoadShaderTexture(&smp, samps[i].NameTex.c_str(), NULL, NULL, i, eCO_NOSET, eCO_NOSET, DEF_TEXARG0, DEF_TEXARG0);
            if (!smp.m_pTex)
            {
                continue;
            }
            if (smp.m_bGlobal)
            {
                mfAddGlobalSampler(smp);
            }
            else
            {
                pInst->m_pSamplers.push_back(smp);
            }
        }
    }
    else
    {
        assert(pInst->m_pAsync);
        if (pInst->m_pAsync && nMaxSampler >= 0)
        {
            pInst->m_pAsync->m_bPendedSamplers = true;
        }
    }

    pInst->m_nMaxVecs[0] = pInst->m_nMaxVecs[1] = 0;
    if (parameters.size())
    {
        for (i = 0; i < parameters.size(); i++)
        {
            SCGBind* pB = &parameters[i];
            if (pB->m_RegisterOffset & (SHADER_BIND_SAMPLER | SHADER_BIND_TEXTURE))
            {
                continue;
            }
            if (pB->m_BindingSlot < 0 || pB->m_BindingSlot > 2)
            {
                continue;
            }
            for (j = 0; j < Group.Params[0].size(); j++)
            {
                SCGParam* pr = &Group.Params[0][j];
                if (pr->m_RegisterOffset == pB->m_RegisterOffset && pr->m_Name == pB->m_Name)
                {
                    break;
                }
            }
            if (j != Group.Params[0].size())
            {
                continue;
            }
            if (pB->m_BindingSlot < 3)
            {
                pInst->m_nMaxVecs[pB->m_BindingSlot] = max(pB->m_RegisterOffset + pB->m_RegisterCount, pInst->m_nMaxVecs[pB->m_BindingSlot]);
            }
        }
    }
    if (Group.Params[0].size())
    {
        for (i = 0; i < Group.Params[0].size(); i++)
        {
            SCGParam* pr = &Group.Params[0][i];

            if (pr->m_Flags & PF_MATERIAL)
            {
                pInst->m_bHasPMParams = true;
            }
        }

        gRenDev->m_cEF.mfCheckObjectDependParams(Group.Params[0], Group.Params[1], pSH->m_eSHClass, pFXShader);
    }

    for (i = 0; i < 2; i++)
    {
        if (Group.Params[i].size())
        {
            for (j = 0; j < Group.Params[i].size(); j++)
            {
                const SCGParam* pr = &Group.Params[i][j];
                pInst->m_nMaxVecs[i] = max(pr->m_RegisterOffset + pr->m_RegisterCount, pInst->m_nMaxVecs[i]);
            }
        }
    }
#if !defined(NDEBUG)
    int nMax = AzRHI::GetConstantRegisterCountMax(pSH->m_eSHClass);
    assert(pInst->m_nMaxVecs[0] < nMax);
    assert(pInst->m_nMaxVecs[1] < nMax);
#endif

    if ((pInst->m_Ident.m_RTMask & g_HWSR_MaskBit[HWSR_INSTANCING_ATTR]) && pSH->m_eSHClass == eHWSC_Vertex)
    {
        int nNumInst = 0;
        if (InstBindVars)
        {
            for (i = 0; i < (uint32)InstBindVars->size(); i++)
            {
                SCGBind& b = (*InstBindVars)[i];
                int nID = b.m_RegisterOffset;
                if (!nNumInst)
                {
                    pInst->m_nInstMatrixID = nID;
                }

                SCGBind bn;
                bn.m_RegisterCount = b.m_RegisterCount;
                bn.m_RegisterOffset = nID;

                nNumInst++;
            }
        }
        //assert(cgi->m_nNumInstAttributes == nNumInst);
        pInst->m_nNumInstAttributes = nNumInst;

        if (Group.Params_Inst.size())
        {
            qsort(&Group.Params_Inst[0], Group.Params_Inst.size(), sizeof(SCGParam), CGBindCallback);
            pInst->m_nParams_Inst = CGParamManager::GetParametersGroup(Group, 2);
        }
    }
    if (Group.Params[0].size() > 0)
    {
        qsort(&Group.Params[0][0], Group.Params[0].size(), sizeof(SCGParam), CGBindCallback);
        pInst->m_nParams[0] = CGParamManager::GetParametersGroup(Group, 0);
    }
    if (Group.Params[1].size() > 0)
    {
        qsort(&Group.Params[1][0], Group.Params[1].size(), sizeof(SCGParam), CGBindCallback);
        pInst->m_nParams[1] = CGParamManager::GetParametersGroup(Group, 1);
    }
}

// Vertex shader specific
void CHWShader_D3D::mfUpdateFXVertexFormat([[maybe_unused]] SHWSInstance* pInst, CShader* pSH)
{
    // Update global FX shader's vertex format / flags
    if (pSH)
    {
        AZ::Vertex::Format vertexFormat = pSH->m_vertexFormat;
        bool bCurrent = false;
        for (uint32 i = 0; i < pSH->m_HWTechniques.Num(); i++)
        {
            SShaderTechnique* hw = pSH->m_HWTechniques[i];
            for (uint32 j = 0; j < hw->m_Passes.Num(); j++)
            {
                SShaderPass* pass = &hw->m_Passes[j];
                if (pass->m_VShader)
                {
                    if (pass->m_VShader == this)
                    {
                        bCurrent = true;
                    }
                    bool bUseLM = false;
                    bool bUseTangs = false;
                    bool bUseHWSkin = false;
                    bool bUseVertexVelocity = false;
                    AZ::Vertex::Format currentVertexFormat = pass->m_VShader->mfVertexFormat(bUseTangs, bUseLM, bUseHWSkin, bUseVertexVelocity);
                    if (currentVertexFormat > vertexFormat)
                    {
                        vertexFormat = currentVertexFormat;
                    }
                    if (bUseTangs)
                    {
                        pass->m_PassFlags |= VSM_TANGENTS;
                    }
                    if (bUseHWSkin)
                    {
                        pass->m_PassFlags |= VSM_HWSKIN;
                    }
                    if (bUseVertexVelocity)
                    {
                        pass->m_PassFlags |= VSM_VERTEX_VELOCITY;
                    }
                }
            }
        }
        //assert (bCurrent);
        pSH->m_vertexFormat = vertexFormat;
    }
}

void CHWShader_D3D::mfPostVertexFormat(SHWSInstance* pInst, [[maybe_unused]] CHWShader_D3D* pHWSH, bool bCol, byte bNormal, bool bTC0, bool bTC1, bool bPSize, bool bTangent[2], bool bBitangent[2], bool bHWSkin, [[maybe_unused]] bool bSH[2], bool bVelocity, bool bMorph)
{
    if (bBitangent[0])
    {
        pInst->m_VStreamMask_Decl |= 1 << VSF_TANGENTS;
    }
    else
    if (bTangent[0])
    {
        pInst->m_VStreamMask_Decl |= 1 << VSF_QTANGENTS;
    }
    if (bBitangent[1])
    {
        pInst->m_VStreamMask_Stream |= 1 << VSF_TANGENTS;
    }
    else
    if (bTangent[1])
    {
        pInst->m_VStreamMask_Stream |= 1 << VSF_QTANGENTS;
    }
    if (pInst->m_VStreamMask_Decl & (1 << VSF_TANGENTS))
    {
        bNormal = false;
    }

    if (bHWSkin)
    {
        pInst->m_VStreamMask_Decl |= VSM_HWSKIN;
        pInst->m_VStreamMask_Stream |= VSM_HWSKIN;
    }

    if (bVelocity)
    {
        pInst->m_VStreamMask_Decl |= VSM_VERTEX_VELOCITY;
        pInst->m_VStreamMask_Stream |= VSM_VERTEX_VELOCITY;
    }
    if (bMorph)
    {
        pInst->m_VStreamMask_Decl |= VSM_MORPHBUDDY;
        pInst->m_VStreamMask_Stream |= VSM_MORPHBUDDY;
    }

    pInst->m_vertexFormat = AZ::Vertex::VertFormatForComponents(bCol, bTC0, bTC1, bPSize, bNormal != 0);
}

AZ::Vertex::Format CHWShader_D3D::mfVertexFormat(bool& bUseTangents, bool& bUseLM, bool& bUseHWSkin, bool& bUseVertexVelocity)
{
    uint32 i;

    assert (m_eSHClass == eHWSC_Vertex);

    AZ::Vertex::Format vertexFormat(eVF_Unknown);
    int nStream = 0;
    for (i = 0; i < m_Insts.size(); i++)
    {
        SHWSInstance* pInst = m_Insts[i];
        if (pInst->m_vertexFormat > vertexFormat)
        {
            vertexFormat = pInst->m_vertexFormat;
        }
        nStream |= pInst->m_VStreamMask_Stream;
    }
    bUseTangents = (nStream & VSM_TANGENTS) != 0;
    bUseLM = false;
    bUseHWSkin = (nStream & VSM_HWSKIN) != 0;
    bUseVertexVelocity = (nStream & VSM_VERTEX_VELOCITY) != 0;
    return vertexFormat;
}

AZ::Vertex::Format CHWShader_D3D::mfVertexFormat(SHWSInstance* pInst, CHWShader_D3D* pSH, LPD3D10BLOB pShader)
{
    assert (pSH->m_eSHClass == eHWSC_Vertex);

    byte bNormal = false;
    bool bTangent[2] = {false, false};
    bool bBitangent[2] = {false, false};
    bool bHWSkin = false;
    bool bVelocity = false;
    bool bMorph = false;
    bool bPSize = false;
    bool bSH[2] = {false, false};
    bool bTC0 = false;
    bool bTC1[2] = {false, false};
    bool bCol = false;
    bool bSecCol = false;
    bool bPos = false;

    size_t nSize = pShader->GetBufferSize();
    void* pData = pShader->GetBufferPointer();
    void* pShaderReflBuf;
    HRESULT hr = D3DReflect(pData, nSize, IID_ID3D11ShaderReflection, &pShaderReflBuf);
    assert (SUCCEEDED(hr));
    ID3D11ShaderReflection* pShaderReflection = (ID3D11ShaderReflection*)pShaderReflBuf;
    if (!SUCCEEDED(hr))
    {
        return AZ::Vertex::Format(eVF_Unknown);
    }
    D3D11_SHADER_DESC Desc;
    pShaderReflection->GetDesc(&Desc);
    if (!Desc.InputParameters)
    {
        return AZ::Vertex::Format(eVF_Unknown);
    }
    D3D11_SIGNATURE_PARAMETER_DESC IDesc;
    for (uint32 i = 0; i < Desc.InputParameters; i++)
    {
        pShaderReflection->GetInputParameterDesc(i, &IDesc);
        //if (!IDesc.ReadWriteMask)
        //  continue;
        if (!IDesc.SemanticName)
        {
            continue;
        }
        int nIndex;
        if (!_strnicmp(IDesc.SemanticName, "POSITION", 8)
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DHWSHADERCOMPILING_CPP_SECTION_2
    #include AZ_RESTRICTED_FILE(D3DHWShaderCompiling_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
            || !_strnicmp(IDesc.SemanticName, "SV_POSITION", 11)
#endif
            )
        {
            nIndex = IDesc.SemanticIndex;
            if (nIndex == 0)
            {
                bPos = true;
            }
            else
            if (nIndex == 3)
            {
                bVelocity = true;
            }
            else
            if (nIndex == 4)
            {
                bHWSkin = true;
            }
            else
            if (nIndex == 8)
            {
                bMorph = true;
            }
            else
            {
                assert(false);
            }
        }
        else
        if (!_strnicmp(IDesc.SemanticName, "NORMAL", 6))
        {
            bNormal = true;
        }
        else
        if (!_strnicmp(IDesc.SemanticName, "TEXCOORD", 8))
        {
            nIndex = IDesc.SemanticIndex;
            if (nIndex == 0)
            {
                bTC0 = true;
            }
            else
            if (!(pInst->m_Ident.m_RTMask & g_HWSR_MaskBit[HWSR_INSTANCING_ATTR]))
            {
                if (nIndex == 1)
                {
                    bTC1[0] = true;
                    if (IDesc.ReadWriteMask)
                    {
                        bTC1[1] = true;
                    }
                }
                else
                if (nIndex == 8)
                {
                    bMorph = true;
                }
            }
        }
        else
        if (!_strnicmp(IDesc.SemanticName, "COLOR", 5))
        {
            nIndex = IDesc.SemanticIndex;
            if (nIndex == 0)
            {
                bCol = true;
            }
            else
            if (nIndex == 1)
            {
                bSecCol = true;
            }
            else
            {
                if (nIndex == 2 || nIndex == 3)
                {
                    bSH[0] = true;
                    if (IDesc.ReadWriteMask)
                    {
                        bSH[1] = true;
                    }
                }
                else
                {
                    assert(false);
                }
            }
        }
        else
        if (!azstricmp(IDesc.SemanticName, "TANGENT"))
        {
            bTangent[0] = true;
            if (IDesc.ReadWriteMask)
            {
                bTangent[1] = true;
            }
        }
        else
        if (!azstricmp(IDesc.SemanticName, "BITANGENT") || !azstricmp(IDesc.SemanticName, "BINORMAL"))
        {
            bBitangent[0] = true;
            if (IDesc.ReadWriteMask)
            {
                bBitangent[1] = true;
            }
        }
        else
        if (!_strnicmp(IDesc.SemanticName, "PSIZE", 5))
        {
            bPSize = true;
        }
        else
        if (!_strnicmp(IDesc.SemanticName, "BLENDWEIGHT", 11) || !_strnicmp(IDesc.SemanticName, "BLENDINDICES", 12))
        {
            nIndex = IDesc.SemanticIndex;
            if (nIndex == 0)
            {
                bHWSkin = true;
            }
            else
            if (nIndex == 1)
            {
                bMorph = true;
            }
            else
            {
                assert(0);
            }
        }
        else
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DHWSHADERCOMPILING_CPP_SECTION_3
    #include AZ_RESTRICTED_FILE(D3DHWShaderCompiling_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
        if (!_strnicmp(IDesc.SemanticName, "SV_", 3))
#endif
        {
            // SV_ are valid semantics
        }
        else
        {
            assert(0);
        }
    }
    mfPostVertexFormat(pInst, pSH, bCol, bNormal, bTC0, bTC1[0], bPSize, bTangent, bBitangent, bHWSkin, bSH, bVelocity, bMorph);
    SAFE_RELEASE(pShaderReflection);

    return pInst->m_vertexFormat;
}

void CHWShader_D3D::mfSetDefaultRT(uint64& nAndMask, uint64& nOrMask)
{
    uint32 i, j;
    SShaderGen* pGen = gRenDev->m_cEF.m_pGlobalExt;

    uint32 nBitsPlatform = 0;
    if (CParserBin::m_nPlatform == SF_ORBIS)
    {
        nBitsPlatform |= SHGD_HW_ORBIS;
    }
    else
    if (CParserBin::m_nPlatform == SF_D3D11)
    {
        nBitsPlatform |= SHGD_HW_DX11;
    }
    else
    if (CParserBin::m_nPlatform == SF_GL4)
    {
        nBitsPlatform |= SHGD_HW_GL4;
    }
    else
    if (CParserBin::m_nPlatform == SF_GLES3)
    {
        nBitsPlatform |= SHGD_HW_GLES3;
    }
    // Confetti Nicholas Baldwin: adding metal shader language support
    else
    if (CParserBin::m_nPlatform == SF_METAL)
    {
        nBitsPlatform |= SHGD_HW_METAL;
    }

    // Make a mask of flags affected by this type of shader
    uint32 nType = m_dwShaderType;
    if (nType)
    {
        for (i = 0; i < pGen->m_BitMask.size(); i++)
        {
            SShaderGenBit* pBit = pGen->m_BitMask[i];
            if (!pBit->m_Mask)
            {
                continue;
            }
            if (nBitsPlatform & pBit->m_nDependencyReset)
            {
                nAndMask &= ~pBit->m_Mask;
                continue;
            }
            for (j = 0; j < pBit->m_PrecacheNames.size(); j++)
            {
                if (pBit->m_PrecacheNames[j] == nType)
                {
                    if (nBitsPlatform & pBit->m_nDependencySet)
                    {
                        nOrMask |= pBit->m_Mask;
                    }
                    break;
                }
            }
        }
    }
}

//==================================================================================================================

void CHWShader::mfValidateTokenData([[maybe_unused]] CResFile* pRes)
{
#ifdef _DEBUG
    if (pRes == 0)
    {
        return;
    }

    bool bTokenValid = true;
    ResDir* Dir = pRes->mfGetDirectory();
    for (unsigned int i = 0; i < Dir->size(); i++)
    {
        SDirEntry* pDE = &(*Dir)[i];
        if (pDE->flags & RF_RES_$TOKENS)
        {
            uint32 nSize = pRes->mfFileRead(pDE);
            byte* pData = (byte*)pRes->mfFileGetBuf(pDE);
            if (!pData)
            {
                bTokenValid = false;
                break;
            }

            uint32 nL = *(uint32*)pData;
            if (CParserBin::m_bEndians)
            {
                SwapEndian(nL, eBigEndian);
            }

            if (nL * sizeof(uint32) > nSize)
            {
                bTokenValid = false;
                break;
            }

            int nTableSize = nSize - (4 + nL * sizeof(uint32));
            if (nTableSize < 0)
            {
                bTokenValid = false;
                break;
            }

            pRes->mfCloseEntry(pDE);
        }
    }

    if (!bTokenValid)
    {
        CryFatalError("Invalid token data in shader cache file");
    }
#endif
}

bool CHWShader_D3D::mfStoreCacheTokenMap(FXShaderToken*& Table, TArray<uint32>*& pSHData, const char* szName)
{
    TArray<byte> Data;

    FXShaderTokenItor itor;
    uint32 nSize = pSHData->size();
    if (CParserBin::m_bEndians)
    {
        uint32 nSizeEnd = nSize;
        SwapEndian(nSizeEnd, eBigEndian);
        Data.Copy((byte*)&nSizeEnd, sizeof(uint32));
        for (uint32 i = 0; i < nSize; i++)
        {
            uint32 nToken = (*pSHData)[i];
            SwapEndian(nToken, eBigEndian);
            Data.Copy((byte*)&nToken, sizeof(uint32));
        }
    }
    else
    {
        Data.Copy((byte*)&nSize, sizeof(uint32));
        Data.Copy((byte*)&(*pSHData)[0], nSize * sizeof(uint32));
    }
    for (itor = Table->begin(); itor != Table->end(); itor++)
    {
        STokenD T = *itor;
        if (CParserBin::m_bEndians)
        {
            SwapEndian(T.Token, eBigEndian);
        }
        Data.Copy((byte*)&T.Token, sizeof(DWORD));
        Data.Copy((byte*)T.SToken.c_str(), T.SToken.size() + 1);
    }
    if (!Data.size())
    {
        return false;
    }
    SDirEntry de;
    de.Name = szName;
    de.flags = RF_RES_$TOKENS;
    de.size = Data.size();
    m_pGlobalCache->m_pRes[CACHE_USER]->mfFileAdd(&de);
    SDirEntryOpen* pOE = m_pGlobalCache->m_pRes[CACHE_USER]->mfOpenEntry(&de);
    pOE->pData = &Data[0];
    m_pGlobalCache->m_pRes[CACHE_USER]->mfFlush();
    m_pGlobalCache->m_pRes[CACHE_USER]->mfCloseEntry(&de);

    return true;
}

void CHWShader_D3D::mfGetTokenMap(CResFile* pRes, SDirEntry* pDE, FXShaderToken*& Table, TArray<uint32>*& pSHData)
{
    uint32 i;
    int nSize = pRes->mfFileRead(pDE);
    byte* pData = (byte*)pRes->mfFileGetBuf(pDE);
    if (!pData)
    {
        Table = NULL;
        return;
    }
    Table = new FXShaderToken;
    pSHData = new TArray<uint32>;
    uint32 nL = *(uint32*)pData;
    if (CParserBin::m_bEndians)
    {
        SwapEndian(nL, eBigEndian);
    }
    pSHData->resize(nL);
    if (CParserBin::m_bEndians)
    {
        uint32* pTokens = (uint32*)&pData[4];
        for (i = 0; i < nL; i++)
        {
            uint32 nToken = pTokens[i];
            SwapEndian(nToken, eBigEndian);
            (*pSHData)[i] = nToken;
        }
    }
    else
    {
        memcpy(&(*pSHData)[0], &pData[4], nL * sizeof(uint32));
    }
    pData += 4 + nL * sizeof(uint32);
    nSize -= 4 + nL * sizeof(uint32);
    int nOffs = 0;

    while (nOffs < nSize)
    {
        char* pStr = (char*)&pData[nOffs + sizeof(DWORD)];
        DWORD nToken;
        LoadUnaligned(pData + nOffs, nToken);
        if (CParserBin::m_bEndians)
        {
            SwapEndian(nToken, eBigEndian);
        }
        int nLen = strlen(pStr) + 1;
        STokenD TD;
        TD.Token = nToken;
        TD.SToken = pStr;
        Table->push_back(TD);
        nOffs += sizeof(DWORD) + nLen;
    }
}

bool CHWShader_D3D::mfGetCacheTokenMap(FXShaderToken*& Table, TArray<uint32>*& pSHData, uint64 nMaskGenFX)
{
    if (!m_pGlobalCache || !m_pGlobalCache->isValid())
    {
        if (m_pGlobalCache)
        {
            m_pGlobalCache->Release(false);
        }
        m_pGlobalCache = mfInitCache(NULL, this, true, m_CRC32, true, CRenderer::CV_r_shadersasyncactivation != 0);
    }
    if (!m_pGlobalCache)
    {
        assert(false);
        return false;
    }

    char strName[256];
    sprintf_s(strName, "$MAP_%llx_%llx", nMaskGenFX, m_maskGenStatic);

    if (Table)
    {
        if (m_pGlobalCache->m_pRes[CACHE_READONLY] && m_pGlobalCache->m_pRes[CACHE_READONLY]->mfFileExist(strName))
        {
            return true;
        }
        if (!m_pGlobalCache->m_pRes[CACHE_USER])
        {
            m_pGlobalCache->Release(false);
            m_pGlobalCache = mfInitCache(NULL, this, true, m_CRC32, false);
        }
        if (!m_pGlobalCache || !m_pGlobalCache->m_pRes[CACHE_USER])
        {
            assert(false);
            return false;
        }
        if (!m_pGlobalCache->m_pRes[CACHE_USER]->mfFileExist(strName))
        {
            if (!CRenderer::CV_r_shadersAllowCompilation)
            {
                return false;
            }

            //CryLogAlways("Storing MAP entry '%s' in shader cache file '%s'", strName, m_pGlobalCache->m_Name.c_str());

            return mfStoreCacheTokenMap(Table, pSHData, strName);
        }
        return true;
    }
    SDirEntry* pDE = NULL;
    CResFile* pRes = NULL;
    for (int i = 0; i < 2; i++)
    {
        pRes = m_pGlobalCache->m_pRes[i];
        if (!pRes)
        {
            continue;
        }
        pDE = pRes->mfGetEntry(strName);
        if (pDE)
        {
            break;
        }
    }
    if (!pDE || !pRes)
    {
        Warning("Couldn't find tokens MAP entry '%s' in shader cache file '%s'", strName, m_pGlobalCache->m_Name.c_str());
        ASSERT_IN_SHADER(0);
        return false;
    }
    mfGetTokenMap(pRes, pDE, Table, pSHData);
    pRes->mfFileClose(pDE);

    return true;
}

//==============================================================================================================================================================

bool CHWShader_D3D::mfGenerateScript(CShader* pSH, SHWSInstance*& pInst, std::vector<SCGBind>& InstBindVars, uint32 nFlags, FXShaderToken* Table, TArray<uint32>* pSHData, TArray<char>& sNewScr)
{

    bool bTempMap = (Table == NULL);
    assert((Table && pSHData) || (!Table && !pSHData));
    assert (m_pGlobalCache);
    if (CParserBin::m_bEditable && !Table) // Fast path for offline shaders builder
    {
        Table = &m_TokenTable;
        pSHData = &m_TokenData;
        bTempMap = false;
    }
    else
    {
        if (m_pGlobalCache)
        {
            mfGetCacheTokenMap(Table, pSHData, m_nMaskGenShader);
        }
        if (CParserBin::m_bEditable)
        {
            if (bTempMap)
            {
                SAFE_DELETE(Table);
                SAFE_DELETE(pSHData);
            }
            Table = &m_TokenTable;
            pSHData = &m_TokenData;
            bTempMap = false;
        }
    }
    assert (Table && pSHData);
    if (!Table || !pSHData)
    {
        return false;
    }

    ShaderTokensVec NewTokens;

    uint32 eT = eT_unknown;

    switch (pInst->m_eClass)
    {
    case eHWSC_Vertex:
        eT = eT__VS;
        break;
    case eHWSC_Pixel:
        eT = eT__PS;
        break;
    case eHWSC_Geometry:
        eT = eT__GS;
        break;
    case eHWSC_Hull:
        eT = eT__HS;
        break;
    case eHWSC_Compute:
        eT = eT__CS;
        break;
    case eHWSC_Domain:
        eT = eT__DS;
        break;

    default:
        assert(0);
    }
    if (eT != eT_unknown)
    {
        CParserBin::AddDefineToken(eT, NewTokens);
    }

    // Include runtime mask definitions in the script
    SShaderGen* shg = gRenDev->m_cEF.m_pGlobalExt;
    if (shg && pInst->m_Ident.m_RTMask)
    {
        for (uint32 i = 0; i < shg->m_BitMask.Num(); i++)
        {
            SShaderGenBit* bit = shg->m_BitMask[i];
            if (!(bit->m_Mask & pInst->m_Ident.m_RTMask))
            {
                continue;
            }
            CParserBin::AddDefineToken(bit->m_dwToken, NewTokens);
        }
    }

    // Include light mask definitions in the script
    if (m_Flags & HWSG_SUPPORTS_MULTILIGHTS)
    {
        int nLights = pInst->m_Ident.m_LightMask & 0xf;
        if (nLights)
        {
            CParserBin::AddDefineToken(eT__LT_LIGHTS, NewTokens);
        }
        CParserBin::AddDefineToken(eT__LT_NUM, nLights + eT_0, NewTokens);
        bool bHasProj = false;
        for (int i = 0; i < 4; i++)
        {
            int nLightType = (pInst->m_Ident.m_LightMask >> (SLMF_LTYPE_SHIFT + i * SLMF_LTYPE_BITS)) & SLMF_TYPE_MASK;
            if (nLightType == SLMF_PROJECTED)
            {
                bHasProj = true;
            }

            CParserBin::AddDefineToken(eT__LT_0_TYPE + i, nLightType + eT_0, NewTokens);
        }
        if (bHasProj)
        {
            CParserBin::AddDefineToken(eT__LT_HASPROJ, eT_1, NewTokens);
        }
    }
    else
    if (m_Flags & HWSG_SUPPORTS_LIGHTING)
    {
        CParserBin::AddDefineToken(eT__LT_LIGHTS, NewTokens);
        int nLightType = (pInst->m_Ident.m_LightMask >> SLMF_LTYPE_SHIFT) & SLMF_TYPE_MASK;
        if (nLightType == SLMF_PROJECTED)
        {
            CParserBin::AddDefineToken(eT__LT_HASPROJ, eT_1, NewTokens);
        }
    }

    // Include modificator mask definitions in the script
    if ((m_Flags & HWSG_SUPPORTS_MODIF) && pInst->m_Ident.m_MDMask)
    {
        const uint32 tcProjMask = HWMD_TEXCOORD_PROJ;
        const uint32 tcMatrixMask   = HWMD_TEXCOORD_MATRIX;

        if (pInst->m_Ident.m_MDMask & tcProjMask)
        {
            CParserBin::AddDefineToken(eT__TT_TEXCOORD_PROJ, NewTokens);
        }
        if (pInst->m_Ident.m_MDMask & tcMatrixMask)
        {
            CParserBin::AddDefineToken(eT__TT_TEXCOORD_MATRIX, NewTokens);
        }
        if (pInst->m_Ident.m_MDMask & HWMD_TEXCOORD_GEN_OBJECT_LINEAR_DIFFUSE)
        {
            CParserBin::AddDefineToken(eT__TT_TEXCOORD_GEN_OBJECT_LINEAR_DIFFUSE, NewTokens);
        }
        if (pInst->m_Ident.m_MDMask & HWMD_TEXCOORD_GEN_OBJECT_LINEAR_EMITTANCE)
        {
            CParserBin::AddDefineToken(eT__TT_TEXCOORD_GEN_OBJECT_LINEAR_EMITTANCE, NewTokens);
        }
        if (pInst->m_Ident.m_MDMask & HWMD_TEXCOORD_GEN_OBJECT_LINEAR_EMITTANCE_MULT)
        {
            CParserBin::AddDefineToken(eT__TT_TEXCOORD_GEN_OBJECT_LINEAR_EMITTANCE_MULT, NewTokens);
        }
        if (pInst->m_Ident.m_MDMask & HWMD_TEXCOORD_GEN_OBJECT_LINEAR_DETAIL)
        {
            CParserBin::AddDefineToken(eT__TT_TEXCOORD_GEN_OBJECT_LINEAR_DETAIL, NewTokens);
        }
        if (pInst->m_Ident.m_MDMask & HWMD_TEXCOORD_GEN_OBJECT_LINEAR_CUSTOM)
        {
            CParserBin::AddDefineToken(eT__TT_TEXCOORD_GEN_OBJECT_LINEAR_CUSTOM, NewTokens);
        }
    }

    // Include vertex modificator mask definitions in the script
    if ((m_Flags & HWSG_SUPPORTS_VMODIF) && pInst->m_Ident.m_MDVMask)
    {
        int nMDV = pInst->m_Ident.m_MDVMask & 0x0fffffff;
        int nType = nMDV & 0xf;
        if (nType)
        {
            CParserBin::AddDefineToken(eT__VT_TYPE, eT_0 + nType, NewTokens);
        }
        if ((nMDV & MDV_BENDING) || nType == eDT_Bending)
        {
            CParserBin::AddDefineToken(eT__VT_BEND, eT_1, NewTokens);
            if (!(nMDV & 0xf))
            {
                nType = eDT_Bending;
                CParserBin::AddDefineToken(eT__VT_TYPE, eT_0 + nType, NewTokens);
            }
        }
        if (nMDV & MDV_DEPTH_OFFSET)
        {
            CParserBin::AddDefineToken(eT__VT_DEPTH_OFFSET, eT_1, NewTokens);
        }
        if (nMDV & MDV_WIND)
        {
            CParserBin::AddDefineToken(eT__VT_WIND, eT_1, NewTokens);
        }
        if (nMDV & MDV_DET_BENDING)
        {
            CParserBin::AddDefineToken(eT__VT_DET_BEND, eT_1, NewTokens);
        }
        if (nMDV & MDV_DET_BENDING_GRASS)
        {
            CParserBin::AddDefineToken(eT__VT_GRASS, eT_1, NewTokens);
        }
        if (nMDV & ~0xf)
        {
            CParserBin::AddDefineToken(eT__VT_TYPE_MODIF, eT_1, NewTokens);
        }
    }

    if (m_Flags & HWSG_FP_EMULATION)
    {
        //starting at LSB. 8 bits for colorop, 8 bits for alphaop, 3 bits for color arg1, 3 bits for color arg2, 1 bit for srgbwrite, 1 bit unused, 3 bits for
        //  alpha arg1, 3 bits for alpha arg2, 2 bits unused.
        CParserBin::AddDefineToken(eT__FT0_COP, eT_0 + (pInst->m_Ident.m_LightMask & 0xff), NewTokens);
        CParserBin::AddDefineToken(eT__FT0_AOP, eT_0 + ((pInst->m_Ident.m_LightMask & 0xff00) >> 8), NewTokens);

        byte CO_0 = ((pInst->m_Ident.m_LightMask & 0xff0000) >> 16) & 7;
        CParserBin::AddDefineToken(eT__FT0_CARG1, eT_0 + CO_0, NewTokens);

        byte CO_1 = ((pInst->m_Ident.m_LightMask & 0xff0000) >> 19) & 7;
        CParserBin::AddDefineToken(eT__FT0_CARG2, eT_0 + CO_1, NewTokens);

        byte AO_0 = ((pInst->m_Ident.m_LightMask & 0xff000000) >> 24) & 7;
        CParserBin::AddDefineToken(eT__FT0_AARG1, eT_0 + AO_0, NewTokens);

        byte AO_1 = ((pInst->m_Ident.m_LightMask & 0xff000000) >> 27) & 7;
        CParserBin::AddDefineToken(eT__FT0_AARG2, eT_0 + AO_1, NewTokens);

        if (CO_0 == eCA_Specular || CO_1 == eCA_Specular || AO_0 == eCA_Specular || AO_1 == eCA_Specular)
        {
            CParserBin::AddDefineToken(eT__FT_SPECULAR, NewTokens);
        }
        if (CO_0 == eCA_Diffuse || CO_1 == eCA_Diffuse || AO_0 == eCA_Diffuse || AO_1 == eCA_Diffuse)
        {
            CParserBin::AddDefineToken(eT__FT_DIFFUSE, NewTokens);
        }
        if (CO_0 == eCA_Texture || CO_1 == eCA_Texture || AO_0 == eCA_Texture || AO_1 == eCA_Texture)
        {
            CParserBin::AddDefineToken(eT__FT_TEXTURE, NewTokens);
        }
        if (CO_0 == eCA_Texture1 || (CO_1 == eCA_Texture1) || AO_0 == eCA_Texture1 || AO_1 == eCA_Texture1
            || CO_0 == eCA_Previous || (CO_1 == eCA_Previous) || AO_0 == eCA_Previous || AO_1 == eCA_Previous)
        {
            CParserBin::AddDefineToken(eT__FT_TEXTURE1, NewTokens);
        }
        if (CO_0 == eCA_Normal || CO_1 == eCA_Normal || AO_0 == eCA_Normal || AO_1 == eCA_Normal)
        {
            CParserBin::AddDefineToken(eT__FT_NORMAL, NewTokens);
        }
        if (CO_0 == eCA_Constant || (CO_1 == eCA_Constant) || AO_0 == eCA_Constant || AO_1 == eCA_Constant
            || CO_0 == eCA_Previous || (CO_1 == eCA_Previous) || AO_0 == eCA_Previous || AO_1 == eCA_Previous)
        {
            CParserBin::AddDefineToken(eT__FT_PSIZE, NewTokens);
        }

        byte toSRGB = ((pInst->m_Ident.m_LightMask & 0x400000) >> 22) & 1;
        if (toSRGB)
        {
            CParserBin::AddDefineToken(eT__FT_SRGBWRITE, NewTokens);
        }

        if (nFlags & HWSF_STOREDATA)
        {
            int nStreams = pInst->m_Ident.m_LightMask & 0xff;
            if (nStreams & (1 << VSF_QTANGENTS))
            {
                CParserBin::AddDefineToken(eT__FT_QTANGENT_STREAM, NewTokens);
            }
            if (nStreams & (1 << VSF_TANGENTS))
            {
                CParserBin::AddDefineToken(eT__FT_TANGENT_STREAM, NewTokens);
            }
            if (nStreams & VSM_HWSKIN)
            {
                CParserBin::AddDefineToken(eT__FT_SKIN_STREAM, NewTokens);
            }
#if ENABLE_NORMALSTREAM_SUPPORT
            if (CParserBin::m_nPlatform == SF_D3D11 || CParserBin::m_nPlatform == SF_JASPER || CParserBin::m_nPlatform == SF_ORBIS || CParserBin::m_nPlatform == SF_GL4 ||  CParserBin::m_nPlatform == SF_GLES3)
            {
                if (nStreams & VSM_NORMALS)
                {
                    CParserBin::AddDefineToken(eT__FT_NORMAL, NewTokens);
                }
            }
#endif
            if (nStreams & VSM_VERTEX_VELOCITY)
            {
                CParserBin::AddDefineToken(eT__FT_VERTEX_VELOCITY_STREAM, NewTokens);
            }
        }
    }

    int nT = NewTokens.size();
    NewTokens.resize(nT + pSHData->size());
    memcpy(&NewTokens[nT], &(*pSHData)[0], pSHData->size() * sizeof(uint32));

    CParserBin Parser(NULL, pSH);
    Parser.Preprocess(1, NewTokens, Table);
    CorrectScriptEnums(Parser, pInst, InstBindVars, Table);
    RemoveUnaffectedParameters_D3D10(Parser, pInst, InstBindVars);
    ConvertBinScriptToASCII(Parser, pInst, InstBindVars, Table, sNewScr);

    if (bTempMap)
    {
        SAFE_DELETE(Table);
        SAFE_DELETE(pSHData);
    }

    return sNewScr.Num() && sNewScr[0];
}

void CHWShader_D3D::RemoveUnaffectedParameters_D3D10(CParserBin& Parser, [[maybe_unused]] SHWSInstance* pInst, std::vector<SCGBind>& InstBindVars)
{
    int nPos = Parser.FindToken(0, Parser.m_Tokens.size() - 1, eT_cbuffer);
    while (nPos >= 0)
    {
        uint32 nName = Parser.m_Tokens[nPos + 1];
        if (nName == eT_PER_BATCH || nName == eT_PER_INSTANCE)
        {
            int nPosEnd = Parser.FindToken(nPos + 3, Parser.m_Tokens.size() - 1, eT_br_cv_2);
            assert(nPosEnd >= 0);
            int nPosN = Parser.FindToken(nPos + 1, Parser.m_Tokens.size() - 1, eT_br_cv_1);
            assert(nPosN >= 0);
            nPosN++;
            while (nPosN < nPosEnd)
            {
                uint32 nT = Parser.m_Tokens[nPosN + 1];
                int nPosCode = Parser.FindToken(nPosEnd + 1, Parser.m_Tokens.size() - 1, nT);
                if (nPosCode < 0)
                {
                    assert(nPosN > 0 && nPosN < (int)Parser.m_Tokens.size());
                    if (InstBindVars.size())
                    {
                        size_t i = 0;
                        CCryNameR nm(Parser.GetString(nT));
                        for (; i < InstBindVars.size(); i++)
                        {
                            SCGBind& b = InstBindVars[i];
                            if (b.m_Name == nm)
                            {
                                break;
                            }
                        }
                        if (i == InstBindVars.size())
                        {
                            Parser.m_Tokens[nPosN] = eT_comment;
                        }
                    }
                    else
                    {
                        Parser.m_Tokens[nPosN] = eT_comment;
                    }
                }
                nPosN = Parser.FindToken(nPosN + 2, nPosEnd, eT_semicolumn);
                assert(nPosN >= 0);
                nPosN++;
            }
            nPos = Parser.FindToken(nPosEnd + 1, Parser.m_Tokens.size() - 1, eT_cbuffer);
        }
        else
        {
            nPos = Parser.FindToken(nPos + 2, Parser.m_Tokens.size() - 1, eT_cbuffer);
        }
    }
}

struct SStructData
{
    uint32 m_nName;
    uint32 m_nTCs;
    int m_nPos;
};

void CHWShader_D3D::CorrectScriptEnums(CParserBin& Parser, SHWSInstance* pInst, std::vector<SCGBind>& InstBindVars, FXShaderToken* Table)
{
    // correct enumeration of TEXCOORD# interpolators after preprocessing
    int nCur = 0;
    int nSize = Parser.m_Tokens.size();
    uint32* pTokens = &Parser.m_Tokens[0];
    int nInstParam = 0;
    const uint32 Toks[] = {eT_TEXCOORDN, eT_TEXCOORDN_centroid, eT_unknown};

    std::vector<SStructData> SData;
    uint32 i;
    while (true)
    {
        nCur = Parser.FindToken(nCur, nSize - 1, eT_struct);
        if (nCur < 0)
        {
            break;
        }
        int nLastStr = Parser.FindToken(nCur, nSize - 1, eT_br_cv_2);
        assert(nLastStr >= 0);
        if (nLastStr < 0)
        {
            break;
        }
        bool bNested = false;
        for (i = 0; i < SData.size(); i++)
        {
            SStructData& Data = SData[i];
            Data.m_nPos = Parser.FindToken(nCur, nLastStr, Data.m_nName);
            if (Data.m_nPos > 0)
            {
                bNested = true;
            }
        }
        uint32 nName = pTokens[nCur + 1];
        int n = 0;
        while (nCur < nLastStr)
        {
            int nTN = Parser.FindToken(nCur, nLastStr, Toks);
            if (nTN < 0)
            {
                nCur = nLastStr + 1;
                break;
            }
            int nNested = 0;
            if (bNested)
            {
                for (i = 0; i < SData.size(); i++)
                {
                    SStructData& Data = SData[i];
                    if (Data.m_nPos > 0 && nTN > Data.m_nPos)
                    {
                        nNested += Data.m_nTCs;
                    }
                }
            }
            assert(pTokens[nTN - 1] == eT_colon);
            int nArrSize = 1;
            uint32 nTokName;
            if (pTokens[nTN - 2] == eT_br_sq_2)
            {
                nArrSize = pTokens[nTN - 3] - eT_0;
                if ((unsigned int) nArrSize > 15)
                {
                    const char* szArrSize = Parser.GetString(pTokens[nTN - 3], *Table);
                    nArrSize = szArrSize ? atoi(szArrSize) : 0;
                }
                assert(pTokens[nTN - 4] == eT_br_sq_1);
                //const char *szName = Parser.GetString(pTokens[nTN-5], *Table);
                nTokName = pTokens[nTN - 5];
            }
            else
            {
                uint32 nType = pTokens[nTN - 3];
                assert(nType == eT_float || nType == eT_float2 || nType == eT_float3 || nType == eT_float4 || nType == eT_float4x4 || nType == eT_float3x4 || nType == eT_float2x4 || nType == eT_float3x3 ||
                    nType == eT_half  || nType == eT_half2  || nType == eT_half3  || nType == eT_half4  || nType == eT_half4x4  || nType == eT_half3x4  || nType == eT_half2x4  || nType == eT_half3x3);
                if (nType == eT_float4x4 || nType == eT_half4x4)
                {
                    nArrSize = 4;
                }
                else
                if (nType == eT_float3x4 || nType == eT_float3x3 || nType == eT_half3x4 || nType == eT_half3x3)
                {
                    nArrSize = 3;
                }
                else
                if (nType == eT_float2x4 || nType == eT_half2x4)
                {
                    nArrSize = 2;
                }
                nTokName = pTokens[nTN - 2];
            }
            assert(nArrSize > 0 && nArrSize < 16);

            EToken eT = (pTokens[nTN] == eT_TEXCOORDN) ? eT_TEXCOORD0 : eT_TEXCOORD0_centroid;

            pTokens[nTN] = n + nNested + eT;
            n += nArrSize;
            nCur = nTN + 1;

            if (pInst->m_Ident.m_RTMask & g_HWSR_MaskBit[HWSR_INSTANCING_ATTR])
            {
                const char* szName = Parser.GetString(nTokName, *Table);
                if (!_strnicmp(szName, "Inst", 4))
                {
                    char newName[256];
                    int nm = 0;
                    while (szName[4 + nm] > 0x20 && szName[4 + nm] != '[')
                    {
                        newName[nm] = szName[4 + nm];
                        nm++;
                    }
                    newName[nm++] = 0;

                    SCGBind bn;
                    bn.m_RegisterOffset = nInstParam;
                    bn.m_RegisterCount = nArrSize;
                    bn.m_Name = newName;
                    InstBindVars.push_back(bn);

                    nInstParam += nArrSize;
                }
            }
        }
        SStructData SD;
        SD.m_nName = nName;
        SD.m_nPos = -1;
        SD.m_nTCs = n;
        SData.push_back(SD);
    }
    if (InstBindVars.size())
    {
        qsort(&InstBindVars[0], InstBindVars.size(), sizeof(SCGBind), CGBindCallback);
    }
    pInst->m_nNumInstAttributes = nInstParam;
}

static int sFetchInst(uint32& nCur, uint32* pTokens, [[maybe_unused]] uint32 nT, std::vector<uint32>& Parameter)
{
    while (true)
    {
        uint32 nTok = pTokens[nCur];
        if (nTok != eT_br_rnd_1 && nTok != eT_br_rnd_2 && nTok != eT_comma)
        {
            break;
        }
        nCur++;
    }
    int nC = 0;
    Parameter.push_back(pTokens[nCur]);
    nCur++;
    while (pTokens[nCur] == eT_dot)
    {
        nC = 2;
        Parameter.push_back(pTokens[nCur]);
        Parameter.push_back(pTokens[nCur + 1]);
        nCur += 2;
    }
    return nC;
}

namespace HWShaderD3D {
    static void sCR(TArray<char>& Text, int nLevel)
    {
        Text.AddElem('\n');
        for (int i = 0; i < nLevel; i++)
        {
            Text.AddElem(' ');
            Text.AddElem(' ');
        }
    }
}

bool CHWShader_D3D::ConvertBinScriptToASCII(CParserBin& Parser, [[maybe_unused]] SHWSInstance* pInst, [[maybe_unused]] std::vector<SCGBind>& InstBindVars, FXShaderToken* Table, TArray<char>& Text)
{
    using namespace HWShaderD3D;
    uint32 i;
    bool bRes = true;

    uint32* pTokens = &Parser.m_Tokens[0];
    uint32 nT = Parser.m_Tokens.size();
    int nLevel = 0;
    for (i = 0; i < nT; i++)
    {
        uint32 nToken = pTokens[i];
        if (nToken == 0)
        {
            Text.Copy("\n", 1);
            continue;
        }
        if (nToken == eT_skip)
        {
            i++;
            continue;
        }
        if (nToken == eT_skip_1)
        {
            while (i < nT)
            {
                nToken = pTokens[i];
                if (nToken == eT_skip_2)
                {
                    break;
                }
                i++;
            }
            assert(i < nT);
            continue;
        }
        if (nToken == eT_fetchinst)
        {
            char str[512];
            i++;
            std::vector<uint32> ParamDst, ParamSrc;
            TArray<char> sParamDstFull, sParamDstName, sParamSrc;
            int nDst = sFetchInst(i, &Parser.m_Tokens[0], Parser.m_Tokens.size(), ParamDst);
            assert(Parser.m_Tokens[i] == eT_eq);
            if (Parser.m_Tokens[i] != eT_eq)
            { // Should never happen
                int n = CParserBin::FindToken(i, Parser.m_Tokens.size() - 1, &Parser.m_Tokens[0], eT_semicolumn);
                if (n > 0)
                {
                    i = n + 1;
                }
                continue;
            }
            i++;
            int nSrc = sFetchInst(i, &Parser.m_Tokens[0], Parser.m_Tokens.size(), ParamSrc);
            CParserBin::ConvertToAscii(&ParamDst[0], ParamDst.size(), *Table, sParamDstFull);
            CParserBin::ConvertToAscii(&ParamDst[nDst], 1, *Table, sParamDstName);
            CParserBin::ConvertToAscii(&ParamSrc[nSrc], 1, *Table, sParamSrc);
            assert(strncmp(&sParamSrc[0], "Inst", 4) == 0);

            {
                sParamSrc.Free();
                CParserBin::ConvertToAscii(&ParamSrc[0], ParamSrc.size(), *Table, sParamSrc);
                sprintf_s(str, "%s = %s;\n", &sParamDstFull[0], &sParamSrc[0]);
                Text.Copy(str, strlen(str));
            }
            while (Parser.m_Tokens[i] != eT_semicolumn)
            {
                i++;
            }
            continue;
        }
        const char* szStr = CParserBin::GetString(nToken, *Table, false);
        assert(szStr);
        if (!szStr || !szStr[0])
        {
            assert(0);
            bRes = CParserBin::CorrectScript(pTokens, i, nT, Text);
        }
        else
        {
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DHWSHADERCOMPILING_CPP_SECTION_4
    #include AZ_RESTRICTED_FILE(D3DHWShaderCompiling_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined (_DEBUG)
            int n = 0;
            while (szStr[n])
            {
                char c = szStr[n++];
                bool bASC = isascii(c);
                assert(bASC);
            }
#endif
            if (nToken == eT_semicolumn || nToken == eT_br_cv_1)
            {
                if (nToken == eT_br_cv_1)
                {
                    sCR(Text, nLevel);
                    nLevel++;
                }
                Text.Copy(szStr, strlen(szStr));
                if (nToken == eT_semicolumn)
                {
                    if (i + 1 < nT && pTokens[i + 1] == eT_br_cv_2)
                    {
                        sCR(Text, nLevel - 1);
                    }
                    else
                    {
                        sCR(Text, nLevel);
                    }
                }
                else
                if (i + 1 < nT)
                {
                    if (pTokens[i + 1] < eT_br_rnd_1 || pTokens[i + 1] >= eT_float)
                    {
                        sCR(Text, nLevel);
                    }
                }
            }
            else
            {
                if (i + 1 < nT)
                {
                    if (Text.Num())
                    {
                        char cPrev = Text[Text.Num() - 1];
                        if (!SkipChar((uint8)cPrev) && !SkipChar((uint8)szStr[0]))
                        {
                            Text.AddElem(' ');
                        }
                    }
                }
                Text.Copy(szStr, strlen(szStr));
                if (nToken == eT_br_cv_2)
                {
                    nLevel--;
                    if (i + 1 < nT && pTokens[i + 1] != eT_semicolumn)
                    {
                        sCR(Text, nLevel);
                    }
                }
            }
        }
    }
    Text.AddElem(0);

    return bRes;
}

void CHWShader_D3D::mfGetSrcFileName(char* srcName, int nSize)
{
    if (!srcName || nSize <= 0)
    {
        return;
    }
    if (!m_NameSourceFX.empty())
    {
        cry_strcpy(srcName, nSize, m_NameSourceFX.c_str());
        return;
    }
    cry_strcpy(srcName, nSize, gRenDev->m_cEF.m_HWPath);
    if (m_eSHClass == eHWSC_Vertex)
    {
        cry_strcat(srcName, nSize, "Declarations/CGVShaders/");
    }
    else if (m_eSHClass == eHWSC_Pixel)
    {
        cry_strcat(srcName, nSize, "Declarations/CGPShaders/");
    }
    else
    {
        cry_strcat(srcName, nSize, "Declarations/CGGShaders/");
    }
    cry_strcat(srcName, nSize, GetName());
    cry_strcat(srcName, nSize, ".crycg");
}

void CHWShader_D3D::mfGenName(SHWSInstance* pInst, char* dstname, int nSize, byte bType)
{
    if (bType)
    {
        CHWShader::mfGenName(pInst->m_Ident.m_GLMask, pInst->m_Ident.m_RTMask, pInst->m_Ident.m_LightMask, pInst->m_Ident.m_MDMask, pInst->m_Ident.m_MDVMask, pInst->m_Ident.m_pipelineState.opaque, pInst->m_Ident.m_STMask, pInst->m_eClass, dstname, nSize, bType);
    }
    else
    {
        CHWShader::mfGenName(0, 0, 0, 0, 0, 0, 0, eHWSC_Num, dstname, nSize, bType);
    }
}

void CHWShader_D3D::mfGetDstFileName(SHWSInstance* pInst, CHWShader_D3D* pSH, char* dstname, int nSize, byte bType)
{
    cry_strcpy(dstname, nSize, gRenDev->m_cEF.m_ShadersCache.c_str());

    if (pSH->m_eSHClass == eHWSC_Vertex)
    {
        if (bType == 1 || bType == 4)
        {
            cry_strcat(dstname, nSize, "CGVShaders/Debug/");
        }
        else if (bType == 0)
        {
            cry_strcat(dstname, nSize, "CGVShaders/");
        }
        else if (bType == 2 || bType == 3)
        {
            cry_strcat(dstname, nSize, "CGVShaders/Pending/");
        }
    }
    else if (pSH->m_eSHClass == eHWSC_Pixel)
    {
        if (bType == 1 || bType == 4)
        {
            cry_strcat(dstname, nSize, "CGPShaders/Debug/");
        }
        else if (bType == 0)
        {
            cry_strcat(dstname, nSize, "CGPShaders/");
        }
        else if (bType == 2 || bType == 3)
        {
            cry_strcat(dstname, nSize, "CGPShaders/Pending/");
        }
    }
    else if (GEOMETRYSHADER_SUPPORT && pSH->m_eSHClass == eHWSC_Geometry)
    {
        if (bType == 1 || bType == 4)
        {
            cry_strcat(dstname, nSize, "CGGShaders/Debug/");
        }
        else if (bType == 0)
        {
            cry_strcat(dstname, nSize, "CGGShaders/");
        }
        else if (bType == 2 || bType == 3)
        {
            cry_strcat(dstname, nSize, "CGGShaders/Pending/");
        }
    }
    else if (GEOMETRYSHADER_SUPPORT && pSH->m_eSHClass == eHWSC_Hull)
    {
        if (bType == 1 || bType == 4)
        {
            cry_strcat(dstname, nSize, "CGHShaders/Debug/");
        }
        else if (bType == 0)
        {
            cry_strcat(dstname, nSize, "CGHShaders/");
        }
        else if (bType == 2 || bType == 3)
        {
            cry_strcat(dstname, nSize, "CGHShaders/Pending/");
        }
    }
    else if (GEOMETRYSHADER_SUPPORT && pSH->m_eSHClass == eHWSC_Domain)
    {
        if (bType == 1 || bType == 4)
        {
            cry_strcat(dstname, nSize, "CGDShaders/Debug/");
        }
        else if (bType == 0)
        {
            cry_strcat(dstname, nSize, "CGDShaders/");
        }
        else if (bType == 2 || bType == 3)
        {
            cry_strcat(dstname, nSize, "CGDShaders/Pending/");
        }
    }
    else if (GEOMETRYSHADER_SUPPORT && pSH->m_eSHClass == eHWSC_Compute)
    {
        if (bType == 1 || bType == 4)
        {
            cry_strcat(dstname, nSize, "CGCShaders/Debug/");
        }
        else if (bType == 0)
        {
            cry_strcat(dstname, nSize, "CGCShaders/");
        }
        else if (bType == 2 || bType == 3)
        {
            cry_strcat(dstname, nSize, "CGCShaders/Pending/");
        }
    }

    cry_strcat(dstname, nSize, pSH->GetName());

    if (bType == 2)
    {
        cry_strcat(dstname, nSize, "_out");
    }

    if (bType == 0)
    {
        char* s = strchr(dstname, '(');
        if (s)
        {
            s[0] = 0;
        }
    }

    char szGenName[256];
    mfGenName(pInst, szGenName, 256, bType);

    cry_strcat(dstname, nSize, szGenName);
}

//========================================================================================================
// Binary cache support

SShaderCache::~SShaderCache()
{
    if (m_pStreamInfo)
    {
        CResFile* pRes = m_pStreamInfo->m_pRes;
        if (pRes)
        {
            assert(pRes == m_pRes[0] || pRes == m_pRes[1]);
            assert(!pRes->mfIsDirStreaming());
            //bWarn = pRes->mfIsDirStreaming();
        }
        /*if (m_pStreamInfo->m_EntriesQueue.size())
          bWarn = true;
        if (bWarn)
          Warning("Warning: SShader`Cache::~SShaderCache(): '%s' Streaming tasks is still in progress!: %d", m_Name.c_str(), m_pStreamInfo->m_EntriesQueue.size());*/

        m_pStreamInfo->AbortJobs();
    }

    CHWShader::m_ShaderCache.erase(m_Name);
    SAFE_DELETE(m_pRes[CACHE_USER]);
    SAFE_DELETE(m_pRes[CACHE_READONLY]);
    SAFE_RELEASE(m_pStreamInfo);
}

void SShaderCache::Cleanup()
{
    if (m_pRes[0])
    {
        m_pRes[0]->mfDeactivate(true);
    }
    if (m_pRes[1])
    {
        m_pRes[1]->mfDeactivate(true);
    }
}

bool SShaderCache::isValid()
{
    return ((m_pRes[CACHE_READONLY] || m_pRes[CACHE_USER]) && CParserBin::m_nPlatform == m_nPlatform);
}

int SShaderCache::Size()
{
    int nSize = sizeof(SShaderCache);

    if (m_pRes[0])
    {
        nSize += m_pRes[0]->Size();
    }
    if (m_pRes[1])
    {
        nSize += m_pRes[1]->Size();
    }

    return nSize;
}
int SShaderDevCache::Size()
{
    int nSize = sizeof(SShaderDevCache);

    nSize += m_DeviceShaders.size() * sizeof(SD3DShader);

    return nSize;
}

void SShaderCache::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->AddObject(this, sizeof(*this));
    pSizer->AddObject(m_pRes[0]);
    pSizer->AddObject(m_pRes[1]);
}

void SShaderDevCache::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->AddObject(this, sizeof(*this));
    pSizer->AddObject(m_DeviceShaders);
}

SShaderDevCache* CHWShader::mfInitDevCache(const char* name, [[maybe_unused]] CHWShader* pSH)
{
    CCryNameR cryShaderName(name);
    return new SShaderDevCache(cryShaderName);
}

SShaderCacheHeaderItem* CHWShader_D3D::mfGetCompressedItem([[maybe_unused]] uint32 nFlags, int32& nSize)
{
    SHWSInstance*   pInst = m_pCurInst;
    char            name[128];
    {
        azstrcpy(name, AZ_ARRAY_SIZE(name), GetName());
        char* s = strchr(name, '(');
        if (s)
        {
            s[0] = 0;
        }
    }

    CCryNameTSCRC Name = name;
    FXCompressedShadersItor it = CHWShader::m_CompressedShaders.find(Name);
    if (it == CHWShader::m_CompressedShaders.end())
    {
        return NULL;
    }
    SHWActivatedShader* pAS = it->second;
    assert(pAS);
    if (!pAS)
    {
        return NULL;
    }
    mfGenName(pInst, name, 128, 1);
    Name = name;
    FXCompressedShaderRemapItor itR = pAS->m_Remap.find(Name);
    if (itR == pAS->m_Remap.end())
    {
        return NULL;
    }
    int nDevID = itR->second;
    FXCompressedShaderItor itS = pAS->m_CompressedShaders.find(nDevID);
    if (itS == pAS->m_CompressedShaders.end())
    {
        return NULL;
    }
    SCompressedData& CD = itS->second;
    assert(CD.m_pCompressedShader);
    if (!CD.m_pCompressedShader)
    {
        return NULL;
    }
    byte* pData = new byte[CD.m_nSizeDecompressedShader];
    if (!pData)
    {
        return NULL;
    }
    pInst->m_DeviceObjectID = nDevID;
    SShaderCacheHeaderItem* pIt = (SShaderCacheHeaderItem*)pData;
    if (CParserBin::m_bEndians)
    {
        SwapEndian(*pIt, eBigEndian);
    }
    nSize = CD.m_nSizeDecompressedShader;
    return pIt;
}

SShaderCacheHeaderItem* CHWShader_D3D::mfGetCacheItem(uint32& nFlags, int32& nSize)
{
    LOADING_TIME_PROFILE_SECTION(gEnv->pSystem);
    SHWSInstance* pInst = m_pCurInst;
    byte* pData = NULL;
    nSize = 0;
    if (!m_pGlobalCache || !m_pGlobalCache->isValid())
    {
        return NULL;
    }
    CResFile* rf = NULL;
    SDirEntry* de = NULL;
    int i;
    bool bAsync = false;
    int n = CRenderer::CV_r_shadersAllowCompilation == 0 ? 1 : 2;
    for (i = 0; i < n; i++)
    {
        rf = m_pGlobalCache->m_pRes[i];
        if (!rf)
        {
            continue;
        }
        char name[128];
        mfGenName(pInst, name, 128, 1);
        de = rf->mfGetEntry(name, &bAsync);
        if (de || bAsync)
        {
            break;
        }
    }
    if (de)
    {
        if IsCVarConstAccess(constexpr) (CRenderer::CV_r_shadersdebug == 3 || CRenderer::CV_r_shadersdebug == 4)
        {
            iLog->Log("---Cache: LoadedFromGlobal %s': 0x%x", rf->mfGetFileName(), de->Name.get());
        }
        pInst->m_nCache = i;
        SShaderCacheHeaderItem* pIt = NULL;
        nSize = rf->mfFileRead(de);
        pInst->m_bAsyncActivating = (nSize == -1);
        pData = (byte*)rf->mfFileGetBuf(de);
        if (pData && nSize > 0)
        {
            byte* pD = new byte[nSize];
            memcpy(pD, pData, nSize);
            pIt = (SShaderCacheHeaderItem*)pD;
            if (CParserBin::m_bEndians)
            {
                SwapEndian(*pIt, eBigEndian);
            }
            pInst->m_DeviceObjectID = de->Name.get();
            rf->mfFileClose(de);
        }
        if (i == CACHE_USER)
        {
            nFlags |= HWSG_CACHE_USER;
        }
        return pIt;
    }
    else
    {
        pInst->m_bAsyncActivating = bAsync;
        return NULL;
    }
}

bool CHWShader_D3D::mfAddCacheItem(SShaderCache* pCache, SShaderCacheHeaderItem* pItem, const byte* pData, int nLen, bool bFlush, CCryNameTSCRC Name)
{
    if (!pCache)
    {
        return false;
    }
    if (!pCache->m_pRes[CACHE_USER])
    {
        return false;
    }

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_shadersdebug == 3 || CRenderer::CV_r_shadersdebug == 4)
    {
        iLog->Log("---Cache: StoredToGlobal %s': 0x%x", pCache->m_pRes[CACHE_USER]->mfGetFileName(), Name.get());
    }

    pItem->m_CRC32 = CCrc32::Compute(pData, nLen);
    //CryLog("Size: %d: CRC: %x", nLen, pItem->m_CRC32);

    byte* pNew = new byte[sizeof(SShaderCacheHeaderItem) + nLen];
    SDirEntry de;
    de.offset = 0;
    if (CParserBin::m_bEndians)
    {
        SShaderCacheHeaderItem IT = *pItem;
        SwapEndian(IT, eBigEndian);
        memcpy(pNew, &IT, sizeof(SShaderCacheHeaderItem));
    }
    else
    {
        memcpy(pNew, pItem, sizeof(SShaderCacheHeaderItem));
    }
    memcpy(&pNew[sizeof(SShaderCacheHeaderItem)], pData, nLen);
    de.Name = Name;
    de.flags = RF_COMPRESS | RF_TEMPDATA;
    de.size = nLen + sizeof(SShaderCacheHeaderItem);
    pCache->m_pRes[CACHE_USER]->mfFileAdd(&de);
    SDirEntryOpen* pOE = pCache->m_pRes[CACHE_USER]->mfOpenEntry(&de);
    pOE->pData = pNew;
    if (bFlush)
    {
        pCache->m_pRes[CACHE_USER]->mfFlush();
    }

    return true;
}

SEmptyCombination::Combinations SEmptyCombination::s_Combinations;

bool CHWShader_D3D::mfAddEmptyCombination([[maybe_unused]] CShader* pSH, uint64 nRT, uint64 nGL, uint32 nLT)
{
    CD3D9Renderer* rd = gcpRendD3D;
    SRenderPipeline& RESTRICT_REFERENCE rRP = rd->m_RP;

    SEmptyCombination Comb;
    Comb.nGLNew = m_nMaskGenShader;
    Comb.nRTNew = rRP.m_FlagsShader_RT & m_nMaskAnd_RT | m_nMaskOr_RT;
    Comb.nLTNew = rRP.m_FlagsShader_LT;
    Comb.nGLOrg = nGL;
    Comb.nRTOrg = nRT & m_nMaskAnd_RT | m_nMaskOr_RT;
    Comb.nLTOrg = nLT;
    Comb.nMD = rRP.m_FlagsShader_MD;
    Comb.nMDV = rRP.m_FlagsShader_MDV;
    Comb.nST = m_maskGenStatic;
    if (m_eSHClass == eHWSC_Pixel)
    {
        Comb.nMD &= ~HWMD_TEXCOORD_FLAG_MASK;
        Comb.nMDV = 0;
    }

    Comb.pShader = this;
    if (Comb.nRTNew != Comb.nRTOrg || Comb.nGLNew != Comb.nGLOrg || Comb.nLTNew != Comb.nLTOrg)
    {
        SEmptyCombination::s_Combinations.push_back(Comb);
    }

    m_nMaskGenShader = nGL;

    return true;
}

bool CHWShader_D3D::mfStoreEmptyCombination(SEmptyCombination& Comb)
{
    if (!m_pGlobalCache || !m_pGlobalCache->m_pRes[CACHE_USER])
    {
        return false;
    }

    CResFile* rf = m_pGlobalCache->m_pRes[CACHE_USER];
    char nameOrg[128];
    char nameNew[128];
    SShaderCombIdent Ident;
    Ident.m_GLMask = Comb.nGLNew;
    Ident.m_RTMask = Comb.nRTNew;
    Ident.m_LightMask = Comb.nLTNew;
    Ident.m_MDMask = Comb.nMD;
    Ident.m_MDVMask = Comb.nMDV;
    Ident.m_STMask = Comb.nST;
    SHWSInstance* pInstNew = mfGetInstance(gRenDev->m_RP.m_pShader, Ident, 0);
    mfGenName(pInstNew, nameNew, 128, 1);
    SDirEntry* deNew = rf->mfGetEntry(nameNew);
    //assert(deNew);
    if (!deNew)
    {
        return false;
    }

    Ident.m_GLMask = Comb.nGLOrg;
    Ident.m_RTMask = Comb.nRTOrg;
    Ident.m_LightMask = Comb.nLTOrg;
    Ident.m_MDMask = Comb.nMD;
    Ident.m_MDVMask = Comb.nMDV;
    Ident.m_STMask = Comb.nST;
    SHWSInstance* pInstOrg = mfGetInstance(gRenDev->m_RP.m_pShader, Ident, 0);
    mfGenName(pInstOrg, nameOrg, 128, 1);
    SDirEntry* deOrg = rf->mfGetEntry(nameOrg);
    if (deOrg)
    {
        if (deOrg->offset != deNew->offset)
        {
            deOrg->offset = -abs(deNew->offset);
            deOrg->flags |= RF_NOTSAVED;
        }
        return true;
    }
    SDirEntry de;
    de.Name = CCryNameTSCRC(nameOrg);
    de.flags = deNew->flags;
    de.size = deNew->size;
    de.offset = -abs(deNew->offset);
    rf->mfFileAdd(&de);

    return true;
}

bool CHWShader_D3D::mfFlushCacheFile()
{
    uint32 i;

    for (i = 0; i < m_Insts.size(); i++)
    {
        SHWSInstance* pInst = m_Insts[i];
        if (pInst->m_Handle.m_bStatus == 2) // Fake
        {
            pInst->m_Handle.SetShader(NULL);
        }
    }
    return m_pGlobalCache && m_pGlobalCache->m_pRes[CACHE_USER] && m_pGlobalCache->m_pRes[CACHE_USER]->mfFlush();
}

struct SData
{
    CCryNameTSCRC Name;
    uint32 nSizeDecomp;
    uint32 nSizeComp;
    //uint32 CRC;
    uint16 flags;
    int nOffset;
    byte* pData;
    byte bProcessed;

    bool operator <(const SData& o) const
    {
        return Name < o.Name;
    }
};
#if !defined(CONSOLE)
// Remove shader duplicates
bool CHWShader::mfOptimiseCacheFile(SShaderCache* pCache, [[maybe_unused]] bool bForce, SOptimiseStats* pStats)
{
    CResFile* pRes = pCache->m_pRes[CACHE_USER];
    pRes->mfFlush();
    ResDir* Dir = pRes->mfGetDirectory();
    uint32 i, j;
#ifdef _DEBUG
    /*for (i=0; i<Dir->size(); i++)
    {
      SDirEntry *pDE = &(*Dir)[i];
      for (j=i+1; j<Dir->size(); j++)
      {
        SDirEntry *pDE1 = &(*Dir)[j];
        assert(pDE->Name != pDE1->Name);
      }
      }
      */
    mfValidateTokenData(pRes);
#endif

    std::vector<SData> Data;
    bool bNeedOptimise = true;
    if (pStats)
    {
        pStats->nEntries += Dir->size();
    }
    for (i = 0; i < Dir->size(); i++)
    {
        SDirEntry* pDE = &(*Dir)[i];
        if (pDE->flags & RF_RES_$)
        {
            if (pDE->Name == CShaderMan::s_cNameHEAD)
            {
                continue;
            }

            SData d;
            d.nSizeComp = d.nSizeDecomp = 0;
            d.pData = pRes->mfFileReadCompressed(pDE, d.nSizeDecomp, d.nSizeComp);
            assert(d.pData && d.nSizeComp && d.nSizeDecomp);
            if (!d.pData || !d.nSizeComp || !d.nSizeDecomp)
            {
                continue;
            }
            if (pStats)
            {
                pStats->nTokenDataSize += d.nSizeDecomp;
            }
            d.bProcessed = 3;
            d.Name = pDE->Name;
            //d.CRC = 0;
            d.nOffset = 0;
            d.flags = (short)pDE->flags;
            Data.push_back(d);
            continue;
        }
        SData d;
        d.flags = pDE->flags;
        d.nSizeComp = d.nSizeDecomp = 0;
        d.pData = pRes->mfFileReadCompressed(pDE, d.nSizeDecomp, d.nSizeComp);
        assert(d.pData && d.nSizeComp && d.nSizeDecomp);
        if (!d.pData || !d.nSizeComp || !d.nSizeDecomp)
        {
            continue;
        }
        d.nOffset = pDE->offset;
        d.bProcessed = 0;
        d.Name = pDE->Name;
        //d.CRC = 0;
        Data.push_back(d);
        pRes->mfCloseEntry(pDE);
    }
    //FILE *fp = NULL;
    int nDevID = 0x10000000;
    int nOutFiles = Data.size();
    if (bNeedOptimise)
    {
        for (i = 0; i < Data.size(); i++)
        {
            /*if (fp)
            {
              gEnv->pCryPak->FClose(fp);
              fp = NULL;
            }*/
            if (Data[i].bProcessed)
            {
                continue;
            }
            byte* pD = Data[i].pData;
            Data[i].bProcessed = 1;
            Data[i].nOffset = nDevID++;
            int nSizeComp = Data[i].nSizeComp;
            int nSizeDecomp = Data[i].nSizeDecomp;
            for (j = i + 1; j < Data.size(); j++)
            {
                if (Data[j].bProcessed)
                {
                    continue;
                }
                byte* pD1 = Data[j].pData;
                if (nSizeComp != Data[j].nSizeComp || nSizeDecomp != Data[j].nSizeDecomp)
                {
                    continue;
                }
                if (!memcmp(pD, pD1, nSizeComp))
                {
                    /*if (!fp && CRenderer::CV_r_shaderscacheoptimiselog)
                    {
                      char name[256];
                      sprintf_s(name, "Optimise/%s/%s.cache", pRes->mfGetFileName(), Data[i].Name.c_str());
                      fp = gEnv->pCryPak->FOpen(name, "w");
                    }*/
                    Data[j].nOffset = Data[i].nOffset;
                    Data[j].bProcessed = 2;
                    nOutFiles--;
                    //if (fp)
                    //  gEnv->pCryPak->FPrintf(fp, "%s\n", Data[j].Name.c_str());
                }
            }
        }
    }
    /*if (fp)
    {
      gEnv->pCryPak->FClose(fp);
      fp = NULL;
    }*/

    if (nOutFiles != Data.size() || CRenderer::CV_r_shaderscachedeterministic)
    {
        if (nOutFiles == Data.size())
        {
            iLog->Log(" Forcing optimise for deterministic order...");
        }

        iLog->Log(" Optimising shaders resource '%s' (%" PRISIZE_T " items)...", pCache->m_Name.c_str(), Data.size() - 1);

        pRes->mfClose();
        pRes->mfOpen(RA_CREATE | (CParserBin::m_bEndians ? RA_ENDIANS : 0), &gRenDev->m_cEF.m_ResLookupDataMan[CACHE_USER]);

        float fVersion = FX_CACHE_VER;
        uint32 nMinor = (int)(((float)fVersion - (float)(int)fVersion) * 10.1f);
        uint32 nMajor = (int)fVersion;

        SResFileLookupData* pLookupCache = pCache->m_pRes[CACHE_USER]->GetLookupData(false, 0, 0);
        CRY_ASSERT(pLookupCache != NULL);

        if (pLookupCache == NULL || pLookupCache->m_CacheMajorVer != nMajor || pLookupCache->m_CacheMinorVer != nMinor)
        {
            CRY_ASSERT_MESSAGE(pLookupCache == NULL, "Losing ShaderIdents by recreating lookupdata cache");
            pLookupCache = pRes->GetLookupData(true, 0, FX_CACHE_VER);
        }

        pRes->mfFlush();

        if (CRenderer::CV_r_shaderscachedeterministic)
        {
            std::sort(Data.begin(), Data.end());
        }

        for (i = 0; i < Data.size(); i++)
        {
            SData* pD = &Data[i];

            SDirEntry de;
            de.Name = pD->Name;
            de.flags = pD->flags;
            if (pD->bProcessed == 1)
            {
                de.offset = pD->nOffset;
                de.flags |= RF_COMPRESS | RF_COMPRESSED;
                if (pStats)
                {
                    pStats->nSizeUncompressed += pD->nSizeDecomp;
                    pStats->nSizeCompressed += pD->nSizeComp;
                    pStats->nUniqueEntries++;
                }
                assert(pD->pData);
                if (pD->pData)
                {
                    de.size = pD->nSizeComp + 4;
                    SDirEntryOpen* pOE = pRes->mfOpenEntry(&de);
                    byte* pData = new byte[de.size];
                    int nSize = pD->nSizeDecomp;
                    *(int*)pData = nSize;
                    memcpy(&pData[4], pD->pData, pD->nSizeComp);
                    de.flags |= RF_TEMPDATA;
                    pOE->pData = pData;
                    SAFE_DELETE_ARRAY(pD->pData);
                }
            }
            else
            if (pD->bProcessed != 3)
            {
                de.size = pD->nSizeComp + 4;
                de.flags |= RF_COMPRESS;
                de.offset = -pD->nOffset;
                SAFE_DELETE_ARRAY(pD->pData);
            }
            else
            {
                SDirEntryOpen* pOE = pRes->mfOpenEntry(&de);
                pOE->pData = pD->pData;
                de.size = pD->nSizeDecomp;
            }
            pRes->mfFileAdd(&de);
        }
    }

    if (nOutFiles != Data.size())
    {
        iLog->Log("  -- Removed %" PRISIZE_T " duplicated shaders", Data.size() - nOutFiles);
    }

    Data.clear();
    int nSizeDir = pRes->mfFlush(true);
    //int nSizeCompr = pRes->mfFlush();

#ifdef _DEBUG
    mfValidateTokenData(pRes);
#endif

    if (pStats)
    {
        pStats->nDirDataSize += nSizeDir;
    }

    for (i = 0; i < Data.size(); i++)
    {
        SData* pD = &Data[i];
        SAFE_DELETE_ARRAY(pD->pData);
    }

    if (pStats)
    {
        CryLog("  -- Shader cache '%s' stats: Entries: %d, Unique Entries: %d, Size: %.3f Mb, Compressed Size: %.3f Mb, Token data size: %3f Mb, Directory Size: %.3f Mb", pCache->m_Name.c_str(), pStats->nEntries, pStats->nUniqueEntries, pStats->nSizeUncompressed / 1024.0f / 1024.0f, pStats->nSizeCompressed / 1024.0f / 1024.0f, pStats->nTokenDataSize / 1024.0f / 1024.0f, pStats->nDirDataSize / 1024.0f / 1024.0f);
    }

    return true;
}
#endif

int __cdecl sSort(const VOID* arg1, const VOID* arg2)
{
    SDirEntry** pi1 = (SDirEntry**)arg1;
    SDirEntry** pi2 = (SDirEntry**)arg2;
    SDirEntry* ti1 = *pi1;
    SDirEntry* ti2 = *pi2;
    if (ti1->Name < ti2->Name)
    {
        return -1;
    }
    if (ti1->Name == ti2->Name)
    {
        return 0;
    }
    return 1;
}

bool CHWShader::_OpenCacheFile(float fVersion, SShaderCache* pCache, CHWShader* pSH, bool bCheckValid, uint32 CRC32, int nCache, CResFile* pRF, bool bReadOnly)
{
    assert(nCache == CACHE_USER || nCache == CACHE_READONLY);

    bool bValid = true;
    CHWShader_D3D* pSHHW = (CHWShader_D3D*)pSH;
    int nRes = pRF->mfOpen(RA_READ | (CParserBin::m_bEndians ? RA_ENDIANS : 0), &gRenDev->m_cEF.m_ResLookupDataMan[nCache], (nCache == CACHE_READONLY && pCache->m_pStreamInfo) ? pCache->m_pStreamInfo : NULL);
    if (nRes == 0)
    {
        pRF->mfClose();
        bValid = false;
    }
    else
    if (nRes > 0)
    {
        if (bValid)
        {
            SResFileLookupData* pLookup = pRF->GetLookupData(false, 0, 0);
            if (!pLookup)
            {
                bValid = false;
            }
            else
            if (bCheckValid)
            {
                if (fVersion && (pLookup->m_CacheMajorVer != (int)fVersion || pLookup->m_CacheMinorVer != (int)(((float)fVersion - (float)(int)fVersion) * 10.1f)))
                {
                    bValid = false;
                }
                if (!bValid && (CRenderer::CV_r_shadersdebug == 2 || nCache == CACHE_READONLY))
                {
                    LogWarningEngineOnly("WARNING: Shader cache '%s' version mismatch (Cache: %d.%d, Expected: %.1f)", pRF->mfGetFileName(), pLookup->m_CacheMajorVer, pLookup->m_CacheMinorVer, fVersion);
                }
                if (pSH)
                {
                    if (bValid && pLookup->m_CRC32 != pSHHW->m_CRC32)
                    {
                        bValid = false;
                        if IsCVarConstAccess(constexpr) (CRenderer::CV_r_shadersdebug == 2 && (CRenderer::CV_r_shadersdebug == 2 || nCache == CACHE_READONLY))
                        {
                            LogWarningEngineOnly("WARNING: Shader cache '%s' CRC mismatch", pRF->mfGetFileName());
                        }
                    }
                }
            }
        }

        if (nCache == CACHE_USER)
        {
            pRF->mfClose();
            if (bValid)
            {
                int nAcc = CRenderer::CV_r_shadersAllowCompilation != 0 ? (RA_READ | RA_WRITE) : RA_READ;
                if (!pRF->mfOpen(nAcc | (CParserBin::m_bEndians ? RA_ENDIANS : 0), &gRenDev->m_cEF.m_ResLookupDataMan[nCache]))
                {
                    pRF->mfClose();
                    bValid = false;
                }
            }
        }
    }
    if (!bValid && bCheckValid)
    {
        if (nCache == CACHE_USER && !bReadOnly)
        {
            if (!pRF->mfOpen(RA_CREATE | (CParserBin::m_bEndians ? RA_ENDIANS : 0), &gRenDev->m_cEF.m_ResLookupDataMan[nCache]))
            {
                return false;
            }

            pRF->GetLookupData(true, CRC32, FX_CACHE_VER);
            if (pSHHW)
            {
                pRF->mfFlush();
            }
            pCache->m_bNeedPrecache = true;
            bValid = true;
        }
        else
        {
            SAFE_DELETE(pRF);
        }
    }
    pCache->m_pRes[nCache] = pRF;
    pCache->m_bReadOnly[nCache] = bReadOnly;

#ifdef _DEBUG
    mfValidateTokenData(pRF);
#endif

    return bValid;
}

bool CHWShader::mfOpenCacheFile(const char* szName, float fVersion, SShaderCache* pCache, CHWShader* pSH, bool bCheckValid, uint32 CRC32, bool bReadOnly)
{
    bool bValidRO = false;
    bool bValidUser = true;
    // don't load the readonly cache, when shaderediting is true
    if (!CRenderer::CV_r_shadersediting && !pCache->m_pRes[CACHE_READONLY])
    {
        CResFile* rfRO = new CResFile(szName);
        bool bRO = bReadOnly;
        if (!CRenderer::CV_r_shadersAllowCompilation)
        {
            bRO = true;
        }
        bValidRO = _OpenCacheFile(fVersion, pCache, pSH, bCheckValid, CRC32, CACHE_READONLY, rfRO, bRO);
    }
    if (!CRenderer::CV_r_shadersAllowCompilation)
    {
        assert(bReadOnly);
    }
    if ((!bReadOnly || gRenDev->IsShaderCacheGenMode()) && !pCache->m_pRes[CACHE_USER])
    {
        stack_string szUser = stack_string(gRenDev->m_cEF.m_szCachePath.c_str()) + stack_string(szName);
        CResFile* rfUser = new CResFile(szUser.c_str());
        bValidUser = _OpenCacheFile(fVersion, pCache, pSH, bCheckValid, CRC32, CACHE_USER, rfUser, bReadOnly);
    }

    return (bValidRO || bValidUser);
}

SShaderCache* CHWShader::mfInitCache(const char* name, CHWShader* pSH, bool bCheckValid, uint32 CRC32, bool bReadOnly, bool bAsync)
{
    //  LOADING_TIME_PROFILE_SECTION(iSystem);

    CHWShader_D3D* pSHHW = (CHWShader_D3D*)pSH;
    char nameCache[256];

    if (!CRenderer::CV_r_shadersAllowCompilation)
    {
        bCheckValid = false;
    }

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_shadersediting)
    {
        bReadOnly = false;
    }

    if (!name)
    {
        char namedst[256];
        pSHHW->mfGetDstFileName(pSHHW->m_pCurInst, pSHHW, namedst, 256, 0);
        fpStripExtension(namedst, nameCache);
        fpAddExtension(nameCache, ".fxcb");
        name = nameCache;
    }

    SShaderCache* pCache = NULL;
    FXShaderCacheItor it = m_ShaderCache.find(CCryNameR(name));
    if (it != m_ShaderCache.end())
    {
        pCache = it->second;
        pCache->AddRef();
        if (pSHHW)
        {
            if (bCheckValid)
            {
                int nCache[2] = {-1, -1};
                if (!CRenderer::CV_r_shadersAllowCompilation)
                {
                    nCache[0] = CACHE_READONLY;
                }
                else
                {
                    nCache[0] = CACHE_USER;
                    nCache[1] = CACHE_READONLY;
                }
                bool bValid;
                for (int i = 0; i < 2; i++)
                {
                    if (nCache[i] < 0 || !pCache->m_pRes[i])
                    {
                        continue;
                    }
                    CResFile* pRF = pCache->m_pRes[i];
                    SResFileLookupData* pLookup = pRF->GetLookupData(false, 0, FX_CACHE_VER);
                    bValid = (pLookup && pLookup->m_CRC32 == CRC32);
                    if (!bValid)
                    {
                        SAFE_DELETE(pCache->m_pRes[i]);
                    }
                }
                bValid = true;
                if (!CRenderer::CV_r_shadersAllowCompilation && !pCache->m_pRes[CACHE_READONLY])
                {
                    bValid = false;
                }
                else
                {
                    if (bReadOnly && (!pCache->m_pRes[CACHE_READONLY] || !pCache->m_pRes[CACHE_USER]))
                    {
                        bValid = false;
                    }
                    if (!bReadOnly && !pCache->m_pRes[CACHE_USER])
                    {
                        bValid = false;
                    }
                }
                if (!bValid)
                {
                    mfOpenCacheFile(name, FX_CACHE_VER, pCache, pSH, bCheckValid, CRC32, bReadOnly);
                }
            }
        }
    }
    else
    {
        pCache = new SShaderCache;
        if (bAsync)
        {
            pCache->m_pStreamInfo = new SResStreamInfo(pCache);
        }
        pCache->m_nPlatform = CParserBin::m_nPlatform;
        pCache->m_Name = name;
        mfOpenCacheFile(name, FX_CACHE_VER, pCache, pSH, bCheckValid, CRC32, bReadOnly);
        m_ShaderCache.insert(FXShaderCacheItor::value_type(CCryNameR(name), pCache));
    }

    return pCache;
}


byte* CHWShader_D3D::mfBindsToCache([[maybe_unused]] SHWSInstance* pInst, std::vector<SCGBind>* Binds, int nParams, byte* pP)
{
    int i;
    for (i = 0; i < nParams; i++)
    {
        SCGBind* cgb = &(*Binds)[i];
        SShaderCacheHeaderItemVar* pVar = (SShaderCacheHeaderItemVar*)pP;
        pVar->m_nCount = cgb->m_RegisterCount;
        pVar->m_Reg = cgb->m_RegisterOffset;
        if (CParserBin::m_bEndians)
        {
            SwapEndian(pVar->m_nCount, eBigEndian);
            SwapEndian(pVar->m_Reg, eBigEndian);
        }
        int len = strlen(cgb->m_Name.c_str()) + 1;
        memcpy(pVar->m_Name,  cgb->m_Name.c_str(), len);
        pP += offsetof(SShaderCacheHeaderItemVar, m_Name) + strlen(pVar->m_Name) + 1;
    }
    return pP;
}

byte* CHWShader_D3D::mfBindsFromCache(std::vector<SCGBind>*& Binds, int nParams, byte* pP)
{
    int i;
    for (i = 0; i < nParams; i++)
    {
        if (!Binds)
        {
            Binds = new std::vector<SCGBind>;
        }
        SCGBind cgb;
        SShaderCacheHeaderItemVar* pVar = (SShaderCacheHeaderItemVar*)pP;

        short nParameters = pVar->m_nCount;
        if (CParserBin::m_bEndians)
        {
            SwapEndian(nParameters, eBigEndian);
        }
        cgb.m_RegisterCount = nParameters;

        cgb.m_Name = pVar->m_Name;

        int dwBind = pVar->m_Reg;
        if (CParserBin::m_bEndians)
        {
            SwapEndian(dwBind, eBigEndian);
        }
        cgb.m_RegisterOffset = dwBind;

        Binds->push_back(cgb);
        pP += offsetof(SShaderCacheHeaderItemVar, m_Name) + strlen(pVar->m_Name) + 1;
    }
    return pP;
}

byte* CHWShader::mfIgnoreBindsFromCache(int nParams, byte* pP)
{
    int i;
    for (i = 0; i < nParams; i++)
    {
        SShaderCacheHeaderItemVar* pVar = (SShaderCacheHeaderItemVar*)pP;
        pP += offsetof(SShaderCacheHeaderItemVar, m_Name) + strlen(pVar->m_Name) + 1;
    }
    return pP;
}

bool CHWShader_D3D::mfUploadHW(SHWSInstance* pInst, byte* pBuf, uint32 nSize, CShader* pSH, uint32 nFlags)
{
    PROFILE_FRAME(Shader_mfUploadHW);

    const char* sHwShaderName = _HELP("Vertex Shader");
    if (m_eSHClass == eHWSC_Pixel)
    {
        sHwShaderName = _HELP("Pixel Shader");
    }
    
    HRESULT hr = S_OK;
    if (!pInst->m_Handle.m_pShader)
    {
        pInst->m_Handle.SetShader(new SD3DShader);
    }

    if ((m_eSHClass == eHWSC_Vertex) && (!(nFlags & HWSF_PRECACHE) || gRenDev->m_cEF.m_bActivatePhase) && !pInst->m_bFallback)
    {
        mfUpdateFXVertexFormat(pInst, pSH);
    }

    pInst->m_nDataSize = nSize;
    if (m_eSHClass == eHWSC_Pixel)
    {
        s_nDevicePSDataSize += nSize;
    }
    else
    {
        s_nDeviceVSDataSize += nSize;
    }

    if (m_eSHClass == eHWSC_Pixel)
    {
        hr = gcpRendD3D->GetDevice().CreatePixelShader(alias_cast<DWORD*>(pBuf), nSize, NULL, alias_cast<ID3D11PixelShader**>(&pInst->m_Handle.m_pShader->m_pHandle));
    }
    else
    if (m_eSHClass == eHWSC_Vertex)
    {
        hr = gcpRendD3D->GetDevice().CreateVertexShader(alias_cast<DWORD*>(pBuf), nSize, NULL, alias_cast<ID3D11VertexShader**>(&pInst->m_Handle.m_pShader->m_pHandle));
    }
    else
    if (m_eSHClass == eHWSC_Geometry)
    {
        hr = gcpRendD3D->GetDevice().CreateGeometryShader(alias_cast<DWORD*>(pBuf), nSize, NULL, alias_cast<ID3D11GeometryShader**>(&pInst->m_Handle.m_pShader->m_pHandle));
    }
    else
    if (m_eSHClass == eHWSC_Hull)
    {
        hr = gcpRendD3D->GetDevice().CreateHullShader(alias_cast<DWORD*>(pBuf), nSize, NULL, alias_cast<ID3D11HullShader**>(&pInst->m_Handle.m_pShader->m_pHandle));
    }
    else
    if (m_eSHClass == eHWSC_Compute)
    {
        hr = gcpRendD3D->GetDevice().CreateComputeShader(alias_cast<DWORD*>(pBuf), nSize, NULL, alias_cast<ID3D11ComputeShader**>(&pInst->m_Handle.m_pShader->m_pHandle));
    }
    else
    if (m_eSHClass == eHWSC_Domain)
    {
        hr = gcpRendD3D->GetDevice().CreateDomainShader(alias_cast<DWORD*>(pBuf), nSize, NULL, alias_cast<ID3D11DomainShader**>(&pInst->m_Handle.m_pShader->m_pHandle));
    }
    else
    {
        assert(0);
    }

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DHWSHADERCOMPILING_CPP_SECTION_5
    #include AZ_RESTRICTED_FILE(D3DHWShaderCompiling_cpp)
#endif

    // Assign name to Shader for enhanced debugging
#if !defined(RELEASE) && defined(WIN64)
    if (pInst->m_Handle.m_pShader->m_pHandle)
    {
        char name[1024];
        azsprintf(name, "%s_%s(LT%x)@(RT%llx)(MD%x)(MDV%x)(GL%llx)(PSS%llx)(ST%llx)", pSH->GetName(), m_EntryFunc.c_str(), pInst->m_Ident.m_LightMask, pInst->m_Ident.m_RTMask, pInst->m_Ident.m_MDMask, pInst->m_Ident.m_MDVMask, pInst->m_Ident.m_GLMask, pInst->m_Ident.m_pipelineState.opaque, pInst->m_Ident.m_STMask);
        ((ID3D11DeviceChild*)pInst->m_Handle.m_pShader->m_pHandle)->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(name), name);
    }
#endif

    return (hr == S_OK);
}

bool CHWShader_D3D::mfUploadHW(LPD3D10BLOB pShader, SHWSInstance* pInst, CShader* pSH, uint32 nFlags)
{
    bool bResult = true;
    if (m_eSHClass == eHWSC_Vertex && !pInst->m_bFallback)
    {
        mfUpdateFXVertexFormat(pInst, pSH);
    }
    if (pShader && !(m_Flags & HWSG_PRECACHEPHASE))
    {
        DWORD* pCode = (DWORD*)pShader->GetBufferPointer();
        if (gcpRendD3D->m_cEF.m_nCombinationsProcess >= 0 && !gcpRendD3D->m_cEF.m_bActivatePhase)
        {
            pInst->m_Handle.SetFake();
        }
        else
        {
            bResult = mfUploadHW(pInst, (byte*)pCode, (uint32)pShader->GetBufferSize(), pSH, nFlags);
            if (m_eSHClass == eHWSC_Vertex)
            {
                size_t nSize = pShader->GetBufferSize();
                pInst->m_pShaderData = new byte[nSize];
                pInst->m_nDataSize = nSize;
                memcpy(pInst->m_pShaderData, pCode, nSize);
                pInst->m_uniqueNameCRC = AZ::Crc32(GetName());
            }
        }
        if (!bResult)
        {
            if (m_eSHClass == eHWSC_Vertex)
            {
                Warning("CHWShader_D3D::mfUploadHW: Could not create vertex shader '%s'(0x%" PRIx64 ")\n", GetName(), pInst->m_Ident.m_GLMask);
            }
            else
            if (m_eSHClass == eHWSC_Pixel)
            {
                Warning("CHWShader_D3D::mfUploadHW: Could not create pixel shader '%s'(0x%" PRIx64 ")\n", GetName(), pInst->m_Ident.m_GLMask);
            }
            else
            if (m_eSHClass == eHWSC_Geometry)
            {
                Warning("CHWShader_D3D::mfUploadHW: Could not create geometry shader '%s'(0x%" PRIx64 ")\n", GetName(), pInst->m_Ident.m_GLMask);
            }
            else
            if (m_eSHClass == eHWSC_Domain)
            {
                Warning("CHWShader_D3D::mfUploadHW: Could not create domain shader '%s'(0x%" PRIx64 ")\n", GetName(), pInst->m_Ident.m_GLMask);
            }
            else
            if (m_eSHClass == eHWSC_Hull)
            {
                Warning("CHWShader_D3D::mfUploadHW: Could not create hull shader '%s'(0x%" PRIx64 ")\n", GetName(), pInst->m_Ident.m_GLMask);
            }
            else
            if (m_eSHClass == eHWSC_Compute)
            {
                Warning("CHWShader_D3D::mfUploadHW: Could not create compute shader '%s'(0x%" PRIx64 ")\n", GetName(), pInst->m_Ident.m_GLMask);
            }
        }
    }
    return bResult;
}

bool CHWShader_D3D::mfActivateCacheItem(CShader* pSH, SShaderCacheHeaderItem* pItem, uint32 nSize, uint32 nFlags)
{
    SHWSInstance* pInst = m_pCurInst;
    byte* pData = (byte*)pItem;
    pData += sizeof(SShaderCacheHeaderItem);
    byte* pBuf = pData;
    std::vector<SCGBind>* pInstBinds = NULL;
    pInst->Release(m_pDevCache, false);
    pBuf = mfBindsFromCache(pInstBinds, pItem->m_nInstBinds, pBuf);
    nSize -= (uint32)(pBuf - (byte*)pItem);
    pInst->m_eClass = (EHWShaderClass)pItem->m_Class;

#if !defined(_RELEASE)
    if (pItem->m_nVertexFormat >= eVF_Max)
    {
        AZ_Warning("Graphics", false, "Existing vertex format with enum %d not legit (must be less than %d).  Is the shader cache out of date? Defaulting to eVF_P3S_C4B_T2S.", pItem->m_nVertexFormat, eVF_Max);
        pItem->m_nVertexFormat = eVF_P3S_C4B_T2S;
    }
#endif
    pInst->m_vertexFormat = AZ::Vertex::Format(gcpRendD3D->m_RP.m_vertexFormats[pItem->m_nVertexFormat]);

    pInst->m_nInstructions = pItem->m_nInstructions;
    pInst->m_VStreamMask_Decl = pItem->m_StreamMask_Decl;
    pInst->m_VStreamMask_Stream = pItem->m_StreamMask_Stream;
    bool bResult = true;
    SD3DShader* pHandle = NULL;
    SShaderDevCache* pCache = m_pDevCache;
    if (!(nFlags & HWSG_CACHE_USER))
    {
        if (pCache)
        {
            FXDeviceShaderItor it = pCache->m_DeviceShaders.find(pInst->m_DeviceObjectID);
            if (it != pCache->m_DeviceShaders.end())
            {
                pHandle = it->second;
            }
        }
    }
    HRESULT hr = S_OK;
    if (pHandle)
    {
        pInst->m_Handle.SetShader(pHandle);
        pInst->m_Handle.AddRef();

#if D3DHWSHADERCOMPILING_CPP_TRAIT_VERTEX_FORMAT
        if (m_eSHClass == eHWSC_Vertex)
        {
            ID3D10Blob* pS = NULL;
            D3D10CreateBlob(nSize, (LPD3D10BLOB*)&pS);
            DWORD* pBuffer = (DWORD*)pS->GetBufferPointer();
            memcpy(pBuffer, pBuf, nSize);
            mfVertexFormat(pInst, this, pS);
            SAFE_RELEASE(pS);
        }
#endif
        if ((m_eSHClass == eHWSC_Vertex) && (!(nFlags & HWSF_PRECACHE) || gRenDev->m_cEF.m_bActivatePhase) && !pInst->m_bFallback)
        {
            mfUpdateFXVertexFormat(pInst, pSH);
        }
    }
    else
    {
        if (gcpRendD3D->m_cEF.m_nCombinationsProcess > 0 && !gcpRendD3D->m_cEF.m_bActivatePhase)
        {
            pInst->m_Handle.SetFake();
        }
        else
        {
#if D3DHWSHADERCOMPILING_CPP_TRAIT_VERTEX_FORMAT
            if (m_eSHClass == eHWSC_Vertex)
            {
                ID3D10Blob* pS = NULL;
                D3D10CreateBlob(nSize, (LPD3D10BLOB*)&pS);
                DWORD* pBuffer = (DWORD*)pS->GetBufferPointer();
                memcpy(pBuffer, pBuf, nSize);
                mfVertexFormat(pInst, this, pS);
                SAFE_RELEASE(pS);
            }
#endif

            bResult = mfUploadHW(pInst, pBuf, nSize, pSH, nFlags);
        }
        if (!bResult)
        {
            SAFE_DELETE(pInstBinds);
            assert(!"Shader creation error");
            iLog->Log("WARNING: cannot create shader '%s' (FX: %s)", m_EntryFunc.c_str(), GetName());
            return true;
        }
        pCache->m_DeviceShaders.insert(FXDeviceShaderItor::value_type(pInst->m_DeviceObjectID, pInst->m_Handle.m_pShader));
    }
    void* pConstantTable = NULL;
    void* pShaderReflBuf = NULL;
    hr = D3DReflect(pBuf, nSize, IID_ID3D11ShaderReflection, &pShaderReflBuf);
    ID3D11ShaderReflection* pShaderReflection = (ID3D11ShaderReflection*)pShaderReflBuf;
    if (SUCCEEDED(hr))
    {
        pConstantTable = (void*)pShaderReflection;
    }
    if (m_eSHClass == eHWSC_Vertex || gRenDev->IsEditorMode())
    {
        pInst->m_pShaderData = new byte[nSize];
        pInst->m_nDataSize = nSize;
        memcpy(pInst->m_pShaderData, pBuf, nSize);
        pInst->m_uniqueNameCRC = AZ::Crc32(GetName());
    }
    assert(hr == S_OK);
    bResult &= (hr == S_OK);
    if (pConstantTable)
    {
        mfCreateBinds(pInst, pConstantTable, pBuf, nSize);
    }

    mfGatherFXParameters(pInst, pInstBinds, this, 0, pSH);
    SAFE_DELETE(pInstBinds);
    SAFE_RELEASE(pShaderReflection);

    return bResult;
}

bool CHWShader_D3D::mfCreateCacheItem(SHWSInstance* pInst, std::vector<SCGBind>& InstBinds, byte* pData, int nLen, CHWShader_D3D* pSH, bool bShaderThread)
{
    if (!pSH->m_pGlobalCache || !pSH->m_pGlobalCache->m_pRes[CACHE_USER])
    {
        if (pSH->m_pGlobalCache)
        {
            pSH->m_pGlobalCache->Release(false);
        }
        pSH->m_pGlobalCache = mfInitCache(NULL, pSH, true, pSH->m_CRC32, false, false);
    }
    assert(pSH->m_pGlobalCache);
    if (!pSH->m_pGlobalCache || !pSH->m_pGlobalCache->m_pRes[CACHE_USER])
    {
        return false;
    }

    byte* byteData = NULL;
    DWORD* dwordData = NULL;

    SShaderCacheHeaderItem h;
    h.m_nInstBinds = InstBinds.size();
    h.m_nInstructions = pInst->m_nInstructions;
    h.m_nVertexFormat = pInst->m_vertexFormat.GetEnum();
    h.m_Class = pData ? pInst->m_eClass : 255;
    h.m_StreamMask_Decl = pInst->m_VStreamMask_Decl;
    h.m_StreamMask_Stream = (byte)pInst->m_VStreamMask_Stream;
    int nNewSize = (h.m_nInstBinds) * sizeof(SShaderCacheHeaderItemVar) + nLen;
    byte* pNewData = new byte [nNewSize];
    byte* pP = pNewData;
    pP = mfBindsToCache(pInst, &InstBinds, h.m_nInstBinds, pP);
    PREFAST_ASSUME(pData);
    memcpy(pP, pData, nLen);
    delete[] byteData;
    delete[] dwordData;
    pP += nLen;
    char name[256];
    mfGenName(pInst, name, 256, 1);
    CCryNameTSCRC nm = CCryNameTSCRC(name);
    bool bRes = mfAddCacheItem(pSH->m_pGlobalCache, &h, pNewData, (int)(pP - pNewData), false, nm);
    SAFE_DELETE_ARRAY(pNewData);
    if (gRenDev->m_cEF.m_bActivatePhase || (!(pSH->m_Flags & HWSG_PRECACHEPHASE) && gRenDev->m_cEF.m_nCombinationsProcess <= 0))
    {
        if (!gRenDev->m_cEF.m_bActivatePhase)
        {
            if (bShaderThread && false)
            {
                if (pInst->m_pAsync)
                {
                    pInst->m_pAsync->m_bPendedFlush = true;
                }
            }
            else
            {
                pSH->mfFlushCacheFile();
            }
        }
        azstrcpy(name, AZ_ARRAY_SIZE(name), pSH->GetName());
        char* s = strchr(name, '(');
        if (s)
        {
            s[0] = 0;
        }
        if (!bShaderThread || true)
        {
            byte bStore = 1;
            if (pSH->m_Flags & HWSG_FP_EMULATION)
            {
                bStore = 2;
            }
            SShaderCombIdent Ident = pInst->m_Ident;
            ;
            Ident.m_GLMask = pSH->m_nMaskGenFX;
            gRenDev->m_cEF.mfInsertNewCombination(Ident, pInst->m_eClass, name, 0, NULL, bStore);
        }
    }
    pInst->m_nCache = CACHE_USER;

    return bRes;
}

//============================================================================

void CHWShader_D3D::mfSaveCGFile(const char* scr, const char* path)
{
    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_shadersdebug < 1)
    {
        return;
    }
    char name[1024];
    if (path && path[0])
    {
        sprintf_s(name, "%s/%s(LT%x)/(RT%llx)(MD%x)(MDV%x)(GL%llx)(PSS%llx)(ST%llx).cg", path, GetName(), m_pCurInst->m_Ident.m_LightMask, m_pCurInst->m_Ident.m_RTMask, m_pCurInst->m_Ident.m_MDMask, m_pCurInst->m_Ident.m_MDVMask, m_pCurInst->m_Ident.m_GLMask, m_pCurInst->m_Ident.m_pipelineState.opaque, m_pCurInst->m_Ident.m_STMask);
    }
    else
    {
        sprintf_s(name, "@usercache@/shaders/fxerror/%s(GL%llx)/(LT%x)(RT%llx)/(MD%x)(MDV%x)(PSS%llx)(ST%llx).cg", GetName(), m_pCurInst->m_Ident.m_GLMask, m_pCurInst->m_Ident.m_LightMask, m_pCurInst->m_Ident.m_RTMask, m_pCurInst->m_Ident.m_MDMask, m_pCurInst->m_Ident.m_MDVMask, m_pCurInst->m_Ident.m_pipelineState.opaque, m_pCurInst->m_Ident.m_STMask);
    }

    AZ::IO::HandleType fileHandle;

    fileHandle = gEnv->pCryPak->FOpen(name, "w");
    if (fileHandle != AZ::IO::InvalidHandle)
    {
        size_t len = strlen(scr);
        gEnv->pCryPak->FWrite(scr, len, fileHandle);
        gEnv->pCryPak->FClose (fileHandle);
    }
}

void CHWShader_D3D::mfOutputCompilerError(string& strErr, const char* szSrc)
{
    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_shadersdebug)
    {
        AZ::IO::HandleType fileHandle = fxopen("$$err", "w");
        if (fileHandle != AZ::IO::InvalidHandle)
        {
            AZ::IO::FPutS(szSrc, fileHandle);
            gEnv->pFileIO->Close(fileHandle);
        }
    }

    string strE = strErr;

    size_t newlinePos = strE.find('\n');

    while (newlinePos != string::npos)
    {
        iLog->LogError("%s", strE.substr(0, newlinePos).c_str());
        strE = strE.substr(newlinePos + 1);
        newlinePos = strE.find('\n');
    }

    if (strE.size())
    {
        iLog->LogError("%s", strE.c_str());
    }
}

SShaderAsyncInfo::~SShaderAsyncInfo()
{
    {
        AUTO_LOCK(g_cAILock);
        Unlink();
    }
    if (m_pFXShader)
    {
        assert(m_pFXShader->GetID() >= 0 && m_pFXShader->GetID() < MAX_REND_SHADERS);
    }
    SAFE_RELEASE(m_pFXShader);
    SAFE_RELEASE(m_pShader);
}

// Flush pended or processed shaders (main thread task)
void SShaderAsyncInfo::FlushPendingShaders()
{
    //assert (gRenDev->m_pRT->IsRenderThread());

    if (!SShaderAsyncInfo::PendingList().m_Next)
    {
        SShaderAsyncInfo::PendingList().m_Next = &SShaderAsyncInfo::PendingList();
        SShaderAsyncInfo::PendingList().m_Prev = &SShaderAsyncInfo::PendingList();
        SShaderAsyncInfo::PendingListT().m_Next = &SShaderAsyncInfo::PendingListT();
        SShaderAsyncInfo::PendingListT().m_Prev = &SShaderAsyncInfo::PendingListT();
    }

    SShaderAsyncInfo* pAI, * pAINext;
    {
        AUTO_LOCK(g_cAILock);
        for (pAI = PendingListT().m_Next; pAI != &PendingListT(); pAI = pAINext)
        {
            pAINext = pAI->m_Next;
            pAI->Unlink();
            pAI->Link(&PendingList());
        }
    }

    for (pAI = PendingList().m_Next; pAI != &PendingList(); pAI = pAINext)
    {
        pAINext = pAI->m_Next;

        CHWShader_D3D* pSH = pAI->m_pShader;
        if (pSH)
        {
            CHWShader_D3D::SHWSInstance* pInst = pSH->mfGetInstance(pAI->m_pFXShader, pAI->m_nHashInst, pSH->m_nMaskGenShader);
            if (pInst->m_pAsync != pAI)
            {
                CryFatalError("Shader instance async info doesn't match queued async info.");
            }
            pSH->mfAsyncCompileReady(pInst);
        }
    }
}
void CShader::mfFlushPendedShaders()
{
    SShaderAsyncInfo::FlushPendingShaders();
}

void CHWShader::mfFlushPendedShadersWait(int nMaxAllowed)
{
    if (nMaxAllowed > 0 && SShaderAsyncInfo::s_nPendingAsyncShaders < nMaxAllowed)
    {
        return;
    }
    if (CRenderer::CV_r_shadersasynccompiling > 0)
    {
        iLog->Log("Flushing pended shaders...");
Start:
        while (true)
        {
            if (SShaderAsyncInfo::s_nPendingAsyncShaders <= 0)
            {
                break;
            }
            int n = (int)iTimer->GetAsyncCurTime();
            if (!(n % 2))
            {
                iLog->Update();
            }
            if (!(n % 8))
            {
                SShaderAsyncInfo::FlushPendingShaders();
            }
            else
            {
                Sleep(1);
            }
        }
        // Compile FXC shaders or next iteration of internal shaders
        SShaderAsyncInfo::FlushPendingShaders();

        if (SShaderAsyncInfo::s_nPendingAsyncShaders)
        {
            goto Start;
        }

        iLog->Log("Finished flushing pended shaders...");
    }
}

int CHWShader_D3D::mfAsyncCompileReady(SHWSInstance* pInst)
{
    //SHWSInstance *pInst = m_pCurInst;
    //assert(pInst->m_pAsync);
    if (!pInst->m_pAsync)
    {
        return 0;
    }

    gRenDev->m_cEF.m_ShaderCacheStats.m_nNumShaderAsyncCompiles = SShaderAsyncInfo::s_nPendingAsyncShaders;

    SShaderAsyncInfo* pAsync = pInst->m_pAsync;
    int nFrame = gRenDev->GetFrameID(false);
    if (pAsync->m_nFrame == nFrame)
    {
        if (pAsync->m_fMinDistance > gRenDev->m_RP.m_fMinDistance)
        {
            pAsync->m_fMinDistance = gRenDev->m_RP.m_fMinDistance;
        }
    }
    else
    {
        pAsync->m_fMinDistance = gRenDev->m_RP.m_fMinDistance;
        pAsync->m_nFrame = nFrame;
    }

    std::vector<SCGBind> InstBindVars;
    LPD3D10BLOB pShader = NULL;
    void* pConstantTable = NULL;
    LPD3D10BLOB pErrorMsgs = NULL;
    string strErr;
    char nmDst[256], nameSrc[256];
    bool bResult = true;
    int nRefCount;

    SShaderTechnique* pTech = gRenDev->m_RP.m_pCurTechnique;
    CShader* pSH = pAsync->m_pFXShader;
    {
        if (pAsync->m_bPending)
        {
            return 0;
        }

        mfGetDstFileName(pInst, this, nmDst, 256, 3);
        gEnv->pCryPak->AdjustFileName(nmDst, nameSrc, AZ_ARRAY_SIZE(nameSrc), 0);
        if (pAsync->m_pFXShader && pAsync->m_pFXShader->m_HWTechniques.Num())
        {
            pTech = pAsync->m_pFXShader->m_HWTechniques[0];
        }
        if ((pAsync->m_pErrors && !pAsync->m_Errors.empty()) || !pAsync->m_pDevShader)
        {
            if IsCVarConstAccess(constexpr) (CRenderer::CV_r_logShaders)
            {
                gcpRendD3D->LogShv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "Async %d: **Failed to compile 0x%x '%s' shader\n", gRenDev->GetFrameID(false), pInst, nameSrc);
            }
            string Errors = pAsync->m_Errors;
            string Text = pAsync->m_Text;
            CShader* pFXShader = pAsync->m_pFXShader;
            nRefCount = pFXShader ? pFXShader->GetRefCounter() : 0;
            nRefCount = min(nRefCount, pAsync->m_pShader ? pAsync->m_pShader->GetRefCounter() : 0);
            if (nRefCount <= 1) // Just exit if shader was deleted
            {
                pInst->m_pAsync = NULL;
                SAFE_DELETE (pAsync);
                return -1;
            }

            mfOutputCompilerError(Errors, Text.c_str());

            Warning("Couldn't compile HW shader '%s'", GetName());
            mfSaveCGFile(Text.c_str(), NULL);

            bResult = false;
        }
        else if IsCVarConstAccess(constexpr) (CRenderer::CV_r_logShaders)
        {
            gcpRendD3D->LogShv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "Async %d: Finished compiling 0x%x '%s' shader\n", gRenDev->GetFrameID(false), pInst, nameSrc);
        }
        pShader = pAsync->m_pDevShader;
        pErrorMsgs = pAsync->m_pErrors;
        pConstantTable = pAsync->m_pConstants;
        strErr = pAsync->m_Errors;
        InstBindVars = decltype(InstBindVars)(pAsync->m_InstBindVars.begin(), pAsync->m_InstBindVars.end());

        if (pAsync->m_bPendedEnv)
        {
            bResult &= CHWShader_D3D::mfCreateShaderEnv(pAsync->m_nThread, pInst, pAsync->m_pDevShader, pAsync->m_pConstants, pAsync->m_pErrors, InstBindVars, this, false, pAsync->m_pFXShader, pAsync->m_nCombination);
            assert(bResult == true);
        }

        // Load samplers
        if (pAsync->m_bPendedSamplers)
        {
            mfGatherFXParameters(pInst, &InstBindVars, this, 2, pAsync->m_pFXShader);
        }

        if (pAsync->m_bPendedFlush)
        {
            mfFlushCacheFile();
            azstrcpy(nmDst, AZ_ARRAY_SIZE(nmDst), GetName());
            char* s = strchr(nmDst, '(');
            if (s)
            {
                s[0] = 0;
            }
            SShaderCombIdent Ident = pInst->m_Ident;
            Ident.m_GLMask = m_nMaskGenFX;
            gRenDev->m_cEF.mfInsertNewCombination(Ident, pInst->m_eClass, nmDst, 0);
        }

        nRefCount = pAsync->m_pFXShader ? pAsync->m_pFXShader->GetRefCounter() : 0;
        nRefCount = min(nRefCount, pAsync->m_pShader ? pAsync->m_pShader->GetRefCounter() : 0);
        if (nRefCount <= 1) // Just exit if shader was deleted
        {
            pInst->m_pAsync = NULL;
            SAFE_DELETE(pAsync);
            return -1;
        }
        SAFE_DELETE (pInst->m_pAsync);
    }

    if (pErrorMsgs && !strErr.empty())
    {
        return -1;
    }

    bResult &= mfUploadHW(pShader, pInst, pSH, 0);
    SAFE_RELEASE(pShader);

    if (bResult)
    {
        if (pTech)
        {
            mfUpdatePreprocessFlags(pTech);
        }
        return 1;
    }
    return -1;
}

bool CHWShader_D3D::mfRequestAsync(CShader* pSH, SHWSInstance* pInst, std::vector<SCGBind>& InstBindVars, const char* prog_text, const char* szProfile, const char* szEntry)
{
#ifdef SHADER_ASYNC_COMPILATION
    char nameSrc[256], nmDst[256];
    mfGetDstFileName(pInst, this, nmDst, 256, 3);
    gEnv->pCryPak->AdjustFileName(nmDst, nameSrc, AZ_ARRAY_SIZE(nameSrc), 0);

    if (!SShaderAsyncInfo::PendingList().m_Next)
    {
        SShaderAsyncInfo::PendingList().m_Next = &SShaderAsyncInfo::PendingList();
        SShaderAsyncInfo::PendingList().m_Prev = &SShaderAsyncInfo::PendingList();
        SShaderAsyncInfo::PendingListT().m_Next = &SShaderAsyncInfo::PendingListT();
        SShaderAsyncInfo::PendingListT().m_Prev = &SShaderAsyncInfo::PendingListT();
    }

    if (!m_pGlobalCache || !m_pGlobalCache->m_pRes[CACHE_USER])
    {
        if (m_pGlobalCache)
        {
            m_pGlobalCache->Release(false);
        }
        m_pGlobalCache = mfInitCache(NULL, this, true, m_CRC32, false, false);
    }

    pInst->m_pAsync = new SShaderAsyncInfo;
    pInst->m_pAsync->m_fMinDistance = gRenDev->m_RP.m_fMinDistance;
    pInst->m_pAsync->m_nFrame = gRenDev->GetFrameID(false);
    pInst->m_pAsync->m_InstBindVars.assign(InstBindVars.data(), InstBindVars.data() + InstBindVars.size());
    pInst->m_pAsync->m_pShader = this;
    pInst->m_pAsync->m_pShader->AddRef();
    pInst->m_pAsync->m_pFXShader = pSH;
    pInst->m_pAsync->m_pFXShader->AddRef();
    pInst->m_pAsync->m_nCombination = gRenDev->m_cEF.m_nCombinationsProcess;
    assert(!azstricmp(m_NameSourceFX.c_str(), pInst->m_pAsync->m_pFXShader->m_NameFile.c_str()));
    if (m_bUseLookUpTable)
    {
        pInst->m_pAsync->m_nHashInst = pInst->m_nContIndex;
    }
    else
    {
        pInst->m_pAsync->m_nHashInst = pInst->m_Ident.m_nHash;
    }
    pInst->m_pAsync->m_RTMask = pInst->m_Ident.m_RTMask;
    pInst->m_pAsync->m_LightMask = pInst->m_Ident.m_LightMask;
    pInst->m_pAsync->m_MDMask = pInst->m_Ident.m_MDMask;
    pInst->m_pAsync->m_MDVMask = pInst->m_Ident.m_MDVMask;
    pInst->m_pAsync->m_pipelineState.opaque = pInst->m_Ident.m_pipelineState.opaque;
    pInst->m_pAsync->m_eClass = pInst->m_eClass;
    pInst->m_pAsync->m_Text = prog_text;
    pInst->m_pAsync->m_Name = szEntry;
    pInst->m_pAsync->m_Profile = szProfile;

    // Generate request line text to store on the shaderlist for next shader cache gen
    {
        char szShaderGenName[512];
        cry_strcpy(szShaderGenName, GetName());
        char* s = strchr(szShaderGenName, '(');
        if (s)
        {
            s[0] = 0;
        }
        string RequestLine;
        SShaderCombIdent Ident = pInst->m_Ident;
        Ident.m_GLMask = m_nMaskGenFX;
        gRenDev->m_cEF.mfInsertNewCombination(Ident, pInst->m_eClass, szShaderGenName, 0, &RequestLine, false);

        pInst->m_pAsync->m_RequestLine = RequestLine;
    }

    CAsyncShaderTask::InsertPendingShader(pInst->m_pAsync);

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_logShaders)
    {
        gcpRendD3D->LogShv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "Async %d: Requested compiling 0x%x '%s' shader\n", gRenDev->GetFrameID(false), pInst, nameSrc);
    }
#endif
    return false;
}



void CHWShader_D3D::mfSubmitRequestLine(SHWSInstance* pInst, string* pRequestLine)
{
    // Generate request line text.
    char szShaderGenName[512];
    cry_strcpy(szShaderGenName, GetName());
    char* s = strchr(szShaderGenName, '(');
    if (s)
    {
        s[0] = 0;
    }
    string RequestLine;
    SShaderCombIdent Ident = pInst->m_Ident;
    Ident.m_GLMask = m_nMaskGenFX;
    gRenDev->m_cEF.mfInsertNewCombination(Ident, pInst->m_eClass, szShaderGenName, 0, &RequestLine, false);

    if (pRequestLine)
    {
        *pRequestLine = RequestLine;
    }

    if (!CRenderer::CV_r_shaderssubmitrequestline || !CRenderer::CV_r_shadersremotecompiler || pInst->m_bHasSendRequest)
    {
        return;
    }

    // make sure we only send the request once
    pInst->m_bHasSendRequest = true;

#ifdef SHADER_ASYNC_COMPILATION
    if (CRenderer::CV_r_shadersasynccompiling && !(m_Flags & HWSG_SYNC))
    {
        if (!SShaderAsyncInfo::PendingList().m_Next)
        {
            SShaderAsyncInfo::PendingList().m_Next = &SShaderAsyncInfo::PendingList();
            SShaderAsyncInfo::PendingList().m_Prev = &SShaderAsyncInfo::PendingList();
            SShaderAsyncInfo::PendingListT().m_Next = &SShaderAsyncInfo::PendingListT();
            SShaderAsyncInfo::PendingListT().m_Prev = &SShaderAsyncInfo::PendingListT();
        }

        SShaderAsyncInfo* pAsync = new SShaderAsyncInfo;

        if (pAsync)
        {
            pAsync->m_RequestLine = RequestLine;
            pAsync->m_Text = "";
            pAsync->m_bDeleteAfterRequest = true;

            CAsyncShaderTask::InsertPendingShader(pAsync);
        }
    }
    else
#endif
    {
        NRemoteCompiler::CShaderSrv::Instance().RequestLine(GetShaderListFilename().c_str(), RequestLine.c_str());
    }
}

bool CHWShader_D3D::mfCompileHLSL_Int(CShader* pSH, char* prog_text, LPD3D10BLOB* ppShader, void** ppConstantTable, LPD3D10BLOB* ppErrorMsgs, string& strErr, std::vector<SCGBind>& InstBindVars)
{
    HRESULT hr = S_OK;
    SHWSInstance* pInst = m_pCurInst;
    const char* szProfile = mfProfileString(pInst->m_eClass);
    const char* pFunCCryName = m_EntryFunc.c_str();

    bool bRes = true;
    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_shadersdebug == 2)
    {
        mfSaveCGFile(prog_text, "TestCG");
    }
    if (CRenderer::CV_r_shadersasynccompiling && !(m_Flags & HWSG_SYNC))
    {
        return mfRequestAsync(pSH, pInst, InstBindVars, prog_text, szProfile, pFunCCryName);
    }
    else
    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_shadersremotecompiler)
    {
        AZStd::string lsCompilerFlags = NRemoteCompiler::CShaderSrv::Instance().GetShaderCompilerFlags(pInst->m_eClass, pInst->m_Ident.m_pipelineState, pInst->m_Ident.m_MDVMask);

        string RequestLine;
        mfSubmitRequestLine(pInst, &RequestLine);

        std::vector<uint8> Data;
        if (NRemoteCompiler::ESOK != NRemoteCompiler::CShaderSrv::Instance().Compile(Data, szProfile, prog_text, pFunCCryName, lsCompilerFlags.c_str(), RequestLine.c_str()))
        {
            string sErrorText;
            sErrorText.reserve(Data.size());
            for (uint32 i = 0; i < Data.size(); i++)
            {
                sErrorText += Data[i];
            }
            strErr = sErrorText;

            return false;
        }

        D3D10CreateBlob(Data.size(), (LPD3D10BLOB*)ppShader);
        LPD3D10BLOB pShader = (LPD3D10BLOB) * ppShader;
        DWORD* pBuf = (DWORD*) pShader->GetBufferPointer();
        memcpy(pBuf, &Data[0], Data.size());

        *ppShader = (LPD3D10BLOB)pShader;
        pBuf = (DWORD*)pShader->GetBufferPointer();
        size_t nSize = pShader->GetBufferSize();

        bool bReflect = true;

#if !defined(CONSOLE)
        if (CParserBin::PlatformIsConsole())
        {
            bReflect = false;
        }
#endif

        if (bReflect)
        {
            void* pShaderReflBuf;
            hr = D3DReflect(pBuf, nSize, IID_ID3D11ShaderReflection, &pShaderReflBuf);
            if (SUCCEEDED(hr))
            {
                ID3D11ShaderReflection* pShaderReflection = (ID3D11ShaderReflection*)pShaderReflBuf;
                *ppConstantTable = (void*)pShaderReflection;
            }
            else
            {
                assert(0);
            }
        }

        return hr == S_OK;
    }
#if defined(WIN32) || defined(WIN64)
    else
    {
        static bool s_logOnce_WrongPlatform = false;
#   if !defined(OPENGL)
#       if !defined(_RELEASE)
        if (!s_logOnce_WrongPlatform && !(CParserBin::m_nPlatform == SF_D3D11))
        {
            s_logOnce_WrongPlatform = true;
            iLog->LogError("Trying to build non DX11 shader via internal compiler which is not supported. Please use remote compiler instead!");
        }
#       endif
        uint32 nFlags = D3D10_SHADER_PACK_MATRIX_ROW_MAJOR | D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY;
        if (CRenderer::CV_r_shadersdebug == 3) // Generate debug information and do not optimize shader for ease of debugging
        {
            nFlags |= D3D10_SHADER_DEBUG | D3D10_SHADER_SKIP_OPTIMIZATION;
        }
        else if(CRenderer::CV_r_shadersdebug == 4) // Generate debug information, but still optimize shader
        {
            nFlags |= D3D10_SHADER_DEBUG;
        }

        hr = D3DCompile(prog_text, strlen(prog_text), GetName(), NULL, NULL, pFunCCryName, szProfile, nFlags, 0, (ID3DBlob**) ppShader, (ID3DBlob**) ppErrorMsgs);
        if (FAILED(hr) || !*ppShader)
        {
            if (*ppErrorMsgs)
            {
                const char* err = (const char*)ppErrorMsgs[0]->GetBufferPointer();
                strErr += err;
            }
            else
            {
                strErr += "D3DXCompileShader failed";
            }
            bRes = false;
        }
        else
        {
            void* pShaderReflBuf;
            UINT* pData = (UINT*)ppShader[0]->GetBufferPointer();
            UINT nSize = (uint32)ppShader[0]->GetBufferSize();
            hr = D3DReflect(pData, nSize, IID_ID3D11ShaderReflection, &pShaderReflBuf);
            if (SUCCEEDED(hr))
            {
                ID3D11ShaderReflection* pShaderReflection = (ID3D11ShaderReflection*)pShaderReflBuf;
                *ppConstantTable = (void*)pShaderReflection;
            }
            else
            {
                assert(0);
            }
        }
        return bRes;
#   endif
    }
#endif // #if defined(WIN32) || defined(WIN64)

    return false;
}

LPD3D10BLOB CHWShader_D3D::mfCompileHLSL(CShader* pSH, char* prog_text, void** ppConstantTable, LPD3D10BLOB* ppErrorMsgs, [[maybe_unused]] uint32 nFlags, std::vector<SCGBind>& InstBindVars)
{
    //  LOADING_TIME_PROFILE_SECTION(iSystem);

    // Test adding source text to context
    string strErr;
    LPD3D10BLOB pCode = NULL;
    if (!prog_text)
    {
        assert(0);
        return NULL;
    }
    if (!CRenderer::CV_r_shadersAllowCompilation)
    {
        return NULL;
    }

    mfCompileHLSL_Int(pSH, prog_text, &pCode, ppConstantTable, ppErrorMsgs, strErr, InstBindVars);
    if (!pCode)
    {
        if (CRenderer::CV_r_shadersasynccompiling)
        {
            return NULL;
        }
        if (!pCode)
        {
            {
                mfOutputCompilerError(strErr, prog_text);

                Warning("Couldn't compile HW shader '%s'", GetName());
                mfSaveCGFile(prog_text, NULL);
            }
        }
    }

    return pCode;
}

void CHWShader_D3D::mfPrepareShaderDebugInfo(SHWSInstance* pInst, CHWShader_D3D* pSH, const char* szAsm, std::vector<SCGBind>& InstBindVars, void* pConstantTable)
{
    if (szAsm)
    {
        char* szInst = strstr((char*)szAsm, "pproximately ");
        if (szInst)
        {
            pInst->m_nInstructions = atoi(&szInst[13]);
        }
    }
    // Confetti Nicholas Baldwin: adding metal shader language support
    if (CParserBin::m_nPlatform == SF_D3D11 || CParserBin::m_nPlatform == SF_JASPER || CParserBin::m_nPlatform == SF_GL4 || CParserBin::m_nPlatform == SF_GLES3 ||  CParserBin::m_nPlatform == SF_METAL)
    {
        ID3D11ShaderReflection* pShaderReflection = (ID3D11ShaderReflection*) pConstantTable;

        if (pShaderReflection)
        {
            D3D11_SHADER_DESC Desc;
            pShaderReflection->GetDesc(&Desc);

            pInst->m_nInstructions = Desc.InstructionCount;
            pInst->m_nTempRegs = Desc.TempRegisterCount;
        }
    }

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_shadersdebug)
    {
        char nmdst[256];
        mfGetDstFileName(pInst, pSH, nmdst, 256, 4);

        AZStd::string szName = AZStd::string::format("%s%s.fxca", gRenDev->m_cEF.m_szCachePath.c_str(), nmdst);
        AZ::IO::HandleType statusdstFileHandle = gEnv->pCryPak->FOpen(szName.c_str(), "wb");

        if (statusdstFileHandle != AZ::IO::InvalidHandle)
        {
            gEnv->pCryPak->FPrintf(statusdstFileHandle, "\n// %s %s\n\n", "%STARTSHADER", mfProfileString(pInst->m_eClass));
            if (pSH->m_eSHClass == eHWSC_Vertex)
            {
                for (uint32 i = 0; i < (uint32)InstBindVars.size(); i++)
                {
                    SCGBind* pBind = &InstBindVars[i];
                    gEnv->pCryPak->FPrintf(statusdstFileHandle, "//   %s %s %d %d\n", "%%", pBind->m_Name.c_str(), pBind->m_RegisterCount, pBind->m_RegisterOffset);
                }
            }
            gEnv->pCryPak->FPrintf(statusdstFileHandle, "%s", szAsm);
            gEnv->pCryPak->FPrintf(statusdstFileHandle, "\n// %s\n", "%ENDSHADER");
            gEnv->pCryPak->FClose(statusdstFileHandle);
        }
        pInst->m_Handle.m_pShader = NULL;
    }
}

void CHWShader_D3D::mfPrintCompileInfo(SHWSInstance* pInst)
{
    int nConsts = 0;
    int nParams = pInst->m_pBindVars.size();
    for (int i = 0; i < nParams; i++)
    {
        SCGBind* pB = &pInst->m_pBindVars[i];
        nConsts += pB->m_RegisterCount;
    }

    char szGenName[512];
    cry_strcpy(szGenName, GetName());
    char* s = strchr(szGenName, '(');
    if (s)
    {
        s[0] = 0;
    }
    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_shadersdebug == 2)
    {
        string pName;
        SShaderCombIdent Ident(m_nMaskGenFX, pInst->m_Ident);
        gRenDev->m_cEF.mfInsertNewCombination(Ident, pInst->m_eClass, szGenName, 0, &pName, false);
        CryLog(" Compile %s (%d instructions, %d tempregs, %d/%d constants) ... ", pName.c_str(), pInst->m_nInstructions, pInst->m_nTempRegs, nParams, nConsts);
        int nSize = strlen(szGenName);
        mfGenName(pInst, &szGenName[nSize], 512 - nSize, 1);
        CryLog("           --- Cache entry: %s", szGenName);
    }
    else
    {
        int nSize = strlen(szGenName);
        mfGenName(pInst, &szGenName[nSize], 512 - nSize, 1);
        CryLog(" Compile %s (%d instructions, %d tempregs, %d/%d constants) ... ", szGenName, pInst->m_nInstructions, pInst->m_nTempRegs, nParams, nConsts);
    }

    if (gRenDev->m_cEF.m_bActivated && CRenderer::CV_r_shadersdebug > 0)
    {
        CryLog(" Shader %s (%llx)(%x)(%x)(%x)(%llx)(%llx)(%s) wasn't compiled before preactivating phase",
            GetName(), pInst->m_Ident.m_RTMask, pInst->m_Ident.m_LightMask, pInst->m_Ident.m_MDMask, pInst->m_Ident.m_MDVMask, pInst->m_Ident.m_pipelineState.opaque, pInst->m_Ident.m_STMask, mfProfileString(pInst->m_eClass));
    }
}

bool CHWShader_D3D::mfCreateShaderEnv([[maybe_unused]] int nThread, SHWSInstance* pInst, LPD3D10BLOB pShader, void* pConstantTable, LPD3D10BLOB pErrorMsgs, std::vector<SCGBind>& InstBindVars, CHWShader_D3D* pSH, bool bShaderThread, CShader* pFXShader, int nCombination, [[maybe_unused]] const char* src)
{
    // Create asm (.fxca) cache file
    assert(pInst);
    if (!pInst)
    {
        return false;
    }

    CSpinLock lock;

    if (pInst->m_pBindVars.size())
    {
        return true;
    }

    if (pShader && (nCombination < 0))
    {
#if !defined(OPENGL)
        ID3D10Blob* pAsm = NULL;
        ID3D10Blob* pSrc = (ID3D10Blob*)pShader;
        UINT* pBuf = (UINT*)pSrc->GetBufferPointer();
        D3DDisassemble(pBuf, pSrc->GetBufferSize(), 0, NULL, &pAsm);
        if (pAsm)
        {
            char* szAsm = (char*)pAsm->GetBufferPointer();
            mfPrepareShaderDebugInfo(pInst, pSH, szAsm, InstBindVars, pConstantTable);
        }
        SAFE_RELEASE(pAsm);
#endif
    }
    //assert(!pInst->m_pBindVars);

    if (pShader)
    {
        bool bVF = pSH->m_eSHClass == eHWSC_Vertex;
#if !defined(CONSOLE)
        if (CParserBin::PlatformIsConsole())
        {
            bVF = false;
        }
#endif
#if !defined(OPENGL)
        if (CParserBin::m_nPlatform & (SF_GL4 | SF_GLES3))
        {
            bVF = false;
        }
#endif
#if defined(CRY_USE_METAL)
        if (!(CParserBin::m_nPlatform & (SF_METAL)))
        {
            bVF = false;
        }
#endif
        if (bVF)
        {
            mfVertexFormat(pInst, pSH, pShader);
        }
        if (pConstantTable)
        {
            mfCreateBinds(pInst, pConstantTable, (byte*)pShader->GetBufferPointer(), (uint32)pShader->GetBufferSize());
        }
    }
    if (!(pSH->m_Flags & HWSG_PRECACHEPHASE))
    {
        int nConsts = 0;
        int nParams = pInst->m_pBindVars.size();
        for (int i = 0; i < nParams; i++)
        {
            SCGBind* pB = &pInst->m_pBindVars[i];
            nConsts += pB->m_RegisterCount;
        }
        if (gRenDev->m_cEF.m_nCombinationsProcess >= 0)
        {
            //assert(!bShaderThread);

            //if (!(gRenDev->m_cEF.m_nCombination & 0xff))
            if (!CParserBin::m_nPlatform)
            {
                CryLog("%d: Compile %s %s (%d out of %d) - (%d/%d constants) ... ", nThread,
                    mfProfileString(pInst->m_eClass), pSH->GetName(), nCombination, gRenDev->m_cEF.m_nCombinationsProcessOverall,
                    nParams, nConsts);
            }
            else
            {
                CryLog("%d: Compile %s %s (%d out of %d) ... ", nThread,
                    mfProfileString(pInst->m_eClass), pSH->GetName(), nCombination, gRenDev->m_cEF.m_nCombinationsProcessOverall);
            }
        }
        else
        {
            pSH->mfPrintCompileInfo(pInst);
        }
    }

    mfGatherFXParameters(pInst, &InstBindVars, pSH, bShaderThread ? 1 : 0, pFXShader);

    if (pShader)
    {
        mfCreateCacheItem(pInst, InstBindVars, (byte*)pShader->GetBufferPointer(), (uint32)pShader->GetBufferSize(), pSH, bShaderThread);
    }
    else if (CRenderer::CV_r_shadersCacheUnavailableShaders)
    {
        // If the shader is unavailable, cache an invalid item to avoid requesting its compilation again in future executions
        mfCreateCacheItem(pInst, InstBindVars, NULL, 0, pSH, bShaderThread);
    }

#if !defined (NULL_RENDERER)
    ID3D11ShaderReflection* pRFL = (ID3D11ShaderReflection*)pConstantTable;
    ID3D10Blob* pER = (ID3D10Blob*)pErrorMsgs;
    SAFE_RELEASE(pRFL);
    SAFE_RELEASE(pER);
#endif

    return true;
}

// Compile pixel/vertex shader for the current instance properties
bool CHWShader_D3D::mfActivate(CShader* pSH, uint32 nFlags, FXShaderToken* Table, TArray<uint32>* pSHData, bool bCompressedOnly)
{
    PROFILE_FRAME(Shader_HWShaderActivate);
    AZ_TRACE_METHOD();

    bool bResult = true;
    SHWSInstance* pInst = m_pCurInst;

    mfLogShaderRequest(pInst);

    if (mfIsValid(pInst, true) == ED3DShError_NotCompiled)
    {
        //if (!(m_Flags & HWSG_PRECACHEPHASE) && !(nFlags & HWSF_NEXT))
        //  mfSetHWStartProfile(nFlags);

        // We need a different source and desination for fpStripExtension
        // since a call to strcpy with the same src and dst results in
        // undefined behaviour
        char nameCacheUnstripped[256];
        char nameCache[256];
        float t0 = gEnv->pTimer->GetAsyncCurTime();

        mfGetDstFileName(pInst, this, nameCacheUnstripped, 256, 0);
        fpStripExtension(nameCacheUnstripped, nameCache);
        fpAddExtension(nameCache, ".fxcb");
        if (!m_pDevCache)
        {
            m_pDevCache = mfInitDevCache(nameCache, this);
        }

        int32 nSize;
        SShaderCacheHeaderItem* pCacheItem = mfGetCompressedItem(nFlags, nSize);
        if (pCacheItem)
        {
            pInst->m_bCompressed = true;
        }
        else if (bCompressedOnly)
        {
            // don't activate if shader isn't found in compressed shader data
            return false;
        }
        else
        {
            // if shader compiling is enabled, make sure the user folder shader caches are also available
            bool bReadOnly = CRenderer::CV_r_shadersAllowCompilation == 0;
            if (!m_pGlobalCache || m_pGlobalCache->m_nPlatform != CParserBin::m_nPlatform ||
                (!bReadOnly && !m_pGlobalCache->m_pRes[CACHE_USER]))
            {
                SAFE_RELEASE(m_pGlobalCache);
                bool bAsync = CRenderer::CV_r_shadersasyncactivation != 0;
                if (nFlags & HWSF_PRECACHE)
                {
                    bAsync = false;
                }
                m_pGlobalCache = mfInitCache(nameCache, this, true, m_CRC32, bReadOnly, bAsync);
            }
            if (gRenDev->m_cEF.m_nCombinationsProcess >= 0 && !gRenDev->m_cEF.m_bActivatePhase)
            {
                mfGetDstFileName(pInst, this, nameCache, 256, 0);
                fpStripExtension(nameCache, nameCache);
                fpAddExtension(nameCache, ".fxcb");
                FXShaderCacheNamesItor it = m_ShaderCacheList.find(nameCache);
                if (it == m_ShaderCacheList.end())
                {
                    m_ShaderCacheList.insert(FXShaderCacheNamesItor::value_type(nameCache, m_CRC32));
                }
            }
            pCacheItem = mfGetCacheItem(nFlags, nSize);
        }
        if (pCacheItem && pCacheItem->m_Class != 255)
        {
            if (Table && CRenderer::CV_r_shadersAllowCompilation)
            {
                mfGetCacheTokenMap(Table, pSHData, m_nMaskGenShader);
            }
            if (((m_Flags & HWSG_PRECACHEPHASE) || gRenDev->m_cEF.m_nCombinationsProcess >= 0) && !gRenDev->m_cEF.m_bActivatePhase)
            {
                byte* pData = (byte*)pCacheItem;
                SAFE_DELETE_ARRAY(pData);
                return true;
            }
            bool bRes = false;
            bRes = mfActivateCacheItem(pSH, pCacheItem, nSize, nFlags);
            byte* pData = (byte*)pCacheItem;
            SAFE_DELETE_ARRAY(pData);
            pCacheItem = NULL;
            if (CRenderer::CV_r_shaderspreactivate == 2 && !gRenDev->m_cEF.m_bActivatePhase)
            {
                t0 = gEnv->pTimer->GetAsyncCurTime() - t0;
                iLog->Log("Warning: Shader activation (%.3f ms): %s(%llx)(%x)(%x)(%x)(%llx)(%llx)(%s)...",
                        t0 * 1000.0f, GetName(), pInst->m_Ident.m_RTMask, pInst->m_Ident.m_LightMask, pInst->m_Ident.m_MDMask, pInst->m_Ident.m_MDVMask,
                        pInst->m_Ident.m_pipelineState.opaque, pInst->m_Ident.m_STMask, mfProfileString(pInst->m_eClass));
                char name[256];
                azstrcpy(name, AZ_ARRAY_SIZE(name), GetName());
                char* s = strchr(name, '(');
                if (s)
                {
                    s[0] = 0;
                }
                string pName;
                SShaderCombIdent Ident(m_nMaskGenFX, pInst->m_Ident);
                gRenDev->m_cEF.mfInsertNewCombination(Ident, pInst->m_eClass, name, 0, &pName, false);
                iLog->Log("...Shader list entry: %s (%llx)", pName.c_str(), m_nMaskGenFX);
            }
            if (bRes)
            {
                return (pInst->m_Handle.m_pShader != NULL);
            }
            pCacheItem = NULL;
        }
        else
        if (pCacheItem && pCacheItem->m_Class == 255)
        {
            byte* pData = (byte*)pCacheItem;
            SAFE_DELETE_ARRAY(pData);
            pCacheItem = NULL;
            return false;
        }
        else
        if (gRenDev->m_cEF.m_bActivatePhase)
        {
            if IsCVarConstAccess(constexpr) (CRenderer::CV_r_shadersdebug > 0)
            {
                iLog->Log("Warning: Shader %s(%llx)(%x)(%x)(%x)(%llx)(%llx)(%s) wasn't compiled before preactivating phase",
                    GetName(), pInst->m_Ident.m_RTMask, pInst->m_Ident.m_LightMask, pInst->m_Ident.m_MDMask, pInst->m_Ident.m_MDVMask, pInst->m_Ident.m_pipelineState.opaque, pInst->m_Ident.m_STMask, mfProfileString(pInst->m_eClass));
            }
            byte* pData = (byte*)pCacheItem;
            SAFE_DELETE_ARRAY(pData);
            pCacheItem = NULL;
            return false;
        }
        if (pCacheItem)
        {
            byte* pData = (byte*)pCacheItem;
            SAFE_DELETE_ARRAY(pData);
            pCacheItem = NULL;
        }
        //assert(!m_TokenData.empty());

        TArray<char> newScr;

        if (nFlags & HWSF_PRECACHE)
        {
            gRenDev->m_cEF.m_nCombinationsCompiled++;
        }

        float fTime0 = iTimer->GetAsyncCurTime();
        LPD3D10BLOB pShader = NULL;
        void* pConstantTable = NULL;
        LPD3D10BLOB pErrorMsgs = NULL;
        std::vector<SCGBind> InstBindVars;
        m_Flags |= HWSG_WASGENERATED;

        bool bScriptSuccess = false;

        if (CRenderer::CV_r_shadersAllowCompilation)
        {
            // MemReplay shows that 16kb should be enough memory to hold the script without having to reallocate
            newScr.reserve(16 * 1024);
            bScriptSuccess = mfGenerateScript(pSH, pInst, InstBindVars, nFlags, Table, pSHData, newScr);
            ASSERT_IN_SHADER(bScriptSuccess);
        }

        if (!pInst->m_bAsyncActivating && !bCompressedOnly)
        {
            // report miss in global cache to log and/or callback
            mfLogShaderCacheMiss(pInst);

            // still sumit the request line when no compiling to be sure that the shadercompiler will
            // compile it in the next build
            if (!CRenderer::CV_r_shadersAllowCompilation)
            {
                mfSubmitRequestLine(pInst);
            }
        }

        if (!bScriptSuccess)
        {
            if (!pInst->m_bAsyncActivating)
            {
                Warning("Warning: Shader %s(%llx)(%x)(%x)(%x)(%llx)(%llx)(%s) is not existing in the cache\n",
                    GetName(), pInst->m_Ident.m_RTMask, pInst->m_Ident.m_LightMask, pInst->m_Ident.m_MDMask, pInst->m_Ident.m_MDVMask, pInst->m_Ident.m_pipelineState.opaque, pInst->m_Ident.m_STMask, mfProfileString(pInst->m_eClass));
            }
            return false;
        }

        {
            PROFILE_FRAME(Shader_CompileHLSL);
            pShader = mfCompileHLSL(pSH, newScr.Data(), &pConstantTable, &pErrorMsgs, nFlags, InstBindVars);
        }

        gRenDev->m_cEF.m_ShaderCacheStats.m_nNumShaderAsyncCompiles = SShaderAsyncInfo::s_nPendingAsyncShaders;

        if (!pShader)
        {
            if (!CRenderer::CV_r_shadersAllowCompilation || pInst->IsAsyncCompiling())
            {
                return false;
            }
        }
        bResult = mfCreateShaderEnv(0, pInst, pShader, pConstantTable, pErrorMsgs, InstBindVars, this, false, pSH, gRenDev->m_cEF.m_nCombinationsProcess, newScr.Data());
        bResult &= mfUploadHW(pShader, pInst, pSH, nFlags);
        SAFE_RELEASE(pShader);

        fTime0 = iTimer->GetAsyncCurTime() - fTime0;
        //iLog->LogToConsole(" Time activate: %.3f", fTime0);
    }
    else
    if (pSHData)
    {
        mfGetCacheTokenMap(Table, pSHData, m_nMaskGenShader);
    }

    bool bSuccess = (mfIsValid(pInst, true) == ED3DShError_Ok);

    return bSuccess;
}

//////////////////////////////////////////////////////////////////////////

#ifdef SHADER_ASYNC_COMPILATION

CAsyncShaderTask::CAsyncShaderTask()
    : m_thread(this)
{
}

void CAsyncShaderTask::InsertPendingShader(SShaderAsyncInfo* pAsync)
{
    AUTO_LOCK(g_cAILock);
    pAsync->Link(&BuildList());
    CryInterlockedIncrement(&SShaderAsyncInfo::s_nPendingAsyncShaders);
}

void CAsyncShaderTask::FlushPendingShaders()
{
    SShaderAsyncInfo* pAI, * pAI2, * pAINext;
    assert(m_flush_list.m_Prev == &m_flush_list && m_flush_list.m_Next == &m_flush_list); // the flush list must be empty - cleared by the shader compile thread
    if (BuildList().m_Prev == &BuildList() && BuildList().m_Next == &BuildList())
    {
        return; // the build list is empty, might need to do some assert here
    }
    {
        AUTO_LOCK(g_cAILock);
        for (pAI = BuildList().m_Prev; pAI != &BuildList(); pAI = pAINext)
        {
            pAINext = pAI->m_Prev;
            pAI->Unlink();
            pAI->Link(&m_flush_list);
        }
    }

    // Sorting by distance
    if (gRenDev->m_cEF.m_nCombinationsProcess < 0)
    {
        for (pAI = m_flush_list.m_Next; pAI != &m_flush_list; pAI = pAI->m_Next)
        {
            assert(pAI);
            PREFAST_ASSUME(pAI);
            pAINext = NULL;
            int nFrame = pAI->m_nFrame;
            float fDist = pAI->m_fMinDistance;
            for (pAI2 = pAI->m_Next; pAI2 != &m_flush_list; pAI2 = pAI2->m_Next)
            {
                if (pAI2->m_nFrame < nFrame)
                {
                    continue;
                }
                if (pAI2->m_nFrame > nFrame || pAI2->m_fMinDistance < fDist)
                {
                    pAINext = pAI2;
                    nFrame = pAI2->m_nFrame;
                    fDist = pAI2->m_fMinDistance;
                }
            }
            if (pAINext)
            {
                assert(pAI != pAINext);
                SShaderAsyncInfo* pAIP0 = pAI->m_Prev;
                SShaderAsyncInfo* pAIP1 = pAINext->m_Prev == pAI ? pAINext : pAINext->m_Prev;

                pAI->m_Next->m_Prev = pAI->m_Prev;
                pAI->m_Prev->m_Next = pAI->m_Next;
                pAI->m_Next = pAIP1->m_Next;
                pAIP1->m_Next->m_Prev = pAI;
                pAIP1->m_Next = pAI;
                pAI->m_Prev = pAIP1;

                pAI = pAINext;

                pAI->m_Next->m_Prev = pAI->m_Prev;
                pAI->m_Prev->m_Next = pAI->m_Next;
                pAI->m_Next = pAIP0->m_Next;
                pAIP0->m_Next->m_Prev = pAI;
                pAIP0->m_Next = pAI;
                pAI->m_Prev = pAIP0;
            }
        }
    }

    for (pAI = m_flush_list.m_Next; pAI != &m_flush_list; pAI = pAINext)
    {
        pAINext = pAI->m_Next;
        assert(pAI->m_bPending);
        SubmitAsyncRequestLine(pAI);
        if (pAI->m_Text.length() > 0)
        {
            CompileAsyncShader(pAI);
        }

        CryInterlockedDecrement(&SShaderAsyncInfo::s_nPendingAsyncShaders);
        {
            AUTO_LOCK(g_cAILock);

            pAI->Unlink();
            pAI->m_bPending = 0;
            pAI->Link(&SShaderAsyncInfo::PendingListT());
        }

        if (pAI->m_bDeleteAfterRequest)
        {
            SAFE_DELETE(pAI);
        }
    }
}

bool CAsyncShaderTask::PostCompile(SShaderAsyncInfo* pAsync)
{
    bool bResult = true;
    /*if (pAsync->m_nCombination < 0 && false)
    {
      CHWShader_D3D *pSH = pAsync->m_pShader;
      CHWShader_D3D::SHWSInstance *pInst = pSH->mfGetInstance(pAsync->m_nOwner, pSH->m_nMaskGenShader);
      bResult = CHWShader_D3D::mfCreateShaderEnv(m_nThread, pInst, pAsync->m_pDevShader, pAsync->m_pConstants, pAsync->m_pErrors, pAsync->m_InstBindVars, pSH, true, pAsync->m_pFXShader, pAsync->m_nCombination);
      assert(bResult == true);
    }
    else*/
    {
        pAsync->m_nThread = m_nThread;
        pAsync->m_bPendedEnv = true;
    }
    return bResult;
}

void CAsyncShaderTask::SubmitAsyncRequestLine(SShaderAsyncInfo* pAsync)
{
    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_shadersremotecompiler)
    {
        if (!pAsync->m_shaderList.empty())
        {
            NRemoteCompiler::CShaderSrv::Instance().RequestLine(pAsync->m_shaderList.c_str(), pAsync->m_RequestLine.c_str());
        }
        else
        {
            NRemoteCompiler::CShaderSrv::Instance().RequestLine(GetShaderListFilename().c_str(), pAsync->m_RequestLine.c_str());
        }
    }
}

bool CAsyncShaderTask::CompileAsyncShader(SShaderAsyncInfo* pAsync)
{
#if !(defined(MOBILE) || defined(CONSOLE))
    const bool outputShaderSourceFiles = CRenderer::CV_r_OutputShaderSourceFiles != 0;
#else
    const bool outputShaderSourceFiles = false;
#endif
    string shaderSourceOutputFolder;

    if ( outputShaderSourceFiles )
    {
        CryFixedStringT<1024> hlslPath;

        // Create a directory for this shader type, strip the .fxcb extension from the folder name
        shaderSourceOutputFolder.Format("@usercache@/%s",pAsync->m_pShader->m_pDevCache->m_Name.c_str());
        PathUtil::RemoveExtension(shaderSourceOutputFolder);
        gEnv->pFileIO->CreatePath(shaderSourceOutputFolder);

        // Output both HLSL here, and GLSL/Metal below since those require the remote shader compiler to execute first
        hlslPath.Format("%s/[0x%08x].hlsl",shaderSourceOutputFolder.c_str(),pAsync->m_nHashInst);
        {
            AZ::IO::HandleType hlslHandle;

            if ( gEnv->pFileIO->Open(hlslPath.c_str(),AZ::IO::OpenMode::ModeWrite,hlslHandle) )
            {
                gEnv->pFileIO->Write(hlslHandle, pAsync->m_Text.c_str(), pAsync->m_Text.size());
                gEnv->pFileIO->Flush(hlslHandle);
                gEnv->pFileIO->Close(hlslHandle);
            }
        }
    }

    bool bResult = true;
    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_shadersremotecompiler)
    {
        AZStd::string lsCompilerFlags = NRemoteCompiler::CShaderSrv::Instance().GetShaderCompilerFlags(pAsync->m_eClass, pAsync->m_pipelineState, pAsync->m_MDVMask);

        std::vector<uint8> Data;
        if (NRemoteCompiler::ESOK != NRemoteCompiler::CShaderSrv::Instance().Compile(Data, pAsync->m_Profile, pAsync->m_Text.c_str(), pAsync->m_Name.c_str(), lsCompilerFlags.c_str(), pAsync->m_RequestLine.c_str()))
        {
#if !defined(NULL_RENDERER)
            D3D10CreateBlob(sizeof("D3DXCompileShader failed"), (LPD3D10BLOB*)&pAsync->m_pErrors);
            DWORD* pBuf = (DWORD*) pAsync->m_pErrors->GetBufferPointer();
                           memcpy(pBuf, "D3DXCompileShader failed", sizeof("D3DXCompileShader failed"));
#endif
            string sErrorText;

            if (!Data.empty())
            {
                sErrorText.reserve(Data.size());
                for (uint32 i = 0; i < Data.size(); i++)
                {
                    sErrorText += Data[i];
                }
            }
            else
            {
                sErrorText.assign("Unknown Error");
            }

            pAsync->m_Errors += sErrorText;

            return false;
        }

        HRESULT hr = S_OK;
        D3D10CreateBlob(Data.size(), (LPD3D10BLOB*) & pAsync->m_pDevShader);
        LPD3D10BLOB pShader = (LPD3D10BLOB)*&pAsync->m_pDevShader;
        DWORD* pBuf = (DWORD*)pShader->GetBufferPointer();
        memcpy(pBuf, &Data[0], Data.size());

        pAsync->m_pDevShader = (LPD3D10BLOB)pShader;
        pBuf = (DWORD*)pShader->GetBufferPointer();
        size_t nSize = pShader->GetBufferSize();

        if ( outputShaderSourceFiles )
        {
#if defined(OPENGL)
            CryFixedStringT<1024> glslPath;
            glslPath.Format("%s/[0x%08x].glsl",shaderSourceOutputFolder.c_str(),pAsync->m_nHashInst);
            {
                AZ::IO::HandleType glslHandle;
                if ( gEnv->pFileIO->Open(glslPath.c_str(),AZ::IO::OpenMode::ModeWrite,glslHandle) )
                {
                    gEnv->pFileIO->Write(glslHandle,static_cast<const void*>(pBuf),nSize);
                    gEnv->pFileIO->Flush(glslHandle);
                    gEnv->pFileIO->Close(glslHandle);
                }
            }
#endif

#if defined(CRY_USE_METAL)
            CryFixedStringT<1024> metallibPath;
            metallibPath.Format("%s/[0x%08x].metallib",shaderSourceOutputFolder.c_str(),pAsync->m_nHashInst);
            {
                AZ::IO::HandleType metalHandle;
                if ( gEnv->pFileIO->Open(metallibPath.c_str(),AZ::IO::OpenMode::ModeWrite,metalHandle) )
                {
                    gEnv->pFileIO->Write(metalHandle,static_cast<const void*>(pBuf),nSize);
                    gEnv->pFileIO->Flush(metalHandle);
                    gEnv->pFileIO->Close(metalHandle);
                }
            }
#endif
        }

        bool bReflect = true;

#if !defined(CONSOLE)
        if (CParserBin::PlatformIsConsole())
        {
            bReflect = false;
        }
#endif
#if !defined(OPENGL)
        if (CParserBin::m_nPlatform & (SF_GL4 | SF_GLES3))
        {
            bReflect = false;
        }
#endif
#if defined(CRY_USE_METAL)
        if (!(CParserBin::m_nPlatform & (SF_METAL)))
        {
            bReflect = false;
        }
#endif

        if (bReflect)
        {
            ID3D11ShaderReflection* pShaderReflection;
            hr = D3DReflect(pBuf, nSize, IID_ID3D11ShaderReflection, (void**)&pShaderReflection);
            if (SUCCEEDED(hr))
            {
                pAsync->m_pConstants = (void*)pShaderReflection;
            }
        }

        if (SUCCEEDED(hr))
        {
            bResult = PostCompile(pAsync);
        }
        else
        {
            pAsync->m_pDevShader    =   0;
            assert(0);
        }
    }
#if defined(WIN32) && !defined(OPENGL)
    else
    {
        static bool s_logOnce_WrongPlatform = false;
#       if !defined(_RELEASE)
        if (!s_logOnce_WrongPlatform && !(CParserBin::m_nPlatform == SF_D3D11))
        {
            s_logOnce_WrongPlatform = true;
            iLog->LogError("Trying to build non DX11 shader via internal compiler which is not supported. Please use remote compiler instead!");
        }
#       endif
        uint32 nFlags = D3D10_SHADER_PACK_MATRIX_ROW_MAJOR | D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY;
        if (CRenderer::CV_r_shadersdebug == 3) // Generate debug information and do not optimize shader for ease of debugging
        {
            nFlags |= D3D10_SHADER_DEBUG | D3D10_SHADER_SKIP_OPTIMIZATION;
        }
        else if(CRenderer::CV_r_shadersdebug == 4) // Generate debug information, but still optimize shader
        {
            nFlags |= D3D10_SHADER_DEBUG;
        }

        const char* Name = pAsync->m_pShader ? pAsync->m_pShader->GetName() : "Unknown";
        HRESULT hr = S_OK;
        hr = D3DCompile(pAsync->m_Text.c_str(), pAsync->m_Text.size(), Name, NULL, NULL, pAsync->m_Name.c_str(), pAsync->m_Profile.c_str(), nFlags, 0, (ID3DBlob * *) & pAsync->m_pDevShader, (ID3DBlob**) &pAsync->m_pErrors);
        if (FAILED(hr) || !pAsync->m_pDevShader)
        {
            if (pAsync->m_pErrors)
            {
                const char* err = (const char*)pAsync->m_pErrors->GetBufferPointer();
                pAsync->m_Errors += err;
            }
            else
            {
                pAsync->m_Errors += "D3DXCompileShader failed";
            }
            bResult = false;
        }
        else
        {
            ID3D11ShaderReflection* pShaderReflection;
            UINT* pData = (UINT*)pAsync->m_pDevShader->GetBufferPointer();
            size_t nSize = pAsync->m_pDevShader->GetBufferSize();
            hr = D3DReflect(pData, nSize, IID_ID3D11ShaderReflection, (void**)&pShaderReflection);
            if (SUCCEEDED(hr))
            {
                pAsync->m_pConstants = (void*)pShaderReflection;
                bResult = PostCompile(pAsync);
            }
            else
            {
                iLog->LogWarning("ERROR: Shader Reflection Failed!");
                assert(0);
            }
        }
    }
#endif // #if defined(WIN32) || defined(WIN64)
    return bResult;
}

void CAsyncShaderTask::CShaderThread::Run()
{
    CryThreadSetName(-1, SHADER_THREAD_NAME);

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DHWSHADERCOMPILING_CPP_SECTION_6
    #include AZ_RESTRICTED_FILE(D3DHWShaderCompiling_cpp)
#endif

    while (!m_quit)
    {
        m_task->FlushPendingShaders();
        if (!CRenderer::CV_r_shadersasynccompiling)
        {
            Sleep(250);
        }
        else
        {
            Sleep(25);
        }
    }
}

#endif

//===============================================================================================
// Export/Import

#ifdef SHADERS_SERIALIZING

bool STexSamplerFX::Export(SShaderSerializeContext& SC)
{
    bool bRes = true;

    SSTexSamplerFX TS;
    TS.m_nRTIdx = -1;
    TS.m_nsName = SC.AddString(m_szName.c_str());

    TS.m_nsNameTexture = SC.AddString(m_szTexture.c_str());

    TS.m_eTexType = m_eTexType;
    TS.m_nSamplerSlot = m_nSlotId;
    TS.m_nTexFlags = m_nTexFlags;
    if (m_nTexState > 0)
    {
        TS.m_bTexState = 1;
        memcpy(&TS.ST, &CTexture::s_TexStates[m_nTexState], sizeof(TS.ST));
        TS.ST.m_pDeviceState = NULL;
    }

    if (m_pTarget)
    {
        TS.m_nRTIdx = SC.FXTexRTs.Num();

        SHRenderTarget* pRT = m_pTarget;
        SSHRenderTarget RT;
        RT.m_eOrder = pRT->m_eOrder;
        RT.m_nProcessFlags = pRT->m_nProcessFlags;

        RT.m_nsTargetName = SC.AddString(pRT->m_TargetName.c_str());

        RT.m_nWidth = pRT->m_nWidth;
        RT.m_nHeight = pRT->m_nHeight;
        RT.m_eTF = pRT->m_eTF;
        RT.m_nIDInPool = pRT->m_nIDInPool;
        RT.m_eUpdateType = pRT->m_eUpdateType;
        RT.m_bTempDepth = pRT->m_bTempDepth;
        RT.m_ClearColor = pRT->m_ClearColor;
        RT.m_fClearDepth = pRT->m_fClearDepth;
        RT.m_nFlags = pRT->m_nFlags;
        RT.m_nFilterFlags = pRT->m_nFilterFlags;
        SC.FXTexRTs.push_back(RT);
    }

    //crude workaround for TArray push_back() bug a=b operator. Does not init a, breaks texState dtor for a
    SSTexSamplerFX* pNewSampler = SC.FXTexSamplers.AddIndex(1);
    memcpy(pNewSampler, &TS, sizeof(SSTexSamplerFX));

    return bRes;
}
bool STexSamplerFX::Import(SShaderSerializeContext& SC, SSTexSamplerFX* pTS)
{
    bool bRes = true;

    m_szName = sString(pTS->m_nsName, SC.Strings);
    m_szTexture = sString(pTS->m_nsNameTexture, SC.Strings);

    m_eTexType = pTS->m_eTexType;
    m_nSlotId = pTS->m_nSamplerSlot;
    m_nTexFlags = pTS->m_nTexFlags;
    if (pTS->m_bTexState)
    {
        m_nTexState = CTexture::GetTexState(pTS->ST);
    }

    if (pTS->m_nRTIdx != -1)
    {
        SSHRenderTarget* pRT = &SC.FXTexRTs[pTS->m_nRTIdx];

        SHRenderTarget* pDst = new SHRenderTarget;

        pDst->m_eOrder = pRT->m_eOrder;
        pDst->m_nProcessFlags = pRT->m_nProcessFlags;
        pDst->m_TargetName = sString(pRT->m_nsTargetName, SC.Strings);
        pDst->m_nWidth = pRT->m_nWidth;
        pDst->m_nHeight = pRT->m_nHeight;
        pDst->m_eTF = pRT->m_eTF;
        pDst->m_nIDInPool = pRT->m_nIDInPool;
        pDst->m_eUpdateType = pRT->m_eUpdateType;
        pDst->m_bTempDepth = pRT->m_bTempDepth != 0;
        pDst->m_ClearColor = pRT->m_ClearColor;
        pDst->m_fClearDepth = pRT->m_fClearDepth;
        pDst->m_nFlags = pRT->m_nFlags;
        pDst->m_nFilterFlags = pRT->m_nFilterFlags;
        m_pTarget = pDst;
    }

    PostLoad();

    return bRes;
}

bool SFXParam::Export(SShaderSerializeContext& SC)
{
    bool bRes = true;

    SSFXParam PR;
    PR.m_nsName = SC.AddString(m_Name.c_str());
    PR.m_nsAnnotations = SC.AddString(m_Annotations.c_str());
    PR.m_nsSemantic = SC.AddString(m_Semantic.c_str());
    PR.m_nsValues = SC.AddString(m_Values.c_str());

    PR.m_eType = m_eType;
    PR.m_nCB = m_BindingSlot;
    PR.m_nComps = m_ComponentCount;
    PR.m_nFlags = m_nFlags;
    PR.m_nParameters = m_RegisterCount;
    for ( int i=0; i<eHWSC_Num; ++i )
    {
        PR.m_nRegister[i] = m_Register[i];
    }

    SC.FXParams.push_back(PR);

    return bRes;
}
bool SFXParam::Import(SShaderSerializeContext& SC, SSFXParam* pPR)
{
    bool bRes = true;

    m_Name = sString(pPR->m_nsName, SC.Strings);
    m_Annotations = sString(pPR->m_nsAnnotations, SC.Strings);
    m_Semantic = sString(pPR->m_nsSemantic, SC.Strings);
    m_Values = sString(pPR->m_nsValues, SC.Strings);

    m_eType = pPR->m_eType;
    m_BindingSlot = pPR->m_nCB;
    m_ComponentCount = pPR->m_nComps;
    m_nFlags = pPR->m_nFlags;
    m_RegisterCount = pPR->m_nParameters;
    for ( int i=0; i<eHWSC_Num; ++i )
    {
        m_Register[i] = pPR->m_nRegister[i];
    }

    return bRes;
}

bool SFXSampler::Export(SShaderSerializeContext& SC)
{
    bool bRes = true;

    SSFXSampler PR;
    PR.m_nsName = SC.AddString(m_Name.c_str());
    PR.m_nsAnnotations = SC.AddString(m_Annotations.c_str());
    PR.m_nsSemantic = SC.AddString(m_Semantic.c_str());
    PR.m_nsValues = SC.AddString(m_Values.c_str());

    PR.m_eType = m_eType;
    PR.m_nArray = m_nArray;
    PR.m_nFlags = m_nFlags;
    for ( int i=0; i<eHWSC_Num; ++i )
    {
        PR.m_nRegister[i] = m_Register[i];
    }

    SC.FXSamplers.push_back(PR);

    return bRes;
}
bool SFXSampler::Import(SShaderSerializeContext& SC, SSFXSampler* pPR)
{
    bool bRes = true;

    m_Name = sString(pPR->m_nsName, SC.Strings);
    m_Annotations = sString(pPR->m_nsAnnotations, SC.Strings);
    m_Semantic = sString(pPR->m_nsSemantic, SC.Strings);
    m_Values = sString(pPR->m_nsValues, SC.Strings);

    m_eType = pPR->m_eType;
    m_nArray = pPR->m_nArray;
    m_nFlags = pPR->m_nFlags;
    for ( int i=0; i<eHWSC_Num; ++i )
    {
        m_Register[i] = pPR->m_nRegister[i];
    }

    return bRes;
}

bool SFXTexture::Export(SShaderSerializeContext& SC)
{
    bool bRes = true;

    SSFXTexture PR;
    PR.m_nsName = SC.AddString(m_Name.c_str());
    PR.m_nsAnnotations = SC.AddString(m_Annotations.c_str());
    PR.m_nsSemantic = SC.AddString(m_Semantic.c_str());
    PR.m_nsValues = SC.AddString(m_Values.c_str());

    PR.m_nsNameTexture = SC.AddString(m_szTexture.c_str());
    PR.m_bSRGBLookup = m_bSRGBLookup;
    PR.m_eType = m_eType;
    PR.m_nArray = m_nArray;
    PR.m_nFlags = m_nFlags;
    for ( int i=0; i<eHWSC_Num; ++i )
    {
        PR.m_nRegister[i] = m_Register[i];
    }

    SC.FXTextures.push_back(PR);

    return bRes;
}
bool SFXTexture::Import(SShaderSerializeContext& SC, SSFXTexture* pPR)
{
    bool bRes = true;

    m_Name = sString(pPR->m_nsName, SC.Strings);
    m_Annotations = sString(pPR->m_nsAnnotations, SC.Strings);
    m_Semantic = sString(pPR->m_nsSemantic, SC.Strings);
    m_Values = sString(pPR->m_nsValues, SC.Strings);

    m_szTexture = sString(pPR->m_nsNameTexture, SC.Strings);
    m_bSRGBLookup = pPR->m_bSRGBLookup;
    m_eType = pPR->m_eType;
    m_nArray = pPR->m_nArray;
    m_nFlags = pPR->m_nFlags;
    for ( int i=0; i<eHWSC_Num; ++i )
    {
        m_Register[i] = pPR->m_nRegister[i];
    }

    return bRes;
}

bool CHWShader_D3D::ExportSamplers([[maybe_unused]] SCHWShader& SHW, [[maybe_unused]] SShaderSerializeContext& SC)
{
    bool bRes = true;
    return bRes;
}
bool CHWShader::ImportSamplers([[maybe_unused]] SShaderSerializeContext& SC, [[maybe_unused]] SCHWShader* pSHW, [[maybe_unused]] byte*& pData, [[maybe_unused]] std::vector<STexSamplerRT>& Samplers)
{
    bool bRes = true;
    return bRes;
}

bool CHWShader_D3D::ExportParams(SCHWShader& SHW, [[maybe_unused]] SShaderSerializeContext& SC)
{
    bool bRes = true;
    SHW.m_nParams = 0;
    return bRes;
}
bool CHWShader::ImportParams([[maybe_unused]] SShaderSerializeContext& SC, [[maybe_unused]] SCHWShader* pSHW, [[maybe_unused]] byte*& pData, [[maybe_unused]] std::vector<SFXParam>& Params)
{
    bool bRes = true;
    return bRes;
}

bool CHWShader_D3D::Export(SShaderSerializeContext& SC)
{
    bool bRes = true;

    SCHWShader SHW;

    char str[256];
    azstrcpy(str, AZ_ARRAY_SIZE(str), GetName());
    char* c = strchr(str, '(');
    if (c)
    {
        c[0] = 0;
    }

    SHW.m_nsName = SC.AddString(str);
    SHW.m_nsNameSourceFX = SC.AddString(m_NameSourceFX.c_str());
    SHW.m_nsEntryFunc = SC.AddString(m_EntryFunc.c_str());

    SHW.m_eSHClass = m_eSHClass;
    SHW.m_dwShaderType = m_dwShaderType;
    SHW.m_nMaskGenFX = m_nMaskGenFX;
    SHW.m_nMaskGenShader = m_nMaskGenShader;
    SHW.m_nMaskOr_RT = m_nMaskOr_RT;
    SHW.m_nMaskAnd_RT = m_nMaskAnd_RT;
    SHW.m_Flags = m_Flags;

    FXShaderToken* pMap = NULL;
    TArray<uint32>* pData = &m_TokenData;
    bool bDeleteTokenData = false;

    SHW.m_nTokens = 0;
    SHW.m_nTableEntries = 0;

    SCHWShader SHWTemp = SHW;

    SHW.Export(SC.Data);

    bRes &= ExportSamplers(SHW, SC);
    bRes &= ExportParams(SHW, SC);

    if (bRes)
    {
        if (memcmp(&SHW, &SHWTemp, sizeof(SCHWShader)))
        {
            CryFatalError("Export failed");
        }
    }

    if (bDeleteTokenData)
    {
        SAFE_DELETE(pData);
        SAFE_DELETE(pMap);
    }

    return bRes;
}

CHWShader* CHWShader::Import(SShaderSerializeContext& SC, int nOffs, uint32 CRC32, CShader* pSH)
{
    if (nOffs < 0)
    {
        return NULL;
    }

    CHWShader* pHWSH = NULL;
    SCHWShader shaderHW;
    shaderHW.Import(&SC.Data[nOffs]);
    SCHWShader* pSHW = &shaderHW;

    const char* szName = sString(pSHW->m_nsName, SC.Strings);
    const char* szNameSource = sString(pSHW->m_nsNameSourceFX, SC.Strings);
    const char* szNameEntry = sString(pSHW->m_nsEntryFunc, SC.Strings);

    TArray<uint32> SHData;
    SHData.resize(pSHW->m_nTokens);

    FXShaderToken Table, * pTable = NULL;
    Table.reserve(pSHW->m_nTableEntries);

    nOffs = 0;

    //Copy string pool, TODO separate string pool for tokens!
    //TArray<char> tokenStringPool = SC.Strings;

    // Token data is no longer in export data - See CHWShader_D3D::Export where bOutputTokens == false and disables this path
    if (0)    //CRenderer::CV_r_shadersAllowCompilation)
    {
        byte* pData = &SC.Data[nOffs + sizeof(SCHWShader)];

        memcpy(&SHData[0], pData, pSHW->m_nTokens * sizeof(uint32));
        pData += pSHW->m_nTokens * sizeof(uint32);

        pTable = &Table;
        for (uint32 i = 0; i < pSHW->m_nTableEntries; i++)
        {
            // string pool method
            DWORD nToken = *(DWORD*)&pData[nOffs];
            nOffs += sizeof(DWORD);
            uint32 nTokenStrIdx = *(DWORD*)&pData[nOffs];
            nOffs += sizeof(uint32);

            if (CParserBin::m_bEndians)
            {
                SwapEndian(nToken, eBigEndian);
                SwapEndian(nTokenStrIdx, eBigEndian);
            }

            STokenD TD;
            TD.SToken = sString(nTokenStrIdx, SC.Strings);

            TD.Token = nToken;

            Table.push_back(TD);
        }
        pData += nOffs;

        std::vector<STexSamplerRT> Samplers;
        ImportSamplers(SC, pSHW, pData, Samplers);

        std::vector<SFXParam> Params;
        ImportParams(SC, pSHW, pData, Params);
    }


    //static CHWShader *mfForName(const char *name, const char *nameSource, uint32 CRC32, const char *szEntryFunc, EHWShaderClass eClass, TArray<uint32>& SHData, FXShaderToken *pTable, uint32 dwType, CShader *pFX, uint64 nMaskGen=0, uint64 nMaskGenFX=0);
    pHWSH = CHWShader::mfForName(szName,            szNameSource,            CRC32,        szNameEntry,             pSHW->m_eSHClass,     SHData,                  pTable,                pSHW->m_dwShaderType,  pSH, pSHW->m_nMaskGenShader, pSHW->m_nMaskGenFX);

    pHWSH->m_eSHClass = shaderHW.m_eSHClass;
    pHWSH->m_dwShaderType = shaderHW.m_dwShaderType;
    pHWSH->m_nMaskGenFX = shaderHW.m_nMaskGenFX;
    pHWSH->m_nMaskGenShader = shaderHW.m_nMaskGenShader;
    pHWSH->m_nMaskOr_RT = shaderHW.m_nMaskOr_RT;
    pHWSH->m_nMaskAnd_RT = shaderHW.m_nMaskAnd_RT;
    pHWSH->m_Flags = shaderHW.m_Flags;

    return pHWSH;
}

#else
bool CHWShader_D3D::Export(SShaderSerializeContext& SC)
{
    return false;
}
#endif

const char* CHWShader_D3D::mfGetActivatedCombinations([[maybe_unused]] bool bForLevel)
{
    TArray <char> Combinations;
    char* pPtr = NULL;
    uint32 i;

    for (i = 0; i < m_Insts.size(); i++)
    {
        SHWSInstance* pInst = m_Insts[i];
        char name[256];
        azstrcpy(name, AZ_ARRAY_SIZE(name), GetName());
        char* s = strchr(name, '(');
        if (s)
        {
            s[0] = 0;
        }
        string str;
        SShaderCombIdent Ident(m_nMaskGenFX, pInst->m_Ident);
        gRenDev->m_cEF.mfInsertNewCombination(Ident, pInst->m_eClass, name, 0, &str, false);
        assert (str.size());
        if (str.size())
        {
            assert(str[0] == '<' && str[2] == '>');
            string s1;
            if (str[0] == '<' && str[2] == '>')
            {
                s1.Format("<%d>%s", pInst->m_nUsed, &str[3]);
            }
            else
            {
                s1 = str;
            }
            Combinations.Copy(s1.c_str(), s1.size());
            Combinations.Copy("\n", 1);
        }
    }

    if (!Combinations.Num())
    {
        return NULL;
    }
    pPtr = new char [Combinations.Num() + 1];
    memcpy(pPtr, &Combinations[0], Combinations.Num());
    pPtr[Combinations.Num()] = 0;
    return pPtr;
}

const char* CHWShader::GetCurrentShaderCombinations(bool bForLevel)
{
    TArray <char> Combinations;
    char* pPtr = NULL;
    CCryNameTSCRC Name;
    SResourceContainer* pRL;

    Name = CHWShader::mfGetClassName(eHWSC_Vertex);
    pRL = CBaseResource::GetResourcesForClass(Name);
    int nVS = 0;
    int nPS = 0;
    if (pRL)
    {
        ResourcesMapItor itor;
        for (itor = pRL->m_RMap.begin(); itor != pRL->m_RMap.end(); itor++)
        {
            CHWShader* vs = (CHWShader*)itor->second;
            if (!vs)
            {
                continue;
            }
            const char* szCombs = vs->mfGetActivatedCombinations(bForLevel);
            if (!szCombs)
            {
                continue;
            }
            Combinations.Copy(szCombs, strlen(szCombs));
            delete [] szCombs;
            nVS++;
        }
    }

    Name = CHWShader::mfGetClassName(eHWSC_Pixel);
    pRL = CBaseResource::GetResourcesForClass(Name);
    if (pRL)
    {
        ResourcesMapItor itor;
        for (itor = pRL->m_RMap.begin(); itor != pRL->m_RMap.end(); itor++)
        {
            CHWShader* ps = (CHWShader*)itor->second;
            if (!ps)
            {
                continue;
            }
            const char* szCombs = ps->mfGetActivatedCombinations(bForLevel);
            if (!szCombs)
            {
                continue;
            }
            Combinations.Copy(szCombs, strlen(szCombs));
            delete [] szCombs;
            nPS++;
        }
    }

    if (!Combinations.Num())
    {
        return NULL;
    }
    pPtr = new char [Combinations.Num() + 1];
    memcpy(pPtr, &Combinations[0], Combinations.Num());
    pPtr[Combinations.Num()] = 0;
    return pPtr;
}

bool CHWShader::PreactivateShaders()
{
    bool bRes = true;

    if (CRenderer::CV_r_shaderspreactivate)
    {
        gRenDev->m_pRT->RC_PreactivateShaders();
    }

    return bRes;
}

void CHWShader::RT_PreactivateShaders()
{
    gRenDev->m_cEF.mfPreloadBinaryShaders();
}


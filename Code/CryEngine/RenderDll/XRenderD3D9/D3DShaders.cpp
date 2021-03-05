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
#include "Shaders/CShader.h"
#include "DevBuffer.h"
#include "RenderCapabilities.h"
#include <AzCore/std/string/conversions.h>
#include <I3DEngine.h>
#include <IResourceManager.h>
#include <AzFramework/Archive/IArchive.h>

//==================================================================================

bool CShader::FXSetTechnique(const CCryNameTSCRC& Name)
{
    assert (gRenDev->m_pRT->IsRenderThread());

    uint32 i;
    SShaderTechnique* pTech = NULL;
    for (i = 0; i < m_HWTechniques.Num(); i++)
    {
        pTech = m_HWTechniques[i];
        if (pTech && Name == pTech->m_NameCRC)
        {
            break;
        }
    }

    CRenderer* rd = gRenDev;

    if (i == m_HWTechniques.Num())
    { // not found and not set
        rd->m_RP.m_nShaderTechnique = -1;
        rd->m_RP.m_pCurTechnique = NULL;
        return false;
    }

    rd->m_RP.m_pShader = this;
    rd->m_RP.m_nShaderTechnique = i;
    rd->m_RP.m_pCurTechnique = m_HWTechniques[i];

    return true;
}

bool CShader::FXSetCSFloat(const CCryNameR& NameParam, const Vec4 fParams[], int nParams)
{
    CRenderer* rd = gRenDev;
    if (!rd->m_RP.m_pShader || !rd->m_RP.m_pCurTechnique)
    {
        return false;
    }
    SShaderPass* pPass = rd->m_RP.m_pCurPass;
    if (!pPass)
    {
        return false;
    }
    CHWShader_D3D* curCS = (CHWShader_D3D*)pPass->m_CShader;
    if (!curCS)
    {
        return false;
    }
    SCGBind* pBind = curCS->mfGetParameterBind(NameParam);
    if (!pBind)
    {
        return false;
    }

    const AZ::u32 registerCountMax = curCS->m_pCurInst->m_nMaxVecs[pBind->m_BindingSlot];
    AzRHI::ConstantBufferCache::GetInstance().WriteConstants(eHWSC_Compute, pBind, fParams, nParams, registerCountMax);
    return true;
}

bool CShader::FXSetCSFloat(const char* NameParam, const Vec4 fParams[], int nParams)
{
    return FXSetCSFloat(CCryNameR(NameParam), fParams, nParams);
}

bool CShader::FXSetPSFloat(const CCryNameR& NameParam, const Vec4* fParams, int nParams)
{
    CRenderer* rd = gRenDev;
    if (!rd->m_RP.m_pShader || !rd->m_RP.m_pCurTechnique)
    {
        return false;
    }
    SShaderPass* pPass = rd->m_RP.m_pCurPass;
    if (!pPass)
    {
        return false;
    }
    CHWShader_D3D* curPS = (CHWShader_D3D*)pPass->m_PShader;
    if (!curPS)
    {
        return false;
    }
    SCGBind* pBind = curPS->mfGetParameterBind(NameParam);
    if (!pBind)
    {
        return false;
    }

    const AZ::u32 registerCountMax = curPS->m_pCurInst->m_nMaxVecs[pBind->m_BindingSlot];
    AzRHI::ConstantBufferCache::GetInstance().WriteConstants(eHWSC_Pixel, pBind, fParams, nParams, registerCountMax);
    return true;
}

bool CShader::FXSetPSFloat(const char* NameParam, const Vec4* fParams, int nParams)
{
    return FXSetPSFloat(CCryNameR(NameParam), fParams, nParams);
}

bool CShader::FXSetVSFloat(const CCryNameR& NameParam, const Vec4* fParams, int nParams)
{
    CRenderer* rd = gRenDev;
    if (!rd->m_RP.m_pShader || !rd->m_RP.m_pCurTechnique)
    {
        return false;
    }
    SShaderPass* pPass = rd->m_RP.m_pCurPass;
    if (!pPass)
    {
        return false;
    }
    CHWShader_D3D* curVS = (CHWShader_D3D*)pPass->m_VShader;
    if (!curVS)
    {
        return false;
    }
    SCGBind* pBind = curVS->mfGetParameterBind(NameParam);
    if (!pBind)
    {
        return false;
    }

    const AZ::u32 registerCountMax = curVS->m_pCurInst->m_nMaxVecs[pBind->m_BindingSlot];
    AzRHI::ConstantBufferCache::GetInstance().WriteConstants(eHWSC_Vertex, pBind, fParams, nParams, registerCountMax);
    return true;
}

bool CShader::FXSetVSFloat(const char* NameParam, const Vec4* fParams, int nParams)
{
    return FXSetVSFloat(CCryNameR(NameParam), fParams, nParams);
}

bool CShader::FXSetGSFloat(const CCryNameR& NameParam, const Vec4* fParams, int nParams)
{
    CRenderer* rd = gRenDev;
    if (!rd->m_RP.m_pShader || !rd->m_RP.m_pCurTechnique)
    {
        return false;
    }
    SShaderPass* pPass = rd->m_RP.m_pCurPass;
    if (!pPass)
    {
        return false;
    }
    CHWShader_D3D* curGS = (CHWShader_D3D*)pPass->m_GShader;
    if (!curGS)
    {
        return false;
    }
    SCGBind* pBind = curGS->mfGetParameterBind(NameParam);
    if (!pBind)
    {
        return false;
    }

    const AZ::u32 registerCountMax = curGS->m_pCurInst->m_nMaxVecs[pBind->m_BindingSlot];
    AzRHI::ConstantBufferCache::GetInstance().WriteConstants(eHWSC_Geometry, pBind, fParams, nParams, registerCountMax);
    return true;
}

bool CShader::FXSetGSFloat(const char* NameParam, const Vec4* fParams, int nParams)
{
    return FXSetGSFloat(CCryNameR(NameParam), fParams, nParams);
}

bool CShader::FXBegin(uint32* uiPassCount, uint32 nFlags)
{
    CRenderer* rd = gRenDev;
    if (!rd->m_RP.m_pShader || !rd->m_RP.m_pCurTechnique || !rd->m_RP.m_pCurTechnique->m_Passes.Num())
    {
        return false;
    }
    *uiPassCount = rd->m_RP.m_pCurTechnique->m_Passes.Num();
    rd->m_RP.m_nFlagsShaderBegin = nFlags;
    rd->m_RP.m_pCurPass = &rd->m_RP.m_pCurTechnique->m_Passes[0];
    return true;
}

bool CShader::FXBeginPass(uint32 uiPass)
{
    FUNCTION_PROFILER_RENDER_FLAT
    CD3D9Renderer* rd = gcpRendD3D;
    if (!rd->m_RP.m_pShader || !rd->m_RP.m_pCurTechnique || uiPass >= rd->m_RP.m_pCurTechnique->m_Passes.Num())
    {
        return false;
    }
    rd->m_RP.m_pCurPass = &rd->m_RP.m_pCurTechnique->m_Passes[uiPass];
    SShaderPass* pPass = rd->m_RP.m_pCurPass;
    //assert (pPass->m_VShader && pPass->m_PShader);
    //if (!pPass->m_VShader || !pPass->m_PShader)
    //  return false;
    CHWShader_D3D* curVS = (CHWShader_D3D*)pPass->m_VShader;
    CHWShader_D3D* curPS = (CHWShader_D3D*)pPass->m_PShader;
    CHWShader_D3D* curGS = (CHWShader_D3D*)pPass->m_GShader;
    CHWShader_D3D* curCS = (CHWShader_D3D*)pPass->m_CShader;

    bool bResult = true;

    // Set Pixel-shader and all associated textures
    if (curPS)
    {
        if (rd->m_RP.m_nFlagsShaderBegin & FEF_DONTSETTEXTURES)
        {
            bResult &= curPS->mfSet(0);
        }
        else
        {
            bResult &= curPS->mfSet(HWSF_SETTEXTURES);
        }
        curPS->UpdatePerInstanceConstantBuffer();
    }
    // Set Vertex-shader
    if (curVS)
    {
        // Allow vertex shaders to bind textures.
        // Some existing vertex shaders do try to read textures, for example the shader function GetVolumetricFogAnalyticalColor
        if (rd->m_RP.m_nFlagsShaderBegin & FEF_DONTSETTEXTURES)
        {
            bResult &= curVS->mfSet(0);
        }
        else
        {
            bResult &= curVS->mfSet(HWSF_SETTEXTURES);
        }

        curVS->UpdatePerInstanceConstantBuffer();
    }

    // Set Geometry-shader
    if (curGS)
    {
        if (rd->m_RP.m_nFlagsShaderBegin & FEF_DONTSETTEXTURES)
        {
            bResult &= curGS->mfSet(0);
        }
        else
        {
            bResult &= curGS->mfSet(HWSF_SETTEXTURES);
        }

        curGS->UpdatePerInstanceConstantBuffer();
    }
    else
    {
        CHWShader_D3D::mfBindGS(NULL, NULL);
    }

    // Set Compute-shader
    if (curCS)
    {
        if (rd->m_RP.m_nFlagsShaderBegin & FEF_DONTSETTEXTURES)
        {
            bResult &= curCS->mfSet(0);
        }
        else
        {
            bResult &= curCS->mfSet(HWSF_SETTEXTURES);
        }
        curCS->UpdatePerInstanceConstantBuffer();
    }
    else
    {
        CHWShader_D3D::mfBindCS(NULL, NULL);
    }

    if (!(rd->m_RP.m_nFlagsShaderBegin & FEF_DONTSETSTATES))
    {
        rd->FX_SetState(pPass->m_RenderState);
        if (pPass->m_eCull != -1)
        {
            rd->D3DSetCull((ECull)pPass->m_eCull);
        }
    }

    return bResult;
}

bool CShader::FXEndPass()
{
    CRenderer* rd = gRenDev;
    if (!rd->m_RP.m_pShader || !rd->m_RP.m_pCurTechnique || !rd->m_RP.m_pCurTechnique->m_Passes.Num())
    {
        return false;
    }
    rd->m_RP.m_pCurPass = NULL;
    return true;
}

bool CShader::FXEnd()
{
    CRenderer* rd = gRenDev;
    if (!rd->m_RP.m_pShader || !rd->m_RP.m_pCurTechnique)
    {
        return false;
    }
    return true;
}

bool CShader::FXCommit([[maybe_unused]] const uint32 nFlags)
{
    gcpRendD3D->FX_Commit();

    return true;
}


void* CHWShader_D3D::GetVSDataForDecl(const D3D11_INPUT_ELEMENT_DESC* pDecl, int nCount, int& nDataSize)
{
    nDataSize = 0;
    CShader* pSh = CShaderMan::s_ShaderFPEmu;
    if (!pSh || !pSh->m_HWTechniques.Num() || !nCount)
    {
        return NULL;
    }
    uint32 i, j;

    SShaderTechnique* pTech = NULL;
    for (i = 0; i < pSh->m_HWTechniques.Num(); i++)
    {
        if (!azstricmp(pSh->m_HWTechniques[i]->m_NameStr.c_str(), "InputLayout"))
        {
            pTech = pSh->m_HWTechniques[i];
            break;
        }
    }
    if (!pTech || !pTech->m_Passes.Num())
    {
        return NULL;
    }
    SShaderPass& Pass = pTech->m_Passes[0];
    CHWShader_D3D* pVS = (CHWShader_D3D*)Pass.m_VShader;
    if (!pVS)
    {
        return NULL;
    }

    uint32 nMask = 0;
    //m_RP.m_FlagsShader_LT = m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurColorOp[0] | (m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurAlphaOp[0] << 8) | (m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurColorArg[0] << 16) | (m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurAlphaArg[0] << 24);

    for (i = 0; i < nCount; i++)
    {
        const D3D11_INPUT_ELEMENT_DESC& Desc = pDecl[i];
        if (Desc.InputSlot != 0)
        {
            nMask |= 1 << Desc.InputSlot;
            if (nMask & VSM_TANGENTS)
            {
                for (j = 0; j < nCount; j++)
                {
                    const D3D11_INPUT_ELEMENT_DESC& Desc2 = pDecl[j];
                    if (!strcmp(Desc2.SemanticName, "BITANGENT") || !strcmp(Desc2.SemanticName, "BINORMAL"))
                    {
                        nMask |= (1 << VSF_QTANGENTS);
                        break;
                    }
                }
            }
        }
        else
        if (!strcmp(Desc.SemanticName, "POSITION"))
        {
            if (Desc.Format == DXGI_FORMAT_R32G32_FLOAT)
            {
                nMask |= 0 << 8;
            }
            else
            if (Desc.Format == DXGI_FORMAT_R32G32B32_FLOAT)
            {
                nMask |= 1 << 8;
            }
            else
            if (Desc.Format == DXGI_FORMAT_R16G16B16A16_FLOAT || Desc.Format == DXGI_FORMAT_R32G32B32A32_FLOAT)
            {
                nMask |= 2 << 8;
            }
            else
            {
                assert(0);
            }
        }
        else
        if (!strcmp(Desc.SemanticName, "TEXCOORD"))
        {
            int nShift = 0;
            if (Desc.SemanticIndex == 0)
            {
                nMask |= eCA_Texture << 16;
                nShift = 10;
                if (Desc.Format == DXGI_FORMAT_R32G32_FLOAT || Desc.Format == DXGI_FORMAT_R16G16_FLOAT)
                {
                    nMask |= 0 << nShift;
                }
                else
                if (Desc.Format == DXGI_FORMAT_R32G32B32_FLOAT)
                {
                    nMask |= 1 << nShift;
                }
                else
                if (Desc.Format == DXGI_FORMAT_R16G16B16A16_FLOAT || Desc.Format == DXGI_FORMAT_R32G32B32A32_FLOAT)
                {
                    nMask |= 2 << nShift;
                }
                else
                {
                    assert(0);
                }
            }
            else
            if (Desc.SemanticIndex == 1)
            {
                if (nMask & (eCA_Constant << 19))
                {
                    // PSIZE and TEX1 used together
                    nMask &= ~(0x7 << 19);
                    nMask |= eCA_Previous << 19;
                }
                else
                {
                    nMask |= eCA_Texture1 << 19;
                }
            }
            else
            {
                assert(0);
            }
        }
        else
        if (!strcmp(Desc.SemanticName, "COLOR"))
        {
            int nShift = 0;
            if (Desc.SemanticIndex == 0)
            {
                nMask |= eCA_Diffuse << 24;
                nShift = 12;
                if (Desc.Format == DXGI_FORMAT_R32G32B32_FLOAT)
                {
                    nMask |= 1 << nShift;
                }
                else
                if (Desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM || Desc.Format == DXGI_FORMAT_R32G32B32A32_FLOAT)
                {
                    nMask |= 2 << nShift;
                }
                else
                {
                    assert(0);
                }
            }
            else
            if (Desc.SemanticIndex == 1)
            {
                nMask |= eCA_Specular << 27;
            }
            else
            {
                assert(0);
            }
        }
        else
        if (!strcmp(Desc.SemanticName, "NORMAL"))
        {
            if (Desc.SemanticIndex == 0)
            {
                nMask |= eCA_Normal << 27;
            }
            else
            {
                assert(0);
            }
            //assert (Desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM);
        }
        else
        if (!strcmp(Desc.SemanticName, "PSIZE"))
        {
            if (Desc.SemanticIndex == 0)
            {
                if (nMask & (eCA_Texture1 << 19))
                {
                    // PSIZE and TEX1 used together
                    nMask &= ~(0x7 << 19);
                    nMask |= eCA_Previous << 19;
                }
                else
                {
                    nMask |= eCA_Constant << 19;
                }
            }
            else
            {
                assert(0);
            }
            assert (Desc.Format == DXGI_FORMAT_R32G32B32A32_FLOAT);
        }
        else
        {
            assert(0);
        }
    }

    SShaderCombIdent Ident;
    Ident.m_LightMask = nMask;
    Ident.m_RTMask = 0;
    Ident.m_MDMask = 0;
    Ident.m_MDVMask = 0;
    Ident.m_GLMask = 0;
    Ident.m_STMask = 0;

    SHWSInstance* pI = pVS->m_pCurInst;

    uint32 nFlags = HWSF_STOREDATA;
    SHWSInstance* pInst = pVS->mfGetInstance(pSh, Ident, nFlags);
    if (!pVS->mfCheckActivation(pSh, pInst, nFlags))
    {
        pVS->m_pCurInst = pI;
        return NULL;
    }
    pVS->m_pCurInst = pI;

    nDataSize = pInst->m_nDataSize;
    return pInst->m_pShaderData;
}

//===================================================================================

void CRenderer::RefreshSystemShaders()
{
    // make sure all system shaders are properly refreshed during loading!
#if defined(FEATURE_SVO_GI)
    gRenDev->m_cEF.mfRefreshSystemShader("Total_Illumination", CShaderMan::s_ShaderSVOGI);
#endif
    gRenDev->m_cEF.mfRefreshSystemShader("Common", CShaderMan::s_ShaderCommon);
    gRenDev->m_cEF.mfRefreshSystemShader("Debug", CShaderMan::s_ShaderDebug);
    gRenDev->m_cEF.mfRefreshSystemShader("DeferredCaustics", CShaderMan::s_ShaderDeferredCaustics);
    gRenDev->m_cEF.mfRefreshSystemShader("DeferredRain", CShaderMan::s_ShaderDeferredRain);
    gRenDev->m_cEF.mfRefreshSystemShader("DeferredSnow", CShaderMan::s_ShaderDeferredSnow);
    gRenDev->m_cEF.mfRefreshSystemShader("DeferredShading", CShaderMan::s_shDeferredShading);
    gRenDev->m_cEF.mfRefreshSystemShader("DepthOfField", CShaderMan::s_shPostDepthOfField);
    gRenDev->m_cEF.mfRefreshSystemShader("DXTCompress", CShaderMan::s_ShaderDXTCompress);
    gRenDev->m_cEF.mfRefreshSystemShader("LensOptics", CShaderMan::s_ShaderLensOptics);
    gRenDev->m_cEF.mfRefreshSystemShader("SoftOcclusionQuery", CShaderMan::s_ShaderSoftOcclusionQuery);
    gRenDev->m_cEF.mfRefreshSystemShader("MotionBlur", CShaderMan::s_shPostMotionBlur);
    gRenDev->m_cEF.mfRefreshSystemShader("OcclusionTest", CShaderMan::s_ShaderOcclTest);
    gRenDev->m_cEF.mfRefreshSystemShader("PostEffectsGame", CShaderMan::s_shPostEffectsGame);
    gRenDev->m_cEF.mfRefreshSystemShader("ShadowBlur", CShaderMan::s_ShaderShadowBlur);
    gRenDev->m_cEF.mfRefreshSystemShader("Stereo", CShaderMan::s_ShaderStereo);
    gRenDev->m_cEF.mfRefreshSystemShader("Sunshafts", CShaderMan::s_shPostSunShafts);
    gRenDev->m_cEF.mfRefreshSystemShader("Fur", CShaderMan::s_ShaderFur);
}

bool CD3D9Renderer::FX_SetFPMode()
{
    assert (gRenDev->m_pRT->IsRenderThread());

    if (!(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_FP_DIRTY) && CShaderMan::s_ShaderFPEmu == m_RP.m_pShader)
    {
        return true;
    }
    if (m_bDeviceLost)
    {
        return false;
    }
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags &= ~RBPF_FP_DIRTY;
    m_RP.m_pCurObject = m_RP.m_pIdendityRenderObject;
    CShader* pSh = CShaderMan::s_ShaderFPEmu;
    if (!pSh || !pSh->m_HWTechniques.Num())
    {
        return false;
    }
    m_RP.m_FlagsShader_LT = m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurColorOp | (m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurAlphaOp << 8) | (m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurColorArg << 16) | (m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurAlphaArg << 24);
    m_RP.m_FlagsShader_LT |= (m_RP.m_TI[m_RP.m_nProcessThreadID].m_sRGBWrite ? 1 : 0) << 22;

    if (CTexture::s_TexStages[0].m_DevTexture && CTexture::s_TexStages[0].m_DevTexture->IsCube())
    {
        m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_CUBEMAP0];
    }
    else
    {
        m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_CUBEMAP0];
    }

    m_RP.m_pShader = pSh;
    m_RP.m_pCurTechnique = pSh->m_HWTechniques[0];

    bool bRes = pSh->FXBegin(&m_RP.m_nNumRendPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    if (!bRes)
    {
        return false;
    }
    bRes = pSh->FXBeginPass(0);
    FX_Commit();
    return bRes;
}

bool CD3D9Renderer::FX_SetUIMode()
{
    assert (gRenDev->m_pRT->IsRenderThread());

    if (!(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_FP_DIRTY) && CShaderMan::s_ShaderUI == m_RP.m_pShader)
    {
        return true;
    }
    if (m_bDeviceLost)
    {
        return false;
    }
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags &= ~RBPF_FP_DIRTY;
    m_RP.m_pCurObject = m_RP.m_pIdendityRenderObject;
    CShader* pSh = CShaderMan::s_ShaderUI;
    if (!pSh || !pSh->m_HWTechniques.Num())
    {
        return false;
    }
    m_RP.m_FlagsShader_LT = m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurColorOp | (m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurAlphaOp << 8) | (m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurColorArg << 16) | (m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurAlphaArg << 24);
    m_RP.m_FlagsShader_LT |= (m_RP.m_TI[m_RP.m_nProcessThreadID].m_sRGBWrite ? 1 : 0) << 22;

    if (CTexture::s_TexStages[0].m_DevTexture && CTexture::s_TexStages[0].m_DevTexture->IsCube())
    {
        m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_CUBEMAP0];
    }
    else
    {
        m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_CUBEMAP0];
    }

    m_RP.m_pShader = pSh;
    m_RP.m_pCurTechnique = pSh->m_HWTechniques[0];

    bool bRes = pSh->FXBegin(&m_RP.m_nNumRendPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    if (!bRes)
    {
        return false;
    }
    bRes = pSh->FXBeginPass(0);
    FX_Commit();
    return bRes;
}

void CShaderMan::mfCheckObjectDependParams(std::vector<SCGParam>& PNoObj, std::vector<SCGParam>& PObj, [[maybe_unused]] EHWShaderClass eSH, [[maybe_unused]] CShader* pFXShader)
{
    if (!PNoObj.size())
    {
        return;
    }
    uint32 i;
    for (i = 0; i < PNoObj.size(); i++)
    {
        SCGParam* prNoObj = &PNoObj[i];
        if ((prNoObj->m_eCGParamType & 0xff) == ECGP_PM_Tweakable)
        {
            int nType = prNoObj->m_eCGParamType & 0xff;
            prNoObj->m_eCGParamType = (ECGParam)nType;
        }

        if (prNoObj->m_Flags & PF_INSTANCE)
        {
            PObj.push_back(PNoObj[i]);
            PNoObj.erase(PNoObj.begin() + i);
            i--;
        }
        else
        if (prNoObj->m_Flags & PF_MATERIAL)
        {
            PNoObj.erase(PNoObj.begin() + i);
            i--;
        }
    }
    if (PObj.size())
    {
        int n = -1;
        for (i = 0; i < PObj.size(); i++)
        {
            if (PObj[i].m_eCGParamType == ECGP_Matr_PI_ViewProj)
            {
                n = i;
                break;
            }
        }
        if (n > 0)
        {
            SCGParam pr = PObj[n];
            PObj[n] = PObj[0];
            PObj[0] = pr;
        }
    }
}

bool CShaderMan::mfPreactivate2(CResFileLookupDataMan& LevelLookup,
    const AZStd::string& pathPerLevel, const AZStd::string& pathGlobal,
    bool bVS, bool bPersistent)
{
    bool bRes = true;
    
    AZStd::string allFilesPath = pathPerLevel + "/*.*";
    
    AZ::IO::ArchiveFileIterator handle = gEnv->pCryPak->FindFirst(allFilesPath.c_str());
    if (!handle)
    {
        return bRes;
    }

    do
    {
        if (handle.m_filename.front() == '.')
        {
            continue;
        }
        
        AZStd::string fileInfoPerLevel = AZStd::string::format("%s/%.*s", pathPerLevel.c_str(), aznumeric_cast<int>(handle.m_filename.size()), handle.m_filename.data());
        AZStd::string fileInfoGlobal   = AZStd::string::format("%s/%.*s", pathGlobal.c_str(),   aznumeric_cast<int>(handle.m_filename.size()), handle.m_filename.data());
        
        if((handle.m_fileDesc.nAttrib & AZ::IO::FileDesc::Attribute::Subdirectory) == AZ::IO::FileDesc::Attribute::Subdirectory)
        {
            bRes &= mfPreactivate2(LevelLookup, fileInfoPerLevel, fileInfoGlobal, bVS, bPersistent);
            continue;
        }
        CResFile* pRes = new CResFile(fileInfoPerLevel.c_str());
        int bR = pRes ? pRes->mfOpen(RA_READ, &LevelLookup) : 0;
        if (!bR)
        {
            Warning("ShaderCache rejected (damaged?) file %s: %s", fileInfoPerLevel.c_str(), pRes->mfGetError() ? pRes->mfGetError() : "<unknown reason>");
            bRes = false;
            SAFE_DELETE(pRes);
            continue;
        }

        SResFileLookupData* pLookupGlobal = gRenDev->m_cEF.m_ResLookupDataMan[CACHE_READONLY].GetData(fileInfoGlobal.c_str());
        if (!pLookupGlobal)
        {
            SAFE_DELETE(pRes);
            continue;
        }
        SResFileLookupData* pLookupLevel = NULL;
        if (!bPersistent)
        {
            CCryNameTSCRC name = LevelLookup.AdjustName(pRes->mfGetFileName());
            pLookupLevel = LevelLookup.GetData(name);
        }
        else
        {
            // Startup cache
            const char* szName = pRes->mfGetFileName();
            CCryNameTSCRC name;
            if (_strnicmp(szName, "ShaderCache/", 12) == 0)
            {
                stack_string sName = stack_string(g_shaderCache) + stack_string(&szName[12]);
                name = sName.c_str();
            }
            else if (_strnicmp(szName, g_shaderCache, 14) == 0)
            {
                name = LevelLookup.AdjustName(pRes->mfGetFileName());
            }
            else
            {
                Warning("Wrong virtual directory in ShaderCacheStartup.pak");
                SAFE_DELETE(pRes);
                continue;
            }
            pLookupLevel = LevelLookup.GetData(name);
        }
        if (!pLookupLevel)
        {
            SAFE_DELETE(pRes);
            continue;
        }
        if (pLookupLevel->m_CacheMajorVer != pLookupGlobal->m_CacheMajorVer || pLookupLevel->m_CacheMinorVer != pLookupGlobal->m_CacheMinorVer || pLookupLevel->m_CRC32 != pLookupGlobal->m_CRC32)
        {
            SAFE_DELETE(pRes);
            continue;
        }
        const char* s = fileInfoPerLevel.c_str();
        const uint32 nLen = strlen(s);
        int nStart = -1;
        int nEnd = -1;
        bool bPC = false;
        for (uint32 i = 0; i < nLen; i++)
        {
            uint32 n = nLen - i - 1;
            char c = s[n];
            if (c == '.')
            {
                nEnd = n;
            }
            else
            if (!bPC)
            {
                if (c == '@' || c == '/')
                {
                    bPC = true;
                }
            }
            else
            if (c == '/')
            {
                nStart = n + 1;
                break;
            }
        }
        if (nStart < 0 || nEnd < 0)
        {
            SAFE_DELETE(pRes);
            continue;
        }
        char str[128];
        memcpy(str, &s[nStart], nEnd - nStart);
        str[nEnd - nStart] = 0;
        CCryNameTSCRC Name = str;
        FXCompressedShadersItor it = CHWShader::m_CompressedShaders.find(Name);
        SHWActivatedShader* pAS = NULL;
        if (it == CHWShader::m_CompressedShaders.end())
        {
            pAS = new SHWActivatedShader;
            pAS->m_bPersistent = bPersistent;
            CHWShader::m_CompressedShaders.insert(FXCompressedShadersItor::value_type(Name, pAS));
        }
        else
        {
            pAS = it->second;
        }
        ResDir* pDir = pRes->mfGetDirectory();
        for (uint32 i = 0; i < pDir->size(); i++)
        {
            SDirEntry& DE = (*pDir)[i];
            if IsCVarConstAccess(constexpr) (CRenderer::CV_r_shadersdebug == 3 || CRenderer::CV_r_shadersdebug == 4)
            {
                iLog->Log("---Cache: PreactivateForLevel %s': 0x%x", pRes->mfGetFileName(), DE.Name.get());
            }

            FXCompressedShaderRemapItor itR = pAS->m_Remap.find(DE.Name);
            uint32 nIDDev = DE.Name.get();
            assert(DE.offset > 0);
            if (itR == pAS->m_Remap.end())
            {
                pAS->m_Remap.insert(FXCompressedShaderRemapItor::value_type(DE.Name, nIDDev));
                itR = pAS->m_Remap.find(DE.Name);
            }
            FXCompressedShaderItor itS = pAS->m_CompressedShaders.find(itR->second);
            if (itS == pAS->m_CompressedShaders.end())
            {
                uint32 nSizeCompressed;
                uint32 nSizeDecompressed;
                byte* pDataCompressed = pRes->mfFileReadCompressed(&DE, nSizeDecompressed, nSizeCompressed);
                if (!pDataCompressed)
                {
                    SAFE_DELETE_ARRAY(pDataCompressed);
                    bRes = false;
                    continue;
                }

                // only store compressed data - don't store token data for example because this is not compressed
                if (nSizeCompressed == nSizeDecompressed)
                {
                    SAFE_DELETE_ARRAY(pDataCompressed);
                    continue;
                }

                if (nSizeCompressed > std::numeric_limits<uint32>::max() || nSizeDecompressed > std::numeric_limits<uint32>::max())
                {
                    CryFatalError("size of shader %s (compressed: %u, decompressed: %u) is too big to be stored in uint32", pRes->mfGetFileName(), (uint)nSizeCompressed, (uint)nSizeDecompressed);
                    SAFE_DELETE_ARRAY(pDataCompressed);
                    bRes = false;
                    continue;
                }

                SCompressedData CD;
                CD.m_nSizeCompressedShader = nSizeCompressed;
                CD.m_nSizeDecompressedShader = nSizeDecompressed;
                CD.m_pCompressedShader = pDataCompressed;
                assert (CD.m_nSizeCompressedShader != CD.m_nSizeDecompressedShader);

                /*byte *pData = new byte[CD.m_nSizeDecompressedShader+1];
                pData[CD.m_nSizeDecompressedShader] = 0xaa;
                Decodem(CD.m_pCompressedShader, pData, CD.m_nSizeCompressedShader);
                assert(pData[CD.m_nSizeDecompressedShader] == 0xaa);
                SShaderCacheHeaderItem *pIt = (SShaderCacheHeaderItem *)pData;
                if (CParserBin::m_bEndians)
                  SwapEndian(*pIt, eBigEndian);
                SAFE_DELETE_ARRAY(pData);*/

                pAS->m_CompressedShaders.insert(FXCompressedShaderItor::value_type(nIDDev, CD));
            }
            else
            {
#ifdef _DEBUG
                uint32 nSizeCompressed;
                uint32 nSizeDecompressed;
                byte* pDataCompressed = pRes->mfFileReadCompressed(&DE, nSizeDecompressed, nSizeCompressed);
                assert(nSizeCompressed < 65536 && nSizeDecompressed < 65536);
                assert(pDataCompressed);
                if (pDataCompressed)
                {
                    SCompressedData& CD1 = itS->second;
                    assert(CD1.m_nSizeCompressedShader == nSizeCompressed);
                    SAFE_DELETE_ARRAY(pDataCompressed);
                }
#endif
            }
        }
        SAFE_DELETE(pRes);
    } while (handle = gEnv->pCryPak->FindNext(handle));

    gEnv->pCryPak->FindClose (handle);

    return bRes;
}

int SHWActivatedShader::Size()
{
    int nSize = sizeof(SHWActivatedShader);
    nSize += sizeOfMap(m_CompressedShaders);
    nSize += sizeOfMapS(m_Remap);

    return nSize;
}

void SHWActivatedShader::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->AddObject(this, sizeof(SHWActivatedShader));
    pSizer->AddObject(m_CompressedShaders);
    pSizer->AddObject(m_Remap);
}

SHWActivatedShader::~SHWActivatedShader()
{
    FXCompressedShaderItor it;
    for (it = m_CompressedShaders.begin(); it != m_CompressedShaders.end(); ++it)
    {
        SCompressedData& Data = it->second;
        SAFE_DELETE_ARRAY(Data.m_pCompressedShader);
    }
    m_CompressedShaders.clear();
    m_Remap.clear();
}
bool CShaderMan::mfReleasePreactivatedShaderData()
{
    bool bRes = true;
    FXCompressedShadersItor it;
    std::vector<CCryNameTSCRC> DelStuff;
    for (it = CHWShader::m_CompressedShaders.begin(); it != CHWShader::m_CompressedShaders.end(); ++it)
    {
        SHWActivatedShader* pAS = it->second;
        if (pAS && !pAS->m_bPersistent)
        {
            SAFE_DELETE(pAS);
            DelStuff.push_back(it->first);
        }
    }
    uint32 i;
    for (i = 0; i < DelStuff.size(); i++)
    {
        CHWShader::m_CompressedShaders.erase(DelStuff[i]);
    }
    return true;
}

bool CShaderMan::mfPreactivateShaders2(
    [[maybe_unused]] const char* szPak, const char* szPath, bool bPersistent, [[maybe_unused]] const char* szBindRoot)
{
    mfReleasePreactivatedShaderData();

    bool bRes = true;

    // Get shader platform name and make it lower
    AZStd::string shaderLanguageName = GetShaderLanguageName();
    AZStd::to_lower(shaderLanguageName.begin(), shaderLanguageName.end());

    const AZStd::string pathPerLevel = AZStd::string::format( "%s%s/", szPath, shaderLanguageName.c_str());

    CResFileLookupDataMan LevelLookup;
    bool bLoaded = LevelLookup.LoadData((pathPerLevel + "lookupdata.bin").c_str(), CParserBin::m_bEndians, true);
    if (bLoaded)
    {
        const AZStd::string& pathGlobal = gRenDev->m_cEF.m_ShadersCache;
        
        const char* cgcShaders = "cgcshaders";
        const char* cgdShaders = "cgdshaders";
        const char* cggShaders = "cggshaders";
        const char* cghShaders = "cghshaders";
        const char* cgpShaders = "cgpshaders";
        const char* cgvShaders = "cgvshaders";
        
        bRes &= mfPreactivate2(LevelLookup, pathPerLevel + cgcShaders, pathPerLevel + cgcShaders, false, bPersistent);
        bRes &= mfPreactivate2(LevelLookup, pathPerLevel + cgdShaders, pathPerLevel + cgdShaders, false, bPersistent);
        bRes &= mfPreactivate2(LevelLookup, pathPerLevel + cggShaders, pathPerLevel + cggShaders, false, bPersistent);
        bRes &= mfPreactivate2(LevelLookup, pathPerLevel + cghShaders, pathPerLevel + cghShaders, false, bPersistent);
        bRes &= mfPreactivate2(LevelLookup, pathPerLevel + cgpShaders, pathPerLevel + cgpShaders, false, bPersistent);
        bRes &= mfPreactivate2(LevelLookup, pathPerLevel + cgvShaders, pathPerLevel + cgvShaders, true,  bPersistent);
    }

    return bRes;
}


void CHWShader_D3D::mfLogShaderCacheMiss(SHWSInstance* pInst)
{
    CShaderMan& Man = gRenDev->m_cEF;

    // update the stats
    Man.m_ShaderCacheStats.m_nGlobalShaderCacheMisses++;

    // don't do anything else if CVar is disabled and no callback is registered
    if (CRenderer::CV_r_shaderslogcachemisses == 0 && Man.m_ShaderCacheMissCallback == 0)
    {
        return;
    }

    char nameCache[256];
    azstrcpy(nameCache, AZ_ARRAY_SIZE(nameCache), GetName());
    char* s = strchr(nameCache, '(');
    if (s)
    {
        s[0] = 0;
    }
    string sNew;
    SShaderCombIdent Ident = pInst->m_Ident;
    Ident.m_GLMask = m_nMaskGenFX;
    gRenDev->m_cEF.mfInsertNewCombination(Ident, pInst->m_eClass, nameCache, 0, &sNew, 0);

    if (CRenderer::CV_r_shaderslogcachemisses > 1 && !gRenDev->m_bShaderCacheGen && !gEnv->IsEditor())
    {
        CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_WARNING, "[SHADERS] GCM Global Cache Miss: %s\n", sNew.c_str());
    }

    string sEntry;

    sEntry = "[";
    sEntry += GetShaderListFilename().c_str();
    sEntry += "]";
    sEntry += sNew;

    CCryNameTSCRC cryName(sEntry);

    // do we already contain this entry (vec is sorted so lower bound gives us already the good position to insert)
    CShaderMan::ShaderCacheMissesVec::iterator it = std::lower_bound(Man.m_ShaderCacheMisses.begin(), Man.m_ShaderCacheMisses.end(), cryName);
    if (it == Man.m_ShaderCacheMisses.end() || cryName != (*it))
    {
        Man.m_ShaderCacheMisses.insert(it, cryName);

        if (CRenderer::CV_r_shaderslogcachemisses)
        {
            AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
            gEnv->pFileIO->Open(Man.m_ShaderCacheMissPath.c_str(), AZ::IO::OpenMode::ModeAppend | AZ::IO::OpenMode::ModeUpdate, fileHandle);
            if (fileHandle != AZ::IO::InvalidHandle)
            {
                AZ::IO::Print(fileHandle, "%s\n", sEntry.c_str());
                gEnv->pFileIO->Close(fileHandle);
            }
        }

        // call callback if provided to inform client about misses
        if (Man.m_ShaderCacheMissCallback)
        {
            (*Man.m_ShaderCacheMissCallback)(sEntry.c_str());
        }
    }
}

void CHWShader_D3D::mfLogShaderRequest([[maybe_unused]] SHWSInstance* pInst)
{
#if !defined(_RELEASE)
    IF (CRenderer::CV_r_shaderssubmitrequestline > 1, 0)
    {
        mfSubmitRequestLine(pInst);
    }
#endif
}

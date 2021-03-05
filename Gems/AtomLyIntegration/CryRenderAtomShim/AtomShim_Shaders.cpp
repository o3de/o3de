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

//============================================================================

bool CShader::FXSetTechnique([[maybe_unused]] const CCryNameTSCRC& szName)
{
    return true;
}

bool CShader::FXSetPSFloat([[maybe_unused]] const CCryNameR& NameParam, [[maybe_unused]] const Vec4* fParams, [[maybe_unused]] int nParams)
{
    return true;
}

bool CShader::FXSetPSFloat([[maybe_unused]] const char* NameParam, [[maybe_unused]] const Vec4* fParams, [[maybe_unused]] int nParams)
{
    return true;
}

bool CShader::FXSetVSFloat([[maybe_unused]] const CCryNameR& NameParam, [[maybe_unused]] const Vec4* fParams, [[maybe_unused]] int nParams)
{
    return true;
}

bool CShader::FXSetVSFloat([[maybe_unused]] const char* NameParam, [[maybe_unused]] const Vec4* fParams, [[maybe_unused]] int nParams)
{
    return true;
}

bool CShader::FXSetGSFloat([[maybe_unused]] const CCryNameR& NameParam, [[maybe_unused]] const Vec4* fParams, [[maybe_unused]] int nParams)
{
    return true;
}

bool CShader::FXSetGSFloat([[maybe_unused]] const char* NameParam, [[maybe_unused]] const Vec4* fParams, [[maybe_unused]] int nParams)
{
    return true;
}

bool CShader::FXSetCSFloat([[maybe_unused]] const CCryNameR& NameParam, [[maybe_unused]] const Vec4* fParams, [[maybe_unused]] int nParams)
{
    return true;
}

bool CShader::FXSetCSFloat([[maybe_unused]] const char* NameParam, [[maybe_unused]] const Vec4* fParams, [[maybe_unused]] int nParams)
{
    return true;
}
bool CShader::FXBegin([[maybe_unused]] uint32* uiPassCount, [[maybe_unused]] uint32 nFlags)
{
    return true;
}

bool CShader::FXBeginPass([[maybe_unused]] uint32 uiPass)
{
    return true;
}

bool CShader::FXEndPass()
{
    return true;
}

bool CShader::FXEnd()
{
    return true;
}

bool CShader::FXCommit([[maybe_unused]] const uint32 nFlags)
{
    return true;
}

//===================================================================================

FXShaderCache CHWShader::m_ShaderCache;
FXShaderCacheNames CHWShader::m_ShaderCacheList;

void CRenderer::RefreshSystemShaders()
{
}

SShaderCache::~SShaderCache()
{
    CHWShader::m_ShaderCache.erase(m_Name);
    SAFE_DELETE(m_pRes[CACHE_USER]);
    SAFE_DELETE(m_pRes[CACHE_READONLY]);
}

SShaderCache* CHWShader::mfInitCache([[maybe_unused]] const char* name, [[maybe_unused]] CHWShader* pSH, [[maybe_unused]] bool bCheckValid, [[maybe_unused]] uint32 CRC32, [[maybe_unused]] bool bReadOnly, [[maybe_unused]] bool bAsync)
{
    return NULL;
}

#if !defined(CONSOLE)
bool CHWShader::mfOptimiseCacheFile([[maybe_unused]] SShaderCache* pCache, [[maybe_unused]] bool bForce, [[maybe_unused]] SOptimiseStats* Stats)
{
    return true;
}
#endif

bool CHWShader::PreactivateShaders()
{
    bool bRes = true;
    return bRes;
}
void CHWShader::RT_PreactivateShaders()
{
}

const char* CHWShader::GetCurrentShaderCombinations([[maybe_unused]] bool bLevel)
{
    return "";
}

void CHWShader::mfFlushPendedShadersWait([[maybe_unused]] int nMaxAllowed)
{
}

void CShaderResources::Rebuild([[maybe_unused]] IShader* pSH, [[maybe_unused]] AzRHI::ConstantBufferUsage usage)
{
}

void CShaderResources::CloneConstants([[maybe_unused]] const IRenderShaderResources* pSrc)
{
}

void CShaderResources::ReleaseConstants()
{
}

void CShaderResources::UpdateConstants([[maybe_unused]] IShader* pSH)
{
}

void CShader::mfFlushPendedShaders()
{
}

void SShaderCache::Cleanup(void)
{
}

void CShaderResources::AdjustForSpec()
{
}


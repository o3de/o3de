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

// Description : NULL device specific implementation and extensions handling.


#include "RenderDll_precompiled.h"
#include "NULL_Renderer.h"


bool CNULLRenderer::SetGammaDelta(const float fGamma)
{
    m_fDeltaGamma = fGamma;
    return true;
}

int CNULLRenderer::EnumDisplayFormats([[maybe_unused]] SDispFormat* Formats)
{
    return 0;
}

bool CNULLRenderer::ChangeResolution([[maybe_unused]] int nNewWidth, [[maybe_unused]] int nNewHeight, [[maybe_unused]] int nNewColDepth, [[maybe_unused]] int nNewRefreshHZ, [[maybe_unused]] bool bFullScreen, [[maybe_unused]] bool bForce)
{
    return false;
}

WIN_HWND CNULLRenderer::Init([[maybe_unused]] int x, [[maybe_unused]] int y, int width, int height, [[maybe_unused]] unsigned int cbpp, [[maybe_unused]] int zbpp, [[maybe_unused]] int sbits, [[maybe_unused]] bool fullscreen, [[maybe_unused]] bool isEditor, [[maybe_unused]] WIN_HINSTANCE hinst, [[maybe_unused]] WIN_HWND Glhwnd, [[maybe_unused]] bool bReInit, [[maybe_unused]] const SCustomRenderInitArgs* pCustomArgs, [[maybe_unused]] bool bShaderCacheGen)
{
    //=======================================
    // Add init code here
    //=======================================

    FX_SetWireframeMode(R_SOLID_MODE);

    SetWidth(width);
    SetHeight(height);
    m_backbufferWidth = width;
    m_backbufferHeight = height;
    m_Features |= RFT_HW_NVIDIA;

    if (!g_shaderGeneralHeap)
    {
        g_shaderGeneralHeap = CryGetIMemoryManager()->CreateGeneralExpandingMemoryHeap(4 * 1024 * 1024, 0, "Shader General");
    }

    iLog->Log("Init Shaders\n");

    gRenDev->m_cEF.mfInit();
    EF_Init();

#if NULL_SYSTEM_TRAIT_INIT_RETURNTHIS
    return (WIN_HWND)this;//it just get checked against NULL anyway
#else
    return (WIN_HWND)GetDesktopWindow();
#endif
}


bool CNULLRenderer::SetCurrentContext([[maybe_unused]] WIN_HWND hWnd)
{
    return true;
}

bool CNULLRenderer::CreateContext([[maybe_unused]] WIN_HWND hWnd, [[maybe_unused]] bool bAllowMSAA, [[maybe_unused]] int SSX, [[maybe_unused]] int SSY)
{
    return true;
}

bool CNULLRenderer::DeleteContext([[maybe_unused]] WIN_HWND hWnd)
{
    return true;
}

void CNULLRenderer::MakeMainContextActive()
{
}

void CNULLRenderer::ShutDown([[maybe_unused]] bool bReInit)
{
    iLog = nullptr;
    FreeResources(FRR_ALL);
    FX_PipelineShutdown();
}

void CNULLRenderer::ShutDownFast()
{
    FX_PipelineShutdown();
}


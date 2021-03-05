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


#include "CryRenderOther_precompiled.h"
#include "AtomShim_Renderer.h"

#include <AzCore/std/algorithm.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/Math/MatrixUtils.h>

#include <AzFramework/Scene/Scene.h>
#include <AzFramework/Scene/SceneSystemBus.h>
#include <AzFramework/Windowing/WindowBus.h>

#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/WindowContext.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomFeatureProcessorInterface.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

bool CAtomShimRenderer::SetGammaDelta(const float fGamma)
{
    m_fDeltaGamma = fGamma;
    return true;
}

int CAtomShimRenderer::EnumDisplayFormats([[maybe_unused]] SDispFormat* Formats)
{
    return 0;
}

bool CAtomShimRenderer::ChangeResolution([[maybe_unused]] int nNewWidth, [[maybe_unused]] int nNewHeight, [[maybe_unused]] int nNewColDepth, [[maybe_unused]] int nNewRefreshHZ, [[maybe_unused]] bool bFullScreen, [[maybe_unused]] bool bForce)
{
    return false;
}

WIN_HWND CAtomShimRenderer::Init([[maybe_unused]] int x, [[maybe_unused]] int y, int width, int height, [[maybe_unused]] unsigned int cbpp, [[maybe_unused]] int zbpp, [[maybe_unused]] int sbits, [[maybe_unused]] bool fullscreen, [[maybe_unused]] bool isEditor, [[maybe_unused]] WIN_HINSTANCE hinst, WIN_HWND Glhwnd, [[maybe_unused]] bool bReInit, [[maybe_unused]] const SCustomRenderInitArgs* pCustomArgs, [[maybe_unused]] bool bShaderCacheGen)
{
    //=======================================
    // Add init code here
    //=======================================

    FX_SetWireframeMode(R_SOLID_MODE);

    m_width = width;
    m_height = height;
    m_backbufferWidth = width;
    m_backbufferHeight = height;
    m_nativeWidth = width;
    m_nativeHeight = height;
    m_Features |= RFT_HW_NVIDIA;

    m_hWnd = (HWND)Glhwnd;

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


bool CAtomShimRenderer::SetCurrentContext(WIN_HWND hWnd)
{
    auto itr = m_viewContexts.find(hWnd);
    if (itr == m_viewContexts.end())
    {
        return false;
    }

    m_currContext = itr->second;
    return true;
}

bool CAtomShimRenderer::CreateContext(WIN_HWND hWnd, bool /* bAllowMSAA */, int /* SSX */, int /* SSY */)
{
    if (m_viewContexts.find(hWnd) != m_viewContexts.end())
    {
        return true;
    }

    AZ::RPI::RenderPipelinePtr renderPipeline = AZ::RPI::RPISystemInterface::Get()->GetRenderPipelineForWindow(hWnd);
    if (!renderPipeline)
    {
        return false;
    }

    AtomShimViewContext* pContext = new AtomShimViewContext;
    pContext->m_hWnd = (HWND)hWnd;
    pContext->m_width = m_width;
    pContext->m_height = m_height;
    pContext->m_isMainViewport = !gEnv->IsEditor();

    pContext->m_renderPipeline = renderPipeline;
    pContext->m_view = renderPipeline->GetDefaultView();
    pContext->m_scene = renderPipeline->GetScene();

    m_viewContexts[hWnd] = pContext;
    m_currContext = pContext;
    return true;
}

bool CAtomShimRenderer::DeleteContext(WIN_HWND hWnd)
{
    // Attempt to find matching context with this window handle
    auto contextToDeleteIter = m_viewContexts.find(hWnd);

    if (contextToDeleteIter == m_viewContexts.end())
    {
        return false;
    }

    AtomShimViewContext* contextToDelete = contextToDeleteIter->second;

    // remove this context from the map of contexts
    m_viewContexts.erase(contextToDeleteIter);

    
    // If we are deleting the current context then set current context to the first one still in list (if any are left)
    if (m_currContext == contextToDelete)
    {
        if (m_viewContexts.empty())
        {
            m_currContext = nullptr;

            m_width = 0;
            m_height = 0;
        }
        else
        {
            m_currContext = m_viewContexts.begin()->second;

            m_width = m_currContext->m_width;
            m_height = m_currContext->m_height;
        }
    }

    delete contextToDelete;

    return true;
}

void CAtomShimRenderer::MakeMainContextActive()
{
    if (m_viewContexts.empty())
    {
        return;
    }

    m_currContext = m_viewContexts.begin()->second;
}

void CAtomShimRenderer::ShutDown([[maybe_unused]] bool bReInit)
{
    iLog = nullptr;
    FreeResources(FRR_ALL);
    FX_PipelineShutdown();
}

void CAtomShimRenderer::ShutDownFast()
{
    FX_PipelineShutdown();
}


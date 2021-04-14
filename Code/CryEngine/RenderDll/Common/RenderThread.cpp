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
#include <I3DEngine.h>
#include <IFont.h>
#include "RenderAuxGeom.h"
#include "IColorGradingControllerInt.h"
#include "PostProcess/PostEffects.h"

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define RENDERTHREAD_CPP_SECTION_1 1
#define RENDERTHREAD_CPP_SECTION_2 2
#define RENDERTHREAD_CPP_SECTION_3 3
#define RENDERTHREAD_CPP_SECTION_4 4
#define RENDERTHREAD_CPP_SECTION_5 5
#define RENDERTHREAD_CPP_SECTION_6 6
#define RENDERTHREAD_CPP_SECTION_7 7
#define RENDERTHREAD_CPP_SECTION_8 8
#endif

#if !defined(NULL_RENDERER)
#include <DriverD3D.h>
#endif

#include "MainThreadRenderRequestBus.h"
#include "Common/RenderView.h"
#include "Common/Textures/TextureManager.h"
#include "GraphicsPipeline/FurBendData.h"

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Archive/IArchive.h>
#include <AzCore/Debug/EventTraceDrillerBus.h>
#include <IVideoRenderer.h>

#ifdef STRIP_RENDER_THREAD
    #define m_nCurThreadFill 0
    #define m_nCurThreadProcess 0
#endif

#define MULTITHREADED_RESOURCE_CREATION

#ifdef WIN32
HWND SRenderThread::GetRenderWindowHandle()
{
    return (HWND)gRenDev->GetHWND();
}
#endif

CryCriticalSection SRenderThread::s_rcLock;

# define LOADINGLOCK_COMMANDQUEUE (void)0;

void CRenderThread::Run()
{
    //f_Log = fopen("Mouse.txt", "w");
    //fclose(f_Log);

    CryThreadSetName(threadID(THREADID_NULL), RENDER_THREAD_NAME);
    gEnv->pSystem->GetIThreadTaskManager()->MarkThisThreadForDebugging(RENDER_THREAD_NAME, true);
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERTHREAD_CPP_SECTION_1
    #include AZ_RESTRICTED_FILE(RenderThread_cpp)
#endif
    threadID renderThreadId = ::GetCurrentThreadId();
    gRenDev->m_pRT->m_nRenderThread = renderThreadId;
    CNameTableR::m_nRenderThread = renderThreadId;
    gEnv->pCryPak->SetRenderThreadId(AZStd::this_thread::get_id());
    //SetThreadAffinityMask(GetCurrentThread(), 2);
    m_started.Set();

    gRenDev->m_pRT->Process();

    // Check pointers, it's not guaranteed that system is still valid.
    if (gEnv && gEnv->pSystem && gEnv->pSystem->GetIThreadTaskManager())
    {
        gEnv->pSystem->GetIThreadTaskManager()->MarkThisThreadForDebugging(RENDER_THREAD_NAME, false);
    }
}

void CRenderThreadLoading::Run()
{
    CryThreadSetName(threadID(THREADID_NULL), RENDER_LOADING_THREAD_NAME);
    gEnv->pSystem->GetIThreadTaskManager()->MarkThisThreadForDebugging(RENDER_LOADING_THREAD_NAME, true);
    threadID renderThreadId = ::GetCurrentThreadId();
    gRenDev->m_pRT->m_nRenderThreadLoading = renderThreadId;
    CNameTableR::m_nRenderThread = renderThreadId;

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERTHREAD_CPP_SECTION_2
    #include AZ_RESTRICTED_FILE(RenderThread_cpp)
#endif

    // We aren't interested in file access from the render loading thread, and this
    // would overwrite the real render thread id
    //gEnv->pCryPak->SetRenderThreadId( renderThreadId );
    m_started.Set();

    gRenDev->m_pRT->ProcessLoading();

    // Check pointers, it's not guaranteed that system is still valid.
    if (gEnv && gEnv->pSystem && gEnv->pSystem->GetIThreadTaskManager())
    {
        gEnv->pSystem->GetIThreadTaskManager()->MarkThisThreadForDebugging(RENDER_LOADING_THREAD_NAME, false);
    }
}

void SRenderThread::SwitchMode(bool bEnableVideo)
{
    if (bEnableVideo)
    {
        assert(IsRenderThread());
        if (m_pThreadLoading)
        {
            return;
        }
#if !defined(STRIP_RENDER_THREAD)
        SSystemGlobalEnvironment* pEnv = iSystem->GetGlobalEnvironment();
        if (pEnv && !pEnv->bTesting && !pEnv->IsEditor() && pEnv->pi.numCoresAvailableToProcess > 1 && CRenderer::CV_r_multithreaded > 0)
        {
            m_pThreadLoading = new CRenderThreadLoading(1);
        }
        m_eVideoThreadMode = eVTM_Active;
        m_bQuitLoading = false;
        StartRenderLoadingThread();
#endif
    }
    else
    {
        m_eVideoThreadMode = eVTM_ProcessingStop;
    }
}

SRenderThread::SRenderThread()
{
    m_eVideoThreadMode = eVTM_Disabled;
    m_nRenderThreadLoading = 0;
    m_pThreadLoading = 0;
    m_pLoadtimeCallback = 0;
    m_bEndFrameCalled = false;
    m_bBeginFrameCalled = false;
    m_bQuitLoading = false;
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERTHREAD_CPP_SECTION_3
    #include AZ_RESTRICTED_FILE(RenderThread_cpp)
#endif
#if defined(USE_HANDLE_FOR_FINAL_FLUSH_SYNC)
    m_FlushFinishedCondition = CreateEvent(NULL, FALSE, FALSE, "FlushFinishedCondition");
#endif
    Init(2);
}

threadID CNameTableR::m_nRenderThread = 0;

void SRenderThread::Init(int nCPU)
{
    m_bQuit = false;
#ifndef STRIP_RENDER_THREAD
    m_nCurThreadFill = 0;
    m_nCurThreadProcess = 0;
#endif
    InitFlushCond();
    m_nRenderThread = ::GetCurrentThreadId();
    CNameTableR::m_nRenderThread = m_nRenderThread;
    m_nMainThread = m_nRenderThread;
    m_bSuccessful = true;
    m_pThread = NULL;
    m_fTimeIdleDuringLoading = 0;
    m_fTimeBusyDuringLoading = 0;
#if !defined(STRIP_RENDER_THREAD)
    SSystemGlobalEnvironment* pEnv = iSystem->GetGlobalEnvironment();
    if (pEnv && !pEnv->bTesting && !pEnv->IsDedicated() && !pEnv->IsEditor() && pEnv->pi.numCoresAvailableToProcess > 1 && CRenderer::CV_r_multithreaded > 0)
    {
        m_nCurThreadProcess = 1;
        m_pThread = new CRenderThread(nCPU);
    }
#ifndef CONSOLE_CONST_CVAR_MODE
    else
    {
        CRenderer::CV_r_multithreaded = 0;
    }
#endif
#else//STRIP_RENDER_THREAD
    #ifndef CONSOLE_CONST_CVAR_MODE
    CRenderer::CV_r_multithreaded = 0;
    #endif
#endif//STRIP_RENDER_THREAD
    gRenDev->m_RP.m_nProcessThreadID = threadID(m_nCurThreadProcess);
    gRenDev->m_RP.m_nFillThreadID = threadID(m_nCurThreadFill);

    for (uint32 i = 0; i < RT_COMMAND_BUF_COUNT; ++i)
    {
        m_Commands[i].Free();
        m_Commands[i].Create(300 * 1024); // 300 to stop growing in MP levels
        m_Commands[i].SetUse(0);
        gRenDev->m_fTimeWaitForMain[i]   = 0;
        gRenDev->m_fTimeWaitForRender[i] = 0;
        gRenDev->m_fTimeProcessedRT[i]   = 0;
        gRenDev->m_fTimeProcessedGPU[i]  = 0;
    }
    m_eVideoThreadMode = eVTM_Disabled;
}

SRenderThread::~SRenderThread()
{
    QuitRenderLoadingThread();
    QuitRenderThread();
#if defined(USE_HANDLE_FOR_FINAL_FLUSH_SYNC)
    CloseHandle(m_FlushFinishedCondition);
#endif
}

void SRenderThread::ValidateThreadAccess(ERenderCommand eRC)
{
    if (!IsMainThread() && !gRenDev->m_bStartLevelLoading)
    {
        CryFatalError("Trying to add a render command from a non-main thread, eRC = %d", (int)eRC);
    }
}

//==============================================================================================
// NOTE: Render commands can be added from main thread only

bool SRenderThread::RC_CreateDevice()
{
    LOADING_TIME_PROFILE_SECTION;
    AZ_TRACE_METHOD();

#if defined(WIN32) || defined(WIN64) || defined(APPLE) || defined(LINUX) || defined(CREATE_DEVICE_ON_MAIN_THREAD)
    return gRenDev->RT_CreateDevice();
#else
    if (IsRenderThread())
    {
        return gRenDev->RT_CreateDevice();
    }
    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_CreateDevice, 0);
    EndCommand(p);

    FlushAndWait();

    return !IsFailed();
#endif
}

void SRenderThread::RC_ResetDevice()
{
    AZ_TRACE_METHOD();
#if defined(WIN32) || defined(WIN64) || defined(LINUX) || defined(APPLE) || defined(CREATE_DEVICE_ON_MAIN_THREAD)
    gRenDev->RT_Reset();
#else
    if (IsRenderThread())
    {
        gRenDev->RT_Reset();
        return;
    }
    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_ResetDevice, 0);
    EndCommand(p);

    FlushAndWait();
#endif
}

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERTHREAD_CPP_SECTION_4
    #include AZ_RESTRICTED_FILE(RenderThread_cpp)
#endif

void SRenderThread::RC_PreloadTextures()
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        return CTexture::RT_Precache();
    }
    LOADINGLOCK_COMMANDQUEUE
    {
        byte* p = AddCommand(eRC_PreloadTextures, 0);
        EndCommand(p);
        FlushAndWait();
    }
}

void SRenderThread::RC_Init()
{
    LOADING_TIME_PROFILE_SECTION;
    AZ_TRACE_METHOD();

    if (IsRenderThread())
    {
        return gRenDev->RT_Init();
    }
    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_Init, 0);
    EndCommand(p);

    FlushAndWait();
}
void SRenderThread::RC_ShutDown(uint32 nFlags)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        return gRenDev->RT_ShutDown(nFlags);
    }
    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_ShutDown, 4);
    AddDWORD(p, nFlags);

    FlushAndWait();
}


void SRenderThread::RC_ResetGlass()
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->RT_ResetGlass();
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_ResetGlass, 0);
    EndCommand(p);
}


void SRenderThread::RC_ResetToDefault()
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->ResetToDefault();
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_ResetToDefault, 0);
    EndCommand(p);
}

void SRenderThread::RC_ParseShader (CShader* pSH, uint64 nMaskGen, uint32 flags, CShaderResources* pRes)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread(true))
    {
        return gRenDev->m_cEF.RT_ParseShader(pSH, nMaskGen, flags, pRes);
    }

    if (!IsMainThread(true))
    {
        pSH->AddRef();
        if (pRes)
        {
            pRes->AddRef();
        }

        AZStd::function<void()> runOnMainThread = [this, pSH, nMaskGen, flags, pRes]()
            {
                RC_ParseShader(pSH, nMaskGen, flags, pRes);

                pSH->Release();
                if (pRes)
                {
                    pRes->Release();
                }

                // Make sure any materials using this shader get updated appropriately.
                if (gEnv && gEnv->p3DEngine)
                {
                    gEnv->p3DEngine->UpdateShaderItems();
                }
            };

        EBUS_QUEUE_FUNCTION(AZ::MainThreadRenderRequestBus, runOnMainThread);
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    pSH->AddRef();
    if (pRes)
    {
        pRes->AddRef();
    }
    byte* p = AddCommand(eRC_ParseShader, 12 + 2 * sizeof(void*));
    AddPointer(p, pSH);
    AddPointer(p, pRes);
    AddDWORD64(p, nMaskGen);
    AddDWORD(p, flags);
    EndCommand(p);
}

void SRenderThread::RC_UpdateShaderItem (SShaderItem* pShaderItem, _smart_ptr<IMaterial> pMaterial)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread(true))
    {
        return gRenDev->RT_UpdateShaderItem(pShaderItem, pMaterial.get());
    }

    if (!IsMainThread(true))
    {
        AZStd::function<void()> runOnMainThread = [this, pShaderItem, pMaterial]()
            {
                RC_UpdateShaderItem(pShaderItem, pMaterial);
            };

        EBUS_QUEUE_FUNCTION(AZ::MainThreadRenderRequestBus, runOnMainThread);
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    // We pass the raw pointer instead of the smart_ptr because writing/reading smart pointers from
    // the render thread queue causes the ref count to be increased incorrectly in some platforms (e.g. 32 bit architectures). 
    // Because of this we manually increment the reference count before adding it to the queue and decrement it when we finish using it in the RenderThread.
    IMaterial* materialRawPointer = pMaterial.get();
    if (materialRawPointer)
    {
        // Add a reference to prevent it from getting deleted before the RenderThread process the message.
        materialRawPointer->AddRef();
    }

    if (m_eVideoThreadMode == eVTM_Disabled)
    {
        byte* p = AddCommand(eRC_UpdateShaderItem, sizeof(pShaderItem) + sizeof(materialRawPointer));
        AddPointer(p, pShaderItem);
        AddPointer(p, materialRawPointer);
        EndCommand(p);
    }
    else
    {
        // Move command into loading queue, which will be executed in first render frame after loading is done
        byte* p = AddCommandTo(eRC_UpdateShaderItem, sizeof(pShaderItem) + sizeof(materialRawPointer), m_CommandsLoading);
        AddPointer(p, pShaderItem);
        AddPointer(p, pMaterial);
        EndCommandTo(p, m_CommandsLoading);
    }
}

void SRenderThread::RC_RefreshShaderResourceConstants(SShaderItem* shaderItem, IMaterial* material)
{
    if (IsRenderThread())
    {
        return gRenDev->RT_RefreshShaderResourceConstants(shaderItem);
    }

    LOADINGLOCK_COMMANDQUEUE;

    if (material)
    {
        // Add a reference to prevent it from getting deleted before the RenderThread process the message.
        material->AddRef();
    }

    if (m_eVideoThreadMode == eVTM_Disabled)
    {
        byte* p = AddCommand(eRC_RefreshShaderResourceConstants, sizeof(SShaderItem*) + sizeof(IMaterial*));
        AddPointer(p, shaderItem);
        AddPointer(p, material);
        EndCommand(p);
    }
    else
    {
        // Move command into loading queue, which will be executed in first render frame after loading is done
        byte* p = AddCommandTo(eRC_RefreshShaderResourceConstants, sizeof(SShaderItem*) + sizeof(IMaterial*), m_CommandsLoading);
        AddPointer(p, shaderItem);
        AddPointer(p, material);
        EndCommandTo(p, m_CommandsLoading);
    }
}

void SRenderThread::RC_SetShaderQuality(EShaderType eST, EShaderQuality eSQ)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        return gRenDev->m_cEF.RT_SetShaderQuality(eST, eSQ);
    }
    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_SetShaderQuality, 8);
    AddDWORD(p, eST);
    AddDWORD(p, eSQ);
    EndCommand(p);
}



void SRenderThread::RC_ReleaseVBStream(void* pVB, int nStream)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->RT_ReleaseVBStream(pVB, nStream);
        return;
    }
    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_ReleaseVBStream, 4 + sizeof(void*));
    AddPointer(p, pVB);
    AddDWORD(p, nStream);
    EndCommand(p);
}

void SRenderThread::RC_ForceMeshGC(bool instant, bool wait)
{
    AZ_TRACE_METHOD();
    LOADING_TIME_PROFILE_SECTION;

    if (IsRenderThread())
    {
        CRenderMesh::Tick();
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_ForceMeshGC, 0);
    EndCommand(p);

    if (instant)
    {
        if (wait)
        {
            FlushAndWait();
        }
        else
        {
            SyncMainWithRender();
        }
    }
}


void SRenderThread::RC_DevBufferSync()
{
    AZ_TRACE_METHOD();
    LOADING_TIME_PROFILE_SECTION;

    if (IsRenderThread())
    {
        gRenDev->m_DevBufMan.Sync(gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_nFrameUpdateID);
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_DevBufferSync, 0);
    EndCommand(p);
}

void SRenderThread::RC_ReleasePostEffects()
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        if (gRenDev->m_pPostProcessMgr)
        {
            gRenDev->m_pPostProcessMgr->ReleaseResources();
        }
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_ReleasePostEffects, 0);
    EndCommand(p);
}

void SRenderThread::RC_ResetPostEffects(bool bOnSpecChange)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        if (gRenDev->m_RP.m_pREPostProcess)
        {
            gRenDev->m_RP.m_pREPostProcess->Reset(bOnSpecChange);
        }
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(bOnSpecChange ? eRC_ResetPostEffectsOnSpecChange : eRC_ResetPostEffects, 0);
    EndCommand(p);
    FlushAndWait();
}

void SRenderThread::RC_DisableTemporalEffects()
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        return gRenDev->RT_DisableTemporalEffects();
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_DisableTemporalEffects, 0);
    EndCommand(p);
}

void SRenderThread::RC_UpdateTextureRegion(CTexture* pTex, const byte* data, int nX, int nY, int nZ, int USize, int VSize, int ZSize, ETEX_Format eTFSrc)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        return pTex->RT_UpdateTextureRegion(data, nX, nY, nZ, USize, VSize, ZSize, eTFSrc);
    }

    LOADINGLOCK_COMMANDQUEUE
    int nSize = CTexture::TextureDataSize(USize, VSize, ZSize, pTex->GetNumMips(), 1, eTFSrc);
    byte* pData = new byte[nSize];
    cryMemcpy(pData, data, nSize);
    pTex->AddRef();

    if (m_eVideoThreadMode == eVTM_Disabled)
    {
        byte* p = AddCommand(eRC_UpdateTexture, 28 + 2 * sizeof(void*));
        AddPointer(p, pTex);
        AddPointer(p, pData);
        AddDWORD(p, nX);
        AddDWORD(p, nY);
        AddDWORD(p, nZ);
        AddDWORD(p, USize);
        AddDWORD(p, VSize);
        AddDWORD(p, ZSize);
        AddDWORD(p, eTFSrc);
        EndCommand(p);
    }
    else
    {
        // Move command into loading queue, which will be executed in first render frame after loading is done
        byte* p = AddCommandTo(eRC_UpdateTexture, 28 + 2 * sizeof(void*), m_CommandsLoading);
        AddPointer(p, pTex);
        AddPointer(p, pData);
        AddDWORD(p, nX);
        AddDWORD(p, nY);
        AddDWORD(p, nZ);
        AddDWORD(p, USize);
        AddDWORD(p, VSize);
        AddDWORD(p, ZSize);
        AddDWORD(p, eTFSrc);
        EndCommandTo(p, m_CommandsLoading);
    }
}

bool SRenderThread::RC_DynTexUpdate(SDynTexture* pTex, int nNewWidth, int nNewHeight)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        return pTex->RT_Update(nNewWidth, nNewHeight);
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_DynTexUpdate, 8 + sizeof(void*));
    AddPointer(p, pTex);
    AddDWORD(p, nNewWidth);
    AddDWORD(p, nNewHeight);
    EndCommand(p);

    return true;
}

void SRenderThread::RC_EntityDelete(IRenderNode* pRenderNode)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        return SDynTexture_Shadow::RT_EntityDelete(pRenderNode);
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_EntityDelete, sizeof(void*));
    AddPointer(p, pRenderNode);
    EndCommand(p);
}

void TexBlurAnisotropicVertical(CTexture* pTex, int nAmount, float fScale, float fDistribution, bool bAlphaOnly);

void SRenderThread::RC_TexBlurAnisotropicVertical(CTexture* Tex, float fAnisoScale)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        TexBlurAnisotropicVertical(Tex, 1, 8 * max(1.0f - min(fAnisoScale / 100.0f, 1.0f), 0.2f), 1, false);
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_TexBlurAnisotropicVertical, 4 + sizeof(void*));
    AddPointer(p, Tex);
    AddFloat(p, fAnisoScale);
    EndCommand(p);
}

bool SRenderThread::RC_CreateDeviceTexture(CTexture* pTex, const byte* pData[6])
{
    AZ_TRACE_METHOD();
#if !defined(MULTITHREADED_RESOURCE_CREATION)
    if (IsRenderThread())
#endif
    {
        return pTex->RT_CreateDeviceTexture(pData);
    }

#if !defined(MULTITHREADED_RESOURCE_CREATION)
    if (pTex->IsAsyncDevTexCreation())
    {
        return !IsFailed();
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_CreateDeviceTexture, 7 * sizeof(void*));
    AddPointer(p, pTex);
    for (int i = 0; i < 6; i++)
    {
        AddPointer(p, pData[i]);
    }
    EndCommand(p);
    FlushAndWait();

    return !IsFailed();
#endif
}

void SRenderThread::RC_CopyDataToTexture(
    void* pkVoid, unsigned int uiStartMip, unsigned int uiEndMip)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        CTexture* pkTexture = (CTexture*) pkVoid;
        pkTexture->StreamCopyMipsTexToMem(uiStartMip, uiEndMip, true, NULL);
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_CopyDataToTexture, 8 + sizeof(void*));
    AddPointer(p, pkVoid);
    AddDWORD(p, uiStartMip);
    AddDWORD(p, uiEndMip);
    EndCommand(p);
    // -- kenzo: removing this causes crashes because the texture
    // might have already been destroyed. This needs to be fixed
    // somehow that the createtexture doesn't require the renderthread
    // (PC only issue)
    FlushAndWait();
}

void SRenderThread::RC_ClearTarget(void* pkVoid, const ColorF& kColor)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        CTexture* pkTexture = (CTexture*) pkVoid;
        gRenDev->RT_ClearTarget(pkTexture, kColor);
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_ClearTarget, sizeof(void*) + sizeof(ColorF));
    AddPointer(p, pkVoid);
    AddColor(p, kColor);
    EndCommand(p);
    FlushAndWait();
}

void SRenderThread::RC_CreateResource(SResourceAsync* pRes)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->RT_CreateResource(pRes);
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_CreateResource, sizeof(void*));
    AddPointer(p, pRes);
    EndCommand(p);
}

void SRenderThread::RC_StartVideoThread()
{
    AZ_TRACE_METHOD();
    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_StartVideoThread, 0);
    EndCommand(p);
}

void SRenderThread::RC_StopVideoThread()
{
    AZ_TRACE_METHOD();
    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_StopVideoThread, 0);
    EndCommand(p);
}

void SRenderThread::RC_PreactivateShaders()
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        CHWShader::RT_PreactivateShaders();
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_PreactivateShaders, 0);
    EndCommand(p);
}

void SRenderThread::RC_PrecacheShader(CShader* pShader, SShaderCombination& cmb, bool bForce, bool bCompressedOnly, CShaderResources* pRes)
{
    AZ_TRACE_METHOD();
    if (IsRenderLoadingThread())
    {
        // Move command into loading queue, which will be executed in first render frame after loading is done
        byte* p = AddCommandTo(eRC_PrecacheShader, sizeof(void*) * 2 + 8 + sizeof(SShaderCombination), m_CommandsLoading);
        pShader->AddRef();
        if (pRes)
        {
            pRes->AddRef();
        }
        AddPointer(p, pShader);
        memcpy(p, &cmb, sizeof(cmb));
        p += sizeof(SShaderCombination);
        AddDWORD(p, bForce);
        AddDWORD(p, bCompressedOnly);
        AddPointer(p, pRes);
        EndCommandTo(p, m_CommandsLoading);
    }
    else if (IsRenderThread())
    {
        pShader->mfPrecache(cmb, bForce, bCompressedOnly, pRes);
        return;
    }
    else
    {
        LOADINGLOCK_COMMANDQUEUE
        byte* p = AddCommand(eRC_PrecacheShader, sizeof(void*) * 2 + 8 + sizeof(SShaderCombination));
        pShader->AddRef();
        if (pRes)
        {
            pRes->AddRef();
        }
        AddPointer(p, pShader);
        memcpy(p, &cmb, sizeof(cmb));
        p += sizeof(SShaderCombination);
        AddDWORD(p, bForce);
        AddDWORD(p, bCompressedOnly);
        AddPointer(p, pRes);
        EndCommand(p);
    }
}

void SRenderThread::RC_ReleaseBaseResource(CBaseResource* pRes)
{
    AZ_TRACE_METHOD();
    if (IsRenderLoadingThread())
    {
        // Move command into loading queue, which will be executed in first render frame after loading is done
        byte* p = AddCommandTo(eRC_ReleaseBaseResource, sizeof(void*), m_CommandsLoading);
        AddPointer(p, pRes);
        EndCommandTo(p, m_CommandsLoading);
    }
    else if (IsRenderThread())
    {
        SAFE_DELETE(pRes);
        return;
    }
    else
    {
        LOADINGLOCK_COMMANDQUEUE
        byte* p = AddCommand(eRC_ReleaseBaseResource, sizeof(void*));
        AddPointer(p, pRes);
        EndCommand(p);
    }
}

void SRenderThread::RC_ReleaseFont(IFFont* font)
{
    AZ_TRACE_METHOD();
    if (IsRenderLoadingThread())
    {
        // Move command into loading queue, which will be executed in first render frame after loading is done
        byte* p = AddCommandTo(eRC_ReleaseFont, sizeof(void*), m_CommandsLoading);
        AddPointer(p, font);
        EndCommandTo(p, m_CommandsLoading);
    }
    else if (IsRenderThread())
    {
        SAFE_DELETE(font);
        return;
    }
    else
    {
        LOADINGLOCK_COMMANDQUEUE
        byte* p = AddCommand(eRC_ReleaseFont, sizeof(void*));
        AddPointer(p, font);
        EndCommand(p);
    }
}

void SRenderThread::RC_ReleaseSurfaceResource(SDepthTexture* pRes)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        if (pRes)
        {
            pRes->Release(true);
        }
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_ReleaseSurfaceResource, sizeof(void*));
    AddPointer(p, pRes);
    EndCommand(p);
}

void SRenderThread::RC_ReleaseResource(SResourceAsync* pRes)
{
    RC_ReleaseResource(AZStd::unique_ptr<SResourceAsync>(pRes));
}

void SRenderThread::RC_ReleaseResource(AZStd::unique_ptr<SResourceAsync> pRes)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->RT_ReleaseResource(pRes.release());
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
        byte* p = AddCommand(eRC_ReleaseResource, sizeof(void*));
    // Move owner over the SResourceAsync over to the renderer command queue
    AddPointer(p, pRes.release());
    EndCommand(p);
}

void SRenderThread::RC_UnbindTMUs()
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->RT_UnbindTMUs();
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_UnbindTMUs, 0);
    EndCommand(p);
}

void SRenderThread::RC_UnbindResources()
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->RT_UnbindResources();
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_UnbindResources, 0);
    EndCommand(p);
}

void SRenderThread::RC_ReleaseRenderResources()
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->RT_ReleaseRenderResources();
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_ReleaseRenderResources, 0);
    EndCommand(p);
}
void SRenderThread::RC_CreateRenderResources()
{
    LOADING_TIME_PROFILE_SECTION;
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->RT_CreateRenderResources();
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_CreateRenderResources, 0);
    EndCommand(p);
}

void SRenderThread::RC_CreateSystemTargets()
{
    LOADING_TIME_PROFILE_SECTION;
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        CTexture::CreateSystemTargets();
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_CreateSystemTargets, 0);
    EndCommand(p);
}

void SRenderThread::RC_PrecacheDefaultShaders()
{
    LOADING_TIME_PROFILE_SECTION;
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->RT_PrecacheDefaultShaders();
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_PrecacheDefaultShaders, 0);
    EndCommand(p);
}

void SRenderThread::RC_RelinkTexture(CTexture* pTex)
{
    if (IsRenderThread(true))
    {
        pTex->RT_Relink();
        return;
    }

    if (!IsMainThread(true))
    {
        AZStd::function<void()> runOnMainThread = [this, pTex]()
            {
                RC_RelinkTexture(pTex);
            };

        EBUS_QUEUE_FUNCTION(AZ::MainThreadRenderRequestBus, runOnMainThread);
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_RelinkTexture, sizeof(void*));
    AddPointer(p, pTex);
    EndCommand(p);
}

void SRenderThread::RC_UnlinkTexture(CTexture* pTex)
{
    if (IsRenderThread())
    {
        pTex->RT_Unlink();
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_UnlinkTexture, sizeof(void*));
    AddPointer(p, pTex);
    EndCommand(p);
}

void SRenderThread::RC_CreateREPostProcess(CRendElementBase** re)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        return gRenDev->RT_CreateREPostProcess(re);
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_CreateREPostProcess, sizeof(void*));
    AddPointer(p, re);
    EndCommand(p);

    FlushAndWait();
}

bool SRenderThread::RC_CheckUpdate2(CRenderMesh* pMesh, CRenderMesh* pVContainer, uint32 nStreamMask)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        return pMesh->RT_CheckUpdate(pVContainer, nStreamMask);
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_UpdateMesh2, 8 + 2 * sizeof(void*));
    AddPointer(p, pMesh);
    AddPointer(p, pVContainer);
    AddDWORD(p, nStreamMask);
    EndCommand(p);

    FlushAndWait();

    return !IsFailed();
}

void SRenderThread::RC_ReleaseVB(buffer_handle_t  nID)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->m_DevBufMan.Destroy(nID);
        return;
    }
    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_ReleaseVB, sizeof(buffer_handle_t));
    AddDWORD64(p, nID);
    EndCommand(p);
}
void SRenderThread::RC_ReleaseIB(buffer_handle_t  nID)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->m_DevBufMan.Destroy(nID);
        return;
    }
    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_ReleaseIB, sizeof(buffer_handle_t));
    AddDWORD64(p, nID);
    EndCommand(p);
}

void SRenderThread::RC_DrawDynVB(SVF_P3F_C4B_T2F* pBuf, uint16* pInds, int nVerts, int nInds, const PublicRenderPrimitiveType nPrimType)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->RT_DrawDynVB(pBuf, pInds, nVerts, nInds, nPrimType);
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_DrawDynVB, Align4(20 + sizeof(SVF_P3F_C4B_T2F) * nVerts + sizeof(uint16) * nInds));
    AddData(p, pBuf, sizeof(SVF_P3F_C4B_T2F) * nVerts);
    AddData(p, pInds, sizeof(uint16) * nInds);
    AddDWORD(p, nVerts);
    AddDWORD(p, nInds);
    AddDWORD(p, (int)nPrimType);
    EndCommand(p);
}

void SRenderThread::RC_DrawDynUiPrimitiveList(IRenderer::DynUiPrimitiveList& primitives, int totalNumVertices, int totalNumIndices)
{
    AZ_TRACE_METHOD();

    if (IsRenderThread())
    {
        // When this is called on the render thread we do not currently combine the draw calls since we would
        // have to allocate a new buffer to do so using RT_DrawDynVBUI.
        // We could avoid the allocate by having a RT_DrawDynUiPrimitiveList which was only used
        // when RC_DrawDynUiPrimitiveList is called on the render thread. It would have to do some
        // fancy stuff with TempDynVB. Currently we are optimizing the case where there is a separate
        // render thread so this is not a priority.
        for (const IRenderer::DynUiPrimitive& primitive : primitives)
        {
            gRenDev->RT_DrawDynVBUI(primitive.m_vertices, primitive.m_indices, primitive.m_numVertices, primitive.m_numIndices, prtTriangleList);
        }
        return;
    }

    size_t vertsSizeInBytes = Align4(sizeof(SVF_P2F_C4B_T2F_F4B) * totalNumVertices);
    size_t indsSizeInBytes = Align4(sizeof(uint16) * totalNumIndices);

    LOADINGLOCK_COMMANDQUEUE
    const size_t fixedCommandSize = 5 * sizeof(uint32);  // accounts for the 5 calls to AddDWORD below
    byte* p = AddCommand(eRC_DrawDynVBUI, fixedCommandSize + vertsSizeInBytes + indsSizeInBytes);

    // we can't use AddPtr for each primitive since that adds a length then memcpy's the pointer
    // we want all the vertices added to the queue as one length plus one data chunk.
    // Same for indices.

    // We know SVF_P2F_C4B_T2F_F4B is a multiple of 4 bytes so no padding needed
    AddDWORD(p, vertsSizeInBytes);
    for (const IRenderer::DynUiPrimitive& primitive : primitives)
    {
        memcpy(p, primitive.m_vertices, sizeof(SVF_P2F_C4B_T2F_F4B) * primitive.m_numVertices);
        p += sizeof(SVF_P2F_C4B_T2F_F4B) * primitive.m_numVertices;
    }

    AddDWORD(p, indsSizeInBytes);
    // when copying the indicies we have to adjust them to be the correct index in the combined vertex buffer
    uint16 vbOffset = 0;
    for (const IRenderer::DynUiPrimitive& primitive : primitives)
    {
        uint16* pIndex = reinterpret_cast<uint16*>(p);
        for (int i = 0; i < primitive.m_numIndices; ++i)
        {
            pIndex[i] = primitive.m_indices[i] + vbOffset;
        }
        p += sizeof(uint16) * primitive.m_numIndices;
        vbOffset += primitive.m_numVertices;
    }
    // uint16 is not a multiple of 4 bytes so if there is an odd number of indices we need to pad
    unsigned pad = indsSizeInBytes - sizeof(uint16) * totalNumIndices;
    p += pad;

    AddDWORD(p, totalNumVertices);
    AddDWORD(p, totalNumIndices);
    AddDWORD(p, (int)prtTriangleList);
    EndCommand(p);
}

void SRenderThread::RC_Draw2dImageStretchMode(bool bStretch)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->RT_Draw2dImageStretchMode(bStretch);
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_Draw2dImageStretchMode, sizeof(DWORD));
    AddDWORD(p, (DWORD)bStretch);
    EndCommand(p);
}

void SRenderThread::RC_Draw2dImage(float xpos, float ypos, float w, float h, CTexture* pTexture, float s0, float t0, float s1, float t1, float angle, float r, float g, float b, float a, float z)
{
    AZ_TRACE_METHOD();
    DWORD col = D3DRGBA(r, g, b, a);

    if (IsRenderThread())
    {
        // don't render using fixed function pipeline when video mode is active
        if (m_eVideoThreadMode == eVTM_Disabled)
        {
            gRenDev->RT_Draw2dImage(xpos, ypos, w, h, pTexture, s0, t0, s1, t1, angle, col, z);
        }
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_Draw2dImage, 44 + sizeof(void*));
    AddFloat(p, xpos);
    AddFloat(p, ypos);
    AddFloat(p, w);
    AddFloat(p, h);
    AddPointer(p, pTexture);
    AddFloat(p, s0);
    AddFloat(p, t0);
    AddFloat(p, s1);
    AddFloat(p, t1);
    AddFloat(p, angle);
    AddDWORD(p, col);
    AddFloat(p, z);
    EndCommand(p);
}

void SRenderThread::RC_Push2dImage(float xpos, float ypos, float w, float h, CTexture* pTexture, float s0, float t0, float s1, float t1, float angle, float r, float g, float b, float a, float z, float stereoDepth)
{
    AZ_TRACE_METHOD();
    DWORD col = D3DRGBA(r, g, b, a);

    if (IsRenderThread())
    {
        gRenDev->RT_Push2dImage(xpos, ypos, w, h, pTexture, s0, t0, s1, t1, angle, col, z, stereoDepth);
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_Push2dImage, 48 + sizeof(void*));
    AddFloat(p, xpos);
    AddFloat(p, ypos);
    AddFloat(p, w);
    AddFloat(p, h);
    AddPointer(p, pTexture);
    AddFloat(p, s0);
    AddFloat(p, t0);
    AddFloat(p, s1);
    AddFloat(p, t1);
    AddFloat(p, angle);
    AddDWORD(p, col);
    AddFloat(p, z);
    AddFloat(p, stereoDepth);
    EndCommand(p);
}

void SRenderThread::RC_Draw2dImageList()
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->RT_Draw2dImageList();
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_Draw2dImageList, 0);
    EndCommand(p);
}

void SRenderThread::RC_DrawImageWithUV(float xpos, float ypos, float z, float w, float h, int textureid, float* s, float* t, float r, float g, float b, float a, bool filtered)
{
    AZ_TRACE_METHOD();
    DWORD col = D3DRGBA(r, g, b, a);
    if (IsRenderThread())
    {
        gRenDev->RT_DrawImageWithUV(xpos, ypos, z, w, h, textureid, s, t, col, filtered);
        return;
    }

    int i;

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_DrawImageWithUV, 32 + 8 * 4);
    AddFloat(p, xpos);
    AddFloat(p, ypos);
    AddFloat(p, z);
    AddFloat(p, w);
    AddFloat(p, h);
    AddDWORD(p, textureid);
    for (i = 0; i < 4; i++)
    {
        AddFloat(p, s[i]);
    }
    for (i = 0; i < 4; i++)
    {
        AddFloat(p, t[i]);
    }
    AddDWORD(p, col);
    AddDWORD(p, filtered);
    EndCommand(p);
}

void SRenderThread::RC_SetState(int State, int AlphaRef)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->FX_SetState(State, AlphaRef);
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_SetState, 8);
    AddDWORD(p, State);
    AddDWORD(p, AlphaRef);
    EndCommand(p);
}

void SRenderThread::RC_SetStencilState(int st, uint32 nStencRef, uint32 nStencMask, uint32 nStencWriteMask, bool bForceFullReadMask)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->FX_SetStencilState(st, nStencRef, nStencMask, nStencWriteMask, bForceFullReadMask);
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_SetStencilState, 20);
    AddDWORD(p, st);
    AddDWORD(p, nStencRef);
    AddDWORD(p, nStencMask);
    AddDWORD(p, nStencWriteMask);
    AddDWORD(p, bForceFullReadMask);
    EndCommand(p);
}

void SRenderThread::RC_SetColorOp(byte eCo, byte eAo, byte eCa, byte eAa)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->EF_SetColorOp(eCo, eAo, eCa, eAa);
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_SetColorOp, 16);

    AddDWORD(p, eCo);
    AddDWORD(p, eAo);
    AddDWORD(p, eCa);
    AddDWORD(p, eAa);

    EndCommand(p);
}

void SRenderThread::RC_SetSrgbWrite(bool srgbWrite)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->EF_SetSrgbWrite(srgbWrite);
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_SetSrgbWrite, 4);

    AddDWORD(p, srgbWrite);

    EndCommand(p);
}

void SRenderThread::RC_PushWireframeMode(int nMode)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->FX_PushWireframeMode(nMode);
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_PushWireframeMode, 4);
    AddDWORD(p, nMode);
    EndCommand(p);
}

void SRenderThread::RC_PopWireframeMode()
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->FX_PopWireframeMode();
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_PopWireframeMode, 0);
    EndCommand(p);
}


void SRenderThread::RC_SetCull(int nMode)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->RT_SetCull(nMode);
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_SetCull, 4);
    AddDWORD(p, nMode);
    EndCommand(p);
}

void SRenderThread::RC_SetScissor(bool bEnable, int sX, int sY, int sWdt, int sHgt)
{
    if (IsRenderThread())
    {
        gRenDev->RT_SetScissor(bEnable, sX, sY, sWdt, sHgt);
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_SetScissor, sizeof(DWORD) * 5);
    AddDWORD(p, bEnable);
    AddDWORD(p, sX);
    AddDWORD(p, sY);
    AddDWORD(p, sWdt);
    AddDWORD(p, sHgt);
    EndCommand(p);
}

void SRenderThread::RC_PushProfileMarker(const char* label)
{
    AZ_TRACE_METHOD();
    if (IsRenderLoadingThread())
    {
        return;
    }
    if (IsRenderThread())
    {
        gRenDev->SetProfileMarker(label, CRenderer::ESPM_PUSH);
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_PushProfileMarker, sizeof(void*));
    AddPointer(p, label);
    EndCommand(p);
}

void SRenderThread::RC_PopProfileMarker(const char* label)
{
    AZ_TRACE_METHOD();
    if (IsRenderLoadingThread())
    {
        return;
    }
    if (IsRenderThread())
    {
        gRenDev->SetProfileMarker(label, CRenderer::ESPM_POP);
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_PopProfileMarker, sizeof(void*));
    AddPointer(p, label);
    EndCommand(p);
}

void SRenderThread::RC_ReadFrameBuffer(unsigned char* pRGB, int nImageX, int nSizeX, int nSizeY, ERB_Type eRBType, bool bRGBA, int nScaledX, int nScaledY)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->RT_ReadFrameBuffer(pRGB, nImageX, nSizeX, nSizeY, eRBType, bRGBA, nScaledX, nScaledY);
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_ReadFrameBuffer, 28 + sizeof(void*));
    AddPointer(p, pRGB);
    AddDWORD(p, nImageX);
    AddDWORD(p, nSizeX);
    AddDWORD(p, nSizeY);
    AddDWORD(p, eRBType);
    AddDWORD(p, bRGBA);
    AddDWORD(p, nScaledX);
    AddDWORD(p, nScaledY);
    EndCommand(p);

    FlushAndWait();
}

void SRenderThread::RC_SetCamera()
{
    if (!IsRenderThread())
    {
        LOADINGLOCK_COMMANDQUEUE
        size_t commandSize = sizeof(Matrix44) * 3 + sizeof(CameraViewParameters);
        byte* pData = AddCommand(eRC_SetCamera, Align4(commandSize));

        gRenDev->GetProjectionMatrix((float*)&pData[0]);
        gRenDev->GetModelViewMatrix((float*)&pData[sizeof(Matrix44)]);
        *(Matrix44*)((float*)&pData[sizeof(Matrix44) * 2]) = gRenDev->m_CameraZeroMatrix[m_nCurThreadFill];
        *(CameraViewParameters*)(&pData[sizeof(Matrix44) * 3]) = gRenDev->GetViewParameters();

        if (gRenDev->m_RP.m_TI[m_nCurThreadFill].m_PersFlags & RBPF_OBLIQUE_FRUSTUM_CLIPPING)
        {
            Matrix44A mObliqueProjMatrix;
            mObliqueProjMatrix.SetIdentity();

            Plane pPlane = gRenDev->m_RP.m_TI[m_nCurThreadFill].m_pObliqueClipPlane;

            mObliqueProjMatrix.m02 = pPlane.n[0];
            mObliqueProjMatrix.m12 = pPlane.n[1];
            mObliqueProjMatrix.m22 = pPlane.n[2];
            mObliqueProjMatrix.m32 = pPlane.d;

            Matrix44* mProj = (Matrix44*)&pData[0];
            *mProj = (*mProj) * mObliqueProjMatrix;

            gRenDev->m_RP.m_TI[m_nCurThreadFill].m_PersFlags &= ~RBPF_OBLIQUE_FRUSTUM_CLIPPING;
        }

        pData += Align4(commandSize);
        EndCommand(pData);
    }
    else
    {
        gRenDev->RT_SetCameraInfo();
    }
}

void SRenderThread::RC_PostLoadLevel()
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->RT_PostLevelLoading();
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_PostLevelLoading, 0);
    EndCommand(p);
}

void SRenderThread::RC_PushFog()
{
    AZ_TRACE_METHOD();
    if (!IsRenderThread())
    {
        LOADINGLOCK_COMMANDQUEUE
        byte* p = AddCommand(eRC_PushFog, 0);
        EndCommand(p);
    }
    else
    {
        gRenDev->EF_PushFog();
    }
}

void SRenderThread::RC_PopFog()
{
    AZ_TRACE_METHOD();
    if (!IsRenderThread())
    {
        LOADINGLOCK_COMMANDQUEUE
        byte* p = AddCommand(eRC_PopFog, 0);
        EndCommand(p);
    }
    else
    {
        gRenDev->EF_PopFog();
    }
}

void SRenderThread::RC_PushVP()
{
    AZ_TRACE_METHOD();
    if (!IsRenderThread())
    {
        LOADINGLOCK_COMMANDQUEUE
        byte* p = AddCommand(eRC_PushVP, 0);
        EndCommand(p);
    }
    else
    {
        gRenDev->FX_PushVP();
    }
}

void SRenderThread::RC_PopVP()
{
    AZ_TRACE_METHOD();
    if (!IsRenderThread())
    {
        LOADINGLOCK_COMMANDQUEUE
        byte* p = AddCommand(eRC_PopVP, 0);
        EndCommand(p);
    }
    else
    {
        gRenDev->FX_PopVP();
    }
}

void SRenderThread::RC_RenderTextMessages()
{
    AZ_TRACE_METHOD();
    if (!IsRenderThread())
    {
        LOADINGLOCK_COMMANDQUEUE
        byte* p = AddCommand(eRC_RenderTextMessages, 0);
        EndCommand(p);
    }
    else
    {
        gRenDev->RT_RenderTextMessages();
    }
}

void SRenderThread::RC_FlushTextureStreaming(bool bAbort)
{
    AZ_TRACE_METHOD();
    if (!IsRenderThread())
    {
        LOADINGLOCK_COMMANDQUEUE
        byte* p = AddCommand(eRC_FlushTextureStreaming, sizeof(DWORD));
        AddDWORD(p, bAbort);
        EndCommand(p);
    }
    else
    {
        CTexture::RT_FlushStreaming(bAbort);
    }
}

void SRenderThread::RC_ReleaseSystemTextures()
{
    AZ_TRACE_METHOD();
    if (!IsRenderThread())
    {
        LOADINGLOCK_COMMANDQUEUE
        byte* p = AddCommand(eRC_ReleaseSystemTextures, 0);
        EndCommand(p);
    }
    else
    {
        CTextureManager::Instance()->Release();
        CTexture::ReleaseSystemTextures();
    }
}

void SRenderThread::RC_SetEnvTexRT(SEnvTexture* pEnvTex, int nWidth, int nHeight, bool bPush)
{
    AZ_TRACE_METHOD();
    if (!IsRenderThread())
    {
        LOADINGLOCK_COMMANDQUEUE
        byte* p = AddCommand(eRC_SetEnvTexRT, 12 + sizeof(void*));
        AddPointer(p, pEnvTex);
        AddDWORD(p, nWidth);
        AddDWORD(p, nHeight);
        AddDWORD(p, bPush);
        EndCommand(p);
    }
    else
    {
        pEnvTex->m_pTex->RT_SetRT(0, nWidth, nHeight, bPush);
    }
}


void SRenderThread::RC_SetEnvTexMatrix(SEnvTexture* pEnvTex)
{
    AZ_TRACE_METHOD();
    if (!IsRenderThread())
    {
        LOADINGLOCK_COMMANDQUEUE
        byte* p = AddCommand(eRC_SetEnvTexMatrix, sizeof(void*));
        AddPointer(p, pEnvTex);
        EndCommand(p);
    }
    else
    {
        pEnvTex->RT_SetMatrix();
    }
}

void SRenderThread::RC_PushRT(int nTarget, CTexture* pTex, SDepthTexture* pDS, int nS)
{
    AZ_TRACE_METHOD();
    if (!IsRenderThread())
    {
        LOADINGLOCK_COMMANDQUEUE
        byte* p = AddCommand(eRC_PushRT, 8 + 2 * sizeof(void*));
        AddDWORD(p, nTarget);
        AddPointer(p, pTex);
        AddPointer(p, pDS);
        AddDWORD(p, nS);
        EndCommand(p);
    }
    else
    {
        gRenDev->RT_PushRenderTarget(nTarget, pTex, pDS, nS);
    }
}
void SRenderThread::RC_PopRT(int nTarget)
{
    AZ_TRACE_METHOD();
    if (!IsRenderThread())
    {
        LOADINGLOCK_COMMANDQUEUE
        byte* p = AddCommand(eRC_PopRT, 4);
        AddDWORD(p, nTarget);
        EndCommand(p);
    }
    else
    {
        gRenDev->RT_PopRenderTarget(nTarget);
    }
}

void SRenderThread::RC_ForceSwapBuffers()
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->RT_ForceSwapBuffers();
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_ForceSwapBuffers, 0);
    EndCommand(p);
}

void SRenderThread::RC_SwitchToNativeResolutionBackbuffer()
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->RT_SwitchToNativeResolutionBackbuffer(true);
        return;
    }
    else
    {
        gRenDev->SetViewport(0, 0, gRenDev->GetOverlayWidth(), gRenDev->GetOverlayHeight());
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_SwitchToNativeResolutionBackbuffer, 0);
    EndCommand(p);
}

void SRenderThread::RC_BeginFrame()
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->RT_BeginFrame();
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_BeginFrame, 0);
    EndCommand(p);
}
void SRenderThread::RC_EndFrame(bool bWait)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->RT_EndFrame();
        SyncMainWithRender();
        return;
    }
    if (!bWait && CheckFlushCond())
    {
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    gRenDev->GetIRenderAuxGeom()->Commit(); // need to issue flush of main thread's aux cb before EndFrame (otherwise it is processed after p3dDev->EndScene())
    byte* p = AddCommand(eRC_EndFrame, 0);
    EndCommand(p);
    SyncMainWithRender();
}

void SRenderThread::RC_PrecacheResource(ITexture* pTP, float fMipFactor, float fTimeToReady, int Flags, int nUpdateId, int nCounter)
{
    if (IsRenderThread())
    {
        gRenDev->PrecacheTexture(pTP, fMipFactor, fTimeToReady, Flags, nUpdateId, nCounter);
        return;
    }

    if (pTP == NULL)
    {
        return;
    }

    pTP->AddRef();

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_PrecacheTexture, 20 + sizeof(void*));
    AddPointer(p, pTP);
    AddFloat(p, fMipFactor);
    AddFloat(p, fTimeToReady);
    AddDWORD(p, Flags);
    AddDWORD(p, nUpdateId);
    AddDWORD(p, nCounter);
    EndCommand(p);
}

void SRenderThread::RC_ReleaseDeviceTexture(CTexture* pTexture)
{
    AZ_TRACE_METHOD();
    if (!strcmp(pTexture->GetName(), "EngineAssets/TextureMsg/ReplaceMe.tif"))
    {
        pTexture = pTexture;
    }

    if (IsRenderThread())
    {
        CryOptionalAutoLock<CryMutex> lock(m_lockRenderLoading, m_eVideoThreadMode != eVTM_Disabled);
        pTexture->RT_ReleaseDevice();
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_ReleaseDeviceTexture, sizeof(void*));
    AddPointer(p, pTexture);
    EndCommand(p);

    FlushAndWait();
}

void SRenderThread::RC_TryFlush()
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        return;
    }

    // do nothing if the render thread is still busy
    if (CheckFlushCond())
    {
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    gRenDev->GetIRenderAuxGeom()->Flush(); // need to issue flush of main thread's aux cb before EndFrame (otherwise it is processed after p3dDev->EndScene())
    SyncMainWithRender();
}

void SRenderThread::RC_DrawLines(Vec3 v[], int nump, ColorF& col, int flags, float fGround)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->RT_DrawLines(v, nump, col, flags, fGround);
    }
    else
    {
        LOADINGLOCK_COMMANDQUEUE
        // since we use AddData(...) - we need to allocate 4 bytes (DWORD) more because AddData(...) adds hidden data size into command buffer
        byte* p = AddCommand(eRC_DrawLines, Align4(sizeof(int) + 2 * sizeof(int) + sizeof(float) + nump * sizeof(Vec3) + sizeof(ColorF)));
        AddDWORD(p, nump);
        AddColor(p, col);
        AddDWORD(p, flags);
        AddFloat(p, fGround);
        AddData (p, v, nump * sizeof(Vec3));
        EndCommand(p);
    }
}

void SRenderThread::RC_DrawStringU(IFFont_RenderProxy* pFont, float x, float y, float z, const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->RT_DrawStringU(pFont, x, y, z, pStr, asciiMultiLine, ctx);
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_DrawStringU, Align4(16 + sizeof(void*) + sizeof(STextDrawContext) + TextCommandSize(pStr)));
    AddPointer(p, pFont);
    AddFloat(p, x);
    AddFloat(p, y);
    AddFloat(p, z);
    AddDWORD(p, asciiMultiLine ? 1 : 0);
    new(p) STextDrawContext(ctx);
    p += sizeof(STextDrawContext);
    AddText(p, pStr);
    EndCommand(p);
}
void SRenderThread::RC_ClearTargetsImmediately(int8 nType, uint32 nFlags, const ColorF& vColor, float depth)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        switch (nType)
        {
        case 0:
            gRenDev->EF_ClearTargetsImmediately(nFlags);
            return;
        case 1:
            gRenDev->EF_ClearTargetsImmediately(nFlags, vColor, depth, 0);
            return;
        case 2:
            gRenDev->EF_ClearTargetsImmediately(nFlags, vColor);
            return;
        case 3:
            gRenDev->EF_ClearTargetsImmediately(nFlags, depth, 0);
            return;
        }
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_ClearTargetsImmediately, Align4(12 + sizeof(ColorF)));
    AddDWORD(p, nType);
    AddDWORD(p, nFlags);
    AddColor(p, vColor);
    AddFloat(p, depth);
    EndCommand(p);
}

void SRenderThread::RC_SetViewport(int x, int y, int width, int height, int id)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->RT_SetViewport(x, y, width, height, id);
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_SetViewport, 20);
    AddDWORD(p, x);
    AddDWORD(p, y);
    AddDWORD(p, width);
    AddDWORD(p, height);
    AddDWORD(p, id);
    EndCommand(p);
}

void SRenderThread::RC_RenderScene(int nFlags, RenderFunc pRenderFunc)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->RT_RenderScene(nFlags, gRenDev->m_RP.m_TI[m_nCurThreadProcess], pRenderFunc);
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_RenderScene, Align4(8 + sizeof(void*) + sizeof(SThreadInfo)));
    AddDWORD(p, nFlags);
    AddTI(p, gRenDev->m_RP.m_TI[m_nCurThreadFill]);
    AddPointer(p, (void*)pRenderFunc);
    AddDWORD(p, SRendItem::m_RecurseLevel[m_nCurThreadFill]);
    EndCommand(p);
}

void SRenderThread::RC_PrepareStereo(int mode, int output)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->RT_PrepareStereo(mode, output);
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_PrepareStereo, 8);
    AddDWORD(p, mode);
    AddDWORD(p, output);
    EndCommand(p);
}

void SRenderThread::RC_CopyToStereoTex(int channel)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->RT_CopyToStereoTex(channel);
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_CopyToStereoTex, 4);
    AddDWORD(p, channel);
    EndCommand(p);
}

void SRenderThread::RC_SetStereoEye(int eye)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->m_CurRenderEye = eye;
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_SetStereoEye, 4);
    AddDWORD(p, eye);
    EndCommand(p);
}

void SRenderThread::RC_AuxFlush([[maybe_unused]] IRenderAuxGeomImpl* pAux, [[maybe_unused]] SAuxGeomCBRawDataPackaged& data, [[maybe_unused]] size_t begin, [[maybe_unused]] size_t end, [[maybe_unused]] bool reset)
{
    AZ_TRACE_METHOD();
#if defined(ENABLE_RENDER_AUX_GEOM)
    if (IsRenderThread())
    {
        pAux->RT_Flush(data, begin, end);
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_AuxFlush, 4 * sizeof(void*) + sizeof(uint32));
    AddPointer(p, pAux);
    AddPointer(p, data.m_pData);
    AddPointer(p, (void*) begin);
    AddPointer(p, (void*) end);
    AddDWORD  (p, (uint32) reset);
    EndCommand(p);
#endif
}

void SRenderThread::RC_SetTexture(int nTex, int nUnit)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        CTexture::ApplyForID(nUnit, nTex, -1, -1);
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    int nState = CTexture::GetByID(nTex)->GetDefState();
    byte* p = AddCommand(eRC_SetTexture, 12);
    AddDWORD(p, nTex);
    AddDWORD(p, nUnit);
    AddDWORD(p, nState);
    EndCommand(p);
}

bool SRenderThread::RC_OC_ReadResult_Try(uint32 nDefaultNumSamples, CREOcclusionQuery* pRE)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        return pRE->RT_ReadResult_Try(nDefaultNumSamples);
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_OC_ReadResult_Try, 4 + sizeof(void*));
    AddDWORD(p, (uint32)nDefaultNumSamples);
    AddPointer(p, pRE);
    EndCommand(p);

    return true;
}

void SRenderThread::RC_CGCSetLayers(IColorGradingControllerInt* pController, const SColorChartLayer* pLayers, uint32 numLayers)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        pController->RT_SetLayers(pLayers, numLayers);
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_CGCSetLayers, 4 + sizeof(void*));
    AddPointer(p, pController);
    AddDWORD(p, (uint32)numLayers);
    EndCommand(p);

    if (numLayers)
    {
        const size_t copySize = sizeof(SColorChartLayer) * numLayers;
        SColorChartLayer* pLayersDst = (SColorChartLayer*) m_Commands[m_nCurThreadFill].Grow(copySize);
        memcpy(pLayersDst, pLayers, copySize);
    }
}

void SRenderThread::RC_GenerateSkyDomeTextures(CREHDRSky* pSky, int32 width, int32 height)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        pSky->GenerateSkyDomeTextures(width, height);
        return;
    }

    LOADINGLOCK_COMMANDQUEUE

    if (m_eVideoThreadMode == eVTM_Disabled)
    {
        byte* p = AddCommand(eRC_GenerateSkyDomeTextures, sizeof(void*) + sizeof(int32) * 2);
        AddPointer(p, pSky);
        AddDWORD(p, width);
        AddDWORD(p, height);
        EndCommand(p);
    }
    else
    {
        // Move command into loading queue, which will be executed in first render frame after loading is done
        byte* p = AddCommandTo(eRC_GenerateSkyDomeTextures, sizeof(void*) + sizeof(int32) * 2, m_CommandsLoading);
        AddPointer(p, pSky);
        AddDWORD(p, width);
        AddDWORD(p, height);
        EndCommandTo(p, m_CommandsLoading);
    }
}

void SRenderThread::RC_SetRendererCVar(ICVar* pCVar, const char* pArgText, const bool bSilentMode)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->RT_SetRendererCVar(pCVar, pArgText, bSilentMode);
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_SetRendererCVar, sizeof(void*) + TextCommandSize(pArgText) + 4);
    AddPointer(p, pCVar);
    AddText(p, pArgText);
    AddDWORD(p, bSilentMode ? 1 : 0);
    EndCommand(p);
}

void SRenderThread::RC_RenderDebug(bool bRenderStats)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->RT_RenderDebug(bRenderStats);
        return;
    }
    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_RenderDebug, 0);
    EndCommand(p);
}

void SRenderThread::RC_PushSkinningPoolId(uint32 poolId)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        gRenDev->RT_SetSkinningPoolId(poolId);
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_PushSkinningPoolId, 4);
    AddDWORD(p, poolId);
    EndCommand(p);
}

void SRenderThread::RC_ReleaseRemappedBoneIndices(IRenderMesh* pRenderMesh, uint32 guid)
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        pRenderMesh->ReleaseRemappedBoneIndicesPair(guid);
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    pRenderMesh->AddRef(); // don't allow mesh deletion while this command is pending
    byte* p = AddCommand(eRC_ReleaseRemappedBoneIndices, sizeof(void*) + 4);
    AddPointer(p, pRenderMesh);
    AddDWORD(p, guid);
    EndCommand(p);
}

void SRenderThread::RC_InitializeVideoRenderer(AZ::VideoRenderer::IVideoRenderer* pVideoRenderer)
{
    AZ_TRACE_METHOD();

    if (IsRenderThread())
    {
        gRenDev->RT_InitializeVideoRenderer(pVideoRenderer);
        return;
    }

    LOADINGLOCK_COMMANDQUEUE;
    byte* p = AddCommand(eRC_InitializeVideoRenderer, sizeof(AZ::VideoRenderer::IVideoRenderer*));
    AddPointer(p, pVideoRenderer);
    EndCommand(p);

    // We want to block until the resources have been created.
    SyncMainWithRender();
}

void SRenderThread::RC_CleanupVideoRenderer(AZ::VideoRenderer::IVideoRenderer* pVideoRenderer)
{
    AZ_TRACE_METHOD();

    if (IsRenderThread())
    {
        gRenDev->RT_CleanupVideoRenderer(pVideoRenderer);
        return;
    }

    LOADINGLOCK_COMMANDQUEUE;
    byte* p = AddCommand(eRC_CleanupVideoRenderer, sizeof(AZ::VideoRenderer::IVideoRenderer*));
    AddPointer(p, pVideoRenderer);
    EndCommand(p);

    // We want to block until the cleanup is complete.
    SyncMainWithRender();
}

void SRenderThread::RC_DrawVideoRenderer(AZ::VideoRenderer::IVideoRenderer* pVideoRenderer, const AZ::VideoRenderer::DrawArguments& drawArguments)
{
    AZ_TRACE_METHOD();

    if (IsRenderThread())
    {
        gRenDev->RT_DrawVideoRenderer(pVideoRenderer, drawArguments);
        return;
    }

    LOADINGLOCK_COMMANDQUEUE;
    byte* p = AddCommand(eRC_DrawVideoRenderer, sizeof(AZ::VideoRenderer::IVideoRenderer*) + sizeof(AZ::VideoRenderer::DrawArguments));
    AddPointer(p, pVideoRenderer);
    memcpy(p, &drawArguments, sizeof(AZ::VideoRenderer::DrawArguments));
    p += sizeof(AZ::VideoRenderer::DrawArguments);
    EndCommand(p);
}

void SRenderThread::EnqueueRenderCommand(RenderCommandCB command)
{
    if (IsRenderThread())
    {
        command();
        return;
    }

    LOADINGLOCK_COMMANDQUEUE
    byte* p = AddCommand(eRC_AzFunction, sizeof(RenderCommandCB));
    new (p) RenderCommandCB(AZStd::move(command));
    p += sizeof(RenderCommandCB);
    EndCommand(p);
}

//===========================================================================================

#ifdef DO_RENDERSTATS
#define START_PROFILE_RT Time = iTimer->GetAsyncTime();
#define END_PROFILE_PLUS_RT(Dst) Dst += iTimer->GetAsyncTime().GetDifferenceInSeconds(Time);
#define END_PROFILE_RT(Dst) Dst = iTimer->GetAsyncTime().GetDifferenceInSeconds(Time);
#else
#define START_PROFILE_RT
#define END_PROFILE_PLUS_RT(Dst)
#define END_PROFILE_RT(Dst)
#endif

#if defined(AZ_PROFILE_TELEMETRY)
static const char* GetRenderCommandName(ERenderCommand renderCommand)
{
    switch (renderCommand)
    {
    // Please add to this list if you have a case that needs watching
    case eRC_PreloadTextures:
        return "PreloadTextures";
    case eRC_ParseShader:
        return "ParseShader";
    case eRC_RenderScene:
        return "RenderScene";
    case eRC_AzFunction:
        return "AzFunction";
    }
    return "<unknown>";
}
#endif

void SRenderThread::ProcessCommands(bool loadTimeProcessing)
{
    AZ_TRACE_METHOD();
#ifndef STRIP_RENDER_THREAD
    assert (IsRenderThread());
    if (!CheckFlushCond())
    {
        return;
    }

#if !defined (NULL_RENDERER)
    DWORD nDeviceOwningThreadID = gcpRendD3D->GetBoundThreadID();
    if (m_eVideoThreadMode == eVTM_Disabled)
    {
        gcpRendD3D->BindContextToThread(CryGetCurrentThreadId());
    }
# endif
#if defined(OPENGL) && !DXGL_FULL_EMULATION && !defined(CRY_USE_METAL)
    if (CRenderer::CV_r_multithreaded)
    {
        m_kDXGLContextHandle.Set(&gcpRendD3D->GetDevice());
    }
    if (m_eVideoThreadMode == eVTM_Disabled)
    {
        m_kDXGLDeviceContextHandle.Set(&gcpRendD3D->GetDeviceContext(), !CRenderer::CV_r_multithreaded);
    }
#endif //defined(OPENGL) && !DXGL_FULL_EMULATION


#ifdef DO_RENDERSTATS
    CTimeValue Time;
#endif

    int threadId = m_nCurThreadProcess;

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERTHREAD_CPP_SECTION_5
    #include AZ_RESTRICTED_FILE(RenderThread_cpp)
#endif
    int n = 0;
    m_bSuccessful = true;
    m_hResult = S_OK;
    byte* pP;
    while (n < (int)m_Commands[threadId].Num())
    {
        pP = &m_Commands[threadId][n];
        n += sizeof(int);
        byte nC = (byte) * ((int*)pP);

#if !defined(_RELEASE)
        // Ensure that the command hasn't been processed already
        int* pProcessed = (int*)(pP + sizeof(int));
        IF_UNLIKELY (*pProcessed)
        {
            __debugbreak();
        }
        *pProcessed = 1;
        n += sizeof(int);
#endif

        AZ_PROFILE_SCOPE_DYNAMIC(AZ::Debug::ProfileCategory::RendererDetailed, "SRenderThread::ProcessCommands::Command: %s (%d)", GetRenderCommandName(static_cast<ERenderCommand>(nC)), nC);

        switch (nC)
        {
        case eRC_CreateDevice:
            m_bSuccessful &= gRenDev->RT_CreateDevice();
            break;
        case eRC_ResetDevice:
            if (m_eVideoThreadMode == eVTM_Disabled)
            {
                gRenDev->RT_Reset();
            }
            break;
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERTHREAD_CPP_SECTION_6
    #include AZ_RESTRICTED_FILE(RenderThread_cpp)
#endif
        case eRC_ReleasePostEffects:
            if (gRenDev->m_pPostProcessMgr)
            {
                gRenDev->m_pPostProcessMgr->ReleaseResources();
            }
            break;
        case eRC_ResetPostEffects:
            if (gRenDev->m_RP.m_pREPostProcess)
            {
                gRenDev->m_RP.m_pREPostProcess->mfReset();
            }
            break;
        case eRC_ResetPostEffectsOnSpecChange:
            if (gRenDev->m_RP.m_pREPostProcess)
            {
                gRenDev->m_RP.m_pREPostProcess->Reset(true);
            }
            break;
        case eRC_DisableTemporalEffects:
            gRenDev->RT_DisableTemporalEffects();
            break;
        case eRC_ResetGlass:
            gRenDev->RT_ResetGlass();
            break;
        case eRC_ResetToDefault:
            if (m_eVideoThreadMode == eVTM_Disabled)
            {
                gRenDev->ResetToDefault();
            }
            break;
        case eRC_Init:
            gRenDev->RT_Init();
            break;
        case eRC_ShutDown:
        {
            uint32 nFlags = ReadCommand<uint32>(n);
            gRenDev->RT_ShutDown(nFlags);
        }
        break;
        case eRC_ForceSwapBuffers:
            if (m_eVideoThreadMode == eVTM_Disabled)
            {
                gRenDev->RT_ForceSwapBuffers();
            }
            break;
        case eRC_SwitchToNativeResolutionBackbuffer:
        {
            if (m_eVideoThreadMode == eVTM_Disabled)
            {
                gRenDev->RT_SwitchToNativeResolutionBackbuffer(true);
            }
        }
        break;
        case eRC_BeginFrame:
            if (m_eVideoThreadMode == eVTM_Disabled)
            {
                gRenDev->RT_BeginFrame();
            }
            else
            {
                m_bBeginFrameCalled = true;
            }
            break;
        case eRC_EndFrame:
            if (m_eVideoThreadMode == eVTM_Disabled)
            {
                gRenDev->RT_EndFrame();
            }
            else
            {
                // RLT handles precache commands - so all texture streaming prioritisation
                // needs to happen here. Scheduling and device texture management will happen
                // on the RT later.
                CTexture::RLT_LoadingUpdate();

                m_bEndFrameCalled = true;
                gRenDev->m_nFrameSwapID++;
            }
            break;
        case eRC_PreloadTextures:
            CTexture::RT_Precache();
            break;
        case eRC_PrecacheTexture:
        {
            ITexture* pTP = ReadCommand<ITexture*>(n);

            float fMipFactor = ReadCommand<float>(n);
            float fTimeToReady = ReadCommand<float>(n);
            int Flags = ReadCommand<int>(n);
            int nUpdateId = ReadCommand<int>(n);
            int nCounter = ReadCommand<int>(n);
            gRenDev->PrecacheTexture(pTP, fMipFactor, fTimeToReady, Flags, nUpdateId, nCounter);

            pTP->Release();
        }
        break;
        case eRC_ClearTargetsImmediately:
        {
            uint32 nType = ReadCommand<uint32>(n);
            uint32 nFlags = ReadCommand<uint32>(n);
            ColorF vColor = ReadCommand<ColorF>(n);
            float fDepth = ReadCommand<float>(n);
            if (m_eVideoThreadMode != eVTM_Disabled)
            {
                nFlags &= ~FRT_CLEAR_IMMEDIATE;
                switch (nType)
                {
                case 0:
                    gRenDev->EF_ClearTargetsLater(nFlags);
                    break;
                case 1:
                    gRenDev->EF_ClearTargetsLater(nFlags, vColor, fDepth, 0);
                    break;
                case 2:
                    gRenDev->EF_ClearTargetsLater(nFlags, vColor);
                    break;
                case 3:
                    gRenDev->EF_ClearTargetsLater(nFlags, fDepth, 0);
                    break;
                }
            }
            switch (nType)
            {
            case 0:
                gRenDev->EF_ClearTargetsImmediately(nFlags);
                break;
            case 1:
                gRenDev->EF_ClearTargetsImmediately(nFlags, vColor, fDepth, 0);
                break;
            case 2:
                gRenDev->EF_ClearTargetsImmediately(nFlags, vColor);
                break;
            case 3:
                gRenDev->EF_ClearTargetsImmediately(nFlags, fDepth, 0);
                break;
            }
        }
        break;
        case eRC_ReadFrameBuffer:
        {
            byte* pRGB = ReadCommand<byte*>(n);
            int nImageX = ReadCommand<int>(n);
            int nSizeX = ReadCommand<int>(n);
            int nSizeY = ReadCommand<int>(n);
            ERB_Type RBType = ReadCommand<ERB_Type>(n);
            int bRGBA = ReadCommand<int>(n);
            int nScaledX = ReadCommand<int>(n);
            int nScaledY = ReadCommand<int>(n);
            gRenDev->RT_ReadFrameBuffer(pRGB, nImageX, nSizeX, nSizeY, RBType, bRGBA, nScaledX, nScaledY);
        }
        break;
        case eRC_UpdateShaderItem:
        {
            SShaderItem* pShaderItem = ReadCommand<SShaderItem*>(n);
            // The material is necessary at this point because an UpdateShaderItem may have been queued 
            // for a material that was subsequently released and would have been deleted, thus resulting in a
            // dangling pointer and a crash; this keeps it alive until this render command can complete
            IMaterial* pMaterial = ReadCommand<IMaterial*>(n);
            gRenDev->RT_UpdateShaderItem(pShaderItem, pMaterial);
            if (pMaterial)
            {
                // Release the reference we added when we submitted the command.
                pMaterial->Release();
            }
        }
        break;
        case eRC_RefreshShaderResourceConstants:
        {
            SShaderItem* shaderItem = ReadCommand<SShaderItem*>(n);
            // The material is necessary at this point because an RefreshShaderResourceConstants may have been queued 
            // for a material that was subsequently released and would have been deleted, thus resulting in a
            // dangling pointer and a crash; this keeps it alive until this render command can complete
            IMaterial* material = ReadCommand<IMaterial*>(n);
            gRenDev->RT_RefreshShaderResourceConstants(shaderItem);
            if (material)
            {
                // Release the reference we added when we submitted the command.
                material->Release();
            }
        }
        break;
        case eRC_ReleaseDeviceTexture:
            assert (m_eVideoThreadMode == eVTM_Disabled);
            {
                CTexture* pTexture = ReadCommand<CTexture*>(n);
                pTexture->RT_ReleaseDevice();
            }
            break;
        case eRC_AuxFlush:
        {
#if defined(ENABLE_RENDER_AUX_GEOM)
            IRenderAuxGeomImpl* pAux = ReadCommand<IRenderAuxGeomImpl*>(n);
            CAuxGeomCB::SAuxGeomCBRawData* pData = ReadCommand<CAuxGeomCB::SAuxGeomCBRawData*>(n);
            size_t begin = ReadCommand<size_t>(n);
            size_t end = ReadCommand<size_t>(n);
            bool reset = ReadCommand<uint32>(n) != 0;

            if (m_eVideoThreadMode == eVTM_Disabled)
            {
                SAuxGeomCBRawDataPackaged data = SAuxGeomCBRawDataPackaged(pData);
                pAux->RT_Flush(data, begin, end, reset);
            }
#endif
        }
        break;
        case eRC_SetTexture:
            assert (m_eVideoThreadMode == eVTM_Disabled);
            {
                int nTex = ReadCommand<int>(n);
                int nUnit = ReadCommand<int>(n);
                int nState = ReadCommand<int>(n);
                CTexture::ApplyForID(nUnit, nTex, nState, -1);
            }
            break;
        case eRC_DrawLines:
        {
            int    nump    = ReadCommand<int>(n);
            ColorF col     = ReadCommand<ColorF>(n);
            int    flags   = ReadCommand<int>(n);
            float  fGround = ReadCommand<float>(n);


            Vec3* pv = (Vec3*)&m_Commands[threadId][n += 4];
            n += nump * sizeof(Vec3);

            gRenDev->RT_DrawLines(pv, nump, col, flags, fGround);
        }
        break;
        case eRC_DrawStringU:
        {
            IFFont_RenderProxy* pFont = ReadCommand<IFFont_RenderProxy*>(n);
            float x = ReadCommand<float>(n);
            float y = ReadCommand<float>(n);
            float z = ReadCommand<float>(n);
            bool asciiMultiLine = ReadCommand<int>(n) != 0;
            const STextDrawContext* pCtx = (const STextDrawContext*) &m_Commands[threadId][n];
            n += sizeof(STextDrawContext);
            const char* pStr = ReadTextCommand<const char*>(n);

            if (m_eVideoThreadMode == eVTM_Disabled)
            {
                gRenDev->RT_DrawStringU(pFont, x, y, z, pStr, asciiMultiLine, *pCtx);
            }
        }
        break;
        case eRC_SetState:
        {
            int nState = ReadCommand<int>(n);
            int nAlphaRef = ReadCommand<int>(n);
            if (m_eVideoThreadMode == eVTM_Disabled)
            {
                gRenDev->FX_SetState(nState, nAlphaRef);
            }
        }
        break;
        case eRC_PushWireframeMode:
        {
            int nMode = ReadCommand<int>(n);
            if (m_eVideoThreadMode == eVTM_Disabled)
            {
                gRenDev->FX_PushWireframeMode(nMode);
            }
        }
        break;
        case eRC_PopWireframeMode:
        {
            if (m_eVideoThreadMode == eVTM_Disabled)
            {
                gRenDev->FX_PopWireframeMode();
            }
        }
        break;
        case eRC_SetCull:
        {
            int nMode = ReadCommand<int>(n);
            if (m_eVideoThreadMode == eVTM_Disabled)
            {
                gRenDev->RT_SetCull(nMode); // nMode
            }
        }
        break;
        case eRC_SetScissor:
        {
            bool bEnable = ReadCommand<DWORD>(n) != 0;
            int sX = (int)ReadCommand<DWORD>(n);
            int sY = (int)ReadCommand<DWORD>(n);
            int sWdt = (int)ReadCommand<DWORD>(n);
            int sHgt = (int)ReadCommand<DWORD>(n);
            if (m_eVideoThreadMode == eVTM_Disabled)
            {
                gRenDev->RT_SetScissor(bEnable, sX, sY, sWdt, sHgt);
            }
        }
        break;
        case eRC_SetStencilState:
        {
            int st = ReadCommand<DWORD>(n);
            uint32 nStencRef = ReadCommand<DWORD>(n);
            uint32 nStencMask = ReadCommand<DWORD>(n);
            uint32 nStencWriteMask = ReadCommand<DWORD>(n);
            bool bForceFullReadMask = ReadCommand<DWORD>(n);
            if (m_eVideoThreadMode == eVTM_Disabled)
            {
                gRenDev->FX_SetStencilState(st, nStencRef, nStencMask, nStencWriteMask, bForceFullReadMask);
            }
        }
        break;
        case eRC_UpdateTexture:
        {
            CTexture* pTexture = ReadCommand<CTexture*>(n);
            byte* pData = ReadCommand<byte*>(n);
            int nX = ReadCommand<int>(n);
            int nY = ReadCommand<int>(n);
            int nZ = ReadCommand<int>(n);
            int nUSize = ReadCommand<int>(n);
            int nVSize = ReadCommand<int>(n);
            int nZSize = ReadCommand<int>(n);
            ETEX_Format eTFSrc = (ETEX_Format)ReadCommand<uint32>(n);
            pTexture->RT_UpdateTextureRegion(pData, nX, nY, nZ, nUSize, nVSize, nZSize, eTFSrc);
            delete [] pData;
            pTexture->Release();
        }
        break;
        case eRC_CreateResource:
        {
            SResourceAsync* pRA = ReadCommand<SResourceAsync*>(n);
            gRenDev->RT_CreateResource(pRA);
        }
        break;
        case eRC_ReleaseResource:
        {
            SResourceAsync* pRes = ReadCommand<SResourceAsync*>(n);
            gRenDev->RT_ReleaseResource(pRes);
        }
        break;
        case eRC_ReleaseRenderResources:
            gRenDev->RT_ReleaseRenderResources();
            break;
        case eRC_UnbindTMUs:
            gRenDev->RT_UnbindTMUs();
            break;
        case eRC_UnbindResources:
            gRenDev->RT_UnbindResources();
            break;
        case eRC_CreateRenderResources:
            gRenDev->RT_CreateRenderResources();
            break;
        case eRC_CreateSystemTargets:
            CTexture::CreateSystemTargets();
            break;
        case eRC_PrecacheDefaultShaders:
            gRenDev->RT_PrecacheDefaultShaders();
            break;
        case eRC_ReleaseSurfaceResource:
        {
            SDepthTexture* pRes = ReadCommand<SDepthTexture*>(n);
            if (pRes)
            {
                pRes->Release(true);
            }
        }
        break;
        case eRC_ReleaseBaseResource:
        {
            CBaseResource* pRes = ReadCommand<CBaseResource*>(n);
            RC_ReleaseBaseResource(pRes);
        }
        break;
        case eRC_ReleaseFont:
        {
            IFFont* font = ReadCommand<IFFont*>(n);
            RC_ReleaseFont(font);
        }
        break;
        case eRC_UpdateMesh2:
            assert(m_eVideoThreadMode == eVTM_Disabled);
            {
                CRenderMesh* pMesh = ReadCommand<CRenderMesh*>(n);
                CRenderMesh* pVContainer = ReadCommand<CRenderMesh*>(n);
                uint32 nStreamMask = ReadCommand<uint32>(n);
                pMesh->RT_CheckUpdate(pVContainer, nStreamMask);
            }
            break;
        case eRC_CreateDeviceTexture:
        {
            CryOptionalAutoLock<CryMutex> lock(m_lockRenderLoading, loadTimeProcessing);
            CTexture* pTex = ReadCommand<CTexture*>(n);
            const byte* pData[6];
            for (int i = 0; i < 6; i++)
            {
                pData[i] = ReadCommand<byte*>(n);
            }
            m_bSuccessful = pTex->RT_CreateDeviceTexture(pData);
        }
        break;
        case eRC_CopyDataToTexture:
        {
            void* pkTexture = ReadCommand<void*>(n);
            unsigned int uiStartMip = ReadCommand<unsigned int>(n);
            unsigned int uiEndMip = ReadCommand<unsigned int>(n);

            RC_CopyDataToTexture(pkTexture, uiStartMip, uiEndMip);
        }
        break;
        case eRC_ClearTarget:
        {
            void* pkTexture = ReadCommand<void*>(n);
            ColorF kColor = ReadCommand<ColorF>(n);

            RC_ClearTarget(pkTexture, kColor);
        }
        break;
        case eRC_CreateREPostProcess:
        {
            CRendElementBase** pRE = ReadCommand<CRendElementBase**>(n);
            gRenDev->RT_CreateREPostProcess(pRE);
        }
        break;

        case eRC_DrawDynVB:
        {
            pP = &m_Commands[threadId][0];
            uint32 nSize = *(uint32*)&pP[n];
            SVF_P3F_C4B_T2F* pBuf = (SVF_P3F_C4B_T2F*)&pP[n + 4];
            n += nSize + 4;
            nSize = *(uint32*)&pP[n];
            uint16* pInds = (nSize > 0) ? (uint16*)&pP[n + 4] : nullptr;
            n += nSize + 4;
            int nVerts = ReadCommand<int>(n);
            int nInds = ReadCommand<int>(n);
            const PublicRenderPrimitiveType nPrimType = (PublicRenderPrimitiveType)ReadCommand<int>(n);
            if (m_eVideoThreadMode == eVTM_Disabled)
            {
                gRenDev->RT_DrawDynVB(pBuf, pInds, nVerts, nInds, nPrimType);
            }
        }
        break;
        case eRC_Draw2dImage:
        {
            START_PROFILE_RT;

            float xpos = ReadCommand<float>(n);
            float ypos = ReadCommand<float>(n);
            float w = ReadCommand<float>(n);
            float h = ReadCommand<float>(n);
            CTexture* pTexture = ReadCommand<CTexture*>(n);
            float s0 = ReadCommand<float>(n);
            float t0 = ReadCommand<float>(n);
            float s1 = ReadCommand<float>(n);
            float t1 = ReadCommand<float>(n);
            float angle = ReadCommand<float>(n);
            int col = ReadCommand<int>(n);
            float z = ReadCommand<float>(n);
            if (m_eVideoThreadMode == eVTM_Disabled)
            {
                gRenDev->RT_Draw2dImage(xpos, ypos, w, h, pTexture, s0, t0, s1, t1, angle, col, z);
            }
            END_PROFILE_PLUS_RT(gRenDev->m_fRTTimeMiscRender);
        }
        break;
        case eRC_DrawDynVBUI:
            {
                pP = &m_Commands[threadId][0];
                uint32 nSize = *(uint32*)&pP[n];
                SVF_P2F_C4B_T2F_F4B* pBuf = (SVF_P2F_C4B_T2F_F4B*)&pP[n + 4];
                n += nSize + 4;
                nSize = *(uint32*)&pP[n];
                uint16* pInds = (nSize > 0) ? (uint16*)&pP[n + 4] : nullptr;
                n += nSize + 4;
                int nVerts = ReadCommand<int>(n);
                int nInds = ReadCommand<int>(n);
                const PublicRenderPrimitiveType nPrimType = (PublicRenderPrimitiveType)ReadCommand<int>(n);
                if (m_eVideoThreadMode == eVTM_Disabled)
                {
                    gRenDev->RT_DrawDynVBUI(pBuf, pInds, nVerts, nInds, nPrimType);
                }
            }
        break;

        case eRC_Draw2dImageStretchMode:
        {
            START_PROFILE_RT;

            int mode = ReadCommand<int>(n);
            gRenDev->RT_Draw2dImageStretchMode(mode);
            END_PROFILE_PLUS_RT(gRenDev->m_fRTTimeMiscRender);
        }
        break;
        case eRC_Push2dImage:
        {
            float xpos = ReadCommand<float>(n);
            float ypos = ReadCommand<float>(n);
            float w = ReadCommand<float>(n);
            float h = ReadCommand<float>(n);
            CTexture* pTexture = ReadCommand<CTexture*>(n);
            float s0 = ReadCommand<float>(n);
            float t0 = ReadCommand<float>(n);
            float s1 = ReadCommand<float>(n);
            float t1 = ReadCommand<float>(n);
            float angle = ReadCommand<float>(n);
            int col = ReadCommand<int>(n);
            float z = ReadCommand<float>(n);
            float stereoDepth = ReadCommand<float>(n);
            if (m_eVideoThreadMode == eVTM_Disabled)
            {
                gRenDev->RT_Push2dImage(xpos, ypos, w, h, pTexture, s0, t0, s1, t1, angle, col, z, stereoDepth);
            }
        }
        break;
        case eRC_Draw2dImageList:
        {
            if (m_eVideoThreadMode == eVTM_Disabled)
            {
                gRenDev->RT_Draw2dImageList();
            }
        }
        break;
        case eRC_DrawImageWithUV:
            if (m_eVideoThreadMode == eVTM_Disabled)
            {
                pP = &m_Commands[threadId][n];
                gRenDev->RT_DrawImageWithUV(*(float*)pP, // xpos
                    *(float*)&pP[4], // ypos
                    *(float*)&pP[8], // z
                    *(float*)&pP[12], // w
                    *(float*)&pP[16], // h
                    *(int*)&pP[20], // textureid
                    (float*)&pP[24], // s
                    (float*)&pP[40], // t
                    *(int*)&pP[56], // col
                    *(int*)&pP[60] != 0); // filtered
            }
            n += 64;
            break;
        case eRC_PushProfileMarker:
        {
            char* label = ReadCommand<char*>(n);
            gRenDev->PushProfileMarker(label);
        }
        break;
        case eRC_PopProfileMarker:
        {
            char* label = ReadCommand<char*>(n);
            gRenDev->PopProfileMarker(label);
        }
        break;
        case eRC_SetCamera:
        {
            START_PROFILE_RT;

            Matrix44A ProjMat = ReadCommand<Matrix44>(n);
            Matrix44A ViewMat = ReadCommand<Matrix44>(n);
            Matrix44A CameraZeroMat = ReadCommand<Matrix44>(n);
            CameraViewParameters viewParameters = ReadCommand<CameraViewParameters>(n);

            if (m_eVideoThreadMode == eVTM_Disabled)
            {
                gRenDev->SetMatrices(ProjMat.GetData(), ViewMat.GetData());
                gRenDev->m_CameraZeroMatrix[threadId] = CameraZeroMat;
                gRenDev->SetViewParameters(viewParameters);

                gRenDev->RT_SetCameraInfo();
            }

            END_PROFILE_PLUS_RT(gRenDev->m_fRTTimeMiscRender);
        }
        break;
        case eRC_AzFunction:
        {
            // Lock only when processing on the RenderLoadThread - multiple AzFunctions make calls that cause crashes if invoked concurrently with render
            CryOptionalAutoLock<CryMutex> lock(m_lockRenderLoading, loadTimeProcessing);

            // We "build" the command from the buffer memory (instead of copying it)
            RenderCommandCB* command = alias_cast<RenderCommandCB*>(reinterpret_cast<uint32*>(&m_Commands[threadId][n]));
            (*command)();
            // We need to destroy the object that we created using placement new.
            command->~RenderCommandCB();
            n += Align4(sizeof(RenderCommandCB));
        }
        break;
        case eRC_ReleaseVBStream:
            assert (m_eVideoThreadMode == eVTM_Disabled);
            {
                void* pVB = ReadCommand<void*>(n);
                int nStream = ReadCommand<int>(n);
                gRenDev->RT_ReleaseVBStream(pVB, nStream);
            }
            break;
        case eRC_ReleaseVB:
            assert (m_eVideoThreadMode == eVTM_Disabled);
            {
                buffer_handle_t nID = ReadCommand<buffer_handle_t>(n);
                gRenDev->m_DevBufMan.Destroy(nID);
            }
            break;
        case eRC_ReleaseIB:
            assert (m_eVideoThreadMode == eVTM_Disabled);
            {
                buffer_handle_t nID = ReadCommand<buffer_handle_t>(n);
                gRenDev->m_DevBufMan.Destroy(nID);
            }
            break;
        case eRC_RenderScene:
        {
            START_PROFILE_RT;

            int nFlags = ReadCommand<int>(n);
            SThreadInfo TI;
            LoadUnaligned((uint32*)&m_Commands[threadId][n], TI);
            n += sizeof(SThreadInfo);
            RenderFunc pRenderFunc = ReadCommand<RenderFunc>(n);
            int nR = ReadCommand<int>(n);
            int nROld = SRendItem::m_RecurseLevel[threadId];
            SRendItem::m_RecurseLevel[threadId] = nR;
            // when we are in video mode, don't execute the command
            if (m_eVideoThreadMode == eVTM_Disabled)
            {
                gRenDev->RT_RenderScene(nFlags, TI, pRenderFunc);
            }
            else
            {
                // cleanup when showing loading render screen
                if (nR == 1)
                {
                    ////////////////////////////////////////////////
                    // wait till all SRendItems for this frame have finished preparing
                    gRenDev->GetFinalizeRendItemJobExecutor(gRenDev->m_RP.m_nProcessThreadID)->WaitForCompletion();
                    gRenDev->GetFinalizeShadowRendItemJobExecutor(gRenDev->m_RP.m_nProcessThreadID)->WaitForCompletion();

                    ////////////////////////////////////////////////
                    // to non-thread safe remaing work for *::Render functions
                    CRenderMesh::FinalizeRendItems(gRenDev->m_RP.m_nProcessThreadID);
                    CMotionBlur::InsertNewElements();
                    FurBendData::Get().InsertNewElements();
                }
            }
            SRendItem::m_RecurseLevel[threadId] = nROld;

            END_PROFILE_PLUS_RT(gRenDev->m_fRTTimeSceneRender);
        }
        break;
        case eRC_PrepareStereo:
        {
            int mode = ReadCommand<int>(n);
            int output = ReadCommand<int>(n);
            if (m_eVideoThreadMode == eVTM_Disabled)
            {
                gRenDev->RT_PrepareStereo(mode, output);
            }
        }
        break;
        case eRC_CopyToStereoTex:
        {
            int channel = ReadCommand<int>(n);
            gRenDev->RT_CopyToStereoTex(channel);
        }
        break;
        case eRC_SetStereoEye:
        {
            int eye = ReadCommand<int>(n);
            gRenDev->m_CurRenderEye = eye;
        }
        break;
        case eRC_DynTexUpdate:
            assert (m_eVideoThreadMode == eVTM_Disabled);
            {
                SDynTexture* pTex = ReadCommand<SDynTexture*>(n);
                int nNewWidth = ReadCommand<int>(n);
                int nNewHeight = ReadCommand<int>(n);
                pTex->RT_Update(nNewWidth, nNewHeight);
            }
            break;
        case eRC_ParseShader:
        {
            CShader* pSH = ReadCommand<CShader*>(n);
            CShaderResources* pRes = ReadCommand<CShaderResources*>(n);
            uint64 nMaskGen = ReadCommand<uint64>(n);
            uint32 nFlags = ReadCommand<uint32>(n);

            gRenDev->m_cEF.RT_ParseShader(pSH, nMaskGen, nFlags, pRes);
            pSH->Release();
            if (pRes)
            {
                pRes->Release();
            }
        }
        break;
        case eRC_SetShaderQuality:
        {
            EShaderType eST = (EShaderType) ReadCommand<uint32>(n);
            EShaderQuality eSQ = (EShaderQuality) ReadCommand<uint32>(n);
            gRenDev->m_cEF.RT_SetShaderQuality(eST, eSQ);
        }
        break;
        case eRC_PushFog:
            gRenDev->EF_PushFog();
            break;
        case eRC_PopFog:
            gRenDev->EF_PopFog();
            break;
        case eRC_PushVP:
            gRenDev->FX_PushVP();
            break;
        case eRC_PopVP:
            gRenDev->FX_PopVP();
            break;
        case eRC_RenderTextMessages:
            gRenDev->RT_RenderTextMessages();
            break;
        case eRC_FlushTextureStreaming:
        {
            bool bAbort = ReadCommand<DWORD>(n) != 0;
            CTexture::RT_FlushStreaming(bAbort);
        }
        break;
        case eRC_ReleaseSystemTextures:
            CTextureManager::Instance()->Release();
            CTexture::ReleaseSystemTextures();
            break;
        case eRC_SetEnvTexRT:
        {
            SEnvTexture* pTex = ReadCommand<SEnvTexture*>(n);
            int nWidth = ReadCommand<int>(n);
            int nHeight = ReadCommand<int>(n);
            int bPush = ReadCommand<int>(n);
            if (m_eVideoThreadMode == eVTM_Disabled)
            {
                pTex->m_pTex->RT_SetRT(0, nWidth, nHeight, bPush);
            }
        }
        break;
        case eRC_SetEnvTexMatrix:
        {
            SEnvTexture* pTex = ReadCommand<SEnvTexture*>(n);
            pTex->RT_SetMatrix();
        }
        break;
        case eRC_PushRT:
            assert (m_eVideoThreadMode == eVTM_Disabled);
            {
                int nTarget = ReadCommand<int>(n);
                CTexture* pTex = ReadCommand<CTexture*>(n);
                SDepthTexture* pDS = ReadCommand<SDepthTexture*>(n);
                int nS = ReadCommand<int>(n);
                if (m_eVideoThreadMode == eVTM_Disabled)
                {
                    gRenDev->RT_PushRenderTarget(nTarget, pTex, pDS, nS);
                }
            }
            break;
        case eRC_PopRT:
        {
            int nTarget = ReadCommand<int>(n);
            if (m_eVideoThreadMode == eVTM_Disabled)
            {
                gRenDev->RT_PopRenderTarget(nTarget);
            }
        }
        break;
        case eRC_EntityDelete:
        {
            IRenderNode* pRN = ReadCommand<IRenderNode*>(n);
            SDynTexture_Shadow::RT_EntityDelete(pRN);
        }
        break;
        case eRC_PreactivateShaders:
        {
            CHWShader::RT_PreactivateShaders();
        }
        break;
        case eRC_PrecacheShader:
        {
            CShader* pShader = ReadCommand<CShader*>(n);
            SShaderCombination cmb = ReadCommand<SShaderCombination>(n);
            bool bForce = ReadCommand<DWORD>(n) != 0;
            bool bCompressedOnly = ReadCommand<DWORD>(n) != 0;
            CShaderResources* pRes = ReadCommand<CShaderResources*>(n);

            pShader->mfPrecache(cmb, bForce, bCompressedOnly, pRes);

            SAFE_RELEASE(pRes);
            pShader->Release();
        }
        break;
        case eRC_SetViewport:
        {
            int nX = ReadCommand<int>(n);
            int nY = ReadCommand<int>(n);
            int nWidth = ReadCommand<int>(n);
            int nHeight = ReadCommand<int>(n);
            int nID = ReadCommand<int>(n);
            // when we are in video mode, don't execute the command
            if (m_eVideoThreadMode == eVTM_Disabled)
            {
                gRenDev->RT_SetViewport(nX, nY, nWidth, nHeight, nID);
            }
        }
        break;
        case eRC_TexBlurAnisotropicVertical:
        {
            CTexture* pTex = ReadCommand<CTexture*>(n);
            float fAnisoScale = ReadCommand<float>(n);
            if (m_eVideoThreadMode == eVTM_Disabled)
            {
                TexBlurAnisotropicVertical(pTex, 1, 8 * max(1.0f - min(fAnisoScale / 100.0f, 1.0f), 0.2f), 1, false);
            }
        }
        break;

        case eRC_OC_ReadResult_Try:
        {
            uint32 nDefaultNumSamples = ReadCommand<uint32>(n);
            CREOcclusionQuery* pRE = ReadCommand<CREOcclusionQuery*>(n);
            if (m_eVideoThreadMode == eVTM_Disabled)
            {
                pRE->RT_ReadResult_Try(nDefaultNumSamples);
            }
        }
        break;

        case eRC_PostLevelLoading:
        {
            gRenDev->RT_PostLevelLoading();
        }
        break;

        case eRC_StartVideoThread:
            m_eVideoThreadMode = eVTM_RequestStart;
            break;
        case eRC_StopVideoThread:
            m_eVideoThreadMode = eVTM_RequestStop;
            break;

        case eRC_CGCSetLayers:
        {
            IColorGradingControllerInt* pController = ReadCommand<IColorGradingControllerInt*>(n);
            uint32 numLayers = ReadCommand<uint32>(n);
            const SColorChartLayer* pLayers = numLayers ? (const SColorChartLayer*) &m_Commands[threadId][n] : 0;
            n += sizeof(SColorChartLayer) * numLayers;
            if (m_eVideoThreadMode == eVTM_Disabled)
            {
                pController->RT_SetLayers(pLayers, numLayers);
            }
        }
        break;

        case eRC_GenerateSkyDomeTextures:
        {
            CREHDRSky* pSky = ReadCommand<CREHDRSky*>(n);
            int32 width = ReadCommand<int32>(n);
            int32 height = ReadCommand<int32>(n);
            pSky->GenerateSkyDomeTextures(width, height);
        }
        break;

        case eRC_SetRendererCVar:
        {
            ICVar* pCVar = ReadCommand<ICVar*>(n);
            const char* pArgText = ReadTextCommand<const char*>(n);
            const bool bSilentMode = ReadCommand<int>(n) != 0;

            gRenDev->RT_SetRendererCVar(pCVar, pArgText, bSilentMode);
        }
        break;

        case eRC_RenderDebug:
        {
            if (m_eVideoThreadMode == eVTM_Disabled)
            {
                gRenDev->RT_RenderDebug();
            }
            else
            {
                gRenDev->RT_RenderTextMessages();
            }
        }
        break;

        case eRC_ForceMeshGC:
            if (m_eVideoThreadMode == eVTM_Disabled)
            {
                CRenderMesh::Tick();
            }
            break;

        case eRC_DevBufferSync:
            if (m_eVideoThreadMode == eVTM_Disabled)
            {
                gRenDev->m_DevBufMan.Sync(gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_nFrameUpdateID);
            }
            break;

        case eRC_UnlinkTexture:
        {
            CTexture* tex = ReadCommand<CTexture*>(n);
            tex->RT_Unlink();
        }
        break;

        case eRC_RelinkTexture:
        {
            CTexture* tex = ReadCommand<CTexture*>(n);
            tex->RT_Relink();
        }
        break;

        case eRC_PushSkinningPoolId:
            gRenDev->RT_SetSkinningPoolId(ReadCommand< uint32 >(n));
            break;
        case eRC_ReleaseRemappedBoneIndices:
        {
            IRenderMesh* pRenderMesh = ReadCommand<IRenderMesh*>(n);
            uint32 guid = ReadCommand<uint32>(n);
            pRenderMesh->ReleaseRemappedBoneIndicesPair(guid);
            pRenderMesh->Release();
        }
        break;

        case eRC_SetColorOp:
        {
            int eCo = ReadCommand<int>(n);
            int eAo = ReadCommand<int>(n);
            int eCa = ReadCommand<int>(n);
            int eAa = ReadCommand<int>(n);
            if (m_eVideoThreadMode == eVTM_Disabled)
            {
                gRenDev->EF_SetColorOp(eCo, eAo, eCa, eAa);
            }
        }
        break;

        case eRC_SetSrgbWrite:
        {
            bool srgbWrite = ReadCommand<int>(n) != 0;
            if (m_eVideoThreadMode == eVTM_Disabled)
            {
                gRenDev->EF_SetSrgbWrite(srgbWrite);
            }
        }
        break;

        case eRC_InitializeVideoRenderer:
        {
            AZ::VideoRenderer::IVideoRenderer* pVideoRenderer = ReadCommand<AZ::VideoRenderer::IVideoRenderer*>(n);
            gRenDev->RT_InitializeVideoRenderer(pVideoRenderer);
        }
        break;

        case eRC_CleanupVideoRenderer:
        {
            AZ::VideoRenderer::IVideoRenderer* pVideoRenderer = ReadCommand<AZ::VideoRenderer::IVideoRenderer*>(n);
            gRenDev->RT_CleanupVideoRenderer(pVideoRenderer);
        }
        break;

        case eRC_DrawVideoRenderer:
        {
            AZ::VideoRenderer::IVideoRenderer* pVideoRenderer = ReadCommand<AZ::VideoRenderer::IVideoRenderer*>(n);
            const AZ::VideoRenderer::DrawArguments drawArguments = ReadCommand<AZ::VideoRenderer::DrawArguments>(n);
            gRenDev->RT_DrawVideoRenderer(pVideoRenderer, drawArguments);
        }
        break;

        default:
        {
            assert(0);
        }
        break;
        }
    }

#if !defined (NULL_RENDERER)
    if (m_eVideoThreadMode == eVTM_Disabled)
    {
        gcpRendD3D->BindContextToThread(nDeviceOwningThreadID);
    }
# endif

#endif//STRIP_RENDER_THREAD
}

void SRenderThread::Process()
{
    AZ_TRACE_METHOD();
    while (true)
    {
        CTimeValue Time = iTimer->GetAsyncTime();

        if (m_bQuit)
        {
            SignalFlushFinishedCond();
            break;//put it here to safely shut down
        }

        WaitFlushCond();

        const uint64 start = CryGetTicks();

        CTimeValue TimeAfterWait = iTimer->GetAsyncTime();
        gRenDev->m_fTimeWaitForMain[m_nCurThreadProcess] += TimeAfterWait.GetDifferenceInSeconds(Time);
        if (gRenDev->m_bStartLevelLoading)
        {
            m_fTimeIdleDuringLoading += TimeAfterWait.GetDifferenceInSeconds(Time);
        }

        float fT = 0.f;

        if (m_eVideoThreadMode == eVTM_Disabled)
        {
            //gRenDev->m_fRTTimeBeginFrame = 0;
            //gRenDev->m_fRTTimeEndFrame = 0;
            gRenDev->m_fRTTimeSceneRender = 0;
            gRenDev->m_fRTTimeMiscRender = 0;

            ProcessCommands(false /*loadTimeProcessing*/);

            CTimeValue TimeAfterProcess = iTimer->GetAsyncTime();
            fT = TimeAfterProcess.GetDifferenceInSeconds(TimeAfterWait);
            gRenDev->m_fTimeProcessedRT[m_nCurThreadProcess] += fT;

            if (m_eVideoThreadMode == eVTM_RequestStart)
            {
            }

            SignalFlushFinishedCond();
        }

        if (gRenDev->m_bStartLevelLoading)
        {
            m_fTimeBusyDuringLoading += fT;
        }
#if !defined (NULL_RENDERER)
        if (m_eVideoThreadMode == eVTM_RequestStart)
        {
            uint32 frameId = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_nFrameUpdateID;
            DWORD nDeviceOwningThreadID = gcpRendD3D->GetBoundThreadID();
            gcpRendD3D->BindContextToThread(CryGetCurrentThreadId());
            gRenDev->m_DevBufMan.Sync(frameId); // make sure no request are flying when switching to render loading thread

            // Guarantee default resources
            gRenDev->InitSystemResources(0);

            // Create another render thread;
            SwitchMode(true);

            {
                CTimeValue lastTime = gEnv->pTimer->GetAsyncTime();
                CTimeValue workingStart = lastTime;
                CTimeValue workingEnd = lastTime;

                while (m_eVideoThreadMode != eVTM_ProcessingStop)
                {
                    AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Renderer, "Loading Frame");

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERTHREAD_CPP_SECTION_7
    #include AZ_RESTRICTED_FILE(RenderThread_cpp)
#endif
                    frameId += 1;

                    const CTimeValue curTime = gEnv->pTimer->GetAsyncTime();
                    const CTimeValue deltaTime = curTime - lastTime;
                    const CTimeValue workingTime = workingEnd - workingStart;

                    const float deltaTimeInSeconds = AZStd::max<float>(deltaTime.GetSeconds(), 0.0f);

                    lastTime = curTime;

                    // If we spent less than half of the last frame doing anything, try to spend most of that time sleeping this frame.
                    // This will help us spend less time in the lock while presenting when vsync is enabled.
                    if (workingTime.GetValue() < (deltaTime.GetValue() / 2))
                    {
                        const uint32_t sleepTime = AZStd::min<uint32_t>(aznumeric_cast<uint32_t>(deltaTime.GetMilliSeconds()) / 2, 16);
                        CrySleep(sleepTime);
                    }

                    workingStart = gEnv->pTimer->GetAsyncTime();

                    {
                        CryAutoLock<CryMutex> lock(m_lockRenderLoading);
                        gRenDev->m_DevBufMan.Update(frameId, true);
                    }

                    if (m_pLoadtimeCallback)
                    {
                        CryAutoLock<CryMutex> lock(m_lockRenderLoading);
                        m_pLoadtimeCallback->LoadtimeUpdate(deltaTimeInSeconds);
                    }

                    {
                        ////////////////////////////////////////////////
                        // wait till all SRendItems for this frame have finished preparing
                        const threadID processThreadID = gRenDev->m_RP.m_nProcessThreadID;
                        gRenDev->GetFinalizeRendItemJobExecutor(processThreadID)->WaitForCompletion();
                        gRenDev->GetFinalizeShadowRendItemJobExecutor(processThreadID)->WaitForCompletion();

                        {
                            CryAutoLock<CryMutex> lock(m_lockRenderLoading);

                            gRenDev->SetViewport(0, 0, gRenDev->GetOverlayWidth(), gRenDev->GetOverlayHeight());

                            SPostEffectsUtils::AcquireFinalCompositeTarget(false);

                            if (m_pLoadtimeCallback)
                            {
                                m_pLoadtimeCallback->LoadtimeRender();
                            }

                            gRenDev->m_DevBufMan.ReleaseEmptyBanks(frameId);

                            workingEnd = gEnv->pTimer->GetAsyncTime();

                            gRenDev->RT_PresentFast();

                            CRenderMesh::Tick();
                            CTexture::RT_LoadingUpdate();
                        }
                    }

                    // Make sure we aren't running with thousands of FPS with VSync disabled
                    gRenDev->LimitFramerate(120, true);

#if defined(SUPPORT_DEVICE_INFO_MSG_PROCESSING)
                    gcpRendD3D->DevInfo().ProcessSystemEventQueue();
#endif
                }
            }
            if (m_pThreadLoading)
            {
                QuitRenderLoadingThread();
            }
            m_eVideoThreadMode = eVTM_Disabled;

            if (m_bBeginFrameCalled)
            {
                m_bBeginFrameCalled = false;
                gRenDev->RT_BeginFrame();
            }
            if (m_bEndFrameCalled)
            {
                m_bEndFrameCalled = false;
                gRenDev->RT_EndFrame();
            }
            gcpRendD3D->BindContextToThread(nDeviceOwningThreadID);
        }
#endif
        const uint64 elapsed = CryGetTicks() - start;
        gEnv->pSystem->GetCurrentUpdateTimeStats().RenderTime = elapsed;
    }
#if defined(OPENGL) && !DXGL_FULL_EMULATION && !defined(CRY_USE_METAL)
    m_kDXGLDeviceContextHandle.Set(NULL, !CRenderer::CV_r_multithreaded);
    m_kDXGLContextHandle.Set(NULL);
#endif //defined(OPENGL) && !DXGL_FULL_EMULATION
}

void SRenderThread::ProcessLoading()
{
    AZ_TRACE_METHOD();
    while (true)
    {
        float fTime = iTimer->GetAsyncCurTime();
        WaitFlushCond();
        if (m_bQuitLoading)
        {
            SignalFlushFinishedCond();
            break;//put it here to safely shut down
        }
        float fTimeAfterWait = iTimer->GetAsyncCurTime();
        gRenDev->m_fTimeWaitForMain[m_nCurThreadProcess] += fTimeAfterWait - fTime;
        if (gRenDev->m_bStartLevelLoading)
        {
            m_fTimeIdleDuringLoading += fTimeAfterWait - fTime;
        }

        ProcessCommands(true /*loadTimeProcessing*/);

        SignalFlushFinishedCond();
        float fTimeAfterProcess = iTimer->GetAsyncCurTime();
        gRenDev->m_fTimeProcessedRT[m_nCurThreadProcess] += fTimeAfterProcess - fTimeAfterWait;
        if (gRenDev->m_bStartLevelLoading)
        {
            m_fTimeBusyDuringLoading += fTimeAfterProcess - fTimeAfterWait;
        }
        if (m_eVideoThreadMode == eVTM_RequestStop)
        {
            // Switch to general render thread
            SwitchMode(false);
        }
    }
#if defined(OPENGL) && !DXGL_FULL_EMULATION && !defined(CRY_USE_METAL)
    m_kDXGLDeviceContextHandle.Set(NULL, !CRenderer::CV_r_multithreaded);
    m_kDXGLContextHandle.Set(NULL);
#endif //defined(OPENGL) && !DXGL_FULL_EMULATION
}

#ifndef STRIP_RENDER_THREAD
// Flush current frame and wait for result (main thread only)
void SRenderThread::FlushAndWait()
{
    AZ_TRACE_METHOD();
    if (IsRenderThread())
    {
        return;
    }

    FUNCTION_PROFILER_LEGACYONLY(GetISystem(), PROFILE_RENDERER);

    if (!m_pThread)
    {
        return;
    }

    SyncMainWithRender();
    SyncMainWithRender();
}
#endif//STRIP_RENDER_THREAD

// Flush current frame without waiting (should be called from main thread)
void SRenderThread::SyncMainWithRender()
{
    AZ_PROFILE_FUNCTION_STALL(AZ::Debug::ProfileCategory::Renderer);
    FUNCTION_PROFILER_LEGACYONLY(GetISystem(), PROFILE_RENDERER);

#if defined(USE_HANDLE_FOR_FINAL_FLUSH_SYNC)
    if (m_bQuit && m_pThread && !m_pThread->IsRunning())
    {
        // We're in shutdown and the render thread is not running.
        // We should not attempt to wait for the render thread to signal us.
        return;
    }
#endif // defined(USE_HANDLE_FOR_FINAL_FLUSH_SYNC)

    if (!IsMultithreaded())
    {
        gRenDev->SyncMainWithRender();
        gRenDev->m_fTimeProcessedRT[m_nCurThreadProcess] = 0;
        gRenDev->m_fTimeWaitForMain[m_nCurThreadProcess] = 0;
        gRenDev->m_fTimeWaitForGPU[m_nCurThreadProcess] = 0;
        return;
    }

#ifndef STRIP_RENDER_THREAD
    CTimeValue time = iTimer->GetAsyncTime();
    WaitFlushFinishedCond();

    CPostEffectsMgr* pPostEffectMgr = PostEffectMgr();
    if (pPostEffectMgr)
    {
        // Must be called before the thread ID's get swapped
        pPostEffectMgr->SyncMainWithRender();
    }

    gRenDev->SyncMainWithRender();

    gRenDev->m_fTimeWaitForRender[m_nCurThreadFill] = iTimer->GetAsyncTime().GetDifferenceInSeconds(time);
    //  gRenDev->ToggleMainThreadAuxGeomCB();
    gRenDev->m_RP.m_TI[m_nCurThreadProcess].m_nFrameUpdateID = gRenDev->m_RP.m_TI[m_nCurThreadFill].m_nFrameUpdateID;
    gRenDev->m_RP.m_TI[m_nCurThreadProcess].m_nFrameID = gRenDev->m_RP.m_TI[m_nCurThreadFill].m_nFrameID;
    m_nCurThreadProcess = m_nCurThreadFill;
    m_nCurThreadFill = (m_nCurThreadProcess + 1) & 1;
    gRenDev->m_RP.m_nProcessThreadID = threadID(m_nCurThreadProcess);
    gRenDev->m_RP.m_nFillThreadID = threadID(m_nCurThreadFill);
    m_Commands[m_nCurThreadFill].SetUse(0);
    gRenDev->m_fTimeProcessedRT[m_nCurThreadProcess] = 0;
    gRenDev->m_fTimeWaitForMain[m_nCurThreadProcess] = 0;
    gRenDev->m_fTimeWaitForGPU[m_nCurThreadProcess] = 0;

    gRenDev->m_RP.m_pCurrentRenderView = gRenDev->m_RP.m_pRenderViews[gRenDev->m_RP.m_nProcessThreadID].get();
    gRenDev->m_RP.m_pCurrentFillView = gRenDev->m_RP.m_pRenderViews[gRenDev->m_RP.m_nFillThreadID].get();
    gRenDev->m_RP.m_pCurrentRenderView->PrepareForRendering();

    SignalFlushCond();
#endif
}

void SRenderThread::QuitRenderThread()
{
    AZ_TRACE_METHOD();
    if (IsMultithreaded() && m_pThread)
    {
        SignalQuitCond();
#if defined(USE_LOCKS_FOR_FLUSH_SYNC)
        FlushAndWait();
#endif
        m_pThread->WaitForThread();
        SAFE_DELETE(m_pThread);

#if !defined(STRIP_RENDER_THREAD)
        m_nCurThreadProcess = m_nCurThreadFill;
#endif
    }
    m_bQuit = 1;
}

void SRenderThread::QuitRenderLoadingThread()
{
    AZ_TRACE_METHOD();
    if (IsMultithreaded() && m_pThreadLoading)
    {
        FlushAndWait();
        m_bQuitLoading = true;
        m_pThreadLoading->WaitForThread();
        SAFE_DELETE(m_pThreadLoading);
        m_nRenderThreadLoading = 0;
        CNameTableR::m_nRenderThread = m_nRenderThread;
    }
}

bool SRenderThread::IsFailed()
{
    AZ_TRACE_METHOD();
    return !m_bSuccessful;
}

bool CRenderer::FlushRTCommands(bool bWait, bool bImmediatelly, bool bForce)
{
    AZ_TRACE_METHOD();
    SRenderThread* pRT = m_pRT;
    IF (!pRT, 0)
    {
        return true;
    }
    if (pRT->IsRenderThread(true))
    {
        SSystemGlobalEnvironment* pEnv = iSystem->GetGlobalEnvironment();
        if (pEnv && pEnv->IsEditor())
        {
            CPostEffectsMgr* pPostEffectMgr = PostEffectMgr();
            if (pPostEffectMgr)
            {
                pPostEffectMgr->SyncMainWithRender();
            }
        }
        return true;
    }
    if (!bForce && (!m_bStartLevelLoading || !pRT->IsMultithreaded()))
    {
        return false;
    }
    if (!bImmediatelly && pRT->CheckFlushCond())
    {
        return false;
    }
    if (bWait)
    {
        pRT->FlushAndWait();
    }

    return true;
}

bool CRenderer::ForceFlushRTCommands()
{
    AZ_TRACE_METHOD();
    LOADING_TIME_PROFILE_SECTION;
    return FlushRTCommands(true, true, true);
}

// Must be executed from main thread
void SRenderThread::WaitFlushFinishedCond()
{
    AZ_TRACE_METHOD();
    CTimeValue time = iTimer->GetAsyncTime();

#ifdef USE_LOCKS_FOR_FLUSH_SYNC
    m_LockFlushNotify.Lock();
    while (*(volatile int*)&m_nFlush)
    {
#if defined(USE_HANDLE_FOR_FINAL_FLUSH_SYNC)
        m_LockFlushNotify.Unlock();
        MsgWaitForMultipleObjects(1, &m_FlushFinishedCondition, FALSE, 1, QS_ALLINPUT);
        m_LockFlushNotify.Lock();
        AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::PumpSystemEventLoopUntilEmpty);
        if (m_bQuit && m_pThread && !m_pThread->IsRunning())
        {
            // We're in shutdown and the render thread is not running.
            // We should not attempt to wait for the render thread to signal us -
            // we force signal the flush condition to exit out of this wait loop.
            m_nFlush = 0;
        }
#else
        const int OneHunderdMilliseconds = 100;
        bool timedOut = !(m_FlushFinishedCondition.TimedWait(m_LockFlushNotify, OneHunderdMilliseconds));
#if defined(AZ_PLATFORM_IOS) && !defined(_RELEASE)
        // When we trigger asserts or warnings from a thread other than the main thread, the dialog box has to be
        // presented from the main thread. So, we need to pump the system event loop while the main thread is waiting.
        // We're using locks for waiting on iOS. This means that once the main thread goes into the wait, it's not
        // going to be able to pump system events. To handle this, we use a timed wait with 100ms. In most cases,
        // the render thread will complete within 100ms. But, when we need to display a dialog from the render thread,
        // it times out and pumps the system event loop so we can display the dialog. After that, since m_nFlush is still
        // true, we will go back into the wait and let the render thread complete.
        if (timedOut)
        {
            AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::PumpSystemEventLoopUntilEmpty);
        }
#endif
#endif
    }
    m_LockFlushNotify.Unlock();
#else
    READ_WRITE_BARRIER
    while (*(volatile int*)&m_nFlush)
    {
#ifdef WIN32
        AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::PumpSystemEventLoopUntilEmpty);
        Sleep(0);
#elif defined(AZ_PLATFORM_MAC) && !defined(_RELEASE)
        // On MacOS, we display blocking alerts(dialogs) to provide notifications to users(eg: assert failed).
        // These alerts(NSAlert) can be triggered only from the main thread. If we run into an assert on the render thread,
        // this block of code ensures that the alert is displayed on the main thread and we're not deadlocked with render thread.
        if (!gEnv->IsEditor())
        {
            AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::PumpSystemEventLoopUntilEmpty);
        }
#endif
        READ_WRITE_BARRIER
    }
#endif
}

// Must be executed from render thread
void SRenderThread::WaitFlushCond()
{
    AZ_TRACE_METHOD();
    AZ_PROFILE_FUNCTION_STALL(AZ::Debug::ProfileCategory::Renderer);
    FUNCTION_PROFILER_LEGACYONLY(GetISystem(), PROFILE_RENDERER);

    CTimeValue time = iTimer->GetAsyncTime();
#ifdef USE_LOCKS_FOR_FLUSH_SYNC
    m_LockFlushNotify.Lock();
    while (!*(volatile int*)&m_nFlush)
    {
        m_FlushCondition.Wait(m_LockFlushNotify);
    }
    m_LockFlushNotify.Unlock();
#else
    READ_WRITE_BARRIER
    while (!*(volatile int*)&m_nFlush)
    {
        if (m_bQuit)
        {
            break;
        }
        ::Sleep(0);
        READ_WRITE_BARRIER
    }
#endif
}

#undef m_nCurThreadFill
#undef m_nCurThreadProcess

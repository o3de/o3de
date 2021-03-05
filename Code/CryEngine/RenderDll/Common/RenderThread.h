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

// Description : Render thread commands processing.


#pragma once

#include <AzCore/std/parallel/mutex.h>
#include "UnalignedBlit.h"

// Remove this include once the restricted platform separation process is complete
#include "RendererDefs.h"


#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define RENDERTHREAD_H_SECTION_1 1
#define RENDERTHREAD_H_SECTION_2 2
#define RENDERTHREAD_H_SECTION_3 3
#endif

#if defined(ANDROID)
#include <sched.h>
#include <unistd.h>
#endif

#define RENDER_THREAD_NAME "RenderThread"
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERTHREAD_H_SECTION_1
    #include AZ_RESTRICTED_FILE(RenderThread_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    #define RENDER_THREAD_PRIORITY THREAD_PRIORITY_NORMAL
#endif

#define RENDER_LOADING_THREAD_NAME "RenderLoadingThread"

typedef void (* RenderFunc)(void);

//====================================================================

struct IRenderAuxGeomImpl;
struct SAuxGeomCBRawDataPackaged;
struct IColorGradingControllerInt;
struct SColorChartLayer;
class CRenderMesh;
struct SDynTexture;
struct STexStreamInState;
struct SDepthTexture;

#if RENDERTHREAD_H_TRAIT_USE_LOCKS_FOR_FLUSH_SYNC
#define USE_LOCKS_FOR_FLUSH_SYNC
#elif defined(WIN32)
#define USE_LOCKS_FOR_FLUSH_SYNC
#define USE_HANDLE_FOR_FINAL_FLUSH_SYNC
#endif

enum ERenderCommand
{
    eRC_Unknown = 0,
    eRC_Init,
    eRC_ShutDown,
    eRC_CreateDevice,
    eRC_ResetDevice,
    eRC_SuspendDevice,
    eRC_ResumeDevice,
    eRC_BeginFrame,
    eRC_EndFrame,
    eRC_ClearTargetsImmediately,
    eRC_RenderTextMessages,
    eRC_FlushTextureStreaming,
    eRC_ReleaseSystemTextures,
    eRC_PreloadTextures,
    eRC_ReadFrameBuffer,
    eRC_ForceSwapBuffers,
    eRC_SwitchToNativeResolutionBackbuffer,

    eRC_DrawLines,
    eRC_DrawStringU,
    eRC_UpdateTexture,
    eRC_UpdateMesh2,
    eRC_ReleaseBaseResource,
    eRC_ReleaseFont,
    eRC_ReleaseSurfaceResource,
    eRC_ReleaseIB,
    eRC_ReleaseVB,
    eRC_ReleaseVBStream,
    eRC_CreateResource,
    eRC_ReleaseResource,
    eRC_ReleaseRenderResources,
    eRC_PrecacheDefaultShaders,
    eRC_UnbindTMUs,
    eRC_UnbindResources,
    eRC_CreateRenderResources,
    eRC_CreateSystemTargets,
    eRC_CreateDeviceTexture,
    eRC_CopyDataToTexture,
    eRC_ClearTarget,
    eRC_CreateREPostProcess,
    eRC_ParseShader,
    eRC_SetShaderQuality,
    eRC_UpdateShaderItem,
    eRC_RefreshShaderResourceConstants,
    eRC_ReleaseDeviceTexture,
    eRC_FlashRender,
    eRC_FlashRenderLockless,
    eRC_AuxFlush,
    eRC_RenderScene,
    eRC_PrepareStereo,
    eRC_CopyToStereoTex,
    eRC_SetStereoEye,
    eRC_SetCamera,

    eRC_PushProfileMarker,
    eRC_PopProfileMarker,

    eRC_PostLevelLoading,
    eRC_SetState,
    eRC_PushWireframeMode,
    eRC_PopWireframeMode,
    eRC_SetCull,
    eRC_SetScissor,
    eRC_SetStencilState,
    eRC_SelectGPU,
    eRC_DrawDynVB,
    eRC_DrawDynVBUI,
    eRC_Draw2dImage,
    eRC_Draw2dImageStretchMode,
    eRC_Push2dImage,
    eRC_PushUITexture,
    eRC_Draw2dImageList,
    eRC_DrawImageWithUV,

    eRC_PreprGenerateFarTrees,
    eRC_DynTexUpdate,
    eRC_PushFog,
    eRC_PopFog,
    eRC_PushVP,
    eRC_PopVP,
    eRC_SetEnvTexRT,
    eRC_SetEnvTexMatrix,
    eRC_PushRT,
    eRC_PopRT,
    eRC_SetViewport,
    eRC_TexBlurAnisotropicVertical,

    eRC_OC_ReadResult_Try,

    eRC_CGCSetLayers,
    eRC_EntityDelete,
    eRC_ForceMeshGC,
    eRC_DevBufferSync,

    // streaming queries
    eRC_PrecacheTexture,
    eRC_SetTexture,

    eRC_StartVideoThread,
    eRC_StopVideoThread,

    eRC_RenderDebug,
    eRC_PreactivateShaders,
    eRC_PrecacheShader,

    eRC_RelinkTexture,
    eRC_UnlinkTexture,

    eRC_ReleasePostEffects,
    eRC_ResetPostEffects,
    eRC_ResetPostEffectsOnSpecChange,
    eRC_DisableTemporalEffects,

    eRC_ResetGlass,
    eRC_ResetToDefault,

    eRC_GenerateSkyDomeTextures,

    eRC_PushSkinningPoolId,

    eRC_ReleaseRemappedBoneIndices,

    eRC_SetRendererCVar,
    eRC_SetColorOp,
    eRC_SetSrgbWrite,

    eRC_InitializeVideoRenderer,
    eRC_CleanupVideoRenderer,
    eRC_DrawVideoRenderer,

    eRC_AzFunction
};

//====================================================================

struct CRenderThread
    : CrySimpleThread<>
{
    int m_nCPU;
    CRenderThread(int nCPU)
    {
        m_nCPU = CLAMP(nCPU, 1, 5);
    }

    virtual ~CRenderThread()
    {
        // Stop thread task.
        Stop();
    }

    CryEvent    m_started;

protected:
    virtual void Run();
};
struct CRenderThreadLoading
    : public CRenderThread
{
    CRenderThreadLoading(int nCPU)
        : CRenderThread(nCPU) {}
protected:
    virtual void Run();
};

struct SRenderThread
{
    CRenderThread* m_pThread;
    CRenderThreadLoading* m_pThreadLoading;
    ILoadtimeCallback* m_pLoadtimeCallback;
    CryMutex m_lockRenderLoading;
    AZStd::mutex m_CommandsMutex;
    bool m_bQuit;
    bool m_bQuitLoading;
    bool m_bSuccessful;
    bool m_bBeginFrameCalled;
    bool m_bEndFrameCalled;
#ifndef STRIP_RENDER_THREAD
    int m_nCurThreadProcess;
    int m_nCurThreadFill;
#endif
#ifdef USE_LOCKS_FOR_FLUSH_SYNC
    int m_nFlush;
    CryMutex                            m_LockFlushNotify;
    CryConditionVariable    m_FlushCondition;
#if defined(USE_HANDLE_FOR_FINAL_FLUSH_SYNC)
    HANDLE m_FlushFinishedCondition;
#else
    CryConditionVariable    m_FlushFinishedCondition;
#endif
#else
    volatile int m_nFlush;
#endif
    threadID m_nRenderThread;
    threadID m_nRenderThreadLoading;
    threadID m_nMainThread;
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERTHREAD_H_SECTION_2
    #include AZ_RESTRICTED_FILE(RenderThread_h)
#endif
    HRESULT m_hResult;
#if defined(OPENGL) && !DXGL_FULL_EMULATION && !defined(CRY_USE_METAL)
    SDXGLContextThreadLocalHandle m_kDXGLContextHandle;
    SDXGLDeviceContextThreadLocalHandle m_kDXGLDeviceContextHandle;
#endif //defined(OPENGL) && !DXGL_FULL_EMULATION
    float m_fTimeIdleDuringLoading;
    float m_fTimeBusyDuringLoading;
    TArray<byte> m_Commands[RT_COMMAND_BUF_COUNT]; // m_nCurThreadFill shows which commands are filled by main thread

    // The below loading queue contains all commands that were submitted and require full device access during loading.
    // Will be blit into the first render frame's command queue after loading and subsequently resized to 0.
    TArray<byte> m_CommandsLoading;

    static CryCriticalSection s_rcLock;

    enum EVideoThreadMode
    {
        eVTM_Disabled = 0,
        eVTM_RequestStart,
        eVTM_Active,
        eVTM_RequestStop,
        eVTM_ProcessingStop,
    };
    EVideoThreadMode m_eVideoThreadMode;

    SRenderThread();
    ~SRenderThread();

    inline void SignalFlushFinishedCond()
    {
#ifdef USE_LOCKS_FOR_FLUSH_SYNC
        m_LockFlushNotify.Lock();
#endif
        m_nFlush = 0;
#ifdef USE_LOCKS_FOR_FLUSH_SYNC
#if defined(USE_HANDLE_FOR_FINAL_FLUSH_SYNC)
        SetEvent(m_FlushFinishedCondition);
#else
        m_FlushFinishedCondition.Notify();
#endif
        m_LockFlushNotify.Unlock();
#else
        READ_WRITE_BARRIER
#endif
    }

    inline void SignalFlushCond()
    {
#ifdef USE_LOCKS_FOR_FLUSH_SYNC
        m_LockFlushNotify.Lock();
#endif
        m_nFlush = 1;
#ifdef USE_LOCKS_FOR_FLUSH_SYNC
        m_FlushCondition.Notify();
        m_LockFlushNotify.Unlock();
#else
        READ_WRITE_BARRIER
#endif
    }

    inline void SignalQuitCond()
    {
#ifdef USE_LOCKS_FOR_FLUSH_SYNC
        m_LockFlushNotify.Lock();
#endif
        m_bQuit = 1;
#ifdef USE_LOCKS_FOR_FLUSH_SYNC
        m_FlushCondition.Notify();
        m_LockFlushNotify.Unlock();
#else
        READ_WRITE_BARRIER
#endif
    }

    void WaitFlushCond();

#ifdef WIN32
    HWND GetRenderWindowHandle();
#endif

    void WaitFlushFinishedCond();

    inline void InitFlushCond()
    {
        m_nFlush = 0;
#if !defined(USE_LOCKS_FOR_FLUSH_SYNC)
        READ_WRITE_BARRIER
#endif
    }

    inline bool CheckFlushCond()
    {
#if !defined(USE_LOCKS_FOR_FLUSH_SYNC)
        READ_WRITE_BARRIER
#endif
        return *(int*)&m_nFlush != 0;
    }

    static constexpr size_t RenderThreadStackSize = 128ull * 1024ull;

    void StartRenderThread()
    {
        if (m_pThread != NULL)
        {
            int32 renderThreadPriority = RENDER_THREAD_PRIORITY;
#if defined(AZ_PLATFORM_IOS) || defined(AZ_PLATFORM_MAC)
            //Apple recommends to never use 0 as a render thread priority.
            //In this case we are getting the max thread priority and going 2 levels below for ideal performance.
            int thread_policy;
            sched_param thread_sched_param;
            pthread_getschedparam(pthread_self(), &thread_policy, &thread_sched_param);
            renderThreadPriority = sched_get_priority_max(thread_policy) - 2;
#endif
            
            m_pThread->Start(AFFINITY_MASK_RENDERTHREAD, RENDER_THREAD_NAME, renderThreadPriority, RenderThreadStackSize);
            m_pThread->m_started.Wait();
        }
    }

    void StartRenderLoadingThread()
    {
        if (m_pThreadLoading != NULL)
        {
            m_pThreadLoading->Start(AFFINITY_MASK_USERTHREADS, RENDER_THREAD_NAME, RENDER_THREAD_PRIORITY + 1, RenderThreadStackSize);
            m_pThreadLoading->m_started.Wait();
        }
    }

    bool IsFailed();
    void ValidateThreadAccess(ERenderCommand eRC);

    _inline size_t Align4(size_t value)
    {
        return (value + 3) & ~((size_t)3);
    }

    _inline byte* AddCommandTo(ERenderCommand eRC, size_t nParamBytes, TArray<byte>& queue)
    {
        AZ_Assert(nParamBytes == Align4(nParamBytes), "Input nParamBytes is %" PRISIZE_T " bytes, which not aligned to 4 bytes.", nParamBytes);

        m_CommandsMutex.lock();

        assert(m_pThread != NULL);
        uint32 cmdSize = sizeof(uint32) + nParamBytes;
#if !defined(_RELEASE)
        cmdSize += sizeof(uint32);
#endif
        byte* ptr = queue.Grow(cmdSize);
        AddDWORD(ptr, eRC);
#if !defined(_RELEASE)
        // Processed flag
        AddDWORD(ptr, 0);
#endif
        return ptr;
    }

    _inline void EndCommandTo([[maybe_unused]] byte* ptr, [[maybe_unused]] TArray<byte>& queue)
    {
#ifndef _RELEASE
        if (ptr - queue.Data() != queue.Num())
        {
            CryFatalError("Bad render command size - check the parameters and round each up to 4-byte boundaries [expected queue size = %" PRISIZE_T ", actual size = %u]", (size_t)(ptr - queue.Data()), queue.Num());
        }
#endif
        m_CommandsMutex.unlock();
    }

    _inline byte* AddCommand(ERenderCommand eRC, size_t nParamBytes)
    {
        AZ_Assert(nParamBytes == Align4(nParamBytes), "Input nParamBytes is %" PRISIZE_T " bytes, which not aligned to 4 bytes.", nParamBytes);

#ifdef STRIP_RENDER_THREAD
        return NULL;
#else
        return AddCommandTo(eRC, nParamBytes, m_Commands[m_nCurThreadFill]);
#endif
    }

    _inline void EndCommand(byte* ptr)
    {
#ifndef STRIP_RENDER_THREAD
        EndCommandTo(ptr, m_Commands[m_nCurThreadFill]);
#endif
    }

    _inline void AddDWORD(byte*& ptr, uint32 nVal)
    {
        *(uint32*)ptr = nVal;
        ptr += sizeof(uint32);
    }
    _inline void AddDWORD64(byte*& ptr, uint64 nVal)
    {
        StoreUnaligned((uint32*)ptr, nVal); // uint32 because command list maintains 4-byte alignment
        ptr += sizeof(uint64);
    }
    _inline void AddTI(byte*& ptr, SThreadInfo& TI)
    {
        memcpy((SThreadInfo*)ptr, &TI, sizeof(SThreadInfo));
        ptr += sizeof(SThreadInfo);
    }
    _inline void AddRenderingPassInfo(byte*& ptr, SRenderingPassInfo* pPassInfo)
    {
        memcpy((SRenderingPassInfo*)ptr, pPassInfo, sizeof(SRenderingPassInfo));
        ptr += sizeof(SRenderingPassInfo);
    }
    _inline void AddFloat(byte*& ptr, const float fVal)
    {
        *(float*)ptr = fVal;
        ptr += sizeof(float);
    }
    _inline void AddVec3(byte*& ptr, const Vec3& cVal)
    {
        *(Vec3*)ptr = cVal;
        ptr += sizeof(Vec3);
    }
    _inline void AddColor(byte*& ptr, const ColorF& cVal)
    {
        float* fData = (float*)ptr;
        fData[0] = cVal[0];
        fData[1] = cVal[1];
        fData[2] = cVal[2];
        fData[3] = cVal[3];
        ptr += sizeof(ColorF);
    }
    _inline void AddColorB(byte*& ptr, const ColorB& cVal)
    {
        ptr[0] = cVal[0];
        ptr[1] = cVal[1];
        ptr[2] = cVal[2];
        ptr[3] = cVal[3];
        ptr += sizeof(ColorB);
    }
    _inline void AddPointer(byte*& ptr, const void* pVal)
    {
        StoreUnaligned((uint32*)ptr, pVal); // uint32 because command list maintains 4-byte alignment
        ptr += sizeof(void*);
    }
    _inline void AddData(byte*& ptr, const void* pData, int nLen)
    {
        unsigned pad = (unsigned)-nLen % 4;
        AddDWORD(ptr, nLen + pad);
        memcpy(ptr, pData, nLen);
        ptr += nLen + pad;
    }
    _inline void AddText(byte*& ptr, const char* pText)
    {
        int nLen = strlen(pText) + 1;
        unsigned pad = (unsigned)-nLen % 4;
        AddDWORD(ptr, nLen);
        memcpy(ptr, pText, nLen);
        ptr += nLen + pad;
    }
    _inline size_t TextCommandSize(const char* pText)
    {
        return 4 + Align4(strlen(pText) + 1);
    }
    _inline void AddText(byte*& ptr, const wchar_t* pText)
    {
        int nLen = (wcslen(pText) + 1) * sizeof(wchar_t);
        unsigned pad = (unsigned)-nLen % 4;
        AddDWORD(ptr, nLen);
        memcpy(ptr, pText, nLen);
        ptr += nLen + pad;
    }
    _inline size_t TextCommandSize(const wchar_t* pText)
    {
        return 4 + Align4((wcslen(pText) + 1) * sizeof(wchar_t));
    }
    template<class T>
    T ReadCommand(int& nIndex)
    {
        T Res;
        LoadUnaligned((uint32*)&m_Commands[m_nCurThreadProcess][nIndex], Res);
        nIndex += (sizeof(T) + 3) & ~((size_t)3);
        return Res;
    }
    template<class T>
    T ReadTextCommand(int& nIndex)
    {
        unsigned int strLen = ReadCommand<unsigned int>(nIndex);
        T Res = (T)&m_Commands[m_nCurThreadProcess][nIndex];
        nIndex += strLen;
        nIndex = (nIndex + 3) & ~((size_t)3);
        return Res;
    }

    _inline threadID GetCurrentThreadId(bool bAlwaysCheck = false)
    {
#ifdef STRIP_RENDER_THREAD
        return m_nRenderThread;
#else
        if (!bAlwaysCheck && m_nRenderThread == m_nMainThread)
        {
            return m_nRenderThread;
        }

        return ::GetCurrentThreadId();
#endif
    }
    void SwitchMode(bool enableVideo);

    void Init(int nCPU);
    void QuitRenderThread();
    void QuitRenderLoadingThread();
    void SyncMainWithRender();
    void FlushAndWait();
    void ProcessCommands(bool loadTimeProcessing);
    void Process();         // Render thread
    void ProcessLoading();  // Loading thread
    int  GetThreadList();
#if defined(AZ_PLATFORM_ANDROID)
    bool IsRenderThread(bool bAlwaysCheck = true);
    bool IsRenderLoadingThread(bool bAlwaysCheck = true);
#else
    bool IsRenderThread(bool bAlwaysCheck = false);
    bool IsRenderLoadingThread(bool bAlwaysCheck = false);
#endif
    bool IsMainThread(bool bAlwaysCheck = false);
    bool IsMultithreaded();
    int  CurThreadFill() const;
    void    RC_Init();
    void    RC_ShutDown(uint32 nFlags);
    bool    RC_CreateDevice();
    void    RC_ResetDevice();
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERTHREAD_H_SECTION_3
    #include AZ_RESTRICTED_FILE(RenderThread_h)
#endif
    void    RC_PreloadTextures();
    void    RC_ReadFrameBuffer(unsigned char* pRGB, int nImageX, int nSizeX, int nSizeY, ERB_Type eRBType, bool bRGBA, int nScaledX, int nScaledY);
    bool    RC_CreateDeviceTexture(CTexture* pTex, const byte* pData[6]);
    void    RC_CopyDataToTexture(void* pkTexture, unsigned int uiStartMip, unsigned int uiEndMip);
    void    RC_ClearTarget(void* pkTexture, const ColorF& kColor);
    void    RC_CreateResource(SResourceAsync* pRes);
    void    RC_ReleaseRenderResources();
    void        RC_PrecacheDefaultShaders();
    void    RC_UnbindResources();
    void    RC_UnbindTMUs();
    void    RC_FreeObject(CRenderObject* pObj);
    void    RC_CreateRenderResources();
    void    RC_CreateSystemTargets();
    void    RC_ReleaseBaseResource(CBaseResource* pRes);
    void    RC_ReleaseFont(IFFont* font);
    void    RC_ReleaseSurfaceResource(SDepthTexture* pRes);
    void    RC_ReleaseResource(SResourceAsync* pRes);
    void    RC_ReleaseResource(AZStd::unique_ptr<SResourceAsync> pRes);
    void        RC_RelinkTexture(CTexture* pTex);
    void        RC_UnlinkTexture(CTexture* pTex);
    void    RC_CreateREPostProcess(CRendElementBase** re);
    bool    RC_CheckUpdate2(CRenderMesh* pMesh, CRenderMesh* pVContainer, uint32 nStreamMask);
    void    RC_ReleaseVB(buffer_handle_t nID);
    void    RC_ReleaseIB(buffer_handle_t nID);
    void    RC_DrawDynVB(SVF_P3F_C4B_T2F* pBuf, uint16* pInds, int nVerts, int nInds, const PublicRenderPrimitiveType nPrimType);
    void    RC_DrawDynUiPrimitiveList(IRenderer::DynUiPrimitiveList& primitives, int totalNumVertices, int totalNumIndices);
    void    RC_Draw2dImage(float xpos, float ypos, float w, float h, CTexture* pTexture, float s0, float t0, float s1, float t1, float angle, float r, float g, float b, float a, float z);
    void    RC_Draw2dImageStretchMode(bool bStretch);
    void    RC_Push2dImage(float xpos, float ypos, float w, float h, CTexture* pTexture, float s0, float t0, float s1, float t1, float angle, float r, float g, float b, float a, float z, float stereoDepth);
    void    RC_PushUITexture(float xpos, float ypos, float w, float h, CTexture* pTexture, float s0, float t0, float s1, float t1, CTexture* pTarget, float r, float g, float b, float a);
    void    RC_Draw2dImageList();
    void    RC_DrawImageWithUV(float xpos, float ypos, float z, float w, float h, int texture_id, float s[4], float t[4], float r, float g, float b, float a, bool filtered = true);
    void        RC_UpdateTextureRegion(CTexture* pTex, const byte* data, int nX, int nY, int nZ, int USize, int VSize, int ZSize, ETEX_Format eTFSrc);
    void    RC_SetState(int State, int AlphaRef);
    void    RC_SetStencilState(int st, uint32 nStencRef, uint32 nStencMask, uint32 nStencWriteMask, bool bForceFullReadMask);
    void    RC_SetColorOp(byte eCo, byte eAo, byte eCa, byte eAa);
    void    RC_SetSrgbWrite(bool srgbWrite);
    void        RC_PushWireframeMode(int nMode);
    void        RC_PopWireframeMode();
    void    RC_SetCull(int nMode);
    void        RC_SetScissor(bool bEnable, int sX, int sY, int sWdt, int sHgt);
    void    RC_PreactivateShaders();
    void        RC_PrecacheShader(CShader* pShader, SShaderCombination& cmb, bool bForce, bool bCompressedOnly, CShaderResources* pRes);
    void        RC_PushProfileMarker(const char* label);
    void        RC_PopProfileMarker(const char* label);
    void    RC_DrawLines(Vec3 v[], int nump, ColorF& col, int flags, float fGround);
    void    RC_DrawStringU(IFFont_RenderProxy* pFont, float x, float y, float z, const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx);
    void    RC_ReleaseDeviceTexture(CTexture* pTex);
    void        RC_PrecacheResource(ITexture* pTP, float fMipFactor, float fTimeToReady, int Flags, int nUpdateId, int nCounter = 1);
    void RC_ClearTargetsImmediately(int8 nType, uint32 nFlags, const ColorF& vColor, float depth);
    void    RC_RenderTextMessages();
    void    RC_FlushTextureStreaming(bool bAbort);
    void    RC_ReleaseSystemTextures();
    void     RC_AuxFlush(IRenderAuxGeomImpl* pAux, SAuxGeomCBRawDataPackaged& data, size_t begin, size_t end, bool reset);
    void    RC_ParseShader (CShader* pSH, uint64 nMaskGen, uint32 flags, CShaderResources* pRes);
    void        RC_SetShaderQuality(EShaderType eST, EShaderQuality eSQ);
    void    RC_UpdateShaderItem(SShaderItem* pShaderItem, _smart_ptr<IMaterial> pMaterial);
    void    RC_RefreshShaderResourceConstants(SShaderItem* pShaderItem, IMaterial* pMaterial);
    void    RC_SetCamera();
    void    RC_RenderScene(int nFlags, RenderFunc pRenderFunc);
    void    RC_BeginFrame();
    void    RC_EndFrame(bool bWait);
    void    RC_TryFlush();
    void    RC_PrepareStereo(int mode, int output);
    void    RC_CopyToStereoTex(int channel);
    void    RC_SetStereoEye(int eye);
    bool    RC_DynTexUpdate(SDynTexture* pTex, int nNewWidth, int nNewHeight);
    void    RC_PushFog();
    void    RC_PopFog();
    void    RC_PushVP();
    void    RC_PopVP();
    void        RC_ForceSwapBuffers();
    void    RC_SwitchToNativeResolutionBackbuffer();

    void    RC_StartVideoThread();
    void    RC_StopVideoThread();
    void    RT_StartVideoThread();
    void    RT_StopVideoThread();

    void    RC_PostLoadLevel();
    void    RC_SetEnvTexRT(SEnvTexture* pEnvTex, int nWidth, int nHeight, bool bPush);
    void    RC_SetEnvTexMatrix(SEnvTexture* pEnvTex);
    void    RC_PushRT(int nTarget, CTexture* pTex, SDepthTexture* pDS, int nS);
    void    RC_PopRT(int nTarget);
    void    RC_TexBlurAnisotropicVertical(CTexture* Tex, float fAnisoScale);
    void    RC_EntityDelete(IRenderNode* pRenderNode);
    void    RC_SetTexture(int nTex, int nUnit);

    bool    RC_OC_ReadResult_Try(uint32 nDefaultNumSamples, CREOcclusionQuery* pRE);

    void    RC_SetViewport(int x, int y, int width, int height, int id = 0);

    void    RC_ReleaseVBStream(void* pVB, int nStream);
    void    RC_ForceMeshGC(bool instant = false, bool wait = false);
    void    RC_DevBufferSync();

    void    RC_ReleasePostEffects();
    void    RC_ResetPostEffects(bool bOnSpecChange = false);
    void        RC_ResetGlass();
    void        RC_DisableTemporalEffects();
    void    RC_ResetToDefault();

    void RC_CGCSetLayers(IColorGradingControllerInt* pController, const SColorChartLayer* pLayers, uint32 numLayers);

    void        RC_GenerateSkyDomeTextures(CREHDRSky* pSky, int32 width, int32 height);

    void        RC_SetRendererCVar(ICVar* pCVar, const char* pArgText, const bool bSilentMode = false);

    void        RC_RenderDebug(bool bRenderStats = true);
    void    RC_PushSkinningPoolId(uint32);
    void        RC_ReleaseRemappedBoneIndices(IRenderMesh* pRenderMesh, uint32 guid);

    void RC_InitializeVideoRenderer(AZ::VideoRenderer::IVideoRenderer* pVideoRenderer);
    void RC_CleanupVideoRenderer(AZ::VideoRenderer::IVideoRenderer* pVideoRenderer);
    void RC_DrawVideoRenderer(AZ::VideoRenderer::IVideoRenderer* pVideoRenderer, const AZ::VideoRenderer::DrawArguments& drawArguments);

    using RenderCommandCB = AZStd::function<void()>;
    void EnqueueRenderCommand(RenderCommandCB command);

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        for (uint32 i = 0; i < RT_COMMAND_BUF_COUNT; ++i)
        {
            pSizer->AddObject(m_Commands[i]);
        }
    }
} _ALIGN(128);//align to cache line

_inline int SRenderThread::GetThreadList()
{
#ifdef STRIP_RENDER_THREAD
    return m_nCurThreadFill;
#else
    if (IsRenderThread())
    {
        return m_nCurThreadProcess;
    }

    return m_nCurThreadFill;
#endif
}

_inline bool SRenderThread::IsRenderThread(bool bAlwaysCheck)
{
#ifdef STRIP_RENDER_THREAD
    return true;
#else
    threadID threadId = this->GetCurrentThreadId(bAlwaysCheck);

    return (threadId == m_nRenderThreadLoading || threadId == m_nRenderThread);
#endif
}

_inline bool SRenderThread::IsRenderLoadingThread(bool bAlwaysCheck)
{
#ifdef STRIP_RENDER_THREAD
    return false;
#else
    threadID threadId = this->GetCurrentThreadId(bAlwaysCheck);

    return threadId == m_nRenderThreadLoading;
#endif
}

_inline bool SRenderThread::IsMainThread(bool bAlwaysCheck)
{
#ifdef STRIP_RENDER_THREAD
    return false;
#else
    threadID threadId = this->GetCurrentThreadId(bAlwaysCheck);

    return threadId == m_nMainThread;
#endif
}

_inline bool SRenderThread::IsMultithreaded()
{
#ifdef STRIP_RENDER_THREAD
    return false;
#else
    return m_pThread != NULL;
#endif
}

_inline int SRenderThread::CurThreadFill() const
{
#ifdef STRIP_RENDER_THREAD
    return 0;
#else
    return m_nCurThreadFill;
#endif
}

#ifdef STRIP_RENDER_THREAD
_inline void SRenderThread::FlushAndWait()
{
    return;
}
#endif

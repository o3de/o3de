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

// Description : Implementation of the NULL renderer API


#include "RenderDll_precompiled.h"
#include "NULL_Renderer.h"
#include <IColorGradingController.h>
#include "IStereoRenderer.h"
#include "../Common/Textures/TextureManager.h"

#include <IEngineModule.h>
#include <CryExtension/Impl/ClassWeaver.h>
// init memory pool usage

#include "GraphicsPipeline/FurBendData.h"

#include <random>  //std::random_device

CCryNameTSCRC CTexture::s_sClassName = CCryNameTSCRC("CTexture");
CCryNameTSCRC CHWShader::s_sClassNameVS = CCryNameTSCRC("CHWShader_VS");
CCryNameTSCRC CHWShader::s_sClassNamePS = CCryNameTSCRC("CHWShader_PS");
CCryNameTSCRC CShader::s_sClassName = CCryNameTSCRC("CShader");

CNULLRenderer* gcpNULL = NULL;

//////////////////////////////////////////////////////////////////////

class CNullColorGradingController
    : public IColorGradingController
{
public:
    virtual int LoadColorChart([[maybe_unused]] const char* pChartFilePath) const { return 0; }
    virtual int LoadDefaultColorChart() const { return 0; }
    virtual void UnloadColorChart([[maybe_unused]] int texID) const {}
    virtual void SetLayers([[maybe_unused]] const SColorChartLayer* pLayers, [[maybe_unused]] uint32 numLayers) {}
};

//////////////////////////////////////////////////////////////////////

class CNullStereoRenderer
    : public IStereoRenderer
{
public:
    virtual EStereoDevice GetDevice() { return STEREO_DEVICE_NONE; }
    virtual EStereoDeviceState GetDeviceState() { return STEREO_DEVSTATE_UNSUPPORTED_DEVICE; }
    virtual void GetInfo(EStereoDevice* device, EStereoMode* mode, EStereoOutput* output, EStereoDeviceState* state) const
    {
        if (device)
        {
            *device = STEREO_DEVICE_NONE;
        }
        if (mode)
        {
            *mode = STEREO_MODE_NO_STEREO;
        }
        if (output)
        {
            *output = STEREO_OUTPUT_STANDARD;
        }
        if (state)
        {
            *state = STEREO_DEVSTATE_OK;
        }
    }
    virtual bool GetStereoEnabled() { return false; }
    virtual float GetStereoStrength() { return 0; }
    virtual float GetMaxSeparationScene([[maybe_unused]] bool half = true) { return 0; }
    virtual float GetZeroParallaxPlaneDist() { return 0; }
    virtual void GetNVControlValues([[maybe_unused]] bool& stereoEnabled, [[maybe_unused]] float& stereoStrength) {};
    virtual void OnHmdDeviceChanged() {}
    virtual bool IsRenderingToHMD() override { return false; }
    Status GetStatus() const override { return IStereoRenderer::Status::kIdle; }
};

//////////////////////////////////////////////////////////////////////
CNULLRenderer::CNULLRenderer()
{
    gcpNULL = this;
    m_pNULLRenderAuxGeom = CNULLRenderAuxGeom::Create(*this);
    m_pNULLColorGradingController = new CNullColorGradingController();
    m_pNULLStereoRenderer = new CNullStereoRenderer();
    m_pixelAspectRatio = 1.0f;
}

//////////////////////////////////////////////////////////////////////////
bool QueryIsFullscreen()
{
    return false;
}


#include <stdio.h>
//////////////////////////////////////////////////////////////////////
CNULLRenderer::~CNULLRenderer()
{
    ShutDown();
    delete m_pNULLRenderAuxGeom;
    delete m_pNULLColorGradingController;
    delete m_pNULLStereoRenderer;
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::EnableTMU([[maybe_unused]] bool enable)
{
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::CheckError([[maybe_unused]] const char* comment)
{
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::BeginFrame()
{
    m_RP.m_TI[m_RP.m_nFillThreadID].m_nFrameID++;
    m_RP.m_TI[m_RP.m_nFillThreadID].m_nFrameUpdateID++;
    m_RP.m_TI[m_RP.m_nFillThreadID].m_RealTime = iTimer->GetCurrTime();

    m_pNULLRenderAuxGeom->BeginFrame();
}

//////////////////////////////////////////////////////////////////////
bool CNULLRenderer::ChangeDisplay([[maybe_unused]] unsigned int width, [[maybe_unused]] unsigned int height, [[maybe_unused]] unsigned int bpp)
{
    return false;
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::ChangeViewport([[maybe_unused]] unsigned int x, [[maybe_unused]] unsigned int y, [[maybe_unused]] unsigned int width, [[maybe_unused]] unsigned int height, [[maybe_unused]] bool bMainViewport, [[maybe_unused]] float scaleWidth, [[maybe_unused]] float scaleHeight)
{
}

void CNULLRenderer::RenderDebug([[maybe_unused]] bool bRenderStats)
{
}

void CNULLRenderer::EndFrame()
{
    //m_pNULLRenderAuxGeom->Flush(true);
    m_pNULLRenderAuxGeom->EndFrame();
    m_pRT->RC_EndFrame(!m_bStartLevelLoading);
}

void CNULLRenderer::TryFlush()
{
}

void CNULLRenderer::GetMemoryUsage([[maybe_unused]] ICrySizer* Sizer)
{
}

WIN_HWND CNULLRenderer::GetHWND()
{
#if defined(WIN32)
    return GetDesktopWindow();
#else
    return NULL;
#endif
}

bool CNULLRenderer::SetWindowIcon([[maybe_unused]] const char* path)
{
    return false;
}

void TexBlurAnisotropicVertical([[maybe_unused]] CTexture* pTex, [[maybe_unused]] int nAmount, [[maybe_unused]] float fScale, [[maybe_unused]] float fDistribution, [[maybe_unused]] bool bAlphaOnly)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//IMAGES DRAWING
////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::Draw2dImage([[maybe_unused]] float xpos, [[maybe_unused]] float ypos, [[maybe_unused]] float w, [[maybe_unused]] float h, [[maybe_unused]] int texture_id, [[maybe_unused]] float s0, [[maybe_unused]] float t0, [[maybe_unused]] float s1, [[maybe_unused]] float t1, [[maybe_unused]] float angle, [[maybe_unused]] float r, [[maybe_unused]] float g, [[maybe_unused]] float b, [[maybe_unused]] float a, [[maybe_unused]] float z)
{
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::Push2dImage([[maybe_unused]] float xpos, [[maybe_unused]] float ypos, [[maybe_unused]] float w, [[maybe_unused]] float h, [[maybe_unused]] int texture_id, [[maybe_unused]] float s0, [[maybe_unused]] float t0, [[maybe_unused]] float s1, [[maybe_unused]] float t1, [[maybe_unused]] float angle, [[maybe_unused]] float r, [[maybe_unused]] float g, [[maybe_unused]] float b, [[maybe_unused]] float a, [[maybe_unused]] float z, [[maybe_unused]] float stereoDepth)
{
}

void CNULLRenderer::Draw2dImageList()
{
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::DrawImage([[maybe_unused]] float xpos, [[maybe_unused]] float ypos, [[maybe_unused]] float w, [[maybe_unused]] float h, [[maybe_unused]] int texture_id, [[maybe_unused]] float s0, [[maybe_unused]] float t0, [[maybe_unused]] float s1, [[maybe_unused]] float t1, [[maybe_unused]] float r, [[maybe_unused]] float g, [[maybe_unused]] float b, [[maybe_unused]] float a, [[maybe_unused]] bool filtered)
{
}

void CNULLRenderer::DrawImageWithUV([[maybe_unused]] float xpos, [[maybe_unused]] float ypos, [[maybe_unused]] float z, [[maybe_unused]] float w, [[maybe_unused]] float h, [[maybe_unused]] int texture_id, [[maybe_unused]] float s[4], [[maybe_unused]] float t[4], [[maybe_unused]] float r, [[maybe_unused]] float g, [[maybe_unused]] float b, [[maybe_unused]] float a, [[maybe_unused]] bool filtered)
{
}

///////////////////////////////////////////
void CNULLRenderer::DrawBuffer([[maybe_unused]] CVertexBuffer* pVBuf, [[maybe_unused]] CIndexBuffer* pIBuf, [[maybe_unused]] int nNumIndices, [[maybe_unused]] int nOffsIndex, [[maybe_unused]] const PublicRenderPrimitiveType nPrmode, [[maybe_unused]] int nVertStart, [[maybe_unused]] int nVertStop)
{
}

void CNULLRenderer::DrawPrimitivesInternal([[maybe_unused]] CVertexBuffer* src, [[maybe_unused]] int vert_num, [[maybe_unused]] const eRenderPrimitiveType prim_type)
{
}

void CRenderMesh::DrawImmediately()
{
}

///////////////////////////////////////////
void CNULLRenderer::SetCullMode([[maybe_unused]] int mode)
{
}

///////////////////////////////////////////
bool CNULLRenderer::EnableFog([[maybe_unused]] bool enable)
{
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//MISC EXTENSIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////
void CNULLRenderer::EnableVSync([[maybe_unused]] bool enable)
{
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::SelectTMU([[maybe_unused]] int tnum)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//MATRIX FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////
void CNULLRenderer::PushMatrix()
{
}

///////////////////////////////////////////
void CNULLRenderer::RotateMatrix([[maybe_unused]] float a, [[maybe_unused]] float x, [[maybe_unused]] float y, [[maybe_unused]] float z)
{
}

void CNULLRenderer::RotateMatrix([[maybe_unused]] const Vec3& angles)
{
}

///////////////////////////////////////////
void CNULLRenderer::TranslateMatrix([[maybe_unused]] float x, [[maybe_unused]] float y, [[maybe_unused]] float z)
{
}

void CNULLRenderer::MultMatrix([[maybe_unused]] const float* mat)
{
}

void CNULLRenderer::TranslateMatrix([[maybe_unused]] const Vec3& pos)
{
}

///////////////////////////////////////////
void CNULLRenderer::ScaleMatrix([[maybe_unused]] float x, [[maybe_unused]] float y, [[maybe_unused]] float z)
{
}

///////////////////////////////////////////
void CNULLRenderer::PopMatrix()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
void CNULLRenderer::LoadMatrix([[maybe_unused]] const Matrix34* src)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//MISC
////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////
void CNULLRenderer::PushWireframeMode([[maybe_unused]] int mode){}
void CNULLRenderer::PopWireframeMode(){}
void CNULLRenderer::FX_PushWireframeMode([[maybe_unused]] int mode){}
void CNULLRenderer::FX_PopWireframeMode(){}
void CNULLRenderer::FX_SetWireframeMode([[maybe_unused]] int mode){}

///////////////////////////////////////////
void CNULLRenderer::SetCamera(const CCamera& cam)
{
    int nThreadID = m_pRT->GetThreadList();
    m_RP.m_TI[nThreadID].m_cam = cam;
}

void CNULLRenderer::GetViewport(int* x, int* y, int* width, int* height) const
{
    *x = 0;
    *y = 0;
    *width = GetWidth();
    *height = GetHeight();
}

void CNULLRenderer::SetViewport([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int width, [[maybe_unused]] int height, [[maybe_unused]] int id)
{
}

void CNULLRenderer::SetScissor([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int width, [[maybe_unused]] int height)
{
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::GetModelViewMatrix(float* mat)
{
    memcpy(mat, &m_IdentityMatrix, sizeof(m_IdentityMatrix));
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::GetProjectionMatrix(float* mat)
{
    memcpy(mat, &m_IdentityMatrix, sizeof(m_IdentityMatrix));
}

//////////////////////////////////////////////////////////////////////
ITexture* CNULLRenderer::EF_LoadTexture([[maybe_unused]] const char * nameTex, [[maybe_unused]] const uint32 flags)
{
    return CTextureManager::Instance()->GetNoTexture();
}

//////////////////////////////////////////////////////////////////////
ITexture* CNULLRenderer::EF_LoadDefaultTexture(const char * nameTex)
{
    return CTextureManager::Instance()->GetDefaultTexture(nameTex);
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::DrawQuad([[maybe_unused]] const Vec3& right, [[maybe_unused]] const Vec3& up, [[maybe_unused]] const Vec3& origin, [[maybe_unused]] int nFlipmode /*=0*/)
{
}

//////////////////////////////////////////////////////////////////////
bool CNULLRenderer::ProjectToScreen([[maybe_unused]] float ptx, [[maybe_unused]] float pty, [[maybe_unused]] float ptz, [[maybe_unused]] float* sx, [[maybe_unused]] float* sy, [[maybe_unused]] float* sz)
{
    return false;
}

int CNULLRenderer::UnProject([[maybe_unused]] float sx, [[maybe_unused]] float sy, [[maybe_unused]] float sz,
    [[maybe_unused]] float* px, [[maybe_unused]] float* py, [[maybe_unused]] float* pz,
    [[maybe_unused]] const float modelMatrix[16],
    [[maybe_unused]] const float projMatrix[16],
    [[maybe_unused]] const int    viewport[4])
{
    return 0;
}

//////////////////////////////////////////////////////////////////////
int CNULLRenderer::UnProjectFromScreen([[maybe_unused]] float  sx, [[maybe_unused]] float  sy, [[maybe_unused]] float  sz,
    [[maybe_unused]] float* px, [[maybe_unused]] float* py, [[maybe_unused]] float* pz)
{
    return 0;
}

//////////////////////////////////////////////////////////////////////
bool CNULLRenderer::ScreenShot([[maybe_unused]] const char* filename, [[maybe_unused]] int width)
{
    return true;
}

int CNULLRenderer::ScreenToTexture([[maybe_unused]] int nTexID)
{
    return 0;
}

void CNULLRenderer::ResetToDefault()
{
}

///////////////////////////////////////////
void CNULLRenderer::SetMaterialColor([[maybe_unused]] float r, [[maybe_unused]] float g, [[maybe_unused]] float b, [[maybe_unused]] float a)
{
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::ClearTargetsImmediately([[maybe_unused]] uint32 nFlags) {}
void CNULLRenderer::ClearTargetsImmediately([[maybe_unused]] uint32 nFlags, [[maybe_unused]] const ColorF& Colors, [[maybe_unused]] float fDepth) {}
void CNULLRenderer::ClearTargetsImmediately([[maybe_unused]] uint32 nFlags, [[maybe_unused]] const ColorF& Colors) {}
void CNULLRenderer::ClearTargetsImmediately([[maybe_unused]] uint32 nFlags, [[maybe_unused]] float fDepth) {}

void CNULLRenderer::ClearTargetsLater([[maybe_unused]] uint32 nFlags) {}
void CNULLRenderer::ClearTargetsLater([[maybe_unused]] uint32 nFlags, [[maybe_unused]] const ColorF& Colors, [[maybe_unused]] float fDepth) {}
void CNULLRenderer::ClearTargetsLater([[maybe_unused]] uint32 nFlags, [[maybe_unused]] const ColorF& Colors) {}
void CNULLRenderer::ClearTargetsLater([[maybe_unused]] uint32 nFlags, [[maybe_unused]] float fDepth) {}

void CNULLRenderer::ReadFrameBuffer([[maybe_unused]] unsigned char* pRGB, [[maybe_unused]] int nImageX, [[maybe_unused]] int nSizeX, [[maybe_unused]] int nSizeY, [[maybe_unused]] ERB_Type eRBType, [[maybe_unused]] bool bRGBA, [[maybe_unused]] int nScaledX, [[maybe_unused]] int nScaledY)
{
}

void CNULLRenderer::ReadFrameBufferFast([[maybe_unused]] uint32* pDstARGBA8, [[maybe_unused]] int dstWidth, [[maybe_unused]] int dstHeight, [[maybe_unused]] bool BGRA)
{
}

bool CNULLRenderer::CaptureFrameBufferFast([[maybe_unused]] unsigned char* pDstRGBA8, [[maybe_unused]] int destinationWidth, [[maybe_unused]] int destinationHeight)
{
    return false;
}
bool CNULLRenderer::CopyFrameBufferFast([[maybe_unused]] unsigned char* pDstRGBA8, [[maybe_unused]] int destinationWidth, [[maybe_unused]] int destinationHeight)
{
    return false;
}

bool CNULLRenderer::InitCaptureFrameBufferFast([[maybe_unused]] uint32 bufferWidth, [[maybe_unused]] uint32 bufferHeight)
{
    return(false);
}

void CNULLRenderer::CloseCaptureFrameBufferFast(void)
{
}

bool CNULLRenderer::RegisterCaptureFrame([[maybe_unused]] ICaptureFrameListener* pCapture)
{
    return(false);
}
bool CNULLRenderer::UnRegisterCaptureFrame([[maybe_unused]] ICaptureFrameListener* pCapture)
{
    return(false);
}

void CNULLRenderer::CaptureFrameBufferCallBack(void)
{
}


void CNULLRenderer::SetFogColor([[maybe_unused]] const ColorF& color)
{
}

void CNULLRenderer::DrawQuad([[maybe_unused]] float dy, [[maybe_unused]] float dx, [[maybe_unused]] float dz, [[maybe_unused]] float x, [[maybe_unused]] float y, [[maybe_unused]] float z)
{
}

//////////////////////////////////////////////////////////////////////

int CNULLRenderer::CreateRenderTarget([[maybe_unused]] const char* name, [[maybe_unused]] int nWidth, [[maybe_unused]] int nHeight, [[maybe_unused]] const ColorF& cClear, [[maybe_unused]] ETEX_Format eTF)
{
    return 0;
}

bool CNULLRenderer::DestroyRenderTarget([[maybe_unused]] int nHandle)
{
    return true;
}

bool CNULLRenderer::ResizeRenderTarget([[maybe_unused]] int nHandle, [[maybe_unused]] int nWidth, [[maybe_unused]] int nHeight)
{
    return true;
}

bool CNULLRenderer::SetRenderTarget([[maybe_unused]] int nHandle, [[maybe_unused]] SDepthTexture* pDepthSurf)
{
    return true;
}

SDepthTexture* CNULLRenderer::CreateDepthSurface([[maybe_unused]] int nWidth, [[maybe_unused]] int nHeight, [[maybe_unused]] bool shaderResourceView)
{
    return nullptr;
}

void CNULLRenderer::DestroyDepthSurface([[maybe_unused]] SDepthTexture* pDepthSurf)
{
}

void CNULLRenderer::WaitForParticleBuffer([[maybe_unused]] threadID nThreadId)
{
}

int CNULLRenderer::GetOcclusionBuffer([[maybe_unused]] uint16* pOutOcclBuffer, [[maybe_unused]] Matrix44* pmCamBuffe)
{
    return 0;
}

IColorGradingController* CNULLRenderer::GetIColorGradingController()
{
    return m_pNULLColorGradingController;
}

IStereoRenderer* CNULLRenderer::GetIStereoRenderer()
{
    return m_pNULLStereoRenderer;
}

ITexture* CNULLRenderer::Create2DTexture([[maybe_unused]] const char* name, [[maybe_unused]] int width, [[maybe_unused]] int height, [[maybe_unused]] int numMips, [[maybe_unused]] int flags, [[maybe_unused]] unsigned char* data, [[maybe_unused]] ETEX_Format format)
{
    return nullptr;
}

//=========================================================================================


ILog* iLog;
IConsole* iConsole;
ITimer* iTimer;
ISystem* iSystem;

StaticInstance<CNULLRenderer> g_nullRenderer;

extern "C" DLL_EXPORT IRenderer * CreateCryRenderInterface(ISystem * pSystem)
{
    ModuleInitISystem(pSystem, "CryRenderer");

    gbRgb = false;

    iConsole    = gEnv->pConsole;
    iLog            = gEnv->pLog;
    iTimer      = gEnv->pTimer;
    iSystem     = gEnv->pSystem;

    CRenderer* rd = g_nullRenderer;
    if (rd)
    {
        rd->InitRenderer();
    }

    std::random_device randDev;
    srand(static_cast<int>(randDev()));

    return rd;
}

class CEngineModule_CryRenderer
    : public IEngineModule
{
    CRYINTERFACE_SIMPLE(IEngineModule)
    CRYGENERATE_SINGLETONCLASS(CEngineModule_CryRenderer, "EngineModule_CryRenderer", 0x540c91a7338e41d3, 0xaceeac9d55614450)

    virtual const char* GetName() const {
        return "CryRenderer";
    }
    virtual const char* GetCategory() const {return "CryEngine"; }

    virtual bool Initialize(SSystemGlobalEnvironment& env, [[maybe_unused]] const SSystemInitParams& initParams)
    {
        ISystem* pSystem = env.pSystem;
        env.pRenderer = CreateCryRenderInterface(pSystem);
        return env.pRenderer != 0;
    }
};

CRYREGISTER_SINGLETON_CLASS(CEngineModule_CryRenderer)

CEngineModule_CryRenderer::CEngineModule_CryRenderer()
{
};

CEngineModule_CryRenderer::~CEngineModule_CryRenderer()
{
};

void COcclusionQuery::Create()
{
}

void COcclusionQuery::Release()
{
}

void COcclusionQuery::BeginQuery()
{
}

void COcclusionQuery::EndQuery()
{
}

uint32 COcclusionQuery::GetVisibleSamples([[maybe_unused]] bool bAsynchronous)
{
    return 0;
}

/*static*/ FurBendData& FurBendData::Get()
{
    static FurBendData s_instance;
    return s_instance;
}

void FurBendData::InsertNewElements()
{
}

void FurBendData::FreeData()
{
}

void FurBendData::OnBeginFrame()
{
}

TArray<SRenderLight>* CRenderer::EF_GetDeferredLights([[maybe_unused]] const SRenderingPassInfo& passInfo, [[maybe_unused]] const eDeferredLightType eLightType)
{
    static TArray<SRenderLight> lights;
    return &lights;
}

SRenderLight* CRenderer::EF_GetDeferredLightByID([[maybe_unused]] const uint16 nLightID, [[maybe_unused]] const eDeferredLightType eLightType)
{
    return nullptr;
}


void CRenderer::BeginSpawningGeneratingRendItemJobs([[maybe_unused]] int nThreadID)
{
}

void CRenderer::BeginSpawningShadowGeneratingRendItemJobs([[maybe_unused]] int nThreadID)
{
}

void CRenderer::EndSpawningGeneratingRendItemJobs()
{
}

void CNULLRenderer::PrecacheResources()
{
}

bool CNULLRenderer::EF_PrecacheResource([[maybe_unused]] SShaderItem* pSI, [[maybe_unused]] float fMipFactorSI, [[maybe_unused]] float fTimeToReady, [[maybe_unused]] int Flags, [[maybe_unused]] int nUpdateId, [[maybe_unused]] int nCounter)
{
    return true;
}

ITexture* CNULLRenderer::EF_CreateCompositeTexture([[maybe_unused]] int type, [[maybe_unused]] const char* szName, [[maybe_unused]] int nWidth, [[maybe_unused]] int nHeight, [[maybe_unused]] int nDepth, [[maybe_unused]] int nMips, [[maybe_unused]] int nFlags, [[maybe_unused]] ETEX_Format eTF, [[maybe_unused]] const STexComposition* pCompositions, [[maybe_unused]] size_t nCompositions, [[maybe_unused]] int8 nPriority)
{
    return CTextureManager::Instance()->GetNoTexture();
}

void CNULLRenderer::FX_ClearTarget([[maybe_unused]] ITexture* pTex)
{
}

void CNULLRenderer::FX_ClearTarget([[maybe_unused]] SDepthTexture* pTex)
{
}

bool CNULLRenderer::FX_SetRenderTarget([[maybe_unused]] int nTarget, [[maybe_unused]] void* pTargetSurf, [[maybe_unused]] SDepthTexture* pDepthTarget, [[maybe_unused]] uint32 nTileCount)
{
    return true;
}

bool CNULLRenderer::FX_PushRenderTarget([[maybe_unused]] int nTarget, [[maybe_unused]] void* pTargetSurf, [[maybe_unused]] SDepthTexture* pDepthTarget, [[maybe_unused]] uint32 nTileCount)
{
    return true;
}

bool CNULLRenderer::FX_SetRenderTarget([[maybe_unused]] int nTarget, [[maybe_unused]] CTexture* pTarget, [[maybe_unused]] SDepthTexture* pDepthTarget, [[maybe_unused]] bool bPush, [[maybe_unused]] int nCMSide, [[maybe_unused]] bool bScreenVP, [[maybe_unused]] uint32 nTileCount)
{
    return true;
}

bool CNULLRenderer::FX_PushRenderTarget([[maybe_unused]] int nTarget, [[maybe_unused]] CTexture* pTarget, [[maybe_unused]] SDepthTexture* pDepthTarget, [[maybe_unused]] int nCMSide, [[maybe_unused]] bool bScreenVP, [[maybe_unused]] uint32 nTileCount)
{
    return true;
}
bool CNULLRenderer::FX_RestoreRenderTarget([[maybe_unused]] int nTarget)
{
    return true;
}
bool CNULLRenderer::FX_PopRenderTarget([[maybe_unused]] int nTarget)
{
    return true;
}

void CNULLRenderer::FX_SetActiveRenderTargets([[maybe_unused]] bool bAllowDIP)
{
}

IDynTexture* CNULLRenderer::CreateDynTexture2([[maybe_unused]] uint32 nWidth, [[maybe_unused]] uint32 nHeight, [[maybe_unused]] uint32 nTexFlags, [[maybe_unused]] const char* szSource, [[maybe_unused]] ETexPool eTexPool)
{
    return nullptr;
}

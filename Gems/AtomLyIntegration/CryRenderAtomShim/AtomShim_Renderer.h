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

#ifndef NULL_RENDERER_H
#define NULL_RENDERER_H

#if _MSC_VER > 1000
# pragma once
#endif

/*
===========================================
The NULLRenderer interface Class
===========================================
*/

#define MAX_TEXTURE_STAGES 4

#include "CryArray.h"
#include "AtomShim_RenderAuxGeom.h"

#include <AzCore/Module/Module.h>

#include <AtomCore/Instance/Instance.h>

#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI/ImageView.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicDrawInterface.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>

#include <AzFramework/Components/CameraBus.h>

// Forward declaration.
namespace AZ
{
    namespace RHI
    {
        class Image;
    }

    namespace RPI
    {
        class Scene;
        class RenderPipeline;
        class WindowContext;
        class View;
        class ViewportContext;
    }
}

//! A vector of these structs is used to keep track of the different viewports using Atom to render.
//! Each viewport currently has its own scene and pipeline.
struct AtomShimViewContext
{
    HWND m_hWnd;
    bool m_isMainViewport;

    // width and height of the viewport.
    // These are not fully used currently since each viewport window sends OnWindowResized messages to the WindowContext
    // these could instead be sent via the AtomShim in future if desired.
    int m_width;
    int m_height;

    AZ::RPI::Scene* m_scene = nullptr;
    AZStd::shared_ptr<AZ::RPI::RenderPipeline> m_renderPipeline;
    AZStd::shared_ptr<AZ::RPI::View> m_view;
};

#define ATOM_SHIM_TEXTURE_TYPE eTT_MaxTexType

struct AtomShimTexture
    : public CTexture
    , public AZ::Data::AssetBus::Handler
{
    AtomShimTexture(uint32 nFlags) : CTexture(nFlags)
    {
    }

    virtual ~AtomShimTexture();

    void QueueForHotReload(const AZ::Data::AssetId& assetId);

    void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

    void CreateFromStreamingImageAsset(const AZ::Data::Asset<AZ::RPI::StreamingImageAsset>& streamingImageAsset);
    void CreateFromImage(const AZ::Data::Instance<AZ::RPI::Image>& image);

    virtual void SetClamp(bool bEnable)
    {
        uint32 flags = GetFlags();
        if (bEnable)
        {
            flags |= FT_STATE_CLAMP;
        }
        else
        {
            flags &= ~FT_STATE_CLAMP;
        }
        SetFlags(flags);
    }

    AZ::Data::Instance<AZ::RPI::Image> m_instance; // This is only set for textures loaded from an asset
    AZ::RHI::Ptr<AZ::RHI::Image> m_image;   // This is only set for textures created dynamically (e.g. font images)
    AZ::RHI::Ptr<AZ::RHI::ImageView> m_imageView;
};

//////////////////////////////////////////////////////////////////////
class CAtomShimRenderer
    : public CRenderer
    , public AZ::Module     // This is a base class so that StaticNames in the NameDictionary work in this DLL
    , Camera::ActiveCameraRequestBus::Handler
{
public:

    ////---------------------------------------------------------------------------------------------------------------------
    virtual SRenderPipeline* GetRenderPipeline() override { return nullptr; }
    virtual SRenderThread* GetRenderThread() override { return nullptr; }
    virtual void FX_SetState(int st, int AlphaRef = -1, int RestoreState = 0) override;
    void SetCull([[maybe_unused]] ECull eCull, [[maybe_unused]] bool bSkipMirrorCull = false) override {}
    virtual SDepthTexture* GetDepthBufferOrig() override { return nullptr; }
    virtual uint32 GetBackBufferWidth() override { return 0; }
    virtual uint32 GetBackBufferHeight() override { return 0; };
    virtual const SRenderTileInfo* GetRenderTileInfo() const override { return nullptr; }

    virtual void FX_CommitStates([[maybe_unused]] const SShaderTechnique* pTech, [[maybe_unused]] const SShaderPass* pPass, [[maybe_unused]] bool bUseMaterialState) override {}
    virtual void FX_Commit([[maybe_unused]] bool bAllowDIP = false) override {}
    virtual long FX_SetVertexDeclaration([[maybe_unused]] int StreamMask, [[maybe_unused]] const AZ::Vertex::Format& vertexFormat) override { return 0; }
    virtual void FX_DrawIndexedPrimitive([[maybe_unused]] const eRenderPrimitiveType eType, [[maybe_unused]] const int nVBOffset, [[maybe_unused]] const int nMinVertexIndex, [[maybe_unused]] const int nVerticesCount, [[maybe_unused]] const int nStartIndex, [[maybe_unused]] const int nNumIndices, [[maybe_unused]] bool bInstanced = false) override {}
    virtual SDepthTexture* FX_GetDepthSurface([[maybe_unused]] int nWidth, [[maybe_unused]] int nHeight, [[maybe_unused]] bool bAA, [[maybe_unused]] bool shaderResourceView = false) override { return nullptr; }
    virtual long FX_SetIStream([[maybe_unused]] const void* pB, [[maybe_unused]] uint32 nOffs, [[maybe_unused]] RenderIndexType idxType) override { return -1; }
    virtual long FX_SetVStream([[maybe_unused]] int nID, [[maybe_unused]] const void* pB, [[maybe_unused]] uint32 nOffs, [[maybe_unused]] uint32 nStride, [[maybe_unused]] uint32 nFreq = 1) override { return -1; }
    virtual void FX_DrawPrimitive([[maybe_unused]] eRenderPrimitiveType eType, [[maybe_unused]] int nStartVertex, [[maybe_unused]] int nVerticesCount, [[maybe_unused]] int nInstanceVertices = 0) {}
    virtual void DrawQuad3D([[maybe_unused]] const Vec3& v0, [[maybe_unused]] const Vec3& v1, [[maybe_unused]] const Vec3& v2, [[maybe_unused]] const Vec3& v3, [[maybe_unused]] const ColorF& color, [[maybe_unused]] float ftx0, [[maybe_unused]] float fty0, [[maybe_unused]] float ftx1, [[maybe_unused]] float fty1) override {}
    virtual void DrawQuad([[maybe_unused]] float x0, [[maybe_unused]] float y0, [[maybe_unused]] float x1, [[maybe_unused]] float y1, [[maybe_unused]] const ColorF& color, [[maybe_unused]] float z = 1.0f, [[maybe_unused]] float s0 = 0.0f, [[maybe_unused]] float t0 = 0.0f, [[maybe_unused]] float s1 = 1.0f, [[maybe_unused]] float t1 = 1.0f) override {};
    virtual void FX_ClearTarget(ITexture* pTex) override;
    virtual void FX_ClearTarget(SDepthTexture* pTex) override;

    virtual bool FX_SetRenderTarget(int nTarget, void* pTargetSurf, SDepthTexture* pDepthTarget, uint32 nTileCount = 1) override;
    virtual bool FX_PushRenderTarget(int nTarget, void* pTargetSurf, SDepthTexture* pDepthTarget, uint32 nTileCount = 1) override;
    virtual bool FX_SetRenderTarget(int nTarget, CTexture* pTarget, SDepthTexture* pDepthTarget, bool bPush = false, int nCMSide = -1, bool bScreenVP = false, uint32 nTileCount = 1) override;
    virtual bool FX_PushRenderTarget(int nTarget, CTexture* pTarget, SDepthTexture* pDepthTarget, int nCMSide = -1, bool bScreenVP = false, uint32 nTileCount = 1) override;
    virtual bool FX_RestoreRenderTarget(int nTarget) override;
    virtual bool FX_PopRenderTarget(int nTarget) override;
    virtual void FX_SetActiveRenderTargets([[maybe_unused]] bool bAllowDIP = false) override {}
    virtual void EF_Scissor([[maybe_unused]] bool bEnable, [[maybe_unused]] int sX, [[maybe_unused]] int sY, [[maybe_unused]] int sWdt, [[maybe_unused]] int sHgt) override {};
    virtual void FX_ResetPipe() override {};

    ////---------------------------------------------------------------------------------------------------------------------

    CAtomShimRenderer();
    virtual ~CAtomShimRenderer();

    virtual WIN_HWND Init(int x, int y, int width, int height, unsigned int cbpp, int zbpp, int sbits, bool fullscreen, bool isEditor, WIN_HINSTANCE hinst, WIN_HWND Glhwnd = 0, bool bReInit = false, const SCustomRenderInitArgs* pCustomArgs = 0, bool bShaderCacheGen = false);
    virtual WIN_HWND GetHWND();
    virtual bool SetWindowIcon(const char* path);

    virtual ERenderType GetRenderType() const override;

    virtual const char* GetRenderDescription() const override;

    /////////////////////////////////////////////////////////////////////////////////
    // Render-context management
    /////////////////////////////////////////////////////////////////////////////////
    virtual bool SetCurrentContext(WIN_HWND hWnd);
    virtual bool CreateContext(WIN_HWND hWnd, bool bAllowMSAA, int SSX, int SSY);
    virtual bool DeleteContext(WIN_HWND hWnd);
    virtual void MakeMainContextActive();
    virtual WIN_HWND GetCurrentContextHWND() { return m_currContext ? m_currContext->m_hWnd : m_hWnd; }
    virtual bool IsCurrentContextMainVP() { return m_currContext ? m_currContext->m_isMainViewport : true; }

    virtual int GetCurrentContextViewportWidth() const { return -1; }
    virtual int GetCurrentContextViewportHeight() const { return -1; }
    /////////////////////////////////////////////////////////////////////////////////

    virtual int  CreateRenderTarget(const char* name, int nWidth, int nHeight, const ColorF& cClear, ETEX_Format eTF = eTF_R8G8B8A8);
    virtual bool ResizeRenderTarget(int nHandle, int nWidth, int nHeight);
    virtual bool DestroyRenderTarget(int nHandle);
    virtual bool SetRenderTarget(int nHandle, SDepthTexture* pDepthSurf = nullptr);
    virtual SDepthTexture* CreateDepthSurface(int nWidth, int nHeight, bool shaderResourceView = false);
    virtual void DestroyDepthSurface(SDepthTexture* pDepthSurf);

    virtual int GetOcclusionBuffer(uint16* pOutOcclBuffer, Matrix44* pmCamBuffe);
    virtual void WaitForParticleBuffer(threadID nThreadId);

    virtual void GetVideoMemoryUsageStats([[maybe_unused]] size_t& vidMemUsedThisFrame, [[maybe_unused]] size_t& vidMemUsedRecently, [[maybe_unused]] bool bGetPoolsSizes = false) {}

    virtual   void SetRenderTile([[maybe_unused]] f32 nTilesPosX, [[maybe_unused]] f32 nTilesPosY, [[maybe_unused]] f32 nTilesGridSizeX, [[maybe_unused]] f32 nTilesGridSizeY) {}

    virtual void EF_InvokeShadowMapRenderJobs([[maybe_unused]] int nFlags){}
    //! Fills array of all supported video formats (except low resolution formats)
    //! Returns number of formats, also when called with NULL
    virtual int   EnumDisplayFormats(SDispFormat* Formats);

    //! Return all supported by video card video AA formats
    virtual int   EnumAAFormats([[maybe_unused]] SAAFormat* Formats) { return 0; }

    //! Changes resolution of the window/device (doen't require to reload the level
    virtual bool  ChangeResolution(int nNewWidth, int nNewHeight, int nNewColDepth, int nNewRefreshHZ, bool bFullScreen, bool bForce);

    virtual Vec2 SetViewportDownscale([[maybe_unused]] float xscale, [[maybe_unused]] float yscale) { return Vec2(0, 0); }
    virtual void SetCurDownscaleFactor([[maybe_unused]] Vec2 sf) {};

    virtual EScreenAspectRatio GetScreenAspect([[maybe_unused]] int nWidth, [[maybe_unused]] int nHeight) { return eAspect_4_3; }

    virtual void SwitchToNativeResolutionBackbuffer() {}

    virtual void  ShutDown(bool bReInit = false);
    virtual void  ShutDownFast();

    virtual void  BeginFrame();
    virtual void  RenderDebug(bool bRernderStats = true);
    virtual void    EndFrame();
    virtual void    LimitFramerate([[maybe_unused]] const int maxFPS, [[maybe_unused]] const bool bUseSleep) {}

    virtual void    TryFlush();

    virtual void  Reset (void) {};
    virtual void    RT_ReleaseCB(void*){}

    virtual void InitSystemResources(int nFlags);
    virtual void ForceGC() {}
    virtual void FlushPendingTextureTasks() {}

    virtual void SetTexture(int tnum);
    virtual void SetTexture(int tnum, int nUnit);
    virtual void SetState(int State, int AlphaRef);

    virtual void DrawStringU(IFFont_RenderProxy* pFont, float x, float y, float z, const char* pStr, bool asciiMultiLine, const STextDrawContext& ctx) const;
    virtual void DrawDynVB(SVF_P3F_C4B_T2F* pBuf, uint16* pInds, int nVerts, int nInds, PublicRenderPrimitiveType nPrimType);
    virtual void DrawDynUiPrimitiveList(DynUiPrimitiveList& primitives, int totalNumVertices, int totalNumIndices);

    virtual void  DrawBuffer(CVertexBuffer* pVBuf, CIndexBuffer* pIBuf, int nNumIndices, int nOffsIndex, const PublicRenderPrimitiveType nPrmode, int nVertStart = 0, int nVertStop = 0);

    virtual   void    CheckError(const char* comment);

    virtual void DrawLine([[maybe_unused]] const Vec3& vPos1, [[maybe_unused]] const Vec3& vPos2) {};
    virtual void Graph([[maybe_unused]] byte* g, [[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int wdt, [[maybe_unused]] int hgt, [[maybe_unused]] int nC, [[maybe_unused]] int type, [[maybe_unused]] const char* text, [[maybe_unused]] ColorF& color, [[maybe_unused]] float fScale) {};

    virtual   void    SetCamera(const CCamera& cam);
    virtual   void    SetViewport(int x, int y, int width, int height, int id = 0);
    virtual   void    SetScissor(int x = 0, int y = 0, int width = 0, int height = 0);
    virtual void  GetViewport(int* x, int* y, int* width, int* height) const;

    virtual void  SetCullMode (int mode = R_CULL_BACK);
    virtual bool  EnableFog   (bool enable);
    virtual void  SetFogColor(const ColorF& color);
    virtual   void    EnableVSync(bool enable);

    virtual void  DrawPrimitivesInternal(CVertexBuffer* src, int vert_num, const eRenderPrimitiveType prim_type);

    virtual void  PushMatrix();
    virtual void  RotateMatrix(float a, float x, float y, float z);
    virtual void  RotateMatrix(const Vec3& angels);
    virtual void  TranslateMatrix(float x, float y, float z);
    virtual void  ScaleMatrix(float x, float y, float z);
    virtual void  TranslateMatrix(const Vec3& pos);
    virtual void  MultMatrix(const float* mat);
    virtual   void    LoadMatrix(const Matrix34* src = 0);
    virtual void  PopMatrix();

    virtual   void    EnableTMU(bool enable);
    virtual void  SelectTMU(int tnum);

    virtual bool ChangeDisplay(unsigned int width, unsigned int height, unsigned int cbpp);
    virtual void ChangeViewport(unsigned int x, unsigned int y, unsigned int width, unsigned int height, bool bMainViewport = false, float scaleWidth = 1.0f, float scaleHeight = 1.0f);

    virtual   bool SaveTga([[maybe_unused]] unsigned char* sourcedata, [[maybe_unused]] int sourceformat, [[maybe_unused]] int w, [[maybe_unused]] int h, [[maybe_unused]] const char* filename, [[maybe_unused]] bool flip) const { return false; }

    //download an image to video memory. 0 in case of failure
    virtual void CreateResourceAsync([[maybe_unused]] SResourceAsync* Resource) {};
    virtual void ReleaseResourceAsync([[maybe_unused]] SResourceAsync* Resource) {};
    void ReleaseResourceAsync(AZStd::unique_ptr<SResourceAsync> Resource) override {};
    virtual unsigned int DownLoadToVideoMemory([[maybe_unused]] const byte* data, [[maybe_unused]] int w, [[maybe_unused]] int h, [[maybe_unused]] int d, [[maybe_unused]] ETEX_Format eTFSrc, [[maybe_unused]] ETEX_Format eTFDst, [[maybe_unused]] int nummipmap, [[maybe_unused]] ETEX_Type eTT, [[maybe_unused]] bool repeat = true, [[maybe_unused]] int filter = FILTER_BILINEAR, [[maybe_unused]] int Id = 0, [[maybe_unused]] const char* szCacheName = NULL, [[maybe_unused]] int flags = 0, [[maybe_unused]] EEndian eEndian = eLittleEndian, [[maybe_unused]] RectI* pRegion = NULL, [[maybe_unused]] bool bAsynDevTexCreation = false) { return 0; }
    virtual unsigned int DownLoadToVideoMemory([[maybe_unused]] const byte* data, [[maybe_unused]] int w, [[maybe_unused]] int h, [[maybe_unused]] ETEX_Format eTFSrc, [[maybe_unused]] ETEX_Format eTFDst, [[maybe_unused]] int nummipmap, [[maybe_unused]] bool repeat = true, [[maybe_unused]] int filter = FILTER_BILINEAR, [[maybe_unused]] int Id = 0, [[maybe_unused]] const char* szCacheName = NULL, [[maybe_unused]] int flags = 0, [[maybe_unused]] EEndian eEndian = eLittleEndian, [[maybe_unused]] RectI* pRegion = NULL, [[maybe_unused]] bool bAsynDevTexCreation = false) { return 0; }
    virtual unsigned int DownLoadToVideoMemoryCube([[maybe_unused]] const byte* data, [[maybe_unused]] int w, [[maybe_unused]] int h, [[maybe_unused]] ETEX_Format eTFSrc, [[maybe_unused]] ETEX_Format eTFDst, [[maybe_unused]] int nummipmap, [[maybe_unused]] bool repeat = true, [[maybe_unused]] int filter = FILTER_BILINEAR, [[maybe_unused]] int Id = 0, [[maybe_unused]] const char* szCacheName = NULL, [[maybe_unused]] int flags = 0, [[maybe_unused]] EEndian eEndian = eLittleEndian, [[maybe_unused]] RectI* pRegion = NULL, [[maybe_unused]] bool bAsynDevTexCreation = false) { return 0; }
    virtual unsigned int DownLoadToVideoMemory3D([[maybe_unused]] const byte* data, [[maybe_unused]] int w, [[maybe_unused]] int h, [[maybe_unused]] int d, [[maybe_unused]] ETEX_Format eTFSrc, [[maybe_unused]] ETEX_Format eTFDst, [[maybe_unused]] int nummipmap, [[maybe_unused]] bool repeat = true, [[maybe_unused]] int filter = FILTER_BILINEAR, [[maybe_unused]] int Id = 0, [[maybe_unused]] const char* szCacheName = NULL, [[maybe_unused]] int flags = 0, [[maybe_unused]] EEndian eEndian = eLittleEndian, [[maybe_unused]] RectI* pRegion = NULL, [[maybe_unused]] bool bAsynDevTexCreation = false) { return 0; }
    virtual void UpdateTextureInVideoMemory([[maybe_unused]] uint32 tnum, [[maybe_unused]] const byte* newdata, [[maybe_unused]] int posx, [[maybe_unused]] int posy, [[maybe_unused]] int w, [[maybe_unused]] int h, [[maybe_unused]] ETEX_Format eTFSrc = eTF_R8G8B8A8, [[maybe_unused]] int posz = 0, [[maybe_unused]] int sizez = 1){}

    virtual   bool SetGammaDelta(float fGamma);
    virtual void RestoreGamma(void) {};

    virtual   void RemoveTexture([[maybe_unused]] unsigned int TextureId) {}
    virtual   void DeleteFont([[maybe_unused]] IFFont* font) {}

    virtual void Draw2dImage(float xpos, float ypos, float w, float h, int texture_id, float s0 = 0, float t0 = 0, float s1 = 1, float t1 = 1, float angle = 0, float r = 1, float g = 1, float b = 1, float a = 1, float z = 1);
    virtual void Push2dImage(float xpos, float ypos, float w, float h, int texture_id, float s0 = 0, float t0 = 0, float s1 = 1, float t1 = 1, float angle = 0, float r = 1, float g = 1, float b = 1, float a = 1, float z = 1, float stereoDepth = 0);
    virtual void Draw2dImageList();
    virtual void  Draw2dImageStretchMode([[maybe_unused]] bool stretch) {};
    virtual void DrawImage(float xpos, float ypos, float w, float h, int texture_id, float s0, float t0, float s1, float t1, float r, float g, float b, float a, bool filtered = true);
    virtual void DrawImageWithUV(float xpos, float ypos, float z, float w, float h, int texture_id, float s[4], float t[4], float r, float g, float b, float a, bool filtered = true);

    virtual void PushWireframeMode(int mode);
    virtual void PopWireframeMode();
    virtual void FX_PushWireframeMode(int mode);
    virtual void FX_PopWireframeMode();
    virtual void FX_SetWireframeMode(int mode);

    virtual void FX_PreRender([[maybe_unused]] int Stage) override {}
    virtual void FX_PostRender() override {}

    virtual void ResetToDefault();
    virtual void SetDefaultRenderStates() {}

    virtual int  GenerateAlphaGlowTexture(float k);

    virtual void ApplyViewParameters(const CameraViewParameters&) override;
    virtual void SetMaterialColor(float r, float g, float b, float a);

    virtual void GetMemoryUsage(ICrySizer* Sizer);

    // Project/UnProject.  Returns true if successful.
    virtual bool ProjectToScreen(float ptx, float pty, float ptz,
        float* sx, float* sy, float* sz);
    virtual int UnProject(float sx, float sy, float sz,
        float* px, float* py, float* pz,
        const float modelMatrix[16],
        const float projMatrix[16],
        const int    viewport[4]);
    virtual int UnProjectFromScreen(float  sx, float  sy, float  sz,
        float* px, float* py, float* pz);

    // Shadow Mapping
    virtual bool PrepareDepthMap(ShadowMapFrustum* SMSource, int nFrustumLOD = 0, bool bClearPool = false);
    virtual void DrawAllShadowsOnTheScreen();
    virtual void OnEntityDeleted([[maybe_unused]] IRenderNode* pRenderNode) {};

    virtual void FX_SetClipPlane (bool bEnable, float* pPlane, bool bRefract);

    virtual void SetColorOp(byte eCo, byte eAo, byte eCa, byte eAa);
    virtual void EF_SetColorOp([[maybe_unused]] byte eCo, [[maybe_unused]] byte eAo, [[maybe_unused]] byte eCa, [[maybe_unused]] byte eAa) {};

    virtual void SetSrgbWrite([[maybe_unused]] bool srgbWrite) {};
    virtual void EF_SetSrgbWrite([[maybe_unused]] bool sRGBWrite) {};

    //for editor
    virtual void  GetModelViewMatrix(float* mat);
    virtual void  GetProjectionMatrix(float* mat);

    //for texture
    virtual ITexture* EF_LoadTexture(const char* nameTex, uint32 flags = 0);
    virtual ITexture* EF_LoadDefaultTexture(const char* nameTex);

    virtual void DrawQuad(const Vec3& right, const Vec3& up, const Vec3& origin, int nFlipMode = 0);
    virtual void DrawQuad(float dy, float dx, float dz, float x, float y, float z);
    // NOTE: deprecated
    virtual void ClearTargetsImmediately(uint32 nFlags);
    virtual void ClearTargetsImmediately(uint32 nFlags, const ColorF& Colors, float fDepth);
    virtual void ClearTargetsImmediately(uint32 nFlags, const ColorF& Colors);
    virtual void ClearTargetsImmediately(uint32 nFlags, float fDepth);

    virtual void ClearTargetsLater(uint32 nFlags);
    virtual void ClearTargetsLater(uint32 nFlags, const ColorF& Colors, float fDepth);
    virtual void ClearTargetsLater(uint32 nFlags, const ColorF& Colors);
    virtual void ClearTargetsLater(uint32 nFlags, float fDepth);

    virtual void ReadFrameBuffer(unsigned char* pRGB, int nImageX, int nSizeX, int nSizeY, ERB_Type eRBType, bool bRGBA, int nScaledX = -1, int nScaledY = -1);
    virtual void ReadFrameBufferFast(uint32* pDstARGBA8, int dstWidth, int dstHeight, bool BGRA = true);

    virtual bool CaptureFrameBufferFast(unsigned char* pDstRGBA8, int destinationWidth, int destinationHeight);
    virtual bool CopyFrameBufferFast(unsigned char* pDstRGBA8, int destinationWidth, int destinationHeight);
    virtual bool RegisterCaptureFrame(ICaptureFrameListener* pCapture);
    virtual bool UnRegisterCaptureFrame(ICaptureFrameListener* pCapture);
    virtual bool InitCaptureFrameBufferFast(uint32 bufferWidth, uint32 bufferHeight);
    virtual void CloseCaptureFrameBufferFast(void);
    virtual void CaptureFrameBufferCallBack(void);


    virtual void ReleaseHWShaders() {}
    virtual void PrintResourcesLeaks() {}

    //misc
    virtual bool ScreenShot(const char* filename = NULL, int width = 0);

    virtual void Set2DMode(uint32 orthoWidth, uint32 orthoHeight, TransformationMatrices& backupMatrices, float znear = -1e10f, float zfar = 1e10f);
    virtual void Unset2DMode(const TransformationMatrices& restoringMatrices);
    virtual void Set2DModeNonZeroTopLeft(float orthoLeft, float orthoTop, float orthoWidth, float orthoHeight, TransformationMatrices& backupMatrices, float znear = -1e10f, float zfar = 1e10f);

    virtual int ScreenToTexture(int nTexID);

    virtual void DrawPoints([[maybe_unused]] Vec3 v[], [[maybe_unused]] int nump, [[maybe_unused]] ColorF& col, [[maybe_unused]] int flags) {};
    virtual void DrawLines([[maybe_unused]] Vec3 v[], [[maybe_unused]] int nump, [[maybe_unused]] ColorF& col, [[maybe_unused]] int flags, [[maybe_unused]] float fGround) {};

    virtual void    RefreshSystemShaders() {}

    // Shaders/Shaders support
    // RE - RenderElement

    virtual void EF_Release(int nFlags);
    virtual void FX_PipelineShutdown(bool bFastShutdown = false);

    //==========================================================
    // external interface for shaders
    //==========================================================

    virtual bool EF_SetLightHole(Vec3 vPos, Vec3 vNormal, int idTex, float fScale = 1.0f, bool bAdditive = true);

    // Draw all shaded REs in the list
    virtual void EF_EndEf3D (int nFlags, int nPrecacheUpdateId, int nNearPrecacheUpdateId, const SRenderingPassInfo& passInfo);

    // 2d interface for shaders
    virtual void EF_EndEf2D(bool bSort);
    virtual bool EF_PrecacheResource(SShaderItem* pSI, float fMipFactor, float fTimeToReady, int Flags, int nUpdateId, int nCounter);
    virtual bool EF_PrecacheResource(ITexture* pTP, float fDist, float fTimeToReady, int Flags, int nUpdateId, int nCounter);
    virtual void PrecacheResources();
    virtual void PostLevelLoading() {}
    virtual void PostLevelUnload() {}

    virtual ITexture* EF_CreateCompositeTexture(int type, const char* szName, int nWidth, int nHeight, int nDepth, int nMips, int nFlags, ETEX_Format eTF, const STexComposition* pCompositions, size_t nCompositions, int8 nPriority = -1);

    void EF_Init();

    virtual IDynTexture* MakeDynTextureFromShadowBuffer(int nSize, IDynTexture* pDynTexture);
    virtual void MakeSprite(IDynTexture*& rTexturePtr, float _fSpriteDistance, int nTexSize, float angle, float angle2, IStatObj* pStatObj, const float fBrightnessMultiplier, SRendParams& rParms);
    virtual uint32 RenderOccludersIntoBuffer([[maybe_unused]] const CCamera& viewCam, [[maybe_unused]] int nTexSize, [[maybe_unused]] PodArray<IRenderNode*>& lstOccluders, [[maybe_unused]] float* pBuffer) { return 0; }

    virtual IRenderAuxGeom* GetIRenderAuxGeom([[maybe_unused]] void* jobID = 0)
    {
        return m_pAtomShimRenderAuxGeom;
    }

    virtual IColorGradingController* GetIColorGradingController();
    virtual IStereoRenderer* GetIStereoRenderer();

    virtual ITexture* Create2DTexture(const char* name, int width, int height, int numMips, int flags, unsigned char* data, ETEX_Format format);

    //////////////////////////////////////////////////////////////////////
    // All font functions are not implemented since the font is rendered by AtomFont
    int FontCreateTexture(
        [[maybe_unused]] int Width, [[maybe_unused]] int Height, [[maybe_unused]] byte* pData,
        [[maybe_unused]] ETEX_Format eTF = eTF_R8G8B8A8, [[maybe_unused]] bool genMips = false,
        [[maybe_unused]] const char* textureName = nullptr) override
    {
        return -1;
    }
    bool FontUpdateTexture(
        [[maybe_unused]] int nTexId, [[maybe_unused]] int X, [[maybe_unused]] int Y, [[maybe_unused]] int USize, [[maybe_unused]] int VSize,
        [[maybe_unused]] byte* pData) override
    {
        return true;
    }
    void FontSetTexture([[maybe_unused]] int nTexId, [[maybe_unused]] int nFilterMode) override { }
    void FontSetRenderingState([[maybe_unused]] bool overrideViewProjMatrices, [[maybe_unused]] TransformationMatrices& backupMatrices) override { }
    void FontSetBlending([[maybe_unused]] int src, [[maybe_unused]] int dst, [[maybe_unused]] int baseState) override { }
    void FontRestoreRenderingState([[maybe_unused]] bool overrideViewProjMatrices, [[maybe_unused]] const TransformationMatrices& restoringMatrices) override { }

    virtual void GetLogVBuffers(void) {}

    virtual void RT_PresentFast() {}

    virtual void RT_ForceSwapBuffers() {}
    virtual void RT_SwitchToNativeResolutionBackbuffer([[maybe_unused]] bool resolveBackBuffer) {}

    virtual void RT_BeginFrame() {}
    virtual void RT_EndFrame() {}
    virtual void RT_Init() {}
    virtual void RT_ShutDown([[maybe_unused]] uint32 nFlags) {}
    virtual bool RT_CreateDevice() { return true; }
    virtual void RT_Reset() {}
    virtual void RT_SetCull([[maybe_unused]] int nMode) {}
    virtual void RT_SetScissor([[maybe_unused]] bool bEnable, [[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int width, [[maybe_unused]] int height){}
    virtual void RT_RenderScene([[maybe_unused]] int nFlags, [[maybe_unused]] SThreadInfo& TI, [[maybe_unused]] RenderFunc pRenderFunc) {}
    virtual void RT_PrepareStereo([[maybe_unused]] int mode, [[maybe_unused]] int output) {}
    virtual void RT_CopyToStereoTex([[maybe_unused]] int channel) {}
    virtual void RT_UpdateTrackingStates() {}
    virtual void RT_DisplayStereo() {}
    virtual void RT_SetCameraInfo() {}
    virtual void RT_SetStereoCamera() {}
    virtual void RT_ReadFrameBuffer([[maybe_unused]] unsigned char* pRGB, [[maybe_unused]] int nImageX, [[maybe_unused]] int nSizeX, [[maybe_unused]] int nSizeY, [[maybe_unused]] ERB_Type eRBType, [[maybe_unused]] bool bRGBA, [[maybe_unused]] int nScaledX, [[maybe_unused]] int nScaledY) {}
    virtual void RT_RenderScene([[maybe_unused]] int nFlags, [[maybe_unused]] SThreadInfo& TI, [[maybe_unused]] int nR, [[maybe_unused]] RenderFunc pRenderFunc) {};
    virtual void RT_CreateResource([[maybe_unused]] SResourceAsync* Res) {};
    virtual void RT_ReleaseResource([[maybe_unused]] SResourceAsync* Res) {};
    virtual void RT_ReleaseRenderResources() {};
    virtual void RT_UnbindResources() {};
    virtual void RT_UnbindTMUs() {};
    virtual void RT_PrecacheDefaultShaders() {};
    virtual void RT_CreateRenderResources() {};
    virtual void RT_ClearTarget([[maybe_unused]] ITexture* pTex, [[maybe_unused]] const ColorF& color) {};
    virtual void RT_RenderDebug([[maybe_unused]] bool bRenderStats = true) {};

    virtual HRESULT RT_CreateVertexBuffer([[maybe_unused]] UINT Length, [[maybe_unused]] DWORD Usage, [[maybe_unused]] DWORD FVF, [[maybe_unused]] UINT Pool, [[maybe_unused]] void** ppVertexBuffer, [[maybe_unused]] HANDLE* pSharedHandle) { return S_OK; }
    virtual HRESULT RT_CreateIndexBuffer([[maybe_unused]] UINT Length, [[maybe_unused]] DWORD Usage, [[maybe_unused]] DWORD Format, [[maybe_unused]] UINT Pool, [[maybe_unused]] void** ppVertexBuffer, [[maybe_unused]] HANDLE* pSharedHandle) { return S_OK; };
    virtual HRESULT RT_CreateVertexShader([[maybe_unused]] DWORD* pBuf, [[maybe_unused]] void** pShader, [[maybe_unused]] void* pInst) { return S_OK; };
    virtual HRESULT RT_CreatePixelShader([[maybe_unused]] DWORD* pBuf, [[maybe_unused]] void** pShader) { return S_OK; };
    virtual void RT_ReleaseVBStream([[maybe_unused]] void* pVB, [[maybe_unused]] int nStream) {};
    virtual void RT_DrawDynVB([[maybe_unused]] int Pool, [[maybe_unused]] uint32 nVerts) {}
    virtual void RT_DrawDynVB([[maybe_unused]] SVF_P3F_C4B_T2F* pBuf, [[maybe_unused]] uint16* pInds, [[maybe_unused]] uint32 nVerts, [[maybe_unused]] uint32 nInds, [[maybe_unused]] const PublicRenderPrimitiveType nPrimType) {}
    virtual void RT_DrawDynVBUI([[maybe_unused]] SVF_P2F_C4B_T2F_F4B* pBuf, [[maybe_unused]] uint16* pInds, [[maybe_unused]] uint32 nVerts, [[maybe_unused]] uint32 nInds, [[maybe_unused]] const PublicRenderPrimitiveType nPrimType) {}
    virtual void RT_DrawStringU([[maybe_unused]] IFFont_RenderProxy* pFont, [[maybe_unused]] float x, [[maybe_unused]] float y, [[maybe_unused]] float z, [[maybe_unused]] const char* pStr, [[maybe_unused]] bool asciiMultiLine, [[maybe_unused]] const STextDrawContext& ctx) const {}
    virtual void RT_DrawLines([[maybe_unused]] Vec3 v[], [[maybe_unused]] int nump, [[maybe_unused]] ColorF& col, [[maybe_unused]] int flags, [[maybe_unused]] float fGround) {}
    virtual void RT_Draw2dImage([[maybe_unused]] float xpos, [[maybe_unused]] float ypos, [[maybe_unused]] float w, [[maybe_unused]] float h, [[maybe_unused]] CTexture* pTexture, [[maybe_unused]] float s0, [[maybe_unused]] float t0, [[maybe_unused]] float s1, [[maybe_unused]] float t1, [[maybe_unused]] float angle, [[maybe_unused]] DWORD col, [[maybe_unused]] float z) {}
    virtual void RT_Push2dImage([[maybe_unused]] float xpos, [[maybe_unused]] float ypos, [[maybe_unused]] float w, [[maybe_unused]] float h, [[maybe_unused]] CTexture* pTexture, [[maybe_unused]] float s0, [[maybe_unused]] float t0, [[maybe_unused]] float s1, [[maybe_unused]] float t1, [[maybe_unused]] float angle, [[maybe_unused]] DWORD col, [[maybe_unused]] float z, [[maybe_unused]] float stereoDepth) {}
    virtual void RT_Draw2dImageList() {}
    virtual void RT_Draw2dImageStretchMode([[maybe_unused]] bool bStretch) {}
    virtual void RT_DrawImageWithUV([[maybe_unused]] float xpos, [[maybe_unused]] float ypos, [[maybe_unused]] float z, [[maybe_unused]] float w, [[maybe_unused]] float h, [[maybe_unused]] int texture_id, [[maybe_unused]] float* s, [[maybe_unused]] float* t, [[maybe_unused]] DWORD col, [[maybe_unused]] bool filtered = true) {}
    virtual void EF_ClearTargetsImmediately([[maybe_unused]] uint32 nFlags) {}
    virtual void EF_ClearTargetsImmediately([[maybe_unused]] uint32 nFlags, [[maybe_unused]] const ColorF& Colors, [[maybe_unused]] float fDepth, [[maybe_unused]] uint8 nStencil) {}
    virtual void EF_ClearTargetsImmediately([[maybe_unused]] uint32 nFlags, [[maybe_unused]] const ColorF& Colors) {}
    virtual void EF_ClearTargetsImmediately([[maybe_unused]] uint32 nFlags, [[maybe_unused]] float fDepth, [[maybe_unused]] uint8 nStencil) {}

    virtual void EF_ClearTargetsLater([[maybe_unused]] uint32 nFlags) {}
    virtual void EF_ClearTargetsLater([[maybe_unused]] uint32 nFlags, [[maybe_unused]] const ColorF& Colors, [[maybe_unused]] float fDepth, [[maybe_unused]] uint8 nStencil) {}
    virtual void EF_ClearTargetsLater([[maybe_unused]] uint32 nFlags, [[maybe_unused]] const ColorF& Colors) {}
    virtual void EF_ClearTargetsLater([[maybe_unused]] uint32 nFlags, [[maybe_unused]] float fDepth, [[maybe_unused]] uint8 nStencil) {}

    virtual void RT_PushRenderTarget([[maybe_unused]] int nTarget, [[maybe_unused]] CTexture* pTex, [[maybe_unused]] SDepthTexture* pDS, [[maybe_unused]] int nS)  {};
    virtual void RT_PopRenderTarget([[maybe_unused]] int nTarget) {};
    virtual void RT_SetViewport([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int width, [[maybe_unused]] int height, [[maybe_unused]] int id) {}

    virtual void RT_SetRendererCVar([[maybe_unused]] ICVar* pCVar, [[maybe_unused]] const char* pArgText, [[maybe_unused]] const bool bSilentMode = false) {};
    virtual void SetRendererCVar([[maybe_unused]] ICVar* pCVar, [[maybe_unused]] const char* pArgText, [[maybe_unused]] bool bSilentMode = false) {};

    virtual void SetMatrices(float* pProjMat, float* pViewMat);

    virtual void PushProfileMarker([[maybe_unused]] const char* label) {}
    virtual void PopProfileMarker([[maybe_unused]] const char* label) {}

    virtual void RT_InsertGpuCallback([[maybe_unused]] uint32 context, [[maybe_unused]] GpuCallbackFunc callback) {}
    virtual void EnablePipelineProfiler([[maybe_unused]] bool bEnable) {}

    virtual IOpticsElementBase* CreateOptics([[maybe_unused]] EFlareType type) const { return NULL;        }

    virtual bool BakeMesh([[maybe_unused]] const SMeshBakingInputParams* pInputParams, [[maybe_unused]] SMeshBakingOutput* pReturnValues) { return false; }
    virtual PerInstanceConstantBufferPool* GetPerInstanceConstantBufferPoolPointer() override { return nullptr; }

    IDynTexture* CreateDynTexture2(uint32 nWidth, uint32 nHeight, uint32 nTexFlags, const char* szSource, ETexPool eTexPool) override;

    virtual void BeginProfilerSection([[maybe_unused]] const char* name, [[maybe_unused]] uint32 eProfileLabelFlags = 0) override {}
    virtual void EndProfilerSection([[maybe_unused]] const char* name) override {}
    virtual void AddProfilerLabel([[maybe_unused]] const char* name) override {}

#ifdef SUPPORT_HW_MOUSE_CURSOR
    virtual IHWMouseCursor* GetIHWMouseCursor() { return NULL; }
#endif

    virtual void StartLoadtimePlayback([[maybe_unused]] ILoadtimeCallback* pCallback) {}
    virtual void StopLoadtimePlayback() {}

    // used to track current textures
    void SetTextureForUnit(int unit, int textureId);

    void RT_DrawVideoRenderer([[maybe_unused]] AZ::VideoRenderer::IVideoRenderer* pVideoRenderer, [[maybe_unused]] const AZ::VideoRenderer::DrawArguments& drawArguments) override {}

private:
    static constexpr char LogName[] = "CAtomShimRenderer";

    //! Camera::ActiveCameraSystemRequestBus::Handler overrides...
    const AZ::Transform& GetActiveCameraTransform() override;
    const Camera::Configuration& GetActiveCameraConfiguration() override;

    void CacheCameraTransform(const CCamera& camera);
    void CacheCameraConfiguration(const CCamera& camamera);

    HWND      m_hWnd = nullptr;            // The main app window

    AZStd::string m_rendererDescription;

    CAtomShimRenderAuxGeom* m_pAtomShimRenderAuxGeom;
    IColorGradingController* m_pAtomShimColorGradingController;
    IStereoRenderer* m_pAtomShimStereoRenderer;

    AZ::RHI::Ptr<AZ::RPI::DynamicDrawContext> m_dynamicDraw;

    AZ::RPI::ShaderVariantId m_shaderVariantWrap;
    AZ::RPI::ShaderVariantId m_shaderVariantClamp;

    // cached input indices for dynamic draw's draw srg
    AZ::RHI::ShaderInputNameIndex m_imageInputIndex = "m_texture";
    AZ::RHI::ShaderInputNameIndex m_viewProjInputIndex = "m_worldToProj";

    AZStd::unordered_map<WIN_HWND, AtomShimViewContext*> m_viewContexts;
    AtomShimViewContext* m_currContext = nullptr;

    int m_renderPipelineNameSuffix = 1;

    AtomShimTexture* m_currentTextureForUnit[32];
    bool m_clampFlagPerTextureUnit[32];

    int m_currentFontTextureId = -1;

    bool m_isFinalInitializationDone = false;
    bool m_isInFrame = false;   // True when between calls to BeginFrame and EndFrame

    int m_isIn2dModeCounter = 0;

    AZ::Transform m_cameraTransform = AZ::Transform::CreateIdentity();
    Camera::Configuration m_cameraConfiguration;
    AZStd::shared_ptr<AZ::RPI::ViewportContext> m_viewportContext;

    static AtomShimTexture* CastITextureToAtomShimTexture(ITexture* texture)
    {
        // If GetDevTexture returns a non-null value then this is not an AtomShim texture
        if (!(texture && !texture->GetDevTexture()))
        {
            return nullptr;
        }

        return static_cast<AtomShimTexture*>(texture);
    }
};

//=============================================================================

extern CAtomShimRenderer* gcpAtomShim;



#endif //NULL_RENDERER

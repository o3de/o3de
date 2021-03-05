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

// Description : Declaration of the type CContext.

#ifndef __GLCONTEXT__
#define __GLCONTEXT__

#include "GLCommon.hpp"
#include "GLFormat.hpp"
#include "GLState.hpp"
#include "GLResource.hpp"
#include "GLShader.hpp"

//  Confetti BEGIN: Igor Lobanchikov
@protocol MTLCommandBuffer;
@protocol MTLRenderCommandEncoder;
@protocol MTLBuffer;
@protocol MTLDevice;
@protocol MTLDepthStencilState;
@protocol MTLSamplerState;
#import <Metal/MTLRenderCommandEncoder.h>
//  Confetti End: Igor Lobanchikov

namespace NCryMetal
{
    //  Confetti BEGIN: Igor Lobanchikov
    struct SDepthStencilCache
    {
        id<MTLDepthStencilState>    m_DepthStencilState;
        uint32                      m_iStencilRef;
        bool                        m_bDSSDirty;
        bool                        m_bStencilRefDirty;
    };
    //  Confetti End: Igor Lobanchikov

    static const int FASTBUFFER_SIZE_THRESHHOLD = 4*1024;
    
    typedef SBlendState SBlendCache;

    // Additional OpenGL internal state that is implicitly mapped to the rasterizer state
    struct SRasterizerCache
        : SRasterizerState
    {
        //  Confetti BEGIN: Igor Lobanchikov
        enum
        {
            RS_CULL_MODE_DIRTY = 0x01,
            RS_DEPTH_BIAS_DIRTY = 0x02,
            RS_WINDING_DIRTY = 0x04,
            RS_FILL_MODE_DIRTY = 0x08,
            RS_DEPTH_CLIP_MODE_DIRTY = 0x10,
            RS_SCISSOR_ENABLE_DIRTY = 0x20,

            RS_ALL_BUT_SCISSOR_DIRTY = 0x1F,
            RS_ALL_DIRTY = 0x3F,
            RS_NOT_INITIALIZED = 0x80
        };

        uint8 m_RateriserDirty;
        //  Confetti End: Igor Lobanchikov

        MTLScissorRect  m_scissorRect;
    };

    struct SInputAssemblerSlot
    {
        SBuffer* m_pVertexBuffer;
        uint32 m_uStride;
        uint32 m_uOffset;

        SInputAssemblerSlot()
            : m_pVertexBuffer(NULL)
            , m_uStride(0)
            , m_uOffset(0)
        {
        }
    };

    struct SColor
    {
        float m_afRGBA[4];

        inline bool operator==(const SColor& kOther) const
        {
            return memcmp(m_afRGBA, kOther.m_afRGBA, sizeof(m_afRGBA)) == 0;
        }

        inline bool operator!=(const SColor& kOther) const
        {
            return !operator==(kOther);
        }
    };

    // The state that is not directly mapped to any of the DirectX 11 states
    struct SImplicitStateCache
    {
#if DXGL_SUPPORT_MULTISAMPLED_TEXTURES
        bool m_bSampleMaskEnabled;
        GLbitfield m_aSampleMask;
#endif //DXGL_SUPPORT_MULTISAMPLED_TEXTURES

        SColor m_akBlendColor;

        MTLViewport m_CurrentViewport;
        MTLViewport m_DefaultViewport;

        bool    m_bViewportDirty;
        bool    m_bViewportDefault;
        bool    m_bBlendColorDirty;
    };

    //  Confetti BEGIN: Igor Lobanchikov
    struct SBufferStateStageCache
    {
        static const int s_MaxUAVBuffersPerStage = 5;
        static const int s_MaxConstantBuffersPerStage = 25;
        static const int s_MaxBuffersPerStage = s_MaxUAVBuffersPerStage + s_MaxConstantBuffersPerStage;

        void    CheckForDynamicBufferUpdates();

        _smart_ptr<NCryMetal::SBuffer>  m_spBufferResource[s_MaxBuffersPerStage + s_MaxUAVBuffersPerStage];
        id<MTLBuffer>                   m_Buffers[s_MaxBuffersPerStage + s_MaxUAVBuffersPerStage];
        NSUInteger                      m_Offsets[s_MaxBuffersPerStage + s_MaxUAVBuffersPerStage];

        int32           m_MinBufferUsed;
        int32           m_MaxBufferUsed;
    };

    struct SUAVTextureStageCache
    {
        static const int s_MaxUAVTexturesPerStage = 5;

        _smart_ptr<NCryMetal::STexture>  m_UAVTextures[s_MaxUAVTexturesPerStage];
    };


    struct STextureStageState
    {
        //metal support up to 30 texture slots
        static const int s_MaxTexturesPerStage = 25;

        _smart_ptr<SShaderResourceView>     m_Textures[s_MaxTexturesPerStage];

        int32           m_MinTextureUsed;
        int32           m_MaxTextureUsed;
    };

    struct SSamplerStageState
    {
        static const int s_MaxSamplersPerStage = 17;

        id<MTLSamplerState>      m_Samplers[s_MaxSamplersPerStage];

        int32           m_MinSamplerUsed;
        int32           m_MaxSamplerUsed;
    };

    struct SStageStateCache
    {
        enum
        {
            eBST_Vertex = 0,
            eBST_Fragment = 1,
#if DXGL_SUPPORT_COMPUTE
            eBST_Compute = 2,
#endif
            eBST_Count
        };

        SBufferStateStageCache  m_BufferState[eBST_Count];
        STextureStageState      m_TextureState[eBST_Count];
        SSamplerStageState      m_SamplerState[eBST_Count];
        SUAVTextureStageCache   m_UAVTextureState; //Only used in compute shader
    };
    //  Confetti End: Igor Lobanchikov

    // Stores the current state of an OpenGL so that the device can lazily update
    // states without the overhead of calling glGet* functions.
    struct SStateCache
        : SImplicitStateCache
    {
        SBlendCache m_kBlend;
        SDepthStencilCache m_kDepthStencil;
        SRasterizerCache m_kRasterizer;
        //  Confetti BEGIN: Igor Lobanchikov
        SStageStateCache   m_StageCache;
        //  Confetti End: Igor Lobanchikov
    };

    class CGPUEventsHelper
    {
    public:
        enum EActionType
        {
            eAT_Push,
            eAT_Pop,
            eAT_Event
        };

        enum EFlushType
        {
            eFT_Default,
            eFT_FlushEncoder,
            eFT_NewEncoder
        };
    public:
        CGPUEventsHelper();
        ~CGPUEventsHelper();

        void    AddMarker(const char* pszMessage, EActionType eAction);
        void    FlushActions(id <MTLCommandEncoder> pEncoder, EFlushType eFlushType);
        void    OnSetRenderTargets();

    private:
        void    ReplayActions(id <MTLCommandEncoder> pEncoder, uint32 numActions);
        void    MovePushesToNextEncoder();

    private:
        struct SMarkerQueueAction
        {
            NSString*   m_pMessage;
            EActionType m_eAction;
        };

        std::vector<SMarkerQueueAction> m_aMarkerQueue;
        uint32                          m_uiRTSwitchedAt;
        std::vector<NSString*>          m_currentMarkerStack;

        static const uint32 m_sInvalidRTSwitchedAt = 0xFFFFFFFF;
    };

    class CContext
        : public SList::TListEntry
    {
    public:
        enum CopyFilterType
        {
            POINT,
            BILINEAR,
            BICUBIC,
            LANCZOS,
        };


        CContext(CDevice* pDevice);
        ~CContext();

        bool Initialize();
        CDevice* GetDevice() { return m_pDevice; }

        void FlushComputeKernelState();
        void FlushComputeBufferUnits();
        void FlushComputeTextureUnits();
        void FlushComputePipelineState();
        void FlushComputeThreadGroup(uint32 u32_Group_x, uint32 u32_Group_y, uint32 u32_Group_z);
        void Dispatch(uint32 u32_Group_x, uint32 u32_Group_y, uint32 u32_Group_z);
        void SetUAVBuffer(SBuffer* pBuffer, uint32 uStage, uint32 uSlot);
        void SetUAVTexture(STexture* pTexture, uint32 uStage, uint32 uSlot);

        //  Confetti BEGIN: Igor Lobanchikov
        id <MTLCommandBuffer> GetCurrentCommandBuffer() { return m_CurrentCommandBuffer; }
        void FlushFrameBufferState();
        SContextEventHelper* GetCurrentEventHelper();
        void* AllocateMemoryInRingBuffer(uint32 size, MemRingBufferStorage memAllocMode, size_t &ringBufferOffsetOut);
        
        id <MTLBuffer> GetRingBuffer(MemRingBufferStorage memAllocMode);
        void* AllocateQueryInRingBuffer();
        id <MTLBuffer> GetQueryRingBuffer() { return m_QueryRingBuffer.m_Buffer; }

        void FlushCurrentEncoder();
        id <MTLBlitCommandEncoder>  GetBlitCommandEncoder();
        void  ActivateComputeCommandEncoder();

        bool TrySlowCopySubresource(STexture* pDstTexture, uint32 uDstSubresource, uint32 uDstX, uint32 uDstY, uint32 uDstZ,
            STexture* pSrcTexture, uint32 uSrcSubresource, const D3D11_BOX* pSrcBox, const CopyFilterType filterType = POINT);

        void ProfileLabel(const char* szName);
        void ProfileLabelPush(const char* szName);
        void ProfileLabelPop(const char* szName);

        void BeginOcclusionQuery(SOcclusionQuery* pQuery);
        void EndOcclusionQuery(SOcclusionQuery* pQuery);

        bool SetBlendState(const SBlendState& kState);
        bool SetDepthStencilState(id<MTLDepthStencilState> depthStencilState, uint32 iStencilRef);
        bool SetRasterizerState(const SRasterizerState& kState);

        bool GetImplicitStateCache(SImplicitStateCache& kCache);

        void SetViewports(uint32 uNumViewports, const D3D11_VIEWPORT* pViewports);
        void SetScissorRects(uint32 uNumRects, const D3D11_RECT* pRects);
        void ClearRenderTarget(SOutputMergerView* pRenderTargetView, const float afColor[4]);
        void ClearDepthStencil(SOutputMergerView* pDepthStencilView, bool bClearDepth, bool bClearStencil, float fDepthValue, uint8 uStencilValue);
        void SetRenderTargets(uint32 uNumRTViews, SOutputMergerView** apRenderTargetViews, SOutputMergerView* pDepthStencilView);
        void SetBlendColor(float fRed, float fGreen, float fBlue, float fAlpha);
        void SetSampleMask(uint32 uSampleMask);
        void SetShader(SShader* pShader, uint32 uStage);
        void SetTexture(SShaderResourceView* pView, uint32 uStage, uint32 uSlot);
        void SetSampler(id<MTLSamplerState> pState, uint32 uStage, uint32 uSlot);
        void SetConstantBuffer(SBuffer* pConstantBuffer, uint32 uStage, uint32 uSlot);
        void SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY eTopology);
        void SetInputLayout(SInputLayout* pInputLayout);
        void SetVertexBuffer(uint32 uSlot, SBuffer* pVertexBuffer, uint32 uStride, uint32 uOffset);
        void SetIndexBuffer(SBuffer* pIndexBuffer, MTLIndexType eIndexType, uint32 uIndexStride, uint32 uOffset);
        void DrawIndexed(uint32 uIndexCount, uint32 uStartIndexLocation, uint32 uBaseVertexLocation);
        void Draw(uint32 uVertexCount, uint32 uBaseVertexLocation);
        void DrawIndexedInstanced(uint32 uIndexCountPerInstance, uint32 uInstanceCount, uint32 uStartIndexLocation, uint32 uBaseVertexLocation, uint32 uStartInstanceLocation);
        void DrawInstanced(uint32 uVertexCountPerInstance, uint32 uInstanceCount, uint32 uStartVertexLocation, uint32 uStartInstanceLocation);

        void Flush(id<CAMetalDrawable> drawable = nil, float syncInterval = 0.0f);
        void FlushBlitEncoderAndWait();

        _smart_ptr<SPipeline> AllocatePipeline(const SPipelineConfiguration& kConfiguration);
        void RemovePipeline(SPipeline* pPipeline, SShader* pInvalidShader);

        bool InitializePipeline(SPipeline* pPipeline);

        void InitMetalFrameResources();

    private:
        void FlushQueries();
        void SetNumPatchControlPoints(uint32 uNumControlPoints);
        void SetVertexOffset(uint32 uVertexOffset);

        void FlushInputAssemblerState();
        void FlushTextureUnits();
        void FlushStateObjects();
        void NextCommandBuffer();
        void FlushPipelineState();
        void FlushDrawState();

        CDevice* m_pDevice;

        SStateCache m_kStateCache;
        MTLIndexType    m_MetalIndexType;
        uint32          m_uIndexStride;
        uint32          m_uIndexOffset;
        SPipelinePtr m_spPipeline;

        // State that is only synchronized during draw calls
        MTLPrimitiveType                m_MetalPrimitiveType;
        _smart_ptr<NCryMetal::SBuffer>  m_spIndexBufferResource;

        SPipelineConfiguration m_kPipelineConfiguration;
        SInputLayout* m_pInputLayout;
        SInputAssemblerSlot m_akInputAssemblerSlots[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
        uint32 m_uVertexOffset;

        // Flags that tell which parts of the state need to be synchronized during the next draw call
        bool m_bFrameBufferStateDirty;
        bool m_bPipelineDirty;
        bool m_bInputLayoutDirty;
        bool m_bInputAssemblerSlotsDirty;

        // Hash maps of persistent frame buffers, pipelines and sampler unit maps that can be re-used every time
        // a compatible configuration is requested.
        struct SPipelineCache* m_pPipelineCache;

        static const int        m_MaxFrameQueueDepth = 3;
        static const int        m_MaxFrameQueueSlots = m_MaxFrameQueueDepth;
        //static const int        m_MaxFrameEventSlots = m_MaxFrameQueueDepth+1;
        //  Need to keep this value at least max engine event queu + 2. At the moment the longest event queue in the engine is 4.
        //  This must be m_MaxFrameQueueDepth+1 at least.
        static const int        m_MaxEngineEvenQueueLength = 4;
        static const int        m_MaxFrameEventSlots = m_MaxEngineEvenQueueLength + 2;
        dispatch_semaphore_t    m_FrameQueueSemaphore;
        int                     m_iCurrentFrameSlot;
        int                     m_iCurrentFrameEventSlot;
        //  there is extra slot to guarantee event data lives extra frame
        std::vector<SContextEventHelper*>   m_eEvents[m_MaxFrameEventSlots];
        int                     m_iCurrentEvent;

        id <MTLCommandBuffer>           m_CurrentCommandBuffer;
        id <MTLRenderCommandEncoder>    m_CurrentEncoder;
        id <MTLComputeCommandEncoder>   m_CurrentComputeEncoder;
        id <MTLBlitCommandEncoder>      m_CurrentBlitEncoder;

        _smart_ptr<SOutputMergerView>   m_CurrentRTs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
        _smart_ptr<SOutputMergerView>   m_CurrentDepth;

        CGPUEventsHelper   m_GPUEventsHelper;

        std::vector<SOcclusionQuery*> m_OcclusionQueryList;

        DXMETAL_TODO("Remove this if Metal rutime bug is fixed anytime soon.")
        //  Motivations: this default state is used to replace NULL sampler state
        //  since for some reason Metal runtime crashes when NULL sampler state is bound.
        //  Metal runtime works just fine if any other NULL state object is bound.
        id<MTLSamplerState> m_defaultSamplerState;

        bool                    m_bPossibleClearPending;

        struct CRingBuffer
        {
            CRingBuffer(id<MTLDevice> device, unsigned int bufferSize, unsigned int alignment, MemRingBufferStorage memAllocMode);

            void    OnFrameStart(int iCurrentFrameSlot, int iNextFrameSlot);
            void*   Allocate(int iCurrentFrameSlot, unsigned int size, size_t& ringBufferOffsetOut, unsigned int alignment = 0);
            void*   AllocateTemp(int iCurrentFrameSlot, unsigned int size, size_t& ringBufferOffsetOut, unsigned int alignment = 0);

            id<MTLBuffer>   m_Buffer;
            unsigned int    m_uiFreePositionPointer;
            unsigned int    m_uiDefaultAlignment;
        private:
            void    ConsumeTrack(int iCurrentFrameSlot, unsigned int size, bool bPadding);
            void    ValidateBufferUsage();

            //  Igor: since memory and performance losses are negligible
            //  just use max number of slots from all usage patters.
            //  If this becomes an issue in future (e.g. ValidateBufferUsage becomes too slow or memory
            //  footprint too big) just move to template.
            unsigned int m_BufferUsedPerFrame[m_MaxFrameEventSlots];
            unsigned int m_BufferPadPerFrame[m_MaxFrameEventSlots];
        };

        CRingBuffer             m_RingBufferShared; //This ring buffer uses CPU/GPU shared memory
#if defined(AZ_PLATFORM_MAC)
        CRingBuffer             m_RingBufferManaged; //This ring buffer uses Managed memory.
#endif
        CRingBuffer             m_QueryRingBuffer;

        class CCopyTextureHelper
        {
        public:
            CCopyTextureHelper();
            ~CCopyTextureHelper();

            bool Initialize(CDevice* pDevice);

            bool DoTopMipSlowCopy(id<MTLTexture> texDst, id<MTLTexture> texSrc, CContext* pContext, CopyFilterType const filterType = POINT);

        private:
            id<MTLRenderPipelineState> SelectPipelineState(MTLPixelFormat pixelFormat, CopyFilterType const filterType);

            id<MTLRenderPipelineState> m_PipelineStateBGRA8Unorm;
            id<MTLRenderPipelineState> m_PipelineStateRGBA8Unorm;
            id<MTLRenderPipelineState> m_PipelineStateRGBA16Float;
            id<MTLRenderPipelineState> m_PipelineStateRGBA32Float;
            id<MTLRenderPipelineState> m_PipelineStateR16Float;
            id<MTLRenderPipelineState> m_PipelineStateRG16Float;
            id<MTLRenderPipelineState> m_PipelineStateR16Unorm;
            id<MTLRenderPipelineState> m_PipelineStateBGRA8UnormLanczos;
            id<MTLRenderPipelineState> m_PipelineStateRGBA8UnormLanczos;
            id<MTLRenderPipelineState> m_PipelineStateBGRA8UnormBicubic;
            id<MTLRenderPipelineState> m_PipelineStateRGBA8UnormBicubic;
            id<MTLSamplerState> m_samplerState;
            id<MTLSamplerState> m_samplerStateLinear;

            struct Uniforms
            {
                float params0[4];
                float params1[4];
                float params2[4];
            } m_Uniforms;

            id<MTLDevice> m_DeviceRef;
        };

        CCopyTextureHelper  m_CopyTextureHelper;
    };
}

#endif

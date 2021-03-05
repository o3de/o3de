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
#include "GLView.hpp"
#include "GLBlitFramebufferHelper.hpp"

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzFramework/API/ApplicationAPI.h>

namespace NCryOpenGL
{
    enum
    {
        // Slots are the virtual binding points accessible through the Direct3D interface
        MAX_STAGE_TEXTURE_SLOTS         = D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,
        MAX_STAGE_SAMPLER_SLOTS         = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,
        MAX_STAGE_IMAGE_SLOTS           = D3D11_PS_CS_UAV_REGISTER_COUNT,
        MAX_STAGE_STORAGE_BUFFER_SLOTS  = D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,
        MAX_STAGE_CONSTANT_BUFFER_SLOTS = D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT,

        MAX_TEXTURE_SLOTS               = eST_NUM * MAX_STAGE_TEXTURE_SLOTS,
        MAX_SAMPLER_SLOTS               = eST_NUM * MAX_STAGE_SAMPLER_SLOTS,
        MAX_IMAGE_SLOTS                 = eST_NUM * MAX_STAGE_IMAGE_SLOTS,
        MAX_STORAGE_BUFFER_SLOTS        = eST_NUM * MAX_STAGE_STORAGE_BUFFER_SLOTS,
        MAX_CONSTANT_BUFFER_SLOTS       = eST_NUM * MAX_STAGE_CONSTANT_BUFFER_SLOTS,

        // Units are the actual OpenGL binding points for resources - these are maximum numbers -
        // the actual supported counts are queried at runtime.
        MAX_TEXTURE_UNITS               = eST_NUM * 64,
        MAX_IMAGE_UNITS                 = eST_NUM * 8,
        MAX_STORAGE_BUFFER_UNITS        = eST_NUM * 64,
        MAX_UNIFORM_BUFFER_UNITS        = eST_NUM * 16,

        MAX_VERTEX_ATTRIBUTES = D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT,
#if DXGL_SUPPORT_VERTEX_ATTRIB_BINDING
        MAX_VERTEX_ATTRIB_BINDINGS = MAX_VERTEX_ATTRIBUTES,
#endif //DXGL_SUPPORT_VERTEX_ATTRIB_BINDING

#if DXGL_STREAMING_CONSTANT_BUFFERS
        MAX_PREVIOUS_FRAMES = 4,
#endif // DXGL_STREAMING_CONSTANT_BUFFERS
    };

    COMPILE_TIME_ASSERT((uint32)MAX_TEXTURE_SLOTS <= (uint32)SUnitMap::MAX_TEXTURE_SLOT_IN_MAP);
    COMPILE_TIME_ASSERT((uint32)MAX_SAMPLER_SLOTS <= (uint32)SUnitMap::MAX_SAMPLER_SLOT_IN_MAP);
    COMPILE_TIME_ASSERT((uint32)MAX_TEXTURE_UNITS <= (uint32)SUnitMap::MAX_TEXTURE_UNIT_IN_MAP);

    inline uint32 TextureSlot(EShaderType eStage, uint32 uIndex) { return uIndex + eStage * MAX_STAGE_TEXTURE_SLOTS; }
    inline uint32 SamplerSlot(EShaderType eStage, uint32 uIndex) { return uIndex + eStage * MAX_STAGE_SAMPLER_SLOTS; }
    inline uint32 ImageSlot(EShaderType eStage, uint32 uIndex) { return uIndex + eStage * MAX_STAGE_IMAGE_SLOTS; }
    inline uint32 StorageBufferSlot(EShaderType eStage, uint32 uIndex) { return uIndex + eStage * MAX_STAGE_STORAGE_BUFFER_SLOTS; }
    inline uint32 ConstantBufferSlot(EShaderType eStage, uint32 uIndex) { return uIndex + eStage * MAX_STAGE_CONSTANT_BUFFER_SLOTS; }

    struct SGlobalConfig
    {
#if DXGL_STREAMING_CONSTANT_BUFFERS
        static int iStreamingConstantBuffersMode;
        static int iStreamingConstantBuffersPersistentMap;
        static int iStreamingConstantBuffersGranularity;
        static int iStreamingConstantBuffersGrowth;
        static int iStreamingConstantBuffersMaxUnits;
#endif
        static int iMinFramePoolSize;
        static int iMaxFramePoolSize;
        static int iBufferUploadMode;
#if DXGL_ENABLE_SHADER_TRACING
        static int iShaderTracingMode;
        static int iShaderTracingHash;
        static int iVertexTracingID;
        static int iPixelTracingX;
        static int iPixelTracingY;
#endif

        static void RegisterVariables();
        static void SetIHVDefaults();
    };

    // The reference values for the stencil operations
    struct SStencilRefCache
    {
        // Stencil reference value for front-facing polygons and non-polygons
        GLint m_iFrontFacesReference;

        // Stencil reference value for back-facing polygons
        GLint m_iBackFacesReference;
    };

    // Additional OpenGL internal state that is implicitly mapped to the depth stencil state
    struct SDepthStencilCache
        : SDepthStencilState
    {
        SStencilRefCache m_kStencilRef;
    };

    typedef SBlendState SBlendCache;

    // Additional OpenGL internal state that is implicitly mapped to the rasterizer state
    struct SRasterizerCache
        : SRasterizerState
    {
        bool m_bPolygonOffsetFillEnabled;
#if !DXGLES
        bool m_bPolygonOffsetLineEnabled;
#endif //!DXGLES
    };

    // Additional OpenGL internal state that is implicitly mapped to the texture unit state
    struct STextureUnitCache
    {
        CResourceName m_kTextureName;
        GLenum m_eTextureTarget;
        GLuint m_uSampler;
    };

    struct STextureSlot
    {
        SShaderTextureBasedView* m_pView;

        STextureSlot()
            : m_pView(NULL)
        {
        }

        bool operator==(const STextureSlot& kOther) const
        {
            return m_pView == kOther.m_pView;
        }

        bool operator!=(const STextureSlot& kOther) const
        {
            return !operator==(kOther);
        }
    };

    struct SSamplerSlot
    {
        SSamplerState* m_pSampler;

        SSamplerSlot(SSamplerState* pSampler = NULL)
            : m_pSampler(pSampler)
        {
        }

        bool operator==(const SSamplerSlot& kOther) const
        {
            return m_pSampler == kOther.m_pSampler;
        }

        bool operator!=(const SSamplerSlot& kOther) const
        {
            return !operator==(kOther);
        }
    };

    struct STextureUnitContext
    {
        STextureUnitCache m_kCurrentUnitState;
        std::vector<STexture*> m_kModifiedTextures;
    };

#if DXGL_SUPPORT_SHADER_IMAGES

    struct SImageUnitCache
    {
        CResourceName m_kTextureName;
        SShaderImageViewConfiguration m_kConfiguration;
    };

#endif //DXGL_SUPPORT_SHADER_IMAGES

    // Additional OpenGL internal state that is implicitly mapped to the input assembler state
    struct SInputAssemblerCache
    {
        typedef uint32 TAttributeBitMask;

        // Bit mask with 1 in the position of every vertex attribute enabled
        TAttributeBitMask m_uVertexAttribsEnabled;

        GLuint m_auVertexAttribDivisors[MAX_VERTEX_ATTRIBUTES];

        struct SVertexAttribPointer
        {
            GLint m_iSize;
            GLenum m_eType;
            GLboolean m_bNormalized;
            GLsizei m_iStride;
            GLvoid* m_pPointer;
            GLboolean m_bInteger;

            bool operator == (const SVertexAttribPointer& kOther) const
            {
                return
                    m_iSize == kOther.m_iSize &&
                    m_eType == kOther.m_eType &&
                    m_bNormalized == kOther.m_bNormalized &&
                    m_iStride == kOther.m_iStride &&
                    m_pPointer == kOther.m_pPointer &&
                    m_bInteger == kOther.m_bInteger;
            }

            bool operator != (const SVertexAttribPointer& kOther) const
            {
                return !operator==(kOther);
            }
        };
        SVertexAttribPointer m_auVertexAttribPointer[MAX_VERTEX_ATTRIBUTES];

        struct SVertexAttribFormat
        {
            GLint m_iSize;
            GLuint m_uRelativeOffset;
            GLenum m_eType;
            GLboolean m_bNormalized;
            GLboolean m_bInteger;

            bool operator == (const SVertexAttribFormat& kOther) const
            {
                return
                    m_iSize           == kOther.m_iSize &&
                    m_uRelativeOffset == kOther.m_uRelativeOffset &&
                    m_eType           == kOther.m_eType &&
                    m_bNormalized     == kOther.m_bNormalized &&
                    m_bInteger        == kOther.m_bInteger;
            }

            bool operator != (const SVertexAttribFormat& kOther) const
            {
                return !operator==(kOther);
            }
        };

        SVertexAttribFormat m_aVertexAttribFormats[MAX_VERTEX_ATTRIBUTES];

        GLuint m_auVertexBindingIndices[MAX_VERTEX_ATTRIBUTES];

#if DXGL_SUPPORT_VERTEX_ATTRIB_BINDING
        // vertex buffer bindings (note that we don't cache buffer, offset and stride since those change for each call to glBindVertexBuffers
        GLuint m_auVertexBindingDivisors[MAX_VERTEX_ATTRIB_BINDINGS];

        // watermark so we can avoid sending redundant state
        GLint m_iLastNonZeroBindingSlot;
#endif //DXGL_SUPPORT_VERTEX_ATTRIB_BINDING
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
        GLfloat m_afRGBA[4];

        inline bool operator==(const SColor& kOther) const
        {
            return memcmp(m_afRGBA, kOther.m_afRGBA, sizeof(m_afRGBA)) == 0;
        }

        inline bool operator!=(const SColor& kOther) const
        {
            return !operator==(kOther);
        }
    };

    struct SIndexedBufferBinding
    {
        CResourceName m_kName;
        SBufferRange m_kRange;

        SIndexedBufferBinding(const CResourceName& kName = CResourceName(), SBufferRange kRange = SBufferRange())
            : m_kName(kName)
            , m_kRange(kRange)
        {
        }

        bool operator==(const SIndexedBufferBinding& kOther) const
        {
            return
                m_kName   == kOther.m_kName &&
                m_kRange  == kOther.m_kRange;
        }

        bool operator!=(const SIndexedBufferBinding& kOther) const
        {
            return !operator==(kOther);
        }
    };

    typedef SIndexedBufferBinding TIndexedBufferBinding;

#if DXGL_SUPPORT_VIEWPORT_ARRAY
    typedef GLdouble TDepthRangeValue;
    typedef GLfloat  TViewportValue;
#else
    typedef GLclampf TDepthRangeValue;
    typedef GLuint   TViewportValue;
#endif

    // The state that is not directly mapped to any of the DirectX 11 states
    struct SImplicitStateCache
    {
        // The name of the frame buffer currently to GL_DRAW_FRAMEBUFFER
        CResourceName m_kDrawFrameBuffer;

        // The name of the frame buffer currently to GL_READ_FRAMEBUFFER
        CResourceName m_kReadFrameBuffer;

#if !DXGLES
        // The enable state of GL_FRAMEBUFFER_SRGB
        bool m_bFrameBufferSRGBEnabled;
#endif //!DXGLES

#if DXGL_SUPPORT_MULTISAMPLED_TEXTURES
        bool m_bSampleMaskEnabled;
        GLbitfield m_aSampleMask;
#endif //DXGL_SUPPORT_MULTISAMPLED_TEXTURES

        SColor m_akBlendColor;

        // Viewport xy ranges
        TViewportValue m_akViewportData[DXGL_NUM_SUPPORTED_VIEWPORTS * 4];

        // Viewport depth ranges
        TDepthRangeValue m_akDepthRangeData[DXGL_NUM_SUPPORTED_VIEWPORTS * 2];

#if DXGL_SUPPORT_TESSELLATION
        GLint m_iNumPatchControlPoints;
#endif //DXGL_SUPPORT_TESSELLATION

        // The buffer currently bound to each buffer target
        CResourceName m_akBuffersBound[eBB_NUM];

        // The buffer range currently bound to each indexed buffer
#if DXGL_SUPPORT_SHADER_STORAGE_BLOCKS
        TIndexedBufferBinding m_akStorageBuffersBound[MAX_STORAGE_BUFFER_UNITS];
#endif //DXGL_SUPPORT_SHADER_STORAGE_BLOCKS
        TIndexedBufferBinding m_akUniformBuffersBound[MAX_UNIFORM_BUFFER_UNITS];

        // Pixelstore values
        GLint m_iUnpackRowLength;
        GLint m_iUnpackImageHeight;
        GLint m_iUnpackAlignment;
        GLint m_iPackRowLength;
#if !DXGLES
        GLint m_iPackImageHeight;
#endif //!DXGLES
        GLint m_iPackAlignment;
        GLenum m_glActiveTexture;
    };

    // Stores the current state of an OpenGL so that the device can lazily update
    // states without the overhead of calling glGet* functions.
    struct SStateCache
        : SImplicitStateCache
    {
        SBlendCache m_kBlend;
        SDepthStencilCache m_kDepthStencil;
        SRasterizerCache m_kRasterizer;
        SStencilRefCache m_kStencilRef;
        STextureUnitCache m_akTextureUnits[MAX_TEXTURE_UNITS];
#if DXGL_SUPPORT_SHADER_IMAGES
        SImageUnitCache m_akImageUnits[MAX_IMAGE_UNITS];
#endif //DXGL_SUPPORT_SHADER_IMAGES
        SInputAssemblerCache m_kInputAssembler;
        GLint m_akGLScissorData[DXGL_NUM_SUPPORTED_SCISSOR_RECTS * 4];
    };

#if DXGL_ENABLE_SHADER_TRACING

    struct SVertexShaderTraceHeader
    {
        uint32 m_uVertexID;
    };

    struct SFragmentShaderTraceHeader
    {
        float m_afFragmentCoordX;
        float m_afFragmentCoordY;
    };

    struct SVertexShaderTraceInfo
    {
        SVertexShaderTraceHeader m_kHeader;
        uint32 m_uVertexIndex;
    };

    struct SFragmentShaderTraceInfo
    {
        SFragmentShaderTraceHeader m_kHeader;
        uint32 m_uFragmentCoordX;
        uint32 m_uFragmentCoordY;
    };

#endif //DXGL_ENABLE_SHADER_TRACING

#if DXGL_STREAMING_CONSTANT_BUFFERS

    struct SConstantBufferSlot
    {
        SBuffer* m_pBuffer;
        SBufferRange m_kRange;

        SConstantBufferSlot()
            : m_pBuffer(NULL) {}
    };

    typedef SConstantBufferSlot TConstantBufferSlot;

    struct SContextFrame
    {
        GLsync m_pEndFence;
        uint32 m_uRefCount;
        SContextFrame** m_pFreeHead;
        SContextFrame* m_pNext;

        SContextFrame(SContextFrame** pFreeHead)
            : m_uRefCount(0)
            , m_pFreeHead(pFreeHead)
            , m_pEndFence(NULL)
        {
        }

        ~SContextFrame()
        {
            m_pNext = *m_pFreeHead;
            * m_pFreeHead = this;
        }

        void AddRef()
        {
            ++m_uRefCount;
        }

        void Release()
        {
            if (--m_uRefCount == 0)
            {
                this->~SContextFrame();
            }
        }
    };

    struct SStreamingBuffer
    {
        CResourceName m_kName;
        uint32 m_uSectionCapacity;
        uint32 m_uRequestedSectionSize;
        uint32 m_uNextPosition;
        uint32 m_uEndPosition;
#if DXGL_SUPPORT_BUFFER_STORAGE
        GLvoid* m_pMappedData;
#endif

        SStreamingBuffer()
            : m_uSectionCapacity(0)
            , m_uRequestedSectionSize(0)
            , m_uNextPosition(0)
            , m_uEndPosition(0)
#if DXGL_SUPPORT_BUFFER_STORAGE
            , m_pMappedData(NULL)
#endif
        {
        }

        ~SStreamingBuffer()
        {
            if (m_kName.IsValid())
            {
                GLuint uName(m_kName.GetName());
                glDeleteBuffers(1, &uName);
            }
        }
    };

    struct SStreamingBufferContext
    {
        SContextFramePtr m_spCurrentFrame;
        SContextFramePtr m_aspPreviousFrames[MAX_PREVIOUS_FRAMES];
        SContextFrame* m_pFreeFramesHead;
        std::vector<SContextFrame*> m_kFramePools;
        uint32 m_uPreviousFrameIndex, m_uNumPreviousFrames;
#if DXGL_SUPPORT_BUFFER_STORAGE
        bool m_bFlushNeeded;
#endif

        SStreamingBuffer m_akStreamingBuffers[MAX_UNIFORM_BUFFER_UNITS];
        uint32 m_uNumStreamingBuffersUnits;

        SStreamingBufferContext();
        ~SStreamingBufferContext();

        void SwitchFrame(CDevice* pDevice);
        void UpdateStreamingSizes(CDevice* pDevice);

        void UploadAndBindUniformData(CContext* pContext, const SConstantBufferSlot& kSlot, uint32 uUnit);
        void FlushUniformData();
    };

#else

    typedef TIndexedBufferBinding TConstantBufferSlot;

#endif

    class CContext
        : public SList::TListEntry
        , public AzFramework::ApplicationLifecycleEvents::Bus::Handler
    {
#if DXGL_STREAMING_CONSTANT_BUFFERS
        friend struct SStreamingBufferContext;
#endif
    public:
        enum ContextType
        {
            RenderingType, // Context used to render to a window
            ResourceType, // Context used for loading resources
            NumContextTypes // Must be last one
        };

        CContext(
            CDevice* pDevice,
            const TRenderingContext& kRenderingContext,
            const TWindowContext& kDefaultWindowContext,
            uint32 uIndex,
            ContextType type);
        ~CContext();

        bool Initialize();

        // Context management
        uint32 IncReservationCount() { return ++m_uReservationCount; }
        uint32 DecReservationCount() { return --m_uReservationCount; }
        const TRenderingContext& GetRenderingContext() { return m_kRenderingContext; }
        const TWindowContext& GetWindowContext() { return m_kWindowContext; }
        CDevice* GetDevice() { return m_pDevice; }
        uint32 GetIndex() { return m_uIndex; }
        ContextType GetType() const { return m_type; }
        CContext* GetReservedContext() { return m_pReservedContext; }
        void SetReservedContext(CContext* pReservedContext) { m_pReservedContext = pReservedContext; }
        void SetWindowContext(const TWindowContext& kWindowContext);

        // Explicit state
        bool SetBlendState(const SBlendState& kState);
        bool SetRasterizerState(const SRasterizerState& kState);
        bool SetDepthStencilState(const SDepthStencilState& kDepthStencilState, GLint iStencilRef);
        void SetBlendColor(GLfloat afRed, GLfloat fGreen, GLfloat fBlue, GLfloat fAlpha);
        void SetSampleMask(GLbitfield aSampleMask);
        void SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY eTopology);
        void SetViewports(uint32 uNumViewports, const D3D11_VIEWPORT* pViewports);
        void SetScissorRects(uint32 uNumRects, const D3D11_RECT* pRects);
        void SetRenderTargets(uint32 uNumRTViews, SOutputMergerView** apRenderTargetViews, SOutputMergerView* pDepthStencilView);
        void SetShader(SShader* pShader, uint32 uStage);
        void SetInputLayout(SInputLayout* pInputLayout);
        void SetVertexBuffer(uint32 uSlot, SBuffer* pVertexBuffer, uint32 uStride, uint32 uOffset);
        void SetIndexBuffer(SBuffer* pIndexBuffer, GLenum eIndexType, GLuint uIndexStride, GLuint uOffset);
        void SetShaderResourceView(SShaderView* pShaderResourceView, uint32 uStage, uint32 uIndex);
        void SetUnorderedAccessView(SShaderView* pUnorderedResourceView, uint32 uStage, uint32 uIndex);
        void SetSampler(SSamplerState* pState, uint32 uStage, uint32 uIndex);
        void SetConstantBuffer(SBuffer* pConstantBuffer, SBufferRange kRange, uint32 uStage, uint32 uIndex);

        // Implicit state
        void SetShaderTexture(SShaderTextureBasedView* pView, uint32 uStage, uint32 uIndex);
#if DXGL_SUPPORT_SHADER_IMAGES
        void SetShaderImage(SShaderImageView* pView, uint32 uStage, uint32 uIndex);
#endif
#if DXGL_SUPPORT_SHADER_STORAGE_BLOCKS
        void SetShaderBuffer(SShaderBufferView* pView, uint32 uStage, uint32 uIndex);
#endif
        void SetUnpackRowLength(GLint iValue);
        void SetUnpackImageHeight(GLint iValue);
        void SetUnpackAlignment(GLint iValue);
        void SetPackRowLength(GLint iValue);
        void SetPackImageHeight(GLint iValue);
        void SetPackAlignment(GLint iValue);
        GLenum BindBuffer(const CResourceName& kBufferName, EBufferBinding eBinding);
        GLenum BindBuffer(SBuffer* pBuffer, EBufferBinding eBinding);

        // Commands
        void ClearRenderTarget(SOutputMergerView* pRenderTargetView, const float afColor[4]);
        void ClearDepthStencil(SOutputMergerView* pDepthStencilView, bool bClearDepth, bool bClearStencil, float fDepthValue, uint8 uStencilValue);
        void DrawIndexed(uint32 uIndexCount, uint32 uStartIndexLocation, uint32 uBaseVertexLocation);
        void Draw(uint32 uVertexCount, uint32 uStartVertexLocation);
        void DrawIndexedInstanced(uint32 uIndexCountPerInstance, uint32 uInstanceCount, uint32 uStartIndexLocation, uint32 uBaseVertexLocation, uint32 uStartInstanceLocation);
        void DrawInstanced(uint32 uVertexCountPerInstance, uint32 uInstanceCount, uint32 uStartVertexLocation, uint32 uStartInstanceLocation);
#if DXGL_SUPPORT_COMPUTE
        void Dispatch(uint32 uGroupX, uint32 uGroupY, uint32 uGroupZ);
        void DispatchIndirect(uint32 uIndirectOffset);
#endif
        void Flush();

        // Cached objects
        _smart_ptr<SFrameBuffer> AllocateFrameBuffer(const SFrameBufferConfiguration& kConfiguration);
        void RemoveFrameBuffer(SFrameBuffer* pFrameBuffer, SOutputMergerView* pInvalidView);
        _smart_ptr<SPipeline> AllocatePipeline(const SPipelineConfiguration& kConfiguration);
        void RemovePipeline(SPipeline* pPipeline, SShader* pInvalidShader);
        _smart_ptr<SUnitMap> AllocateUnitMap(_smart_ptr<SUnitMap> spConfiguration);

        // Copying
        void BlitFrameBuffer(SFrameBufferObject& kSrcFBO, SFrameBufferObject& kDstFBO, GLenum eSrcColorBuffer, GLenum eDstColorBuffer, GLint iSrcXMin, GLint iSrcYMin, GLint iSrcXMax, GLint iSrcYMax, GLint iDstXMin, GLint iDstYMin, GLint iDstXMax, GLint iDstYMax, GLbitfield uMask, GLenum eFilter);
        bool BlitOutputMergerView(SOutputMergerView* pSrcView, SOutputMergerView* pDstView, GLint iSrcXMin, GLint iSrcYMin, GLint iSrcXMax, GLint iSrcYMax, GLint iDstXMin, GLint iDstYMin, GLint iDstXMax, GLint iDstYMax, GLbitfield uMask, GLenum eFilter);
        void ReadbackFrameBufferAttachment(SFrameBufferObject& kFBO, GLenum eColorBuffer, GLint iXMin, GLint iYMin, GLsizei iWidth, GLint iHeight, GLenum eBaseFormat, GLenum eDataType, GLvoid* pvData);
        bool ReadBackOutputMergerView(SOutputMergerView* pView, GLint iXMin, GLint iYMin, GLsizei iWidth, GLint iHeight, GLvoid* pvData);

#if DXGL_ENABLE_SHADER_TRACING
        void TogglePixelTracing(bool bEnable, uint32 uShaderHash, uint32 uPixelX = 0, uint32 uPixelY = 0);
        void ToggleVertexTracing(bool bEnable, uint32 uShaderHash, uint32 uVertexID = 0);
#endif

#if DXGL_TRACE_CALLS
        void CallTraceWrite(const char* szTrace);
        void CallTraceFlush();
#endif

        const CResourceName& GetCopyPixelBuffer() { AZ_Assert(m_kCopyPixelBuffer.IsValid(), "Invalid copy pixel buffer.");  return m_kCopyPixelBuffer; }

        inline void NamedBufferDataFast(CResourceName kBufferName, GLsizeiptr iSize, const void* pData, GLenum eUsage);
        inline void NamedBufferSubDataFast(CResourceName kBufferName, GLintptr iOffset, GLsizeiptr iSize, const void* pData);
        inline GLvoid* MapNamedBufferRangeFast(CResourceName kBufferName, GLintptr iOffset, GLsizeiptr iLength, GLbitfield uAccess);
        inline GLboolean UnmapNamedBufferFast(CResourceName kBufferName);

        // Confetti Begin: David Srour
        // Since binding framebuffers is deferred, we have to ensure that enabling/disabling PLS extension comes after binings.
        inline void TogglePLS(bool const enable);

        // Should only be called by DXGL layer only.
        // This should not be called directly by and should never have to be exposed
        // to the higher abstractions of the engine.
        void UpdatePlsState(bool const preFramebufferBind);
        //  Confetti End: David Srour

    private:
        bool GetBlendCache(SBlendCache& kCache);
        bool GetDepthStencilCache(SDepthStencilCache& kDepthStencilCache);
        bool GetRasterizerCache(SRasterizerCache& kCache);
        bool GetTextureUnitCache(uint32 uUnit, STextureUnitCache& kCache);
#if DXGL_SUPPORT_SHADER_IMAGES
        bool GetImageUnitCache(uint32 uUnit, SImageUnitCache& kCache);
#endif
        bool GetInputAssemblerCache(SInputAssemblerCache& kCache);
        bool GetImplicitStateCache(SImplicitStateCache& kCache);

        bool InitializePipeline(SPipeline* pPipeline);

        void BindUniformBuffer(const TIndexedBufferBinding& kBufferRange, uint32 uUnit);
#if DXGL_SUPPORT_SHADER_STORAGE_BLOCKS
        void BindStorageBuffer(const TIndexedBufferBinding& kBufferRange, uint32 uUnit);
#endif
#if DXGL_SUPPORT_SHADER_IMAGES
        void BindImage(const CResourceName& kName, SShaderImageViewConfiguration kConfiguration, uint32 uUnit);
#endif

        void BindDrawFrameBuffer(const CResourceName& kName);
        void BindReadFrameBuffer(const CResourceName& kName);
        void SetNumPatchControlPoints(GLint uNumControlPoints);
#if !DXGL_SUPPORT_DRAW_WITH_BASE_VERTEX
        void SetVertexOffset(uint32 uVertexOffset);
#endif

        void FlushDrawState();
#if DXGL_SUPPORT_VERTEX_ATTRIB_BINDING
        void FlushInputAssemblerStateVab();
#endif
        void FlushInputAssemblerState();
        void FlushFrameBufferState();
        void FlushPipelineState();
#if DXGL_SUPPORT_COMPUTE
        void FlushDispatchState();
#endif
        void FlushTextureUnits();
        void FlushUniformBufferUnits();
#if DXGL_SUPPORT_SHADER_STORAGE_BLOCKS
        void FlushStorageBufferUnits();
#endif
#if DXGL_SUPPORT_SHADER_IMAGES
        void FlushImageUnits();
#endif
        void FlushResourceUnits();
#if DXGL_ENABLE_SHADER_TRACING
        void FlushShaderTracingState();
#endif

#if defined(ANDROID)
        void FlushFrameBufferDontCareState(bool bOnBind);
#endif
#if defined(DXGL_USE_LAZY_CLEAR)
        void FlushFrameBufferLazyClearState();
#endif

        void SwitchFrame();

#if DXGL_ENABLE_SHADER_TRACING
        void PrepareTraceHeader(uint32 uFirstVertex, uint32 uFirstIndex);
        void BeginTrace(uint32 uFirstVertex, uint32 uFirstIndex);
        void EndTrace();
#endif

        void OnApplicationWindowCreated() override;
        void OnApplicationWindowDestroy() override;

    protected:
        typedef AZStd::pair<uint32, ColorF> ClearColorArg;
        void ClearDepthStencilInternal(bool clearDepth, bool clearStencil, float depthValue, uint8 stencilValue);
        void ClearRenderTargetInternal(const AZStd::vector<ClearColorArg>& args);

    private:
        uint32 m_uIndex;
        CContext* m_pReservedContext;
        CDevice* m_pDevice;
        TRenderingContext m_kRenderingContext;
        TWindowContext m_kWindowContext;
        uint32 m_uReservationCount;

        SStateCache m_kStateCache;
        GLenum m_ePrimitiveTopologyMode;
        GLenum m_eIndexType;
        GLuint m_uIndexStride;
        GLuint m_uIndexOffset;
        SPipelinePtr m_spPipeline;
        SFrameBufferPtr m_spFrameBuffer;
        GLuint m_uGlobalVAO;
        STextureUnitContext m_kTextureUnitContext;

        // State that is only synchronized during draw calls
        SFrameBufferConfiguration m_kFrameBufferConfig;
        SPipelineConfiguration m_kPipelineConfiguration;
        SInputLayout* m_pInputLayout;
        SInputAssemblerSlot m_akInputAssemblerSlots[MAX_VERTEX_ATTRIBUTES];
        STextureSlot m_akTextureSlots[MAX_TEXTURE_SLOTS];
        SSamplerSlot m_akSamplerSlots[MAX_SAMPLER_SLOTS];
#if DXGL_SUPPORT_SHADER_IMAGES
        SImageUnitCache m_akImageSlots[MAX_IMAGE_SLOTS];
#endif
#if DXGL_SUPPORT_SHADER_STORAGE_BLOCKS
        TIndexedBufferBinding m_akStorageBufferSlots[MAX_STORAGE_BUFFER_SLOTS];
#endif
        TConstantBufferSlot m_akConstantBufferSlots[MAX_CONSTANT_BUFFER_SLOTS];
        SUnitMap* m_apResourceUnitMaps[eRUT_NUM];
#if DXGL_STREAMING_CONSTANT_BUFFERS
        SStreamingBufferContext m_kStreamingBuffers;
#endif
#if !DXGL_SUPPORT_DRAW_WITH_BASE_VERTEX
        uint32 m_uVertexOffset;
#endif

        // Flags that tell which parts of the state need to be synchronized during the next draw call
        bool m_bFrameBufferStateDirty;
        bool m_bPipelineDirty;
        bool m_bInputLayoutDirty;
        bool m_bInputAssemblerSlotsDirty;
        bool m_abResourceUnitsDirty[eRUT_NUM];

        // Confetti Begin: David Srour
        // Since binding framebuffers is deferred, we have to ensure that :
        // - enabling PLS extension comes after biding FBO.
        // - disabling PLS extension before biding FBO.
        enum PLS_STATE
        {
            PLS_IGNORE = 0,
            PLS_ENABLE,
            PLS_DISABLE
        };
        PLS_STATE m_PlsExtensionState;
        // Confetti End: David Srour

#if DXGL_ENABLE_SHADER_TRACING
        CResourceName m_kShaderTracingBuffer;
        TIndexedBufferBinding m_kReplacedStorageBuffer;
        EShaderType m_eStageTracing;
        uint32 m_uShaderTraceHash;
        uint32 m_uShaderTraceCount;
        union
        {
            SVertexShaderTraceInfo   m_kVertex;
            SFragmentShaderTraceInfo m_kFragment;
        } m_kStageTracingInfo;
#endif //DXGL_ENABLE_SHADER_TRACING

#if DXGL_TRACE_CALLS
        STraceFile m_kCallTrace;
#endif

        CResourceName m_kCopyPixelBuffer;
        SPipelineCompilationBuffer m_kPipelineCompilationBuffer;

        // Hash maps of persistent frame buffers, pipelines and sampler unit maps that can be re-used every time
        // a compatible configuration is requested.
        struct SFrameBufferCache* m_pFrameBufferCache;
        struct SPipelineCache* m_pPipelineCache;
        struct SUnitMapCache* m_pUnitMapCache;

        ContextType m_type;

    public:
        // Blit a texture into a framebuffer using a shader instead of the glBlitFramebuffer function.
        // If possible use Context::BlitFrameBuffer instead.
        bool BlitTextureToFrameBuffer(SShaderTextureView* srcTexture,
                                        SFrameBufferObject& kDstFBO,
                                        GLenum eDstColorBuffer,
                                        GLint iSrcXMin, GLint iSrcYMin, GLint iSrcXMax, GLint iSrcYMax,
                                        GLint iDstXMin, GLint iDstYMin, GLint iDstXMax, GLint iDstYMax,
                                        GLenum minFilter, GLenum magFilter) 
        {
            return m_blitHelper.BlitTexture(srcTexture, kDstFBO, eDstColorBuffer, 
                                            iSrcXMin, iSrcYMin, iSrcXMax, iSrcYMax, 
                                            iDstXMin, iDstYMin, iDstXMax, iDstYMax, 
                                            minFilter, magFilter);
        }

    private:
        friend class GLBlitFramebufferHelper;
        GLBlitFramebufferHelper m_blitHelper;
    };

    template <typename T>
    inline bool RefreshCache(T& kCache, T kState)
    {
        bool bDirty(kCache != kState);
        kCache = kState;
        return bDirty;
    }

#define CACHE_VAR(_Cache, _State) RefreshCache(_Cache, _State)
#define CACHE_FIELD(_Cache, _State, _Member) RefreshCache((_Cache)._Member, (_State)._Member)

    inline void CContext::NamedBufferDataFast(CResourceName kBufferName, GLsizeiptr iSize, const void* pData, GLenum eUsage)
    {
#if defined(glNamedBufferDataEXT) || !defined(USE_FAST_NAMED_APPROXIMATION)
        glNamedBufferDataEXT(kBufferName.GetName(), iSize, pData, eUsage);
#else
        if (CACHE_VAR(m_kStateCache.m_akBuffersBound[eBB_CopyWrite], kBufferName))
        {
            glBindBuffer(GL_COPY_WRITE_BUFFER, kBufferName.GetName());
        }

        glBufferData(GL_COPY_WRITE_BUFFER, iSize, pData, eUsage);
#endif
    }

    inline void CContext::NamedBufferSubDataFast(CResourceName kBufferName, GLintptr iOffset, GLsizeiptr iSize, const void* pData)
    {
#if defined(glNamedBufferSubDataEXT) || !defined(USE_FAST_NAMED_APPROXIMATION)
        glNamedBufferSubDataEXT(kBufferName.GetName(), iOffset, iSize, pData);
#else
        if (CACHE_VAR(m_kStateCache.m_akBuffersBound[eBB_CopyWrite], kBufferName))
        {
            glBindBuffer(GL_COPY_WRITE_BUFFER, kBufferName.GetName());
        }

        glBufferSubData(GL_COPY_WRITE_BUFFER, iOffset, iSize, pData);
#endif
    }

    inline GLvoid* CContext::MapNamedBufferRangeFast(CResourceName kBufferName, GLintptr iOffset, GLsizeiptr iLength, GLbitfield uAccess)
    {
#if defined(glMapNamedBufferRangeEXT) || !defined(USE_FAST_NAMED_APPROXIMATION)
        return glMapNamedBufferRangeEXT(kBufferName.GetName(), iOffset, iLength, uAccess);
#else
        if (CACHE_VAR(m_kStateCache.m_akBuffersBound[eBB_CopyWrite], kBufferName))
        {
            glBindBuffer(GL_COPY_WRITE_BUFFER, kBufferName.GetName());
        }

        return glMapBufferRange(GL_COPY_WRITE_BUFFER, iOffset, iLength, uAccess);
#endif
    }

    inline GLboolean CContext::UnmapNamedBufferFast(CResourceName kBufferName)
    {
#if defined(glUnmapNamedBufferEXT) || !defined(USE_FAST_NAMED_APPROXIMATION)
        return glUnmapNamedBufferEXT(kBufferName.GetName());
#else
        if (CACHE_VAR(m_kStateCache.m_akBuffersBound[eBB_CopyWrite], kBufferName))
        {
            glBindBuffer(GL_COPY_WRITE_BUFFER, kBufferName.GetName());
        }

        return glUnmapBuffer(GL_COPY_WRITE_BUFFER);
#endif
    }

    inline void CContext::TogglePLS(bool const enable)
    {
        // 0 = don't do anything, 1 = enable, 2 = disable
        // Toggling the extension must be done in a deferred manner.  See m_PlsExtensionState' decleration for further details.
        if (enable)
        {
            m_PlsExtensionState = PLS_ENABLE;
        }
        else
        {
            m_PlsExtensionState = PLS_DISABLE;
        }
    }
}

#endif

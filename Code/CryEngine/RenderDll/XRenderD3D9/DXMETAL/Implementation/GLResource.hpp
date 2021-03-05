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

// Description : Declares the resource types and related functions

#ifndef __GLRESOURCE__
#define __GLRESOURCE__

#include "GLCommon.hpp"
#include "GLFormat.hpp"

//  Confetti BEGIN: Igor Lobanchikov
//@protocol MTLTexture;
@protocol MTLBuffer;

#import "Metal/MTLTexture.h"
#import "Metal/MTLRenderPass.h"
//  Confetti End: Igor Lobanchikov

namespace NCryMetal
{
    class CDevice;
    class CContext;
    struct SSamplerState;
    struct SInputAssemblerSlot;

    DXGL_DECLARE_PTR(struct, SOutputMergerView);
    DXGL_DECLARE_PTR(struct, SOutputMergerTextureView);
    DXGL_DECLARE_PTR(struct, STexture);
    DXGL_DECLARE_PTR(struct, SShaderResourceView);
    DXGL_DECLARE_PTR(struct, SShaderTextureView);
    DXGL_DECLARE_PTR(struct, SShaderBufferView);
    DXGL_DECLARE_PTR(struct, SDefaultOuputMergerTextureView);
    DXGL_DECLARE_PTR(struct, SDefaultFrameBufferTexture);
    DXGL_DECLARE_PTR(struct, SBuffer);
    DXGL_DECLARE_PTR(struct, SQuery);

    DXGL_DECLARE_REF_COUNTED(struct, SResource)
    {
        typedef void (* UpdateSubresourceFunc) (SResource* pResource, uint32 uSubresource, const D3D11_BOX* pDstBox, const void* pSrcData, uint32 uSrcRowPitch, uint32 uSrcDepthPitch, CContext* pContext);
        typedef bool (* MapSubresourceFunc) (SResource* pResource, uint32 uSubresource, D3D11_MAP MapType, UINT MapFlags, D3D11_MAPPED_SUBRESOURCE* pMappedResource, CContext* pContext);
        typedef void (* UnmapSubresourceFunc) (SResource* pResource, uint32 uSubresource, CContext* pContext);

        UpdateSubresourceFunc m_pfUpdateSubresource;
        MapSubresourceFunc m_pfMapSubresource;
        UnmapSubresourceFunc m_pfUnmapSubresource;

        SResource();
        SResource(const SResource&kOther);

        virtual ~SResource();
    };

    struct SMappedSubTexture
    {
        uint8* m_pBuffer;
        uint32 m_uDataOffset;
        bool m_bUpload;
        uint32 m_uRowPitch;
        uint32 m_uImagePitch;

        SMappedSubTexture()
            : m_pBuffer(NULL)
        {
        }
    };

    struct SShaderBufferViewConfiguration
    {
        EGIFormat m_eFormat;
        MTLTextureType  m_eViewType;
        uint32 m_uMinMipLevel;
        uint32 m_uNumMipLevels;
        uint32 m_uMinLayer;
        uint32 m_uNumLayers;

        SShaderBufferViewConfiguration(EGIFormat eFormat, MTLTextureType eViewType, uint32 uMinMipLevel, uint32 uNumMipLevels, uint32 uMinLayer, uint32 uNumLayers)
            : m_eFormat(eFormat)
            , m_eViewType(eViewType)
            , m_uMinMipLevel(uMinMipLevel)
            , m_uNumMipLevels(uNumMipLevels)
            , m_uMinLayer(uMinLayer)
            , m_uNumLayers(uNumLayers)
        {
        }

        bool operator==(const SShaderBufferViewConfiguration& kOther)
        {
            return
                m_eFormat == kOther.m_eFormat &&
                m_eViewType == kOther.m_eViewType &&
                m_uMinMipLevel == kOther.m_uMinMipLevel &&
                m_uNumMipLevels == kOther.m_uNumMipLevels &&
                m_uMinLayer == kOther.m_uMinLayer &&
                m_uNumLayers == kOther.m_uNumLayers;
        }
    };

    struct SShaderTextureViewConfiguration
    {
        EGIFormat m_eFormat;
        MTLTextureType  m_eViewType;
        uint32 m_uMinMipLevel;
        uint32 m_uNumMipLevels;
        uint32 m_uMinLayer;
        uint32 m_uNumLayers;

        SShaderTextureViewConfiguration(EGIFormat eFormat, MTLTextureType eViewType, uint32 uMinMipLevel, uint32 uNumMipLevels, uint32 uMinLayer, uint32 uNumLayers)
            : m_eFormat(eFormat)
            , m_eViewType(eViewType)
            , m_uMinMipLevel(uMinMipLevel)
            , m_uNumMipLevels(uNumMipLevels)
            , m_uMinLayer(uMinLayer)
            , m_uNumLayers(uNumLayers)
        {
        }

        bool operator==(const SShaderTextureViewConfiguration& kOther)
        {
            return
                m_eFormat == kOther.m_eFormat &&
                m_eViewType == kOther.m_eViewType &&
                m_uMinMipLevel == kOther.m_uMinMipLevel &&
                m_uNumMipLevels == kOther.m_uNumMipLevels &&
                m_uMinLayer == kOther.m_uMinLayer &&
                m_uNumLayers == kOther.m_uNumLayers;
        }
    };

    struct SOutputMergerTextureViewConfiguration
    {
        EGIFormat m_eFormat;
        uint32 m_uMipLevel;
        uint32 m_uMinLayer;
        uint32 m_uNumLayers;

        SOutputMergerTextureViewConfiguration(EGIFormat eFormat, uint32 uMipLevel, uint32 uMinLayer, uint32 uNumLayers)
            : m_eFormat(eFormat)
            , m_uMipLevel(uMipLevel)
            , m_uMinLayer(uMinLayer)
            , m_uNumLayers(uNumLayers)
        {
        }

        bool operator==(const SOutputMergerTextureViewConfiguration& kOther)
        {
            return
                m_eFormat == kOther.m_eFormat &&
                m_uMipLevel == kOther.m_uMipLevel &&
                m_uMinLayer == kOther.m_uMinLayer &&
                m_uNumLayers == kOther.m_uNumLayers;
        }
    };

    template <typename T>
    struct STexVec
    {
        T x, y, z;

        STexVec() {}
        STexVec(T kX, T kY, T kZ)
            : x(kX)
            , y(kY)
            , z(kZ) {}
    };

    typedef STexVec<int32> STexSize;
    typedef STexVec<int32> STexPos;

    struct STexSubresourceID
    {
        int32 m_iMipLevel;
        uint32 m_uElement;
    };

    enum EBufferBinding
    {
        eBB_Array,
        eBB_ElementArray,
#if DXGL_SUPPORT_TEXTURE_BUFFERS
        eBB_Texture,
#endif //DXGL_SUPPORT_TEXTURE_BUFFERS
        eBB_UniformBuffer,
#if DXGL_SUPPORT_SHADER_STORAGE_BLOCKS
        eBB_ShaderStorage,
#endif //DXGL_SUPPORT_SHADER_STORAGE_BLOCKS
        eBB_NUM
    };

    struct STexture
        : SResource
    {
        STexture(int32 iWidth, int32 iHeight, int32 iDepth, MTLTextureType eTextureType, EGIFormat eFormat, uint32 uNumMipLevels, uint32 uNumElements);
        virtual ~STexture();

        virtual SShaderTextureViewPtr CreateShaderView(const SShaderTextureViewConfiguration& kConfiguration, CDevice* pDevice);
        virtual SOutputMergerTextureViewPtr CreateOutputMergerView(const SOutputMergerTextureViewConfiguration& kConfiguration, CDevice* pDevice);
        SOutputMergerTextureViewPtr GetCompatibleOutputMergerView(const SOutputMergerTextureViewConfiguration& kConfiguration, CDevice* pDevice);

        void ResetDontCareActionFlags();

        typedef std::vector<SMappedSubTexture> TMappedSubTextures;

        int32 m_iWidth, m_iHeight, m_iDepth;
        MTLTextureType  m_eTextureType;
        EGIFormat m_eFormat;
        uint32 m_uNumMipLevels;
        uint32 m_uNumElements; // array_size * number_of_faces

        TMappedSubTextures m_kMappedSubTextures;

        SShaderTextureView* m_pShaderViewsHead;
        SOutputMergerTextureView* m_pOutputMergerViewsHead;
        SShaderResourceView* m_pBoundModifier; // NULL if no SRV for this texture has bound custom texture parameters

        //  Confetti BEGIN: Igor Lobanchikov
        id<MTLTexture>                  m_Texture;
        id<MTLTexture>                  m_StencilTexture;
        double                          m_ClearColor[4];
        _smart_ptr<SOutputMergerView>   m_spTextureViewToClear;
        _smart_ptr<SOutputMergerView>   m_spStencilTextureViewToClear;
        float                           m_fClearDepthValue;
        bool                            m_bClearDepth;
        bool                            m_bClearStencil;
        uint8                           m_uClearStencilValue;

        uint8*                          m_pMapMemoryCopy;

        bool                            m_bBackBuffer;


        bool m_bColorLoadDontCare;
        bool m_bDepthLoadDontCare;
        bool m_bStencilLoadDontCare;
        bool m_bColorStoreDontCare;
        bool m_bDepthStoreDontCare;
        bool m_bStencilStoreDontCare;
        //  Confetti End: Igor Lobanchikov
    };

    DXGL_DECLARE_REF_COUNTED(struct, SShaderResourceView)
    {
        EGIFormat m_eFormat;

        SShaderResourceView(EGIFormat eFormat);
        virtual ~SShaderResourceView();

        virtual bool GenerateMipmaps(CContext * pContext);

        //  Confetti BEGIN: Igor Lobanchikov
        virtual id<MTLTexture>  GetMetalTexture() { CRY_ASSERT(0); return 0; }
        //  Confetti End: Igor Lobanchikov
        virtual id<MTLBuffer>   GetMetalBuffer() { CRY_ASSERT(0); return 0; }
    };

    struct SShaderTextureView
        : SShaderResourceView
    {
        SShaderTextureView(STexture* pTexture, const SShaderTextureViewConfiguration& kConfiguration);
        virtual ~SShaderTextureView();

        bool Init(CDevice* pDevice);
        bool CreateUniqueView(CDevice* pDevice);

        virtual bool GenerateMipmaps(CContext* pContext);

        STexture* m_pTexture;
        SShaderTextureViewConfiguration m_kConfiguration;
        SShaderTextureView* m_pNextView;

        //  Confetti BEGIN: Igor Lobanchikov
        id<MTLTexture>  m_TextureView;
        virtual id<MTLTexture>  GetMetalTexture() { return m_TextureView ? m_TextureView : m_pTexture->m_Texture; }
        //  Confetti End: Igor Lobanchikov
    };

    DXGL_DECLARE_REF_COUNTED(struct, SOutputMergerView)
    {
        SOutputMergerView(EGIFormat eFormat);
        virtual ~SOutputMergerView();

        EGIFormat m_eFormat;

        //  Confetti BEGIN: Igor Lobanchikov
        virtual SOutputMergerTextureView* AsSOutputMergerTextureView() { return NULL; }
        //  Confetti End: Igor Lobanchikov
    };

    struct SOutputMergerTextureView
        : SOutputMergerView
    {
        SOutputMergerTextureView(STexture* pTexture, const SOutputMergerTextureViewConfiguration& kConfiguration);
        virtual ~SOutputMergerTextureView();

        bool Init(CDevice* pDevice);

        virtual bool CreateUniqueView(const SGIFormatInfo* pFormatInfo, CDevice* pDevice);

        STexture* m_pTexture;
        SOutputMergerTextureViewConfiguration m_kConfiguration;
        SOutputMergerTextureView* m_pNextView;
        int32 m_iMipLevel;
        int32 m_iLayer; //INVALID_LAYER for all layers

        static const int32 INVALID_LAYER;

        //  Confetti BEGIN: Igor Lobanchikov
        id<MTLTexture>      m_RTView;
        virtual SOutputMergerTextureView* AsSOutputMergerTextureView() { return this; }
        //  Confetti End: Igor Lobanchikov
    };

    enum EBufferUsage
    {
        eBU_Default,      //  Generic behaviour. Slow but safe. GPU is used to update the buffer content for dynamic buffers.
        eBU_DirectAccess, //  Engine can accesse buffer content directly. Engine is responsible for all CPU-GPU syncronizations.
        eBU_MapInRingBufferTTLFrame, //  Buffer content is mapped in the ring buffer. This allows to use buffer content during the frame it was mapped only. This is super fast and used for constant buffers

        eBU_MapInRingBufferTTLOnce, //  Buffer holds a reference to the content of the last map in the ring buffer. This allows to use the buffer content once (or untill the next map). This is super fast and used for font rendering and other dynamically generated content
    };

    //The enums that define the type of memory storage
    enum MemRingBufferStorage
    {
        MEM_SHARED_RINGBUFFER,  //This is CPU/GPU shared memory. Available to IOS and OSX.
#if defined(AZ_PLATFORM_MAC)
        MEM_MANAGED_RINGBUFFER,   //OSX supports Managed memory which is faster than shared memory because it creates a seprate copy on the GPU and uses didModifyRange to synchronise with CPU.
#endif
    };
    
    struct SBuffer
        : SResource
    {
        typedef bool (* MapBufferRangeFunc) (SBuffer* pBuffer, size_t uOffset, size_t uSize, D3D11_MAP MapType, UINT MapFlags, D3D11_MAPPED_SUBRESOURCE* pMappedResource, CContext* pContext);

        SBuffer();
        virtual ~SBuffer();

        MapBufferRangeFunc m_pfMapBufferRange;

        SBitMask<eBB_NUM, SUnsafeBitMaskWord>::Type m_kBindings;
        EBufferUsage m_eUsage;
        uint32 m_uSize;
        uint8* m_pSystemMemoryCopy;
        bool m_bMapped;
        size_t m_uMapOffset;
        size_t m_uMapSize;

        //  Confetti BEGIN: Igor Lobanchikov

        id<MTLBuffer>   m_BufferShared;
#if defined(AZ_PLATFORM_MAC)
        id<MTLBuffer>   m_BufferManaged; //Pointer to the fast buffer
#endif
        void*           m_pMappedData;

        /* If popTransientMappedDataQueue = true, function will pop front of
         * m_pTransientMappedData so that next call to GetBufferAndOffset can
         * calculate the correct offset for next transient buffer to be bound.
         */
        bool GetBufferAndOffset(CContext& context,
            uint32 uInputBufferOffset,
            uint32 uBaseOffset,
            uint32 uBaseStride,
            id<MTLBuffer>& buffer,
            uint32& offset,
            bool const popTransientMappedDataQueue = true);
        //  Confetti END: Igor Lobanchikov

        // Confetti BEGIN: David Srour
        /* Some passes like forward decals will cause the engine to map/unbind the
         * same dynamic transient buffer multiple times.
         * We must keep a list of offsets to ensure they are calculated appropriately in
         * CContext::FlushInputAssemblerState().
         *
         * NOTE: THE ENGINE MUST MAP/UNMAP BUFFERS IN BINDING ORDER!
         * Member "m_pMappedData" should equal the last mapped buffer in m_pTransientMappedData.
         *
         */
        std::deque<void*> m_pTransientMappedData; // FIFO order based on map/unmap
        // Confetti END: David Srour
        virtual SShaderBufferViewPtr CreateShaderView(const SShaderBufferViewConfiguration& kConfiguration, CDevice* pDevice);
    };

    struct SShaderBufferView
        : SShaderResourceView
    {
        SShaderBufferView(SBuffer* pBuffer, const SShaderBufferViewConfiguration& kConfiguration);
        virtual ~SShaderBufferView();

        bool Init(CDevice* pDevice);

        virtual bool GenerateMipmaps(CContext* pContext);

        SBuffer* m_pBuffer;
        SShaderBufferViewConfiguration m_kConfiguration;
        //SShaderBufferView* m_pNextView;

        id<MTLBuffer>  m_BufferView;
        virtual id<MTLBuffer>  GetMetalBuffer();
    };

    DXGL_DECLARE_REF_COUNTED(struct, SQuery)
    {
        virtual ~SQuery();

        //  Confetti BEGIN: Igor Lobanchikov
        virtual void Begin(CContext * pContext);
        virtual void End(CContext * pContext);
        //  Confetti End: Igor Lobanchikov

        virtual bool GetData(void* pData, uint32 uDataSize, bool bFlush) = 0;
        virtual uint32 GetDataSize();

        virtual bool IsBufferSubmitted() = 0;
    };

    struct SPlainQuery
        : SQuery
    {
        SPlainQuery();
        virtual ~SPlainQuery();
        //  Confetti BEGIN: Igor Lobanchikov
        virtual void Begin(CContext* pContext);
        virtual void End(CContext* pContext);
        //  Confetti End: Igor Lobanchikov
    };

    //  Confetti BEGIN: Igor Lobanchikov
    struct SContextEventHelper
    {
        volatile bool bCommandBufferPreSubmitted;
        volatile bool bCommandBufferSubmitted;
        volatile bool bTriggered;
    };
    //  Confetti End: Igor Lobanchikov

    //  Confetti BEGIN: Igor Lobanchikov
    struct SOcclusionQuery
        : SQuery
    {
        SOcclusionQuery();

        virtual void Begin(CContext* pContext);
        virtual void End(CContext* pContext);

        virtual bool GetData(void* pData, uint32 uDataSize, bool bFlush);
        virtual uint32 GetDataSize();

        virtual bool IsBufferSubmitted() {CRY_ASSERT(m_pEventHelper); return m_pEventHelper->bCommandBufferSubmitted; }

        SContextEventHelper* m_pEventHelper;
        uint64* m_pQueryData;
    };
    //  Confetti End: Igor Lobanchikov

    struct SFenceSync
        : SQuery
    {
        SFenceSync();
        virtual ~SFenceSync();

        //  Confetti BEGIN: Igor Lobanchikov
        virtual void End(CContext* pContext);
        //  Confetti End: Igor Lobanchikov
        virtual bool GetData(void* pData, uint32 uDataSize, bool bFlush);
        virtual uint32 GetDataSize();

        virtual bool IsBufferSubmitted() {CRY_ASSERT(m_pEventHelper); return m_pEventHelper->bCommandBufferSubmitted; }

        //  Confetti BEGIN: Igor Lobanchikov
        SContextEventHelper* m_pEventHelper;
        //  Confetti End: Igor Lobanchikov
    };

    struct SDefaultFrameBufferTexture
        : STexture
    {
        SDefaultFrameBufferTexture(int32 iWidth, int32 iHeight, EGIFormat eFormat);

        virtual SShaderTextureViewPtr CreateShaderView(const SShaderTextureViewConfiguration& kConfiguration, CDevice* pDevice);
        virtual SOutputMergerTextureViewPtr CreateOutputMergerView(const SOutputMergerTextureViewConfiguration& kConfiguration, CDevice* pDevice);

        static void UpdateSubresource(SResource* pResource, uint32 uSubresource, const D3D11_BOX* pDstBox, const void* pSrcData, uint32 uSrcRowPitch, uint32 uSrcDepthPitch, CContext* pContext);
        static bool MapSubresource(SResource* pResource, uint32 uSubresource, D3D11_MAP MapType, UINT MapFlags, D3D11_MAPPED_SUBRESOURCE* pMappedResource, CContext* pContext);
        static void UnmapSubresource(SResource* pResource, uint32 uSubresource, CContext* pContext);
    };

    STexturePtr CreateTexture1D(const D3D11_TEXTURE1D_DESC& kDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, CDevice* pDevice /*CContext* pContext*/);
    STexturePtr CreateTexture2D(const D3D11_TEXTURE2D_DESC& kDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, CDevice* pDevice /*CContext* pContext*/);
    STexturePtr CreateTexture3D(const D3D11_TEXTURE3D_DESC& kDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, CDevice* pDevice /*CContext* pContext*/);
    SBufferPtr CreateBuffer(const D3D11_BUFFER_DESC& kDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, CDevice* pDevice /*CContext* pContext*/);
    SShaderResourceViewPtr CreateShaderResourceView(SResource* pResource, D3D11_RESOURCE_DIMENSION eDimension, const D3D11_SHADER_RESOURCE_VIEW_DESC& kViewDesc, CDevice* pDevice /*CContext* pContext*/);
    SOutputMergerViewPtr CreateRenderTargetView(SResource* pResource, D3D11_RESOURCE_DIMENSION eDimension, const D3D11_RENDER_TARGET_VIEW_DESC& kViewDesc, CDevice* pDevice /*CContext* pContext*/);
    SOutputMergerViewPtr CreateDepthStencilView(SResource* pResource, D3D11_RESOURCE_DIMENSION eDimension, const D3D11_DEPTH_STENCIL_VIEW_DESC& kViewDesc, CDevice* pDevice /*CContext* pContext*/);
    SQueryPtr CreateQuery(const D3D11_QUERY_DESC& kDesc, CDevice* pDevice /*CContext* pContext*/);
    SDefaultFrameBufferTexturePtr CreateBackBufferTexture(const D3D11_TEXTURE2D_DESC& kDesc);

    void CopyTexture(STexture* pDstTexture, STexture* pSrcTexture, CContext* pContext);
    void CopyBuffer(SBuffer* pDstBuffer, SBuffer* pSrcBuffer, CContext* pContext);
    void CopySubTexture(STexture* pDstTexture, uint32 uDstSubresource, uint32 uDstX, uint32 uDstY, uint32 uDstZ, STexture* pSrcTexture, uint32 uSrcSubresource, const D3D11_BOX* pSrcBox, CContext* pContext);
    void CopySubBuffer(SBuffer* pDstBuffer, uint32 uDstSubresource, uint32 uDstX, uint32 uDstY, uint32 uDstZ, SBuffer* pSrcBuffer, uint32 uSrcSubresource, const D3D11_BOX* pSrcBox, CContext* pContext);
    
    MemRingBufferStorage GetMemAllocModeBasedOnSize(const size_t size);
    id<MTLBuffer> GetMtlBufferBasedOnSize(const SBuffer* pBuffer);

}

#endif //__GLRESOURCE__

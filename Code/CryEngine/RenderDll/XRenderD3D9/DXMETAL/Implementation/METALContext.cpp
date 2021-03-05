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

// Description : Implementation of the type CContext.

#include "RenderDll_precompiled.h"
#include "METALContext.hpp"
#include "MetalDevice.hpp"
#include "METALCopyShaders.h"

#include "GLShader.hpp"
#include "GLExtensions.hpp"
#include "RenderCapabilities.h"

namespace NCryMetal
{
    static const int QUERY_SIZE = 8; // Size to store the query result data (64 bits)
    ////////////////////////////////////////////////////////////////////////////
    // Cache of heavy weight objects indexed by configuration
    ////////////////////////////////////////////////////////////////////////////

    struct SPipelineCache
    {
        struct SHashCompare
        {
            size_t operator()(const SPipelineConfiguration& kConfig) const
            {
                //  Confetti BEGIN: Igor Lobanchikov
                uint32 hash = [(MTLVertexDescriptor*)kConfig.m_pVertexDescriptor hash];
                hash = (uint32)NCryMetal::GetCRC32(reinterpret_cast<const char*>(kConfig.m_apShaders), sizeof(kConfig.m_apShaders), hash);
                hash = (uint32)NCryMetal::GetCRC32(reinterpret_cast<const char*>(&kConfig.m_AttachmentConfiguration), sizeof(kConfig.m_AttachmentConfiguration), hash);

                return (size_t)hash;
                //  Confetti End: Igor Lobanchikov
            }

            bool operator()(const SPipelineConfiguration& kLeft, const SPipelineConfiguration& kRight) const
            {
                //  Confetti BEGIN: Igor Lobanchikov
                {
                    int iCmpShaders(memcmp(kLeft.m_apShaders, kRight.m_apShaders, sizeof(kLeft.m_apShaders)));

                    if (iCmpShaders != 0)
                    {
                        return false;
                    }
                }

                {
                    int iCmpAttachmments(memcmp(&kLeft.m_AttachmentConfiguration, &kRight.m_AttachmentConfiguration, sizeof(kLeft.m_AttachmentConfiguration)));

                    if (iCmpAttachmments != 0)
                    {
                        return false;
                    }
                }

                //  Igor: Metal has 31 of those
                for (int i = 0; i < 31; ++i)
                {
                    MTLVertexAttributeDescriptor* left = ((MTLVertexDescriptor*)kLeft.m_pVertexDescriptor).attributes[i];
                    MTLVertexAttributeDescriptor* right = ((MTLVertexDescriptor*)kRight.m_pVertexDescriptor).attributes[i];
                    if (left.format != right.format)
                    {
                        return false;
                    }

                    if (left.offset != right.offset)
                    {
                        return false;
                    }

                    if (left.bufferIndex != right.bufferIndex)
                    {
                        return false;
                    }
                }

                //  Igor: Metal has 31 of those
                for (int i = 0; i < 31; ++i)
                {
                    MTLVertexBufferLayoutDescriptor* left = ((MTLVertexDescriptor*)kLeft.m_pVertexDescriptor).layouts[i];
                    MTLVertexBufferLayoutDescriptor* right = ((MTLVertexDescriptor*)kRight.m_pVertexDescriptor).layouts[i];
                    if (left.stepFunction != right.stepFunction)
                    {
                        return false;
                    }

                    if (left.stepRate != right.stepRate)
                    {
                        return false;
                    }

                    if (left.stride != right.stride)
                    {
                        return false;
                    }
                }

                return true;
                //  Confetti End: Igor Lobanchikov
            }
        };

        typedef AZStd::unordered_map<SPipelineConfiguration, SPipelinePtr, SHashCompare, SHashCompare> TMap;

        TMap m_kMap;
    };

    void LogCommandBufferError(int errorCode)
    {
        switch (errorCode)
        {
            case MTLCommandBufferErrorNone:
                break;
            case MTLCommandBufferErrorInternal:
                DXGL_ERROR("Internal error has occurred");
                break;
            case MTLCommandBufferErrorTimeout:
                CryLog("Execution of this command buffer took more time than system allows. execution interrupted and aborted.");
                break;
            case MTLCommandBufferErrorPageFault:
                DXGL_ERROR("Execution of this command generated an unserviceable GPU page fault. This error maybe caused by buffer read/write attribute mismatch or outof boundary access");
                break;
            case MTLCommandBufferErrorBlacklisted:
                DXGL_ERROR("Access to this device has been revoked because this client has been responsible for too many timeouts or hangs");
                break;
            case MTLCommandBufferErrorNotPermitted:
                DXGL_ERROR("This process does not have aceess to use device");
                break;
            case MTLCommandBufferErrorOutOfMemory:
                DXGL_ERROR("Insufficient memory");
                break;
            case MTLCommandBufferErrorInvalidResource:
                DXGL_ERROR("The command buffer referenced an invlid resource. This error is most commonly caused when caller deletes a resource before executing a command buffer that refers to it");
                break;
            default:
                DXGL_ERROR("Unknown error status was set on the command buffer.");
                break;
        }
    }

    //  Confetti BEGIN: Igor Lobanchikov
    CContext::CRingBuffer::CRingBuffer(id<MTLDevice> device, unsigned int bufferSize, unsigned int alignment, MemRingBufferStorage memAllocMode)
        : m_uiFreePositionPointer(0)
        , m_uiDefaultAlignment(alignment)
    {
        memset(m_BufferUsedPerFrame, 0, sizeof(m_BufferUsedPerFrame));
        memset(m_BufferPadPerFrame, 0, sizeof(m_BufferPadPerFrame));

        
        if (memAllocMode == MEM_SHARED_RINGBUFFER)
        {
            //  Igor: write combined is used so never write then read from this buffer. This won't work. This behaviour is actually
            //  similar to DX11 implementation of write-only buffers. So just don't do anything fancy on metal and it will be fine.
            m_Buffer = [device newBufferWithLength:bufferSize options:MTLCPUCacheModeWriteCombined|MTLResourceStorageModeShared ];  //Use shared memory
        }
#if defined(AZ_PLATFORM_MAC)
        else
        {
            //Use Managed memory.
            m_Buffer = [device newBufferWithLength:bufferSize options:MTLResourceStorageModeManaged ];
        }
#endif
    }

    void CContext::CRingBuffer::OnFrameStart(int iCurrentFrameSlot, int iNextFrameSlot)
    {
        CRY_ASSERT(!m_BufferPadPerFrame[iCurrentFrameSlot]);

        m_BufferUsedPerFrame[iCurrentFrameSlot] = 0;
        m_BufferPadPerFrame[iNextFrameSlot] = 0;
    }

    void CContext::CRingBuffer::ConsumeTrack(int iCurrentFrameSlot, unsigned int size, bool bPadding)
    {
        if (bPadding && !m_BufferUsedPerFrame[iCurrentFrameSlot])
        {
            m_BufferPadPerFrame[iCurrentFrameSlot] += size;
        }
        else
        {
            m_BufferUsedPerFrame[iCurrentFrameSlot] += size;
        }
    }

    void CContext::CRingBuffer::ValidateBufferUsage()
    {
        unsigned int totalBufferUsed = 0;
        for (int i = 0; i < sizeof(m_BufferUsedPerFrame) / sizeof(m_BufferUsedPerFrame[0]); ++i)
        {
            totalBufferUsed += m_BufferUsedPerFrame[i];
            totalBufferUsed += m_BufferPadPerFrame[i];
        }

        CRY_ASSERT(totalBufferUsed <= m_Buffer.length);

        if (totalBufferUsed > m_Buffer.length)
        {
            DXGL_LOG_MSG("Ring buffer overrun. Rendering artifacts expected.");
        }
    }

    void* CContext::CRingBuffer::Allocate(int iCurrentFrameSlot, unsigned int size, size_t &ringBufferOffsetOut, unsigned int alignment)
    {
        CRY_ASSERT(size <= m_Buffer.length);

        if (!alignment)
        {
            alignment = m_uiDefaultAlignment;
        }

        unsigned int OldFreePositionPointer = m_uiFreePositionPointer;
        //  Igor: this alignes the pointer position
        m_uiFreePositionPointer = (m_uiFreePositionPointer + alignment - 1) / alignment * alignment;
        ConsumeTrack(iCurrentFrameSlot, m_uiFreePositionPointer - OldFreePositionPointer, true);

        DXMETAL_TODO("Consider this ring buffer padding usage.");
        //  Igor: Motivation: This padding is used because Cryengine sometimes binds 256b constant
        //  buffer to the shader slot which expects 1424b constant buffer. Metal runtime considers
        //  this an error, because on the one hand there's no guarantee shader won't try to access
        //  all data it declared in the shader code, on the other hand accessing memory past the end
        //  of the buffer leads to undefined GPU behaviour (this means GPU might hang or crash or whatever)
        //if (m_uiFreePositionPointer + size > m_Buffer.length)
        if (m_uiFreePositionPointer + max(size, 1424u) > m_Buffer.length)
        {
            ConsumeTrack(iCurrentFrameSlot, m_Buffer.length - m_uiFreePositionPointer, true);
            m_uiFreePositionPointer = 0;
        }

        unsigned int res = m_uiFreePositionPointer;

        m_uiFreePositionPointer += size;
        ConsumeTrack(iCurrentFrameSlot, size, false);

        ValidateBufferUsage();
        ringBufferOffsetOut = res;

        return (uint8*)m_Buffer.contents + res;
    }

    CContext::CCopyTextureHelper::CCopyTextureHelper()
        : m_PipelineStateBGRA8Unorm(0)
        , m_PipelineStateRGBA8Unorm(0)
        , m_PipelineStateRGBA16Float(0)
        , m_PipelineStateRGBA32Float(0)
        , m_PipelineStateR16Float(0)
        , m_PipelineStateR16Unorm(0)
        , m_PipelineStateRG16Float(0)
        , m_PipelineStateBGRA8UnormLanczos(0)
        , m_PipelineStateRGBA8UnormLanczos(0)
        , m_PipelineStateBGRA8UnormBicubic(0)
        , m_PipelineStateRGBA8UnormBicubic(0)
        , m_samplerState(0)
        , m_samplerStateLinear(0)
    {
    }

    CContext::CCopyTextureHelper::~CCopyTextureHelper()
    {
        if (m_PipelineStateBGRA8Unorm)
        {
            [m_PipelineStateBGRA8Unorm release];
        }

        if (m_PipelineStateRGBA8Unorm)
        {
            [m_PipelineStateRGBA8Unorm release];
        }

        if (m_PipelineStateRGBA16Float)
        {
            [m_PipelineStateRGBA16Float release];
        }
        
        if (m_PipelineStateRGBA32Float)
        {
            [m_PipelineStateRGBA32Float release];
        }
        
        if (m_PipelineStateR16Float)
        {
            [m_PipelineStateR16Float release];
        }
        
        if (m_PipelineStateR16Unorm)
        {
            [m_PipelineStateR16Unorm release];
        }

        if (m_PipelineStateRG16Float)
        {
            [m_PipelineStateRG16Float release];
        }

        if (m_PipelineStateBGRA8UnormLanczos)
        {
            [m_PipelineStateBGRA8UnormLanczos release];
        }

        if (m_PipelineStateRGBA8UnormLanczos)
        {
            [m_PipelineStateRGBA8UnormLanczos release];
        }

        if (m_PipelineStateBGRA8UnormBicubic)
        {
            [m_PipelineStateBGRA8UnormBicubic release];
        }

        if (m_PipelineStateRGBA8UnormBicubic)
        {
            [m_PipelineStateRGBA8UnormBicubic release];
        }

        if (m_samplerState)
        {
            [m_samplerState release];
        }

        if (m_samplerStateLinear)
        {
            [m_samplerStateLinear release];
        }
    }

    bool CContext::CCopyTextureHelper::Initialize(CDevice* pDevice)
    {
        id<MTLDevice> mtlDevice = pDevice->GetMetalDevice();

        {
            MTLCompileOptions* options = [MTLCompileOptions alloc];
            NSError* error = nil;
            
            id<MTLLibrary> lib = [mtlDevice newLibraryWithSource:metalCopyShaderSource
                                  options:nil //Use default language version which is the most recent language version available
                                  error:&error];

            LOG_METAL_SHADER_SOURCE(@ "%@", metalCopyShaderSource);

            if (error)
            {
                // Error code 4 is warning, but sometimes a 3 (compile error) is returned on warnings only.
                // The documentation indicates that if the lib is nil there is a compile error, otherwise anything
                // in the error is really a warning. Therefore, we check the lib instead of the error code
                if (!lib)
                {
                    LOG_METAL_SHADER_ERRORS(@ "%@", error);
                }
                else
                {
                    LOG_METAL_SHADER_WARNINGS(@ "%@", error);
                }

                error = 0;
            }

            id<MTLFunction> PS = 0;
            id<MTLFunction> VS = 0;


            if (lib)
            {
                PS = [lib newFunctionWithName:@ "mainPS"];
                VS = [lib newFunctionWithName:@ "mainVS"];
                [lib release];
            }


            [options release];


            if (!PS || !VS)
            {
                CRY_ASSERT(!"Can't create copy shaders???");
                return false;
            }

            MTLRenderPipelineDescriptor* desc = [[MTLRenderPipelineDescriptor alloc] init];

            // set the bound shader state settings
            desc.vertexFunction = VS;
            desc.fragmentFunction = PS;

            MTLRenderPipelineReflection* ref;

            {
                desc.colorAttachments[0].pixelFormat = MTLPixelFormatRGBA8Unorm;

                m_PipelineStateRGBA8Unorm = [mtlDevice newRenderPipelineStateWithDescriptor:desc error : &error];

                if (!m_PipelineStateRGBA8Unorm)
                {
                    LOG_METAL_PIPELINE_ERRORS(@ "Error generation pipeline object: %@", error);
                    CRY_ASSERT(!"Can't create copy pipeline???");
                    return false;
                }
            }

            {
                desc.colorAttachments[0].pixelFormat = MTLPixelFormatRGBA16Float;

                m_PipelineStateRGBA16Float = [mtlDevice newRenderPipelineStateWithDescriptor:desc error : &error];

                if (!m_PipelineStateRGBA16Float)
                {
                    LOG_METAL_PIPELINE_ERRORS(@ "Error generation pipeline object: %@", error);
                    CRY_ASSERT(!"Can't create copy pipeline???");
                    return false;
                }
            }
            
            {
                desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
                
                m_PipelineStateBGRA8Unorm = [mtlDevice newRenderPipelineStateWithDescriptor:desc error : &error];
                
                if (!m_PipelineStateBGRA8Unorm)
                {
                    LOG_METAL_PIPELINE_ERRORS(@ "Error generation pipeline object: %@", error);
                    CRY_ASSERT(!"Can't create copy pipeline???");
                    return false;
                }
            }
            
            {
                desc.colorAttachments[0].pixelFormat = MTLPixelFormatRGBA32Float;
                
                m_PipelineStateRGBA32Float = [mtlDevice newRenderPipelineStateWithDescriptor:desc error : &error];
                
                if (!m_PipelineStateRGBA32Float)
                {
                    LOG_METAL_PIPELINE_ERRORS(@ "Error generation pipeline object: %@", error);
                    CRY_ASSERT(!"Can't create copy pipeline???");
                    return false;
                }
            }

            {
                desc.colorAttachments[0].pixelFormat = MTLPixelFormatR16Float;

                m_PipelineStateR16Float = [mtlDevice newRenderPipelineStateWithDescriptor:desc error : &error];

                if (!m_PipelineStateR16Float)
                {
                    LOG_METAL_PIPELINE_ERRORS(@ "Error generation pipeline object: %@", error);
                    CRY_ASSERT(!"Can't create copy pipeline???");
                    return false;
                }
            }
            
            {
                desc.colorAttachments[0].pixelFormat = MTLPixelFormatR16Unorm;
                
                m_PipelineStateR16Unorm = [mtlDevice newRenderPipelineStateWithDescriptor:desc error : &error];
                
                if (!m_PipelineStateR16Unorm)
                {
                    LOG_METAL_PIPELINE_ERRORS(@ "Error generation pipeline object: %@", error);
                    CRY_ASSERT(!"Can't create copy pipeline???");
                    return false;
                }
            }

            {
                desc.colorAttachments[0].pixelFormat = MTLPixelFormatRG16Float;
                m_PipelineStateRG16Float = [mtlDevice newRenderPipelineStateWithDescriptor:desc error : &error];
                if (!m_PipelineStateRG16Float)
                {
                    LOG_METAL_PIPELINE_ERRORS(@ "Error generating pipeline object: %@", error);
                    CRY_ASSERT(!"Can't create copy pippeline???");
                    return false;
                }
            }
            [PS release];
            [VS release];
        }

        {
            MTLCompileOptions* options = [MTLCompileOptions alloc];
            NSError* error = nil;
            
            id<MTLLibrary> lib = [mtlDevice newLibraryWithSource:metalCopyShaderSourceLanczos
                                  options:nil  //Use default language version which is the most recent language version available
                                  error:&error];
            
            LOG_METAL_SHADER_SOURCE(@ "%@", metalCopyShaderSourceLanczos);

            if (error)
            {
                // Error code 4 is warning, but sometimes a 3 (compile error) is returned on warnings only.
                // The documentation indicates that if the lib is nil there is a compile error, otherwise anything
                // in the error is really a warning. Therefore, we check the lib instead of the error code
                if (!lib)
                {
                    LOG_METAL_SHADER_ERRORS(@ "%@", error);
                }
                else
                {
                    LOG_METAL_SHADER_WARNINGS(@ "%@", error);
                }

                error = 0;
            }

            id<MTLFunction> PS_Lanczos = 0;
            id<MTLFunction> PS_Bicubic = 0;
            id<MTLFunction> VS = 0;


            if (lib)
            {
                PS_Lanczos = [lib newFunctionWithName:@ "mainLanczosPS"];
                PS_Bicubic = [lib newFunctionWithName:@ "mainBicubicPS"];
                VS = [lib newFunctionWithName:@ "mainVS"];
                [lib release];
            }


            [options release];


            if (!PS_Lanczos || !PS_Bicubic || !VS)
            {
                CRY_ASSERT(!"Can't create copy shaders???");
                return false;
            }

            MTLRenderPipelineDescriptor* desc = [[MTLRenderPipelineDescriptor alloc] init];

            // set the bound shader state settings
            desc.vertexFunction = VS;

            MTLRenderPipelineReflection* ref;

            {
                desc.colorAttachments[0].pixelFormat = MTLPixelFormatRGBA8Unorm;

                desc.fragmentFunction = PS_Lanczos;
                m_PipelineStateRGBA8UnormLanczos = [mtlDevice newRenderPipelineStateWithDescriptor:desc error : &error];

                desc.fragmentFunction = PS_Bicubic;
                m_PipelineStateRGBA8UnormBicubic = [mtlDevice newRenderPipelineStateWithDescriptor:desc error : &error];

                if (!m_PipelineStateRGBA8UnormLanczos || !m_PipelineStateRGBA8UnormBicubic)
                {
                    LOG_METAL_PIPELINE_ERRORS(@ "Error generation pipeline object: %@", error);
                    CRY_ASSERT(!"Can't create copy pipeline???");
                    return false;
                }
            }

            {
                desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;

                desc.fragmentFunction = PS_Lanczos;
                m_PipelineStateBGRA8UnormLanczos = [mtlDevice newRenderPipelineStateWithDescriptor:desc error : &error];

                desc.fragmentFunction = PS_Bicubic;
                m_PipelineStateBGRA8UnormBicubic = [mtlDevice newRenderPipelineStateWithDescriptor:desc error : &error];

                if (!m_PipelineStateBGRA8UnormLanczos || !m_PipelineStateBGRA8UnormBicubic)
                {
                    LOG_METAL_PIPELINE_ERRORS(@ "Error generation pipeline object: %@", error);
                    CRY_ASSERT(!"Can't create copy pipeline???");
                    return false;
                }
            }

            [PS_Lanczos release];
            [PS_Bicubic release];
            [VS release];
        }

        {
            MTLSamplerDescriptor* desc = [[MTLSamplerDescriptor alloc] init];
            m_samplerState = [mtlDevice newSamplerStateWithDescriptor:desc];

            desc.magFilter = MTLSamplerMinMagFilterLinear;
            desc.minFilter = MTLSamplerMinMagFilterLinear;
            m_samplerStateLinear = [mtlDevice newSamplerStateWithDescriptor:desc];

            [desc release];
        }

        m_DeviceRef = mtlDevice; // used to submit constant buffer when using custom filtering shaders

        return true;
    }

    bool CContext::CCopyTextureHelper::DoTopMipSlowCopy(id<MTLTexture> texDst, id<MTLTexture> texSrc, CContext* pContext, CopyFilterType const filterType)
    {
        pContext->FlushCurrentEncoder();
        id <MTLCommandBuffer> commandBuffer = pContext->GetCurrentCommandBuffer();

        //  Igor: this is autoreleased object.
        MTLRenderPassDescriptor* RenderPass = [MTLRenderPassDescriptor renderPassDescriptor];

        id<MTLTexture> mtlTexture = texDst;
        MTLRenderPassColorAttachmentDescriptor* ColorAttachment = RenderPass.colorAttachments[0];
        ColorAttachment.texture = mtlTexture;
        ColorAttachment.storeAction = MTLStoreActionStore;
        //  Igor: do restore if upading only a part of the texture!
        ColorAttachment.loadAction = MTLLoadActionDontCare;
        ColorAttachment.level = 0;

        if (mtlTexture.textureType == MTLTextureType3D)
        {
            ColorAttachment.depthPlane = 0;
        }
        else
        {
            ColorAttachment.slice = 0;
        }

        // MTLBlitCommandEncoder is an autorelease object
        id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:RenderPass];

        [renderEncoder setRenderPipelineState: SelectPipelineState(texDst.pixelFormat, filterType)];
        if (filterType == BILINEAR || filterType == BICUBIC)
        {
            [renderEncoder setFragmentSamplerState: m_samplerStateLinear atIndex : 0];
        }
        else
        {
            [renderEncoder setFragmentSamplerState: m_samplerState atIndex : 0];
        }
        [renderEncoder setFragmentTexture: texSrc atIndex : 0];

        // Upload uniforms for custom filtering
        if (LANCZOS == filterType)
        {
            int srcWidth  = [texSrc width];
            int srcHeight = [texSrc height];
            int dstWidth  = [texDst width];
            int dstHeight = [texDst height];

            float pixelWidth  = 1.f / dstWidth;
            float pixelHeight = 1.f / dstHeight;

            m_Uniforms.params0[0] = 1.5;
            m_Uniforms.params0[1] = 1.5;
            m_Uniforms.params0[2] = 1.0;
            m_Uniforms.params0[3] = 1.0;

            m_Uniforms.params1[0] = 1.f / srcWidth;
            m_Uniforms.params1[1] = 1.f / srcHeight;
            m_Uniforms.params1[2] = 0.5f * m_Uniforms.params1[0] - m_Uniforms.params0[0] * pixelWidth;
            m_Uniforms.params1[3] = 0.5f * m_Uniforms.params1[1] - m_Uniforms.params0[1] * pixelHeight;

            m_Uniforms.params2[0] = 1.f / ((float)dstWidth / srcWidth);
            m_Uniforms.params2[1] = 1.f / ((float)dstHeight / srcHeight);
            m_Uniforms.params2[2] = -m_Uniforms.params0[0] + 0.5f * m_Uniforms.params2[0];
            m_Uniforms.params2[3] = -m_Uniforms.params0[1] + 0.5f * m_Uniforms.params2[1];

            {
                id<MTLBuffer> uni_buff;
                uni_buff = [m_DeviceRef newBufferWithBytes:(void*)&m_Uniforms length:sizeof(Uniforms)options:0];
                [renderEncoder setFragmentBuffer: uni_buff offset : 0 atIndex : 0];
            }
        }



        [renderEncoder drawPrimitives: MTLPrimitiveTypeTriangle vertexStart : 0 vertexCount : 3];

        [renderEncoder endEncoding];

        return true;
    }

    id<MTLRenderPipelineState> CContext::CCopyTextureHelper::SelectPipelineState(MTLPixelFormat pixelFormat, CopyFilterType const filterType)
    {
        switch (pixelFormat)
        {
        case MTLPixelFormatRGBA8Unorm:
            if (filterType == LANCZOS)
            {
                return m_PipelineStateRGBA8UnormLanczos;
            }
            else if (filterType == BICUBIC)
            {
                return m_PipelineStateRGBA8UnormBicubic;
            }
            else
            {
                return m_PipelineStateRGBA8Unorm;
            }

        case MTLPixelFormatRGBA16Float:
            return m_PipelineStateRGBA16Float;
                
        case MTLPixelFormatRGBA32Float:
            return m_PipelineStateRGBA32Float;
                
        case MTLPixelFormatR16Float:
            return m_PipelineStateR16Float;
                
        case MTLPixelFormatR16Unorm:
            return m_PipelineStateR16Unorm;

        case MTLPixelFormatRG16Float:
            return m_PipelineStateRG16Float;

        case MTLPixelFormatBGRA8Unorm:
            if (filterType == LANCZOS)
            {
                return m_PipelineStateBGRA8UnormLanczos;
            }
            else if (filterType == BICUBIC)
            {
                return m_PipelineStateBGRA8UnormBicubic;
            }
            else
            {
                return m_PipelineStateBGRA8Unorm;
            }

        default:
            return nil;
        }
    }
    //  Confetti End: Igor Lobanchikov

    ////////////////////////////////////////////////////////////////////////////
    // CGPUEventsHelper implementation
    ////////////////////////////////////////////////////////////////////////////
    CGPUEventsHelper::CGPUEventsHelper()
        : m_uiRTSwitchedAt(m_sInvalidRTSwitchedAt)
    {
    }

    CGPUEventsHelper::~CGPUEventsHelper()
    {
        for (int i = 0; i < m_currentMarkerStack.size(); ++i)
        {
            [m_currentMarkerStack[i] release];
        }

        for (int i = 0; i < m_aMarkerQueue.size(); ++i)
        {
            if (m_aMarkerQueue[i].m_pMessage)
            {
                [m_aMarkerQueue[i].m_pMessage release];
            }
        }
    }

    void CGPUEventsHelper::AddMarker(const char* pszMessage, EActionType eAction)
    {
        NSString* source = pszMessage ?
            [[NSString alloc] initWithCString : pszMessage
encoding: NSASCIIStringEncoding]
            : nil;

        m_aMarkerQueue.push_back(SMarkerQueueAction());
        m_aMarkerQueue.back().m_pMessage = source;
        m_aMarkerQueue.back().m_eAction = eAction;
    }

    void CGPUEventsHelper::FlushActions(id <MTLCommandEncoder> pEncoder, EFlushType eFlushType)
    {
        CRY_ASSERT(pEncoder);

        switch (eFlushType)
        {
        case eFT_Default:
            m_uiRTSwitchedAt = m_sInvalidRTSwitchedAt;
            ReplayActions(pEncoder, m_aMarkerQueue.size());
            break;
        case eFT_FlushEncoder:
            if (m_uiRTSwitchedAt != m_sInvalidRTSwitchedAt)
            {
                MovePushesToNextEncoder();
                ReplayActions(pEncoder, m_uiRTSwitchedAt);
                m_uiRTSwitchedAt = m_sInvalidRTSwitchedAt;
            }
            break;
        case eFT_NewEncoder:
            for (size_t i = 0; i < m_currentMarkerStack.size(); ++i)
            {
                [pEncoder pushDebugGroup: m_currentMarkerStack[i]];
            }

            m_uiRTSwitchedAt = m_sInvalidRTSwitchedAt;
            ReplayActions(pEncoder, m_aMarkerQueue.size());
            break;
        default:
            DXMETAL_NOT_IMPLEMENTED;
        }
    }

    void CGPUEventsHelper::OnSetRenderTargets()
    {
        m_uiRTSwitchedAt = m_aMarkerQueue.size();
    }

    void CGPUEventsHelper::ReplayActions(id <MTLCommandEncoder> pEncoder, uint32 numActions)
    {
        CRY_ASSERT(numActions <= m_aMarkerQueue.size());

        for (uint32 i = 0; i < numActions; ++i)
        {
            switch (m_aMarkerQueue[i].m_eAction)
            {
            case eAT_Push:
                [pEncoder pushDebugGroup: m_aMarkerQueue[i].m_pMessage];
                m_currentMarkerStack.push_back(m_aMarkerQueue[i].m_pMessage);
                [m_currentMarkerStack.back() retain];
                break;
            case eAT_Pop:
                [pEncoder popDebugGroup];
                [m_currentMarkerStack.back() release];
                m_currentMarkerStack.pop_back();
                break;
            case eAT_Event:
                [pEncoder insertDebugSignpost: m_aMarkerQueue[i].m_pMessage];
                break;
            default:
                DXMETAL_NOT_IMPLEMENTED;
            }
            if (m_aMarkerQueue[i].m_pMessage)
            {
                [m_aMarkerQueue[i].m_pMessage release];
            }
        }

        m_aMarkerQueue.erase(m_aMarkerQueue.begin(), m_aMarkerQueue.begin() + numActions);
    }

    void CGPUEventsHelper::MovePushesToNextEncoder()
    {
        CRY_ASSERT(m_uiRTSwitchedAt <= m_aMarkerQueue.size());
        uint32 i = m_uiRTSwitchedAt;
        for (; i > 0; --i)
        {
            if (m_aMarkerQueue[i - 1].m_eAction == eAT_Pop)
            {
                break;
            }

            if (m_aMarkerQueue[i - 1].m_eAction == eAT_Push)
            {
                m_uiRTSwitchedAt = i - 1;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // CContext implementation
    ////////////////////////////////////////////////////////////////////////////

    CContext::CContext(CDevice* pDevice)
        : m_pDevice(pDevice)
        , m_pPipelineCache(new SPipelineCache)
        , m_pInputLayout(NULL)
        , m_bFrameBufferStateDirty(false)
        , m_bPipelineDirty(false)
        , m_bInputLayoutDirty(false)
        , m_bInputAssemblerSlotsDirty(false)
        , m_uIndexStride(0)
        , m_uVertexOffset(0)
        //  Confetti BEGIN: Igor Lobanchikov
        , m_CurrentCommandBuffer(nil)
        , m_CurrentEncoder(0)
        , m_CurrentComputeEncoder(0)
        , m_CurrentBlitEncoder(0)
        , m_iCurrentFrameSlot(-1)
        , m_iCurrentFrameEventSlot(-1)
        DXMETAL_TODO("Tune this parameter per project.")
#if defined(AZ_PLATFORM_MAC)
        , m_RingBufferShared(pDevice->GetMetalDevice(), 10 * 1024 * 1024, 256, MEM_SHARED_RINGBUFFER)
        , m_RingBufferManaged(pDevice->GetMetalDevice(), 50 * 1024 * 1024, 256, MEM_MANAGED_RINGBUFFER)
#else
        , m_RingBufferShared(pDevice->GetMetalDevice(), 16 * 1024 * 1024, 256, MEM_SHARED_RINGBUFFER)
#endif
        , m_QueryRingBuffer(pDevice->GetMetalDevice(), 32 * 1024, 8, MEM_SHARED_RINGBUFFER)
        , m_bPossibleClearPending(false)
        //  Confetti End: Igor Lobanchikov
        , m_MetalPrimitiveType(MTLPrimitiveTypePoint)
        , m_MetalIndexType(MTLIndexTypeUInt16)
        , m_defaultSamplerState(0)
    {

        m_kStateCache.m_kDepthStencil.m_DepthStencilState = 0;
        m_kStateCache.m_kDepthStencil.m_iStencilRef = 0;
        m_kStateCache.m_kDepthStencil.m_bDSSDirty = false;
        m_kStateCache.m_kDepthStencil.m_bStencilRefDirty = true;

        m_kStateCache.m_kRasterizer.m_RateriserDirty = SRasterizerCache::RS_NOT_INITIALIZED;

        memset(&m_kStateCache.m_StageCache, 0, sizeof(m_kStateCache.m_StageCache));
        for (int i = 0; i < SStageStateCache::eBST_Count; ++i)
        {
            m_kStateCache.m_StageCache.m_BufferState[i].m_MaxBufferUsed = -1;
            m_kStateCache.m_StageCache.m_BufferState[i].m_MinBufferUsed = SBufferStateStageCache::s_MaxBuffersPerStage;
            m_kStateCache.m_StageCache.m_TextureState[i].m_MaxTextureUsed = -1;
            m_kStateCache.m_StageCache.m_TextureState[i].m_MinTextureUsed = STextureStageState::s_MaxTexturesPerStage;
            m_kStateCache.m_StageCache.m_SamplerState[i].m_MaxSamplerUsed = -1;
            m_kStateCache.m_StageCache.m_SamplerState[i].m_MinSamplerUsed = SSamplerStageState::s_MaxSamplersPerStage;
        }

        for (int i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
        {
            m_kStateCache.m_kBlend.colorAttachements[i].ResetToDefault();
        }
        
        NCryMetal::CacheMinOSVersionInfo();
        NCryMetal::CacheGPUFamilyFeaturSetInfo(m_pDevice->GetMetalDevice());       
    }

    CContext::~CContext()
    {
        //  Confetti BEGIN: Igor Lobanchikov
        if (m_kStateCache.m_kDepthStencil.m_DepthStencilState)
        {
            [m_kStateCache.m_kDepthStencil.m_DepthStencilState release];
        }

        for (int i = 0; i < SStageStateCache::eBST_Count; ++i)
        {
            for (int j = 0; j < SBufferStateStageCache::s_MaxBuffersPerStage; ++j)
            {
                if (m_kStateCache.m_StageCache.m_BufferState[i].m_Buffers[j])
                {
                    [m_kStateCache.m_StageCache.m_BufferState[i].m_Buffers[j] release];
                }
            }

            for (int j = 0; j < STextureStageState::s_MaxTexturesPerStage; ++j)
            {
                if (m_kStateCache.m_StageCache.m_SamplerState[i].m_Samplers[j])
                {
                    [m_kStateCache.m_StageCache.m_SamplerState[i].m_Samplers[j] release];
                }
            }
        }

        //  Confetti End: Igor Lobanchikov

        delete m_pPipelineCache;

        //  Confetti BEGIN: Igor Lobanchikov
        if (m_CurrentCommandBuffer)
        {
            [m_CurrentCommandBuffer release];
        }
        m_CurrentCommandBuffer = nil;
        //  Confetti End: Igor Lobanchikov

        if (m_defaultSamplerState)
        {
            [m_defaultSamplerState release];
        }
    }

    //  Confetti BEGIN: Igor Lobanchikov
    void CContext::InitMetalFrameResources()
    {
        dispatch_semaphore_wait(m_FrameQueueSemaphore, DISPATCH_TIME_FOREVER);
        m_iCurrentFrameSlot = (m_iCurrentFrameSlot + 1) % m_MaxFrameQueueSlots;
        m_iCurrentFrameEventSlot = (m_iCurrentFrameEventSlot + 1) % m_MaxFrameEventSlots;

        m_RingBufferShared.OnFrameStart(m_iCurrentFrameSlot, (m_iCurrentFrameSlot + 1) % m_MaxFrameQueueSlots);
#if defined(AZ_PLATFORM_MAC)
        m_RingBufferManaged.OnFrameStart(m_iCurrentFrameSlot, (m_iCurrentFrameSlot + 1) % m_MaxFrameQueueSlots);
#endif
        m_QueryRingBuffer.OnFrameStart(m_iCurrentFrameEventSlot, (m_iCurrentFrameEventSlot + 1) % m_MaxFrameEventSlots);

        //  At this point all the resources at corresponding slots are considered unused
        m_iCurrentEvent = -1;

        NextCommandBuffer();
    }

    SContextEventHelper* CContext::GetCurrentEventHelper()
    {
        if (m_eEvents[m_iCurrentFrameEventSlot].size() <= m_iCurrentEvent)
        {
            m_eEvents[m_iCurrentFrameEventSlot].push_back((SContextEventHelper*)Malloc(sizeof(SContextEventHelper)));
        }

        return m_eEvents[m_iCurrentFrameEventSlot][m_iCurrentEvent];
    }

    void CContext::NextCommandBuffer()
    {
        CRY_ASSERT(!m_CurrentCommandBuffer);

        //m_CurrentCommandBuffer = [m_pDevice->GetMetalCommandQueue() commandBufferWithUnretainedReferences];
        m_CurrentCommandBuffer = [m_pDevice->GetMetalCommandQueue() commandBuffer];
        [m_CurrentCommandBuffer retain];
        ++m_iCurrentEvent;

        __block SContextEventHelper* pEventHelper = GetCurrentEventHelper();
        pEventHelper->bTriggered = false;
        pEventHelper->bCommandBufferSubmitted = false;
        pEventHelper->bCommandBufferPreSubmitted = false;

        [m_CurrentCommandBuffer addCompletedHandler: ^ (id < MTLCommandBuffer > Buffer)
         {
             CRY_ASSERT(pEventHelper->bCommandBufferPreSubmitted);
             if (!pEventHelper->bCommandBufferSubmitted)
             {
                 LOG_METAL_PIPELINE_ERRORS(@ "Command buffer was finished too fast. Do we have anything to render?");
             }
             pEventHelper->bTriggered = true;
         }];
    }
    //  Confetti End: Igor Lobanchikov

    bool CContext::Initialize()
    {
        DXGL_SCOPED_PROFILE("CContext::Initialize")

        //  Confetti BEGIN: Igor Lobanchikov
        m_FrameQueueSemaphore = dispatch_semaphore_create(m_MaxFrameQueueDepth);
        InitMetalFrameResources();

        if (!m_CopyTextureHelper.Initialize(m_pDevice))
        {
            return false;
        }
        //  Confetti End: Igor Lobanchikov

        {
            MTLSamplerDescriptor* desc = [[MTLSamplerDescriptor alloc] init];
            m_defaultSamplerState = [m_pDevice->GetMetalDevice() newSamplerStateWithDescriptor:desc];
            [desc release];
        }

        return GetImplicitStateCache(m_kStateCache);
    }

    bool CContext::GetImplicitStateCache(SImplicitStateCache& kCache)
    {
        DXGL_SCOPED_PROFILE("CContext::GetImplicitStateCache")

        kCache.m_bBlendColorDirty = false;
        memset(&kCache.m_akBlendColor.m_afRGBA, 0, sizeof(kCache.m_akBlendColor.m_afRGBA));

        kCache.m_bViewportDirty = false;
        kCache.m_bViewportDefault = true;
        memset(&kCache.m_DefaultViewport, 0, sizeof(kCache.m_DefaultViewport));
        kCache.m_DefaultViewport.zfar = 1.0;
        kCache.m_CurrentViewport = kCache.m_DefaultViewport;

        return true;
    }

    template <typename T>
    inline bool RefreshCache(T& kCache, T kState)
    {
        bool bDirty(kCache != kState);
        kCache = kState;
        return bDirty;
    }

#define CACHE_VAR(_Cache, _State) RefreshCache(_Cache, _State)
#define CACHE_FIELD(_Cache, _State, _Member) RefreshCache((_Cache)._Member, (_State)._Member)

    bool CContext::SetBlendState(const SBlendState& kState)
    {
        DXGL_SCOPED_PROFILE("CContext::SetBlendState")

        //  Confetti BEGIN: Igor Lobanchikov
        //  Igor: this is important to keep RT count equal to metal RT count
        CRY_ASSERT(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT == SAttachmentConfiguration::s_ColorAttachmentDescCount);

        for (int i = 0; i < SAttachmentConfiguration::s_ColorAttachmentDescCount; ++i)
        {
            SMetalBlendState& srcBlendState = m_kStateCache.m_kBlend.colorAttachements[i];
            SMetalBlendState& targetBlendState = m_kPipelineConfiguration.m_AttachmentConfiguration.colorAttachments[i];
            bool bTargetIsBound = (m_kPipelineConfiguration.m_AttachmentConfiguration.colorAttachments[i].pixelFormat != MTLPixelFormatInvalid);
            srcBlendState = kState.colorAttachements[i];

            if (!bTargetIsBound && targetBlendState.blendingEnabled)
            {
                m_bPipelineDirty = true;
                targetBlendState.ResetToDefault();
            }
            else if (bTargetIsBound && memcmp(&srcBlendState, &targetBlendState, sizeof(srcBlendState)))
            {
                m_bPipelineDirty = true;
                targetBlendState = srcBlendState;
            }
        }
        //  Confetti End: Igor Lobanchikov

        return true;
    }

    void CContext::SetSampleMask(uint32 uSampleMask)
    {
        DXGL_SCOPED_PROFILE("CContext::SetSampleMask")
        
        //uSampleMask can be 0 when switching maps.
        if(uSampleMask!=0)
        {
            //todo:add support for multisampling on metal
            //Assert to check if calling code is expecting this support
            CRY_ASSERT(uSampleMask & 0xFF == 0xFF);
        }
    }

    void CContext::SetBlendColor(float fRed, float fGreen, float fBlue, float fAlpha)
    {
        DXGL_SCOPED_PROFILE("CContext::SetBlendColor")

        SColor kBlendColor;
        kBlendColor.m_afRGBA[0] = fRed;
        kBlendColor.m_afRGBA[1] = fGreen;
        kBlendColor.m_afRGBA[2] = fBlue;
        kBlendColor.m_afRGBA[3] = fAlpha;
        if (CACHE_VAR(m_kStateCache.m_akBlendColor, kBlendColor))
        {
            m_kStateCache.m_bBlendColorDirty = true;
        }
    }

    //  Confetti BEGIN: Igor Lobanchikov
    bool CContext::SetDepthStencilState(
        id<MTLDepthStencilState> depthStencilState,
        uint32 iStencilRef)
    {
        DXGL_SCOPED_PROFILE("CContext::SetDepthStencilState")

        SDepthStencilCache & kDSCache(m_kStateCache.m_kDepthStencil);
        if (kDSCache.m_iStencilRef != iStencilRef)
        {
            kDSCache.m_iStencilRef = iStencilRef;
            kDSCache.m_bStencilRefDirty = true;
        }

        if (depthStencilState != kDSCache.m_DepthStencilState)
        {
            if (kDSCache.m_DepthStencilState)
            {
                [kDSCache.m_DepthStencilState release];
            }

            kDSCache.m_DepthStencilState = depthStencilState;

            if (kDSCache.m_DepthStencilState)
            {
                [kDSCache.m_DepthStencilState retain];
            }

            kDSCache.m_bDSSDirty = true;
        }

        return true;
    }
    //  Confetti End: Igor Lobanchikov

    bool CContext::SetRasterizerState(const SRasterizerState& kState)
    {
        DXGL_SCOPED_PROFILE("CContext::SetRasterizerState")
        //  Confetti BEGIN: Igor Lobanchikov
        SRasterizerCache & kCache(m_kStateCache.m_kRasterizer);
        SRasterizerState& kMetalCache(m_kStateCache.m_kRasterizer);
        if (kCache.m_RateriserDirty & SRasterizerCache::RS_NOT_INITIALIZED)
        {
            kMetalCache = kState;
            kCache.m_RateriserDirty = SRasterizerCache::RS_ALL_DIRTY;
        }
        else
        {
            if (kMetalCache.cullMode != kState.cullMode)
            {
                kMetalCache.cullMode = kState.cullMode;
                kCache.m_RateriserDirty |= SRasterizerCache::RS_CULL_MODE_DIRTY;
            }

            if (kMetalCache.depthBias != kState.depthBias
                || kMetalCache.depthBiasClamp != kState.depthBiasClamp
                || kMetalCache.depthSlopeScale != kState.depthSlopeScale)
            {
                kMetalCache.depthBias = kState.depthBias;
                kMetalCache.depthBiasClamp = kState.depthBiasClamp;
                kMetalCache.depthSlopeScale = kState.depthSlopeScale;
                kCache.m_RateriserDirty |= SRasterizerCache::RS_DEPTH_BIAS_DIRTY;
            }

            if (kMetalCache.frontFaceWinding != kState.frontFaceWinding)
            {
                kMetalCache.frontFaceWinding = kState.frontFaceWinding;
                kCache.m_RateriserDirty |= SRasterizerCache::RS_WINDING_DIRTY;
            }

            if (kMetalCache.triangleFillMode != kState.triangleFillMode)
            {
                kMetalCache.triangleFillMode = kState.triangleFillMode;
                kCache.m_RateriserDirty |= SRasterizerCache::RS_FILL_MODE_DIRTY;
            }

            if (kMetalCache.depthClipMode != kState.depthClipMode)
            {
                kMetalCache.depthClipMode = kState.depthClipMode;
                kCache.m_RateriserDirty |= SRasterizerCache::RS_DEPTH_CLIP_MODE_DIRTY;
            }

            if (kMetalCache.scissorEnable != kState.scissorEnable)
            {
                kMetalCache.scissorEnable = kState.scissorEnable;
                kCache.m_RateriserDirty |= SRasterizerCache::RS_SCISSOR_ENABLE_DIRTY;
            }
        }
        //  Confetti End: Igor Lobanchikov

        return true;
    }

    //  Confetti BEGIN: Igor Lobanchikov
    id<MTLSamplerState>* patchSamplers(id<MTLSamplerState> samplers[SSamplerStageState::s_MaxSamplersPerStage], id<MTLDevice> device, id<MTLSamplerState> defaultState)
    {
        static id<MTLSamplerState> res[SSamplerStageState::s_MaxSamplersPerStage];
        memcpy(res, samplers, sizeof(res));

        for (int i = 0; i < SSamplerStageState::s_MaxSamplersPerStage; ++i)
        {
            if (!res[i])
            {
                res[i] = defaultState;
            }
        }

        return res;
    }
    //  Confetti End: Igor Lobanchikov


    void CContext::FlushTextureUnits()
    {
        DXGL_SCOPED_PROFILE("CContext::FlushTextureUnits")

        //  Confetti BEGIN: Igor Lobanchikov
        {
            //  Set VS Samplers
            SSamplerStageState& samplerState = m_kStateCache.m_StageCache.m_SamplerState[SStageStateCache::eBST_Vertex];

            NSRange range = {static_cast<NSUInteger>(samplerState.m_MinSamplerUsed),
                             static_cast<NSUInteger>(samplerState.m_MaxSamplerUsed - samplerState.m_MinSamplerUsed + 1)};
            //NSRange range = {samplerState.m_MinSamplerUsed, 1};
            if (samplerState.m_MaxSamplerUsed >= samplerState.m_MinSamplerUsed)
            {
                //[m_CurrentEncoder setVertexSamplerStates:(samplerState.m_Samplers+samplerState.m_MinSamplerUsed)
                [m_CurrentEncoder setVertexSamplerStates: (patchSamplers(samplerState.m_Samplers, GetDevice()->GetMetalDevice(), m_defaultSamplerState) + samplerState.m_MinSamplerUsed)
withRange: range];
            }

            samplerState.m_MinSamplerUsed =  SSamplerStageState::s_MaxSamplersPerStage;
            samplerState.m_MaxSamplerUsed = -1;
        }

        {
            //  Set PS Samplers
            SSamplerStageState& samplerState = m_kStateCache.m_StageCache.m_SamplerState[SStageStateCache::eBST_Fragment];

            NSRange range = {static_cast<NSUInteger>(samplerState.m_MinSamplerUsed),
                             static_cast<NSUInteger>(samplerState.m_MaxSamplerUsed - samplerState.m_MinSamplerUsed + 1)};
            if (samplerState.m_MaxSamplerUsed >= samplerState.m_MinSamplerUsed)
            {
                //    [m_CurrentEncoder setFragmentSamplerStates:(samplerState.m_Samplers+samplerState.m_MinSamplerUsed)
                [m_CurrentEncoder setFragmentSamplerStates: (patchSamplers(samplerState.m_Samplers, GetDevice()->GetMetalDevice(), m_defaultSamplerState) + samplerState.m_MinSamplerUsed)
withRange: range];
            }

            samplerState.m_MinSamplerUsed =  SSamplerStageState::s_MaxSamplersPerStage;
            samplerState.m_MaxSamplerUsed = -1;
        }


        {
            //  Set VS textures
            STextureStageState& textureState = m_kStateCache.m_StageCache.m_TextureState[SStageStateCache::eBST_Vertex];
            id<MTLTexture> tmpTextures[STextureStageState::s_MaxTexturesPerStage];

            for (int i = textureState.m_MinTextureUsed; i <= textureState.m_MaxTextureUsed; ++i)
            {
                tmpTextures[i] = textureState.m_Textures[i] ? textureState.m_Textures[i]->GetMetalTexture() : 0;
            }

            NSRange range = {static_cast<NSUInteger>(textureState.m_MinTextureUsed),
                             static_cast<NSUInteger>(textureState.m_MaxTextureUsed - textureState.m_MinTextureUsed + 1)};
            if (textureState.m_MaxTextureUsed >= textureState.m_MinTextureUsed)
            {
                [m_CurrentEncoder setVertexTextures: (tmpTextures + textureState.m_MinTextureUsed)
withRange: range];
            }

            textureState.m_MinTextureUsed =  STextureStageState::s_MaxTexturesPerStage;
            textureState.m_MaxTextureUsed = -1;
        }

        {
            //  Set PS textures
            STextureStageState& textureState = m_kStateCache.m_StageCache.m_TextureState[SStageStateCache::eBST_Fragment];
            id<MTLTexture> tmpTextures[STextureStageState::s_MaxTexturesPerStage];

            for (int i = textureState.m_MinTextureUsed; i <= textureState.m_MaxTextureUsed; ++i)
            {
                tmpTextures[i] = textureState.m_Textures[i] ? textureState.m_Textures[i]->GetMetalTexture() : 0;
            }

            NSRange range = {static_cast<NSUInteger>(textureState.m_MinTextureUsed),
                             static_cast<NSUInteger>(textureState.m_MaxTextureUsed - textureState.m_MinTextureUsed + 1)};
            if (textureState.m_MaxTextureUsed >= textureState.m_MinTextureUsed)
            {
                [m_CurrentEncoder setFragmentTextures: (tmpTextures + textureState.m_MinTextureUsed)
withRange: range];
            }

            textureState.m_MinTextureUsed =  STextureStageState::s_MaxTexturesPerStage;
            textureState.m_MaxTextureUsed = -1;
        }

        return;
        //  Confetti End: Igor Lobanchikov
    }

    void CContext::FlushInputAssemblerState()
    {
        DXGL_SCOPED_PROFILE("CContext::FlushInputAssemblerState")

        //  Confetti BEGIN: Igor Lobanchikov

        uint32  uMinVBSlot = 16;
        int32   MaxSlot = -1;

        DXMETAL_TODO("Allow to rebind buffers without full update. This will be handled during optimization pass later.");
        //m_bInputAssemblerSlotsDirty = true;

        if (!m_bInputAssemblerSlotsDirty)
        {
            for (uint32 uSlot = 0; uSlot < DXGL_ARRAY_SIZE(m_akInputAssemblerSlots); ++uSlot)
            {
                if (m_akInputAssemblerSlots[uSlot].m_pVertexBuffer != NULL)
                {
                    m_bInputAssemblerSlotsDirty |= m_akInputAssemblerSlots[uSlot].m_pVertexBuffer->m_eUsage == eBU_MapInRingBufferTTLFrame;
                    m_bInputAssemblerSlotsDirty |= m_akInputAssemblerSlots[uSlot].m_pVertexBuffer->m_eUsage == eBU_MapInRingBufferTTLOnce;
                }
            }
        }

        if (m_bInputAssemblerSlotsDirty)
        {
            CRY_ASSERT(m_pInputLayout != NULL);
            CRY_ASSERT(m_CurrentEncoder);

            for (uint32 uSlot = 0; uSlot < DXGL_ARRAY_SIZE(m_akInputAssemblerSlots); ++uSlot)
            {
                SBuffer* vb = m_akInputAssemblerSlots[uSlot].m_pVertexBuffer;

                if (vb != NULL)
                {
                    uint32 VBIndex = (DXMETAL_MAX_ENTRIES_BUFFER_ARG_TABLE - 1) - uSlot;

                    if (((MTLVertexDescriptor*)m_pInputLayout->m_pVertexDescriptor).layouts[VBIndex].stride != m_akInputAssemblerSlots[uSlot].m_uStride)
                    {
                        m_bInputLayoutDirty = true;
                    }

                    uint32 offset = 0;
                    id<MTLBuffer> tmpBuffer = nil;
                    vb->GetBufferAndOffset(*this, m_akInputAssemblerSlots[uSlot].m_uOffset, m_uVertexOffset, m_akInputAssemblerSlots[uSlot].m_uStride, tmpBuffer, offset);

                    [m_CurrentEncoder setVertexBuffer: tmpBuffer offset: offset atIndex: VBIndex];
                    uMinVBSlot = VBIndex;
                    MaxSlot = uSlot;
                }
            }

            m_bInputAssemblerSlotsDirty = false;

            //  Igor: check if vertex buffers bindings conflict with vertex constant buffer bindings
            for (int i = uMinVBSlot; i <= m_kStateCache.m_StageCache.m_BufferState[SStageStateCache::eBST_Vertex].m_MaxBufferUsed; ++i)
            {
                CRY_ASSERT(!m_kStateCache.m_StageCache.m_BufferState[SStageStateCache::eBST_Vertex].m_Buffers[i]);
            }

            //  Igor: in case everything is fine and we just unbound constant buffer slots which are used by vertex buffes
            //  make sure those slots won't be owervritten when we apply the constant buffer state
            m_kStateCache.m_StageCache.m_BufferState[SStageStateCache::eBST_Vertex].m_MaxBufferUsed = min(m_kStateCache.m_StageCache.m_BufferState[SStageStateCache::eBST_Vertex].m_MaxBufferUsed, (int32)uMinVBSlot);

            // David:   check if all transient buffers with usage eBU_MapInRingBufferTTLOnce have
            //          had all their mapped data bound.
            for (uint32 uSlot = 0; uSlot < DXGL_ARRAY_SIZE(m_akInputAssemblerSlots); ++uSlot)
            {
                SBuffer* vb = m_akInputAssemblerSlots[uSlot].m_pVertexBuffer;
                if (vb != NULL && vb->m_eUsage == eBU_MapInRingBufferTTLOnce)
                {
                    // Before "setVertexBuffer", members of the list of transient mapped data are erased
                    // in SBuffer::GetBufferAndOffset.
                    // It should then be empty here. If not, some vertex data was mapped and never bound.
                    CRY_ASSERT(vb->m_pTransientMappedData.empty());
                    vb->m_pTransientMappedData.clear();
                }
            }
        }
        //  Confetti End: Igor Lobanchikov

        if (m_bInputLayoutDirty)
        {
            if (m_pInputLayout != NULL)
            {
                for (uint32 uSlot = 0; uSlot < DXGL_ARRAY_SIZE(m_akInputAssemblerSlots); ++uSlot)
                {
                    if (m_akInputAssemblerSlots[uSlot].m_pVertexBuffer != NULL)
                    {
                        uint32 VBIndex = (DXMETAL_MAX_ENTRIES_BUFFER_ARG_TABLE - 1) - uSlot;
                        ((MTLVertexDescriptor*)m_pInputLayout->m_pVertexDescriptor).layouts[VBIndex].stride = m_akInputAssemblerSlots[uSlot].m_uStride;
                    }
                }
            }

            m_bInputLayoutDirty = false;

            m_bPipelineDirty = true;
            //  Igor: make a copy of the vertex descriptor here, since the original one can be changed next time we
            //  bind the buffer with the different stride. This is unlikely but we should better be on the safe side.
            m_kPipelineConfiguration.SetVertexDescriptor(m_pInputLayout->m_pVertexDescriptor);
        }
    }

    //  Confetti BEGIN: Igor Lobanchikov
    void CContext::ProfileLabel(const char* szName)
    {
        m_GPUEventsHelper.AddMarker(szName, CGPUEventsHelper::eAT_Event);
    }

    void CContext::ProfileLabelPush(const char* szName)
    {
        m_GPUEventsHelper.AddMarker(szName, CGPUEventsHelper::eAT_Push);
    }

    void CContext::ProfileLabelPop(const char* szName)
    {
        m_GPUEventsHelper.AddMarker(szName, CGPUEventsHelper::eAT_Pop);
    }

    void CContext::BeginOcclusionQuery(SOcclusionQuery* pQuery)
    {
        //  Igor: Sanity check. query must be started only once
        for (size_t i = 0; i < m_OcclusionQueryList.size(); ++i)
        {
            CRY_ASSERT(m_OcclusionQueryList[i] != pQuery);
        }

        m_OcclusionQueryList.push_back(pQuery);

        //  Igor: Metal supports one query at a time
        //  Multiple queries can be simulated but this might slow down the rendering
        //  and make the code too complicated. Stick to the simple solution for now.
        CRY_ASSERT(m_OcclusionQueryList.size() <= 1);
    }

    void CContext::EndOcclusionQuery(SOcclusionQuery* pQuery)
    {
        for (size_t i = 0; i < m_OcclusionQueryList.size(); )
        {
            if (m_OcclusionQueryList[i] == pQuery)
            {
                m_OcclusionQueryList.erase(m_OcclusionQueryList.begin() + i);
            }
            else
            {
                ++i;
            }
        }

        //  Igor: Metal supports one query at a time
        //  Multiple queries can be simulated but this might slow down the rendering
        //  and make the code too complicated. Stick to the simple solution for now.
        CRY_ASSERT(m_OcclusionQueryList.size() <= 1);
        if (m_CurrentEncoder)
        {
            [m_CurrentEncoder setVisibilityResultMode: MTLVisibilityResultModeDisabled offset : 0];
        }
    }

    void CContext::FlushQueries()
    {
        //  Igor: Metal supports one query at a time
        //  Multiple queries can be simulated but this might slow down the rendering
        //  and make the code too complicated. Stick to the simple solution for now.
        CRY_ASSERT(m_OcclusionQueryList.size() <= 1);
        CRY_ASSERT(m_CurrentEncoder);

        for (size_t i = 0; i < m_OcclusionQueryList.size(); ++i)
        {
            SOcclusionQuery*& pQuery = m_OcclusionQueryList[i];

            pQuery->m_pEventHelper = GetCurrentEventHelper();

            pQuery->m_pQueryData = (uint64*)AllocateQueryInRingBuffer();
            *pQuery->m_pQueryData = 0;

            uint32 uiOffset = (uint8*)pQuery->m_pQueryData - (uint8*)GetQueryRingBuffer().contents;

            [m_CurrentEncoder setVisibilityResultMode: MTLVisibilityResultModeBoolean offset: uiOffset];
        }
    }

    bool CContext::TrySlowCopySubresource(STexture* pDstTexture, uint32 uDstSubresource, uint32 uDstX, uint32 uDstY, uint32 uDstZ,
        STexture* pSrcTexture, uint32 uSrcSubresource, const D3D11_BOX* pSrcBox, const CopyFilterType filterType)
    {
        //  Igor: Allow full copy only in this case. Might extend this later if need this.
        CRY_ASSERT(!uDstSubresource && !uDstX && !uDstY && !uDstZ && !uSrcSubresource);
        return m_CopyTextureHelper.DoTopMipSlowCopy(pDstTexture->m_Texture, pSrcTexture->m_Texture, this, filterType);
    }


    void CContext::FlushStateObjects()
    {
        DXGL_SCOPED_PROFILE("CContext::FlushStateObjects")
        CRY_ASSERT(m_CurrentEncoder);

        //  Flush depth stencil
        {
            SDepthStencilCache& kDSCache(m_kStateCache.m_kDepthStencil);
            if (kDSCache.m_bStencilRefDirty)
            {
                kDSCache.m_bStencilRefDirty = false;
                [m_CurrentEncoder setStencilReferenceValue: kDSCache.m_iStencilRef];
            }

            if (kDSCache.m_bDSSDirty)
            {
                [m_CurrentEncoder setDepthStencilState: kDSCache.m_DepthStencilState];
                kDSCache.m_bDSSDirty = false;
            }
        }

        //  Flush vertex constant buffers
        {
            uint32 uStage = SStageStateCache::eBST_Vertex;
            m_kStateCache.m_StageCache.m_BufferState[uStage].CheckForDynamicBufferUpdates();
            int UsedBufferCount = m_kStateCache.m_StageCache.m_BufferState[uStage].m_MaxBufferUsed - m_kStateCache.m_StageCache.m_BufferState[uStage].m_MinBufferUsed + 1;
            if (UsedBufferCount > 0)
            {
                NSRange range = {static_cast<NSUInteger>(m_kStateCache.m_StageCache.m_BufferState[uStage].m_MinBufferUsed),
                                 static_cast<NSUInteger>(UsedBufferCount)};
                [m_CurrentEncoder setVertexBuffers: m_kStateCache.m_StageCache.m_BufferState[uStage].m_Buffers + m_kStateCache.m_StageCache.m_BufferState[uStage].m_MinBufferUsed
offsets: m_kStateCache.m_StageCache.m_BufferState[uStage].m_Offsets + m_kStateCache.m_StageCache.m_BufferState[uStage].m_MinBufferUsed
withRange: range];

                m_kStateCache.m_StageCache.m_BufferState[uStage].m_MaxBufferUsed = -1;
                m_kStateCache.m_StageCache.m_BufferState[uStage].m_MinBufferUsed = SBufferStateStageCache::s_MaxBuffersPerStage;
            }
        }

        //  Flush pixel constant buffers
        {
            uint32 uStage = SStageStateCache::eBST_Fragment;
            m_kStateCache.m_StageCache.m_BufferState[uStage].CheckForDynamicBufferUpdates();
            int UsedBufferCount = m_kStateCache.m_StageCache.m_BufferState[uStage].m_MaxBufferUsed - m_kStateCache.m_StageCache.m_BufferState[uStage].m_MinBufferUsed + 1;
            if (UsedBufferCount > 0)
            {
                NSRange range = {static_cast<NSUInteger>(m_kStateCache.m_StageCache.m_BufferState[uStage].m_MinBufferUsed),
                                 static_cast<NSUInteger>(UsedBufferCount)};
                [m_CurrentEncoder setFragmentBuffers: m_kStateCache.m_StageCache.m_BufferState[uStage].m_Buffers + m_kStateCache.m_StageCache.m_BufferState[uStage].m_MinBufferUsed
offsets: m_kStateCache.m_StageCache.m_BufferState[uStage].m_Offsets + m_kStateCache.m_StageCache.m_BufferState[uStage].m_MinBufferUsed
withRange: range];

                m_kStateCache.m_StageCache.m_BufferState[uStage].m_MaxBufferUsed = -1;
                m_kStateCache.m_StageCache.m_BufferState[uStage].m_MinBufferUsed = SBufferStateStageCache::s_MaxBuffersPerStage;
            }
        }

        //  Flush rasterizer state
        if (m_kStateCache.m_kRasterizer.m_RateriserDirty & SRasterizerCache::RS_ALL_BUT_SCISSOR_DIRTY)
        {
            SRasterizerCache& kCache(m_kStateCache.m_kRasterizer);

            if (kCache.m_RateriserDirty & SRasterizerCache::RS_CULL_MODE_DIRTY)
            {
                kCache.m_RateriserDirty &= ~SRasterizerCache::RS_CULL_MODE_DIRTY;
                [m_CurrentEncoder setCullMode: kCache.cullMode];
            }

            if (kCache.m_RateriserDirty & SRasterizerCache::RS_DEPTH_BIAS_DIRTY)
            {
                kCache.m_RateriserDirty &= ~SRasterizerCache::RS_DEPTH_BIAS_DIRTY;
                [m_CurrentEncoder setDepthBias: kCache.depthBias slopeScale: kCache.depthSlopeScale clamp: kCache.depthBiasClamp];
            }

            if (kCache.m_RateriserDirty & SRasterizerCache::RS_WINDING_DIRTY)
            {
                kCache.m_RateriserDirty &= ~SRasterizerCache::RS_WINDING_DIRTY;
                [m_CurrentEncoder setFrontFacingWinding: kCache.frontFaceWinding];
            }

            if (kCache.m_RateriserDirty & SRasterizerCache::RS_FILL_MODE_DIRTY)
            {
                kCache.m_RateriserDirty &= ~SRasterizerCache::RS_FILL_MODE_DIRTY;
                [m_CurrentEncoder setTriangleFillMode: kCache.triangleFillMode];
            }
            
            //setDepthClipMode  not supported on ios.
#if defined(AZ_COMPILER_CLANG) && AZ_COMPILER_CLANG >= 9    //@available was added in Xcode 9
#if defined(__MAC_10_11) || defined(__IPHONE_11_0) || defined(__TVOS_11_0)
            if (RenderCapabilities::SupportsDepthClipping() && kCache.m_RateriserDirty & SRasterizerCache::RS_DEPTH_CLIP_MODE_DIRTY)
            {
                kCache.m_RateriserDirty &= ~SRasterizerCache::RS_DEPTH_CLIP_MODE_DIRTY;
                if(@available(macOS 10.11, iOS 11.0, tvOS 11.0, *))
                {
                    [m_CurrentEncoder setDepthClipMode: kCache.depthClipMode];
                }
            }
#endif
#endif
        }

        {
            SRasterizerCache& kCache(m_kStateCache.m_kRasterizer);
            if (kCache.m_RateriserDirty & SRasterizerCache::RS_SCISSOR_ENABLE_DIRTY)
            {
                kCache.m_RateriserDirty &= ~SRasterizerCache::RS_SCISSOR_ENABLE_DIRTY;
                MTLViewport& viewport = m_kStateCache.m_bViewportDefault? m_kStateCache.m_DefaultViewport : m_kStateCache.m_CurrentViewport;
                if (kCache.scissorEnable)
                {
                    DXMETAL_TODO("this is a hack to fix engine bugs. Can remove it if the bugs are fixed.");
                    MTLScissorRect rect = kCache.m_scissorRect;
                    {
                        if (kCache.m_scissorRect.x + kCache.m_scissorRect.width > viewport.width)
                        {
                            //DXGL_WARNING("Metal: Warning: Scissor rect (x(%d) + width(%d))(%d) must be <= %d", (int)kCache.m_scissorRect.x, (int)kCache.m_scissorRect.width, (int)(kCache.m_scissorRect.x+kCache.m_scissorRect.width), //(int)m_kStateCache.m_DefaultViewport.width);
                            rect.width = viewport.width - kCache.m_scissorRect.x;
                            if (!rect.width)
                            {
                                rect.width = 1;
                                rect.x -= 1;
                            }
                        }

                        if (kCache.m_scissorRect.y + kCache.m_scissorRect.height > viewport.height)
                        {
                            //DXGL_WARNING("Metal: Warning: Scissor rect (y(%d) + height(%d))(%d) must be <= %d", (int)kCache.m_scissorRect.y, (int)kCache.m_scissorRect.height, (int)(kCache.m_scissorRect.y+kCache.m_scissorRect.height), (int)m_kStateCache.m_DefaultViewport.height);
                            rect.height = viewport.height - kCache.m_scissorRect.y;
                            if (!rect.height)
                            {
                                rect.height = 1;
                                rect.y -= 1;
                            }
                        }
                    }

                    [m_CurrentEncoder setScissorRect: rect];
                }
                else
                {
                    MTLScissorRect rect = {static_cast<NSUInteger>(viewport.originX),
                                           static_cast<NSUInteger>(viewport.originY),
                                           static_cast<NSUInteger>(viewport.width),
                                           static_cast<NSUInteger>(viewport.height)};
                    [m_CurrentEncoder setScissorRect: rect];
                }
            }
        }

        //  Igor: flush blend color
        if (m_kStateCache.m_bBlendColorDirty)
        {
            m_kStateCache.m_bBlendColorDirty = false;
            [m_CurrentEncoder setBlendColorRed: m_kStateCache.m_akBlendColor.m_afRGBA[0]
                                         green: m_kStateCache.m_akBlendColor.m_afRGBA[1]
                                          blue: m_kStateCache.m_akBlendColor.m_afRGBA[2]
                                         alpha: m_kStateCache.m_akBlendColor.m_afRGBA[3]];
        }

        //  Flush viewport if needed
        if (m_kStateCache.m_bViewportDirty)
        {
            bool bInvalidViewport = false;
            if (m_kStateCache.m_CurrentViewport.originX + m_kStateCache.m_CurrentViewport.width > m_kStateCache.m_DefaultViewport.width)
            {
                LOG_METAL_PIPELINE_ERRORS(@ "DXMETAL: Error: (viewport.originX + viewport.width)(%f) must be between 0.0f and [framebuffer width](%f)", m_kStateCache.m_CurrentViewport.originX + m_kStateCache.m_CurrentViewport.width, m_kStateCache.m_DefaultViewport.width);

                bInvalidViewport = true;
            }

            if (m_kStateCache.m_CurrentViewport.originY + m_kStateCache.m_CurrentViewport.height > m_kStateCache.m_DefaultViewport.height)
            {
                LOG_METAL_PIPELINE_ERRORS(@ "DXMETAL: Error: (viewport.originY + viewport.height)(%f) must be between 0.0f and [framebuffer height](%f)", m_kStateCache.m_CurrentViewport.originY + m_kStateCache.m_CurrentViewport.height, m_kStateCache.m_DefaultViewport.height);

                bInvalidViewport = true;
            }

            if (!bInvalidViewport)
            {
                [m_CurrentEncoder setViewport: m_kStateCache.m_bViewportDefault ? m_kStateCache.m_DefaultViewport : m_kStateCache.m_CurrentViewport];
            }

            m_kStateCache.m_bViewportDirty = false;
        }
    }

    void CContext::FlushCurrentEncoder()
    {
        if (m_CurrentEncoder)
        {
            m_GPUEventsHelper.FlushActions(m_CurrentEncoder, CGPUEventsHelper::eFT_FlushEncoder);

            [m_CurrentEncoder endEncoding];
            [m_CurrentEncoder release];
            m_CurrentEncoder = nil;
            DXMETAL_TODO("Might want to commit command buffer here.");
            //  Pro: more command buffers mean less latency when synchronizing to GPU since we can sync to command buffer end only
            //  Con: too many command buffers mean more work for CPU and less readable frame capture
            //  Call Flush(false) to commit command buffer and start a new one.
        }

        if (m_CurrentBlitEncoder)
        {
            m_GPUEventsHelper.FlushActions(m_CurrentBlitEncoder, CGPUEventsHelper::eFT_FlushEncoder);

            [m_CurrentBlitEncoder endEncoding];
            [m_CurrentBlitEncoder release];
            m_CurrentBlitEncoder = nil;
            DXMETAL_TODO("Might want to commit command buffer here.");
            //  Pro: more command buffers mean less latency when synchronizing to GPU since we can sync to command buffer end only
            //  Con: too many command buffers mean more work for CPU and less readable frame capture
            //  Call Flush(false) to commit command buffer and start a new one.
        }

        if (m_CurrentComputeEncoder)
        {
            m_GPUEventsHelper.FlushActions(m_CurrentComputeEncoder, CGPUEventsHelper::eFT_FlushEncoder);

            [m_CurrentComputeEncoder endEncoding];
            [m_CurrentComputeEncoder release];
            m_CurrentComputeEncoder = nil;
            //m_kPipelineConfiguration.m_apShaders[eST_Compute] = NULL;
            DXMETAL_TODO("Might want to commit command buffer here.");
            //  Pro: more command buffers mean less latency when synchronizing to GPU since we can sync to command buffer end only
            //  Con: too many command buffers mean more work for CPU and less readable frame capture
            //  Call Flush(false) to commit command buffer and start a new one.
        }
    }
    //  Confetti End: Igor Lobanchikov
    id <MTLBlitCommandEncoder> CContext::GetBlitCommandEncoder()
    {
        if (!m_CurrentBlitEncoder)
        {
            FlushCurrentEncoder();
            m_CurrentBlitEncoder = [m_CurrentCommandBuffer blitCommandEncoder];
            // MTLBlitCommandEncoder is an autorelease object
            [m_CurrentBlitEncoder retain];
            m_GPUEventsHelper.FlushActions(m_CurrentBlitEncoder, CGPUEventsHelper::eFT_NewEncoder);
        }
        else
        {
            m_GPUEventsHelper.FlushActions(m_CurrentBlitEncoder, CGPUEventsHelper::eFT_Default);
        }

        return m_CurrentBlitEncoder;
    }
    
    void  CContext::ActivateComputeCommandEncoder()
    {
        if (!m_CurrentComputeEncoder)
        {
            FlushCurrentEncoder();
            m_CurrentComputeEncoder = [m_CurrentCommandBuffer computeCommandEncoder];
            // MTLComputeCommandEncoder is an autorelease object
            [m_CurrentComputeEncoder retain];
            m_GPUEventsHelper.FlushActions(m_CurrentComputeEncoder, CGPUEventsHelper::eFT_NewEncoder);
        }
        else
        {
            m_GPUEventsHelper.FlushActions(m_CurrentComputeEncoder, CGPUEventsHelper::eFT_Default);
        }
    }

    void CContext::FlushFrameBufferState()
    {
        DXGL_SCOPED_PROFILE("CContext::FlushFrameBufferState")

        //  Confetti BEGIN: Igor Lobanchikov

        bool bDoBindRTs = m_bFrameBufferStateDirty || !m_CurrentEncoder;

        //  Igor: if someone requested clear check if this was for one of the current resources.
        //  Rebind them if necessary.
        if (!bDoBindRTs && m_bPossibleClearPending)
        {
            m_bPossibleClearPending = false;

            for (int i = 0; i < DXGL_ARRAY_SIZE(m_CurrentRTs); ++i)
            {
                SOutputMergerTextureView* pTextureView = m_CurrentRTs[i] ? m_CurrentRTs[i]->AsSOutputMergerTextureView() : NULL;
                STexture* pTexture = pTextureView ? pTextureView->m_pTexture : NULL;
                if (pTexture)
                {
                    if (pTexture->m_spTextureViewToClear.get())
                    {
                        CRY_ASSERT(pTexture->m_spTextureViewToClear == m_CurrentRTs[i]);
                        if (pTexture->m_spTextureViewToClear != m_CurrentRTs[i])
                        {
                            DXGL_ERROR("RT View used for rendering does not match view which was used to clear RT. This behaviour is not supported for METAL");
                        }
                        bDoBindRTs = true;
                    }
                }
            }

            if (m_CurrentDepth)
            {
                SOutputMergerTextureView* pTextureView = m_CurrentDepth ? m_CurrentDepth->AsSOutputMergerTextureView() : NULL;
                STexture* pTexture = pTextureView ? pTextureView->m_pTexture : NULL;

                {
                    //  Igor: deside if need to clear at this point
                    if (pTexture->m_spTextureViewToClear.get() && pTexture->m_bClearDepth)
                    {
                        CRY_ASSERT(pTexture->m_spTextureViewToClear == m_CurrentDepth);
                        if (pTexture->m_spTextureViewToClear != m_CurrentDepth)
                        {
                            DXGL_ERROR("Depth View used for rendering does not match view which was used to clear RT. This behaviour is not supported for METAL");
                        }
                        bDoBindRTs = true;
                    }
                }

                //  Igor: attach stencil too
                if (pTexture->m_StencilTexture)
                {
                    //  Igor: deside if need to clear at this point
                    if (pTexture->m_spStencilTextureViewToClear.get() && pTexture->m_bClearStencil)
                    {
                        CRY_ASSERT(pTexture->m_spStencilTextureViewToClear == m_CurrentDepth);
                        if (pTexture->m_spStencilTextureViewToClear != m_CurrentDepth)
                        {
                            DXGL_ERROR("Stencil View used for rendering does not match view which was used to clear RT. This behaviour is not supported for METAL");
                        }
                        bDoBindRTs = true;
                    }
                }
            }

            if (bDoBindRTs)
            {
                ProfileLabel("Forced new MTLRenderCommandEncoder because of RT clear");
            }
        }


        if (bDoBindRTs)
        {
            FlushCurrentEncoder();

            //  Igor: this is autoreleased object.
            MTLRenderPassDescriptor* RenderPass = [MTLRenderPassDescriptor renderPassDescriptor];

            RenderPass.visibilityResultBuffer = m_QueryRingBuffer.m_Buffer;

            //  Igor: this is important to keep RT count equal to metal RT count
            CRY_ASSERT(DXGL_ARRAY_SIZE(m_CurrentRTs) == SAttachmentConfiguration::s_ColorAttachmentDescCount);

            for (int i = 0; i < DXGL_ARRAY_SIZE(m_CurrentRTs); ++i)
            {
                SOutputMergerTextureView* pTextureView = m_CurrentRTs[i] ? m_CurrentRTs[i]->AsSOutputMergerTextureView() : NULL;
                STexture* pTexture = pTextureView ? pTextureView->m_pTexture : NULL;
                MTLRenderPassColorAttachmentDescriptor* ColorAttachment = RenderPass.colorAttachments[i];
                if (pTexture)
                {
                    id<MTLTexture> mtlTexture = pTexture->m_bBackBuffer ? pTexture->m_Texture : pTextureView->m_RTView;
                    ColorAttachment.texture = mtlTexture;

                    // NOTE: ternary syntax crashes profile/release builds!!!
                    // /BuildRoot/Library/Caches/com.apple.xbs/Sources/Metal/Metal-55.1.1/Framework/MTLRenderPass.m:117: failed assertion `storeAction is not a valid MTLStoreAction.'
                    //ColorAttachment.storeAction = pTexture->m_bColorStoreDontCare ? MTLStoreActionDontCare : MTLStoreActionStore;
                    if (pTexture->m_bColorStoreDontCare)
                    {
                        ColorAttachment.storeAction = MTLStoreActionDontCare;
                    }
                    else
                    {
                        ColorAttachment.storeAction = MTLStoreActionStore;
                    }

                    m_kStateCache.m_DefaultViewport.width = pTexture->m_iWidth >> pTextureView->m_iMipLevel;
                    m_kStateCache.m_DefaultViewport.height = pTexture->m_iHeight >> pTextureView->m_iMipLevel;

                    //  Igor: deside if need to clear at this point
                    if (pTexture->m_spTextureViewToClear.get())
                    {
                        CRY_ASSERT(pTexture->m_spTextureViewToClear == m_CurrentRTs[i]);
                        if (pTexture->m_spTextureViewToClear != m_CurrentRTs[i])
                        {
                            DXGL_ERROR("RT View used for rendering does not match view which was used to clear RT. This behaviour is not supported for METAL");
                        }

                        ColorAttachment.loadAction = MTLLoadActionClear;
                        ColorAttachment.clearColor = MTLClearColorMake(pTexture->m_ClearColor[0], pTexture->m_ClearColor[1], pTexture->m_ClearColor[2], pTexture->m_ClearColor[3]);

                        pTexture->m_spTextureViewToClear.reset();
                    }
                    else
                    {
                        // NOTE: ternary syntax crashes profile/release builds!!!
                        // /BuildRoot/Library/Caches/com.apple.xbs/Sources/Metal/Metal-55.1.1/Framework/MTLRenderPass.m:117: failed assertion `storeAction is not a valid MTLStoreAction.'
                        //ColorAttachment.loadAction = pTexture->m_bColorLoadDontCare ? MTLLoadActionDontCare : MTLLoadActionLoad;
                        if (pTexture->m_bColorLoadDontCare)
                        {
                            ColorAttachment.loadAction = MTLLoadActionDontCare;
                        }
                        else
                        {
                            ColorAttachment.loadAction = MTLLoadActionLoad;
                        }
                    }


                    ColorAttachment.level = pTextureView->m_iMipLevel;

                    if (mtlTexture.textureType == MTLTextureType3D)
                    {
                        ColorAttachment.depthPlane = pTextureView->m_iLayer;
                    }
                    else
                    {
                        ColorAttachment.slice = pTextureView->m_iLayer;
                    }
                }
                //  Igor: set color attachement data used for pipeline configuration here
                {
                    m_kPipelineConfiguration.m_AttachmentConfiguration.colorAttachments[i].pixelFormat = ColorAttachment.texture.pixelFormat;
                }

                SMetalBlendState& srcBlendState = m_kStateCache.m_kBlend.colorAttachements[i];
                SMetalBlendState& targetBlendState = m_kPipelineConfiguration.m_AttachmentConfiguration.colorAttachments[i];
                bool bTargetIsBound = (m_kPipelineConfiguration.m_AttachmentConfiguration.colorAttachments[i].pixelFormat != MTLPixelFormatInvalid);
                if (bTargetIsBound)
                {
                    targetBlendState = srcBlendState;
                }
                else
                {
                    targetBlendState.ResetToDefault();
                }
            }

            
            if (m_CurrentDepth)
            {
                SOutputMergerTextureView* pTextureView = m_CurrentDepth ? m_CurrentDepth->AsSOutputMergerTextureView() : NULL;
                STexture* pTexture = pTextureView ? pTextureView->m_pTexture : NULL;
                
                //Since the depth and stencil buffers are combined for OSX we reconfigure the DontCare flags
                //to enforce the correct behaviour. Ideally the driver should be doing this internally but
                //there is no documentation on it.
#if defined(AZ_PLATFORM_MAC) 
                
                //Only do this if the buffers are combined.
                if (pTexture->m_Texture.pixelFormat == MTLPixelFormatDepth32Float_Stencil8 ||
                    pTexture->m_Texture.pixelFormat == MTLPixelFormatDepth24Unorm_Stencil8 ||
                    pTexture->m_Texture.pixelFormat == MTLPixelFormatX32_Stencil8 ||
                    pTexture->m_Texture.pixelFormat == MTLPixelFormatX24_Stencil8)
                {
                    //If depth/stencil is set to load/store dontcare but the other one is not
                    //set the first one to true as well.
                    if(!pTexture->m_bDepthStoreDontCare || !pTexture->m_bStencilStoreDontCare)
                    {
                        pTexture->m_bDepthStoreDontCare= false;
                        pTexture->m_bStencilStoreDontCare = false;
                    }
                
                    if(!pTexture->m_bDepthLoadDontCare || !pTexture->m_bStencilLoadDontCare)
                    {
                        pTexture->m_bDepthLoadDontCare= false;
                        pTexture->m_bStencilLoadDontCare = false;
                    }
                
                    //If depth/stencil is set to LoadDontcare but the other is set to clear set
                    //the first one to clear as well.
                    if(pTexture->m_bDepthLoadDontCare && pTexture->m_bClearStencil)
                    {
                        pTexture->m_bClearDepth = true;
                    }
                    
                    if(pTexture->m_bStencilLoadDontCare && pTexture->m_bClearDepth)
                    {
                        pTexture->m_bClearStencil = true;
                    }
                }
#endif
                
                {
                    MTLRenderPassDepthAttachmentDescriptor* DepthAttachment = RenderPass.depthAttachment;

                    DepthAttachment.texture = pTexture->m_Texture;

                    // NOTE: ternary syntax crashes profile/release builds!!!
                    // /BuildRoot/Library/Caches/com.apple.xbs/Sources/Metal/Metal-55.1.1/Framework/MTLRenderPass.m:117: failed assertion `storeAction is not a valid MTLStoreAction.'
                    //DepthAttachment.storeAction = pTexture->m_bDepthStoreDontCare ? MTLStoreActionDontCare : MTLStoreActionStore;
                    if (pTexture->m_bDepthStoreDontCare)
                    {
                        DepthAttachment.storeAction = MTLStoreActionDontCare;
                    }
                    else
                    {
                        DepthAttachment.storeAction = MTLStoreActionStore;
                    }

                    m_kStateCache.m_DefaultViewport.width = pTexture->m_iWidth >> pTextureView->m_iMipLevel;
                    m_kStateCache.m_DefaultViewport.height = pTexture->m_iHeight >> pTextureView->m_iMipLevel;

                    //  Igor: decide if need to clear at this point
                    if (pTexture->m_spTextureViewToClear.get() &&
                        pTexture->m_bClearDepth)
                    {
                        CRY_ASSERT(pTexture->m_spTextureViewToClear == m_CurrentDepth);
                        if (pTexture->m_spTextureViewToClear != m_CurrentDepth)
                        {
                            DXGL_ERROR("Depth View used for rendering does not match view which was used to clear RT. This behaviour is not supported for METAL");
                        }
                        DepthAttachment.loadAction = MTLLoadActionClear;
                        DepthAttachment.clearDepth = pTexture->m_fClearDepthValue;

                        pTexture->m_bClearDepth = false;
                    }
                    else
                    {
                        // NOTE: ternary syntax crashes profile/release builds!!!
                        // /BuildRoot/Library/Caches/com.apple.xbs/Sources/Metal/Metal-55.1.1/Framework/MTLRenderPass.m:117: failed assertion `storeAction is not a valid MTLStoreAction.'
                        //DepthAttachment.loadAction = pTexture->m_bDepthLoadDontCare ? MTLLoadActionDontCare : MTLLoadActionLoad;
                        if (pTexture->m_bDepthLoadDontCare)
                        {
                            DepthAttachment.loadAction = MTLLoadActionDontCare;
                        }
                        else
                        {
                            DepthAttachment.loadAction = MTLLoadActionLoad;
                        }
                    }
                }

                //  Igor: attach stencil too
                if (pTexture->m_StencilTexture)
                {
                    MTLRenderPassStencilAttachmentDescriptor* StencilAttachment = RenderPass.stencilAttachment;

                    StencilAttachment.texture = pTexture->m_StencilTexture;

                    // NOTE: ternary syntax crashes profile/release builds!!!
                    // /BuildRoot/Library/Caches/com.apple.xbs/Sources/Metal/Metal-55.1.1/Framework/MTLRenderPass.m:117: failed assertion `storeAction is not a valid MTLStoreAction.'
                    //StencilAttachment.storeAction = pTexture->m_bStencilStoreDontCare ? MTLStoreActionDontCare : MTLStoreActionStore;
                    if (pTexture->m_bStencilStoreDontCare)
                    {
                        StencilAttachment.storeAction = MTLStoreActionDontCare;
                    }
                    else
                    {
                        StencilAttachment.storeAction = MTLStoreActionStore;
                    }

                    //  Igor: deside if need to clear at this point
                    if (pTexture->m_spStencilTextureViewToClear.get() &&
                        pTexture->m_bClearStencil)
                    {
                        CRY_ASSERT(pTexture->m_spStencilTextureViewToClear == m_CurrentDepth);
                        if (pTexture->m_spStencilTextureViewToClear != m_CurrentDepth)
                        {
                            DXGL_ERROR("Stencil View used for rendering does not match view which was used to clear RT. This behaviour is not supported for METAL");
                        }
                        StencilAttachment.loadAction = MTLLoadActionClear;
                        StencilAttachment.clearStencil = pTexture->m_uClearStencilValue;

                        pTexture->m_bClearStencil = false;
                    }
                    else
                    {
                        // NOTE: ternary syntax crashes profile/release builds!!!
                        // /BuildRoot/Library/Caches/com.apple.xbs/Sources/Metal/Metal-55.1.1/Framework/MTLRenderPass.m:117: failed assertion `storeAction is not a valid MTLStoreAction.'
                        //StencilAttachment.loadAction = pTexture->m_bStencilLoadDontCare ? MTLLoadActionDontCare : MTLLoadActionLoad;
                        if (pTexture->m_bStencilLoadDontCare)
                        {
                            StencilAttachment.loadAction = MTLLoadActionDontCare;
                        }
                        else
                        {
                            StencilAttachment.loadAction = MTLLoadActionLoad;
                        }
                    }
                }

                if (pTexture)
                {
                    m_kPipelineConfiguration.m_AttachmentConfiguration.depthAttachmentPixelFormat = pTexture->m_Texture ? pTexture->m_Texture.pixelFormat : MTLPixelFormatInvalid;
                    m_kPipelineConfiguration.m_AttachmentConfiguration.stencilAttachmentPixelFormat = pTexture->m_StencilTexture ? pTexture->m_StencilTexture.pixelFormat : MTLPixelFormatInvalid;
                }
                else
                {
                    m_kPipelineConfiguration.m_AttachmentConfiguration.depthAttachmentPixelFormat = MTLPixelFormatInvalid;
                    m_kPipelineConfiguration.m_AttachmentConfiguration.stencilAttachmentPixelFormat = MTLPixelFormatInvalid;
                }


                pTexture->m_spTextureViewToClear.reset();
                pTexture->m_spStencilTextureViewToClear.reset();

                // Reset don't care flags
                pTexture->ResetDontCareActionFlags();
            }
            else
            {
                m_kPipelineConfiguration.m_AttachmentConfiguration.depthAttachmentPixelFormat = MTLPixelFormatInvalid;
                m_kPipelineConfiguration.m_AttachmentConfiguration.stencilAttachmentPixelFormat = MTLPixelFormatInvalid;
            }


            m_CurrentEncoder = [m_CurrentCommandBuffer renderCommandEncoderWithDescriptor:RenderPass];
            [m_CurrentEncoder retain];
            m_bFrameBufferStateDirty = false;

            m_GPUEventsHelper.FlushActions(m_CurrentEncoder, CGPUEventsHelper::eFT_NewEncoder);
            FlushQueries();

            // Since pipeline state depends upon the current renter targets' configuration we
            // need to make a new one and to rebind it.
            m_bPipelineDirty = true;

            //  It seems that the encoder is reset to the default state every time we create a new one.
            //  Mark states as dirty here so that they a rebound.
            m_bInputAssemblerSlotsDirty = true;
            m_kStateCache.m_kDepthStencil.m_bDSSDirty = true;
            m_kStateCache.m_kDepthStencil.m_bStencilRefDirty = true;
            if (!(m_kStateCache.m_kRasterizer.m_RateriserDirty & SRasterizerCache::RS_NOT_INITIALIZED))
            {
                m_kStateCache.m_kRasterizer.m_RateriserDirty = SRasterizerCache::RS_ALL_DIRTY;
            }
            m_kStateCache.m_bBlendColorDirty = true;

            //  Update viewport state
            m_kStateCache.m_bViewportDirty = !m_kStateCache.m_bViewportDefault;

            for (int i = 0; i < SStageStateCache::eBST_Count; ++i)
            {
                m_kStateCache.m_StageCache.m_BufferState[i].m_MaxBufferUsed = SBufferStateStageCache::s_MaxBuffersPerStage - 1;
                m_kStateCache.m_StageCache.m_BufferState[i].m_MinBufferUsed = 0;
                m_kStateCache.m_StageCache.m_TextureState[i].m_MaxTextureUsed = STextureStageState::s_MaxTexturesPerStage - 1;
                m_kStateCache.m_StageCache.m_TextureState[i].m_MinTextureUsed = 0;


                //m_kStateCache.m_StageCache.m_SamplerState[i].m_MaxSamplerUsed = SSamplerStageState::s_MaxSamplersPerStage-1;
                //m_kStateCache.m_StageCache.m_SamplerState[i].m_MinSamplerUsed = 0;
                m_kStateCache.m_StageCache.m_SamplerState[i].m_MaxSamplerUsed = -1;
                m_kStateCache.m_StageCache.m_SamplerState[i].m_MinSamplerUsed = SSamplerStageState::s_MaxSamplersPerStage;
                for (int j = 0; j < SSamplerStageState::s_MaxSamplersPerStage; ++j)
                {
                    if (m_kStateCache.m_StageCache.m_SamplerState[i].m_Samplers[j])
                    {
                        m_kStateCache.m_StageCache.m_SamplerState[i].m_MaxSamplerUsed = max(m_kStateCache.m_StageCache.m_SamplerState[i].m_MaxSamplerUsed, j);
                        m_kStateCache.m_StageCache.m_SamplerState[i].m_MinSamplerUsed = min(m_kStateCache.m_StageCache.m_SamplerState[i].m_MinSamplerUsed, j);
                    }
                }
            }
        }
        else
        {
            //  Igor: flush in this function since:
            //  1. This is the first function which actually flushes the state
            //  2. debug markers are applied to the command buffer encoder. Flushing before command buffer encoder is created for the new RTs will leave markers in incorrect encoder.
            m_GPUEventsHelper.FlushActions(m_CurrentEncoder, CGPUEventsHelper::eFT_Default);
            FlushQueries();
        }
        //  Confetti End: Igor Lobanchikov
    }

    void CContext::FlushComputeKernelState()
    {
        DXGL_SCOPED_PROFILE("CContext::FlushComputeKernelState")

        int i = SStageStateCache::eBST_Compute;
        {
            m_kStateCache.m_StageCache.m_BufferState[i].m_MaxBufferUsed = SBufferStateStageCache::s_MaxBuffersPerStage - 1;
            m_kStateCache.m_StageCache.m_BufferState[i].m_MinBufferUsed = 0;
            m_kStateCache.m_StageCache.m_TextureState[i].m_MaxTextureUsed = STextureStageState::s_MaxTexturesPerStage - 1;
            m_kStateCache.m_StageCache.m_TextureState[i].m_MinTextureUsed = 0;

            m_kStateCache.m_StageCache.m_SamplerState[i].m_MaxSamplerUsed = -1;
            m_kStateCache.m_StageCache.m_SamplerState[i].m_MinSamplerUsed = SSamplerStageState::s_MaxSamplersPerStage;
            for (int j = 0; j < SSamplerStageState::s_MaxSamplersPerStage; ++j)
            {
                if (m_kStateCache.m_StageCache.m_SamplerState[i].m_Samplers[j])
                {
                    m_kStateCache.m_StageCache.m_SamplerState[i].m_MaxSamplerUsed = max(m_kStateCache.m_StageCache.m_SamplerState[i].m_MaxSamplerUsed, j);
                    m_kStateCache.m_StageCache.m_SamplerState[i].m_MinSamplerUsed = min(m_kStateCache.m_StageCache.m_SamplerState[i].m_MinSamplerUsed, j);
                }
            }
        }
    }

    void CContext::FlushComputeBufferUnits()
    {
        DXGL_SCOPED_PROFILE("CContext::FlushComputeBufferUnits")
        //  Flush compute constant buffers
        {
            uint32 uStage = SStageStateCache::eBST_Compute;
            m_kStateCache.m_StageCache.m_BufferState[uStage].CheckForDynamicBufferUpdates();
            int UsedBufferCount = m_kStateCache.m_StageCache.m_BufferState[uStage].m_MaxBufferUsed - m_kStateCache.m_StageCache.m_BufferState[uStage].m_MinBufferUsed + 1;
            if (UsedBufferCount > 0)
            {
                NSRange range = {static_cast<NSUInteger>(m_kStateCache.m_StageCache.m_BufferState[uStage].m_MinBufferUsed),
                                 static_cast<NSUInteger>(UsedBufferCount)};
                [m_CurrentComputeEncoder setBuffers: m_kStateCache.m_StageCache.m_BufferState[uStage].m_Buffers + m_kStateCache.m_StageCache.m_BufferState[uStage].m_MinBufferUsed
offsets: m_kStateCache.m_StageCache.m_BufferState[uStage].m_Offsets + m_kStateCache.m_StageCache.m_BufferState[uStage].m_MinBufferUsed
withRange: range];

                m_kStateCache.m_StageCache.m_BufferState[uStage].m_MaxBufferUsed = -1;
                m_kStateCache.m_StageCache.m_BufferState[uStage].m_MinBufferUsed = SBufferStateStageCache::s_MaxBuffersPerStage;
            }
        }
    }

    void CContext::FlushComputeTextureUnits()
    {
        DXGL_SCOPED_PROFILE("CContext::FlushComputeTextureUnits")
        {
            //  Set CS Samplers
            SSamplerStageState& samplerState = m_kStateCache.m_StageCache.m_SamplerState[SStageStateCache::eBST_Compute];

            NSRange range = {static_cast<NSUInteger>(samplerState.m_MinSamplerUsed),
                             static_cast<NSUInteger>(samplerState.m_MaxSamplerUsed - samplerState.m_MinSamplerUsed + 1)};
            if (samplerState.m_MaxSamplerUsed >= samplerState.m_MinSamplerUsed)
            {
                [m_CurrentComputeEncoder setSamplerStates: (patchSamplers(samplerState.m_Samplers, GetDevice()->GetMetalDevice(), m_defaultSamplerState) + samplerState.m_MinSamplerUsed)
withRange: range];
            }

            samplerState.m_MinSamplerUsed =  SSamplerStageState::s_MaxSamplersPerStage;
            samplerState.m_MaxSamplerUsed = -1;
        }


        {
            //  Set CS textures
            STextureStageState& textureState = m_kStateCache.m_StageCache.m_TextureState[SStageStateCache::eBST_Compute];
            id<MTLTexture> tmpTextures[STextureStageState::s_MaxTexturesPerStage];

            for (int i = textureState.m_MinTextureUsed; i <= textureState.m_MaxTextureUsed; ++i)
            {
                tmpTextures[i] = textureState.m_Textures[i] ? textureState.m_Textures[i]->GetMetalTexture() : 0;
            }

            NSRange range = {static_cast<NSUInteger>(textureState.m_MinTextureUsed),
                             static_cast<NSUInteger>(textureState.m_MaxTextureUsed - textureState.m_MinTextureUsed + 1)};
            if (textureState.m_MaxTextureUsed >= textureState.m_MinTextureUsed)
            {
                [m_CurrentComputeEncoder setTextures: (tmpTextures + textureState.m_MinTextureUsed)
withRange: range];
            }

            textureState.m_MinTextureUsed =  STextureStageState::s_MaxTexturesPerStage;
            textureState.m_MaxTextureUsed = -1;
        }

        {
            //  Set CS UAV Textures
            SUAVTextureStageCache& textureState = m_kStateCache.m_StageCache.m_UAVTextureState;
            id<MTLTexture> tmpTextures[STextureStageState::s_MaxTexturesPerStage];

            for (int i = 0; i <= textureState.s_MaxUAVTexturesPerStage; ++i)
            {
                tmpTextures[i] = textureState.m_UAVTextures[i] ? textureState.m_UAVTextures[i]->m_Texture : 0;
            }

            NSRange range = {STextureStageState::s_MaxTexturesPerStage, SUAVTextureStageCache::s_MaxUAVTexturesPerStage};
            [m_CurrentComputeEncoder setTextures: (tmpTextures)
withRange: range];
        }
    }

    void CContext::FlushComputeThreadGroup(uint32 u32_Group_x, uint32 u32_Group_y, uint32 u32_Group_z)
    {
        DXGL_SCOPED_PROFILE("CContext::FlushComputeThreadGroup")
        CRY_ASSERT(m_kPipelineConfiguration.m_apShaders[eST_Compute] != NULL);
        uint32 thread_x = m_kPipelineConfiguration.m_apShaders[eST_Compute]->m_kReflection.m_uThread_x;
        uint32 thread_y = m_kPipelineConfiguration.m_apShaders[eST_Compute]->m_kReflection.m_uThread_y;
        uint32 thread_z = m_kPipelineConfiguration.m_apShaders[eST_Compute]->m_kReflection.m_uThread_z;

        MTLSize threadsPerGroup = {thread_x, thread_y, thread_z};
        MTLSize numThreadGroup = {u32_Group_x, u32_Group_y, u32_Group_z};

        [m_CurrentComputeEncoder dispatchThreadgroups: numThreadGroup
threadsPerThreadgroup: threadsPerGroup];
    }

    void CContext::FlushComputePipelineState()
    {
        DXGL_SCOPED_PROFILE("CContext::FlushComputePipelineState")
        if (m_bPipelineDirty)
        {
            m_spPipeline = AllocatePipeline(m_kPipelineConfiguration);
            if (m_spPipeline != NULL)
            {
                [m_CurrentComputeEncoder setComputePipelineState: m_spPipeline->m_ComputePipelineState];
            }

            m_bPipelineDirty = false;
        }
        else
        {
            [m_CurrentComputeEncoder setComputePipelineState: m_spPipeline->m_ComputePipelineState];
        }
    }

    void CContext::Dispatch(uint32 u32_Group_x, uint32 u32_Group_y, uint32 u32_Group_z)
    {
        ActivateComputeCommandEncoder();
        FlushComputeKernelState();
        FlushComputePipelineState();
        FlushComputeTextureUnits();
        FlushComputeBufferUnits();
        FlushComputeThreadGroup(u32_Group_x, u32_Group_y, u32_Group_z);
    }

    void CContext::SetUAVBuffer(SBuffer* pConstantBuffer, uint32 uStage, uint32 uSlot)
    {
        uSlot = uSlot + SBufferStateStageCache::s_MaxConstantBuffersPerStage;
        CRY_ASSERT(uStage < SStageStateCache::eBST_Count);
        CRY_ASSERT(uSlot < SBufferStateStageCache::s_MaxBuffersPerStage);

        m_kStateCache.m_StageCache.m_BufferState[uStage].m_spBufferResource[uSlot].reset(pConstantBuffer);

        if (m_kStateCache.m_StageCache.m_BufferState[uStage].m_Buffers[uSlot])
        {
            [m_kStateCache.m_StageCache.m_BufferState[uStage].m_Buffers[uSlot] release];
        }

        if (pConstantBuffer)
        {
            id<MTLBuffer> mtlBuffer = GetMtlBufferBasedOnSize(pConstantBuffer);
            CRY_ASSERT(mtlBuffer);
            
            CRY_ASSERT(pConstantBuffer->m_pMappedData || pConstantBuffer->m_pSystemMemoryCopy || !pConstantBuffer->m_pfMapBufferRange);

            m_kStateCache.m_StageCache.m_BufferState[uStage].m_Buffers[uSlot] = mtlBuffer;
            [m_kStateCache.m_StageCache.m_BufferState[uStage].m_Buffers[uSlot] retain];
            
            m_kStateCache.m_StageCache.m_BufferState[uStage].m_Offsets[uSlot] = (uint8*)pConstantBuffer->m_pMappedData - (uint8*)mtlBuffer.contents;
        }
        else
        {
            m_kStateCache.m_StageCache.m_BufferState[uStage].m_Buffers[uSlot] = 0;
            m_kStateCache.m_StageCache.m_BufferState[uStage].m_Offsets[uSlot] = 0;
        }

        m_kStateCache.m_StageCache.m_BufferState[uStage].m_MaxBufferUsed = max(m_kStateCache.m_StageCache.m_BufferState[uStage].m_MaxBufferUsed, (int32)uSlot);
        m_kStateCache.m_StageCache.m_BufferState[uStage].m_MinBufferUsed = min(m_kStateCache.m_StageCache.m_BufferState[uStage].m_MinBufferUsed, (int32)uSlot);
    }

    void CContext::SetUAVTexture(STexture* pTexture, uint32 uStage, uint32 uSlot)
    {
        if (uSlot >= SUAVTextureStageCache::s_MaxUAVTexturesPerStage)
        {
            DXGL_WARNING("UAVTexture unit slot %d not available for stage %d - uav texture setting ignored", uSlot, uStage);
            return;
        }
        if (pTexture != NULL && pTexture->m_spTextureViewToClear.get())
        {
            //dont clear uav
            pTexture->m_spTextureViewToClear.reset();
        }
        SUAVTextureStageCache& uavState = m_kStateCache.m_StageCache.m_UAVTextureState;

        uavState.m_UAVTextures[uSlot].reset(pTexture);
    }

    void CContext::FlushPipelineState()
    {
        DXGL_SCOPED_PROFILE("CContext::FlushPipelineState")

        //  Confetti BEGIN: Igor Lobanchikov
        if (m_bPipelineDirty)
        {
            m_spPipeline = AllocatePipeline(m_kPipelineConfiguration);
            if (m_spPipeline != NULL)
            {
                [m_CurrentEncoder setRenderPipelineState: m_spPipeline->m_PipelineState];
            }

            m_bPipelineDirty = false;
        }
        //  Confetti End: Igor Lobanchikov
    }

    void CContext::FlushDrawState()
    {
        DXGL_SCOPED_PROFILE("CContext::FlushDrawState")

        //  Confetti BEGIN: Igor Lobanchikov
        FlushFrameBufferState();

        FlushStateObjects();

        FlushInputAssemblerState();
        FlushPipelineState();
        FlushTextureUnits();
        //  Confetti End: Igor Lobanchikov
    }

    void CContext::SetNumPatchControlPoints(uint32 iNumPatchControlPoints)
    {
        DXGL_SCOPED_PROFILE("CContext::SetNumPatchControlPoints");

        DXGL_WARNING("CContext::SetNumPatchControlPoints - OpenGL(ES) Version does not support tesselation");
    }

    void CContext::SetVertexOffset(uint32 uVertexOffset)
    {
        //  Igor: this will trigger vertex buffer rebinding but
        //  won't cause input layout manipulations.
        //  So we won't to rebind pipeline too.
        if (CACHE_VAR(m_uVertexOffset, uVertexOffset))
        {
            m_bInputAssemblerSlotsDirty = true;
        }
    }

    void CContext::SetViewports(uint32 uNumViewports, const D3D11_VIEWPORT* pViewports)
    {
        DXGL_SCOPED_PROFILE("CContext::SetViewports")

        if (uNumViewports > DXGL_NUM_SUPPORTED_VIEWPORTS)
        {
            DXGL_WARNING("Setting more viewports than supported (" DXGL_QUOTE(NUM_SUPPORTED_VIEWPORTS) "), additional viewports are ignored");
            uNumViewports = DXGL_NUM_SUPPORTED_VIEWPORTS;
        }

        if (uNumViewports && pViewports)
        {
            MTLViewport viewPort = {
                pViewports->TopLeftX, pViewports->TopLeftY,
                pViewports->Width,    pViewports->Height,
                pViewports->MinDepth, pViewports->MaxDepth
            };

            if (memcmp(&m_kStateCache.m_CurrentViewport, &viewPort, sizeof(m_kStateCache.m_CurrentViewport)))
            {
                m_kStateCache.m_CurrentViewport = viewPort;
                m_kStateCache.m_bViewportDirty = true;
                m_kStateCache.m_bViewportDefault = false;
            }
        }
        else
        {
            if (memcmp(&m_kStateCache.m_CurrentViewport, &m_kStateCache.m_DefaultViewport, sizeof(m_kStateCache.m_CurrentViewport)))
            {
                m_kStateCache.m_bViewportDirty = true;
                m_kStateCache.m_bViewportDefault = true;
            }
        }
    }

    void CContext::SetScissorRects(uint32 uNumRects, const D3D11_RECT* pRects)
    {
        DXGL_SCOPED_PROFILE("CContext::SetScissorRects")

        if (uNumRects > DXGL_NUM_SUPPORTED_SCISSOR_RECTS)
        {
            DXGL_WARNING("Setting more scissor rectangles than supported (" DXGL_QUOTE(DXGL_NUM_SUPPORTED_SCISSOR_RECTS) "), additional scissor rectangles are ignored");
            uNumRects = DXGL_NUM_SUPPORTED_SCISSOR_RECTS;
        }

        MTLScissorRect& rect = m_kStateCache.m_kRasterizer.m_scissorRect;
        rect.x = pRects->left;
        rect.y = pRects->top;
        rect.width = pRects->right - pRects->left;
        rect.height = pRects->bottom - pRects->top;

        m_kStateCache.m_kRasterizer.m_RateriserDirty |= SRasterizerCache::RS_SCISSOR_ENABLE_DIRTY;
    }

    void CContext::ClearRenderTarget(SOutputMergerView* pRenderTargetView, const float afColor[4])
    {
        DXGL_SCOPED_PROFILE("CContext::ClearRenderTarget")

        //  Confetti BEGIN: Igor Lobanchikov

        SOutputMergerTextureView * pTextureView = pRenderTargetView ? pRenderTargetView->AsSOutputMergerTextureView() : NULL;
        STexture* resToClear = pTextureView ? pTextureView->m_pTexture : NULL;

        if (resToClear)
        {
            CRY_ASSERT(!resToClear->m_spTextureViewToClear.get() || (resToClear->m_spTextureViewToClear.get() == pRenderTargetView));
            if (resToClear->m_spTextureViewToClear.get() && (resToClear->m_spTextureViewToClear.get() != pRenderTargetView))
            {
                DXGL_ERROR("Render target's view was already cleared. Don't support multiple view clears on the same texture.");
            }

            CRY_ASSERT(!resToClear->m_bColorLoadDontCare);
            if (resToClear->m_bColorLoadDontCare)
            {
                DXGL_ERROR("Resource was given MTLLoadActionDontCare flag. Render target's view cannot be set to be cleared.");
            }


            //  Store deferred clear information.
            resToClear->m_spTextureViewToClear = pRenderTargetView;
            for (int i = 0; i < 4; ++i)
            {
                resToClear->m_ClearColor[i] = afColor[i];
            }

            m_bPossibleClearPending = true;
        }
        //  Confetti End: Igor Lobanchikov
    }

    void CContext::ClearDepthStencil(SOutputMergerView* pDepthStencilView, bool bClearDepth, bool bClearStencil, float fDepthValue, uint8 uStencilValue)
    {
        if (!bClearDepth && !bClearStencil)
        {
            return;
        }
        
        DXGL_SCOPED_PROFILE("CContext::ClearDepthStencil")

        //  Confetti BEGIN: Igor Lobanchikov

        SOutputMergerTextureView * pTextureView = pDepthStencilView ? pDepthStencilView->AsSOutputMergerTextureView() : NULL;
        STexture* resToClear = pTextureView ? pTextureView->m_pTexture : NULL;

        if (resToClear)
        {
            if (bClearDepth)
            {
                //  Once the texture is cleared, it must be bound as rt before second clear can be issued unless we clear the same view and different plane (depth and stencil can be cleared in 2 calls).
                CRY_ASSERT(!resToClear->m_spTextureViewToClear.get() || resToClear->m_spTextureViewToClear == pDepthStencilView);
                if (resToClear->m_spTextureViewToClear.get() && resToClear->m_spTextureViewToClear != pDepthStencilView)
                {
                    DXGL_ERROR("Different view of this depth buffer was already cleared. Don't support multiple clears on different views.");
                }
                
                //  Store deferred clear information.
                resToClear->m_spTextureViewToClear = pDepthStencilView;
                
                CRY_ASSERT(!resToClear->m_bDepthLoadDontCare);
                if (resToClear->m_bDepthLoadDontCare)
                {
                    DXGL_ERROR("Resource was given MTLLoadActionDontCare depth flag. Depth target's view cannot be set to be cleared.");
                }

                resToClear->m_bClearDepth = bClearDepth;
                resToClear->m_fClearDepthValue = fDepthValue;
            }

            if (bClearStencil)
            {
                //  Once the texture is cleared, it must be bound as rt before second clear can be issued unless we clear the same view and different plane (depth and stencil can be cleared in 2 calls).
                CRY_ASSERT(!resToClear->m_spStencilTextureViewToClear.get() || resToClear->m_spStencilTextureViewToClear == pDepthStencilView);
                if (resToClear->m_spStencilTextureViewToClear.get() && resToClear->m_spStencilTextureViewToClear != pDepthStencilView)
                {
                    DXGL_ERROR("Different view of this stencil buffer was already cleared. Don't support multiple clears on different views.");
                }
                
                //  Store deferred clear information.
                resToClear->m_spStencilTextureViewToClear = pDepthStencilView;
                
                CRY_ASSERT(!resToClear->m_bStencilLoadDontCare);
                if (resToClear->m_bStencilLoadDontCare)
                {
                    DXGL_ERROR("Resource was given MTLLoadActionDontCare stencil flag. Stencil target's view cannot be set to be cleared.");
                }

                resToClear->m_bClearStencil = bClearStencil;
                resToClear->m_uClearStencilValue = uStencilValue;
            }

            m_bPossibleClearPending = true;
        }
        //  Confetti End: Igor Lobanchikov
    }

    void CContext::SetRenderTargets(uint32 uNumRTViews, SOutputMergerView** apRenderTargetViews, SOutputMergerView* pDepthStencilView)
    {
        DXGL_SCOPED_PROFILE("CContext::SetRenderTargets")

        //  Confetti BEGIN: Igor Lobanchikov
        //  Igor: sometimes the engine re-assignes the same RT. Avoid flushing/resyoring RT in this case.
        //m_bFrameBufferStateDirty = true;

        uint32 uColorView;
        for (uColorView = 0; uColorView < uNumRTViews; ++uColorView)
        {
            m_bFrameBufferStateDirty |= (m_CurrentRTs[uColorView] != apRenderTargetViews[uColorView]);
            m_CurrentRTs[uColorView] = apRenderTargetViews[uColorView];
        }

        for (; uColorView < DXGL_ARRAY_SIZE(m_CurrentRTs); ++uColorView)
        {
            m_bFrameBufferStateDirty |= !!m_CurrentRTs[uColorView];
            m_CurrentRTs[uColorView] = NULL;
        }

        m_bFrameBufferStateDirty |= (m_CurrentDepth != pDepthStencilView);
        m_CurrentDepth = pDepthStencilView;
        //  Confetti End: Igor Lobanchikov

        if (m_bFrameBufferStateDirty)
        {
            m_GPUEventsHelper.OnSetRenderTargets();
        }
    }

    void CContext::SetShader(SShader* pShader, uint32 uStage)
    {
        CRY_ASSERT(uStage < DXGL_ARRAY_SIZE(m_kPipelineConfiguration.m_apShaders));
        m_kPipelineConfiguration.m_apShaders[uStage] = pShader;
        m_bPipelineDirty = true;
    }

    void CContext::SetTexture(SShaderResourceView* pView, uint32 uStage, uint32 uSlot)
    {
        //  Confetti BEGIN: Igor Lobanchikov
        if (uSlot >= STextureStageState::s_MaxTexturesPerStage)
        {
            DXGL_WARNING("Texture unit slot %d not available for stage %d - texture setting ignored", uSlot, uStage);
            return;
        }
        STextureStageState& textureState = m_kStateCache.m_StageCache.m_TextureState[uStage];

        textureState.m_MaxTextureUsed = max(textureState.m_MaxTextureUsed, (int32)uSlot);
        textureState.m_MinTextureUsed = min(textureState.m_MinTextureUsed, (int32)uSlot);

        textureState.m_Textures[uSlot].reset(pView);
        //  Confetti End: Igor Lobanchikov
    }

    //  Confetti BEGIN: Igor Lobanchikov
    void CContext::SetSampler(id<MTLSamplerState> pState, uint32 uStage, uint32 uSlot)
    {
        if (uSlot >= SSamplerStageState::s_MaxSamplersPerStage)
        {
            DXGL_WARNING("Sampler unit slot %d not available for stage %d - sampler setting ignored", uSlot, uStage);
            return;
        }
        SSamplerStageState& samplerState = m_kStateCache.m_StageCache.m_SamplerState[uStage];

        samplerState.m_MaxSamplerUsed = max(samplerState.m_MaxSamplerUsed, (int32)uSlot);
        samplerState.m_MinSamplerUsed = min(samplerState.m_MinSamplerUsed, (int32)uSlot);

        if (samplerState.m_Samplers[uSlot])
        {
            [samplerState.m_Samplers[uSlot] release];
        }

        samplerState.m_Samplers[uSlot] = pState;

        if (samplerState.m_Samplers[uSlot])
        {
            [samplerState.m_Samplers[uSlot] retain];
        }
    }
    //  Confetti End: Igor Lobanchikov

    //  Confetti BEGIN: Igor Lobanchikov

    void SBufferStateStageCache::CheckForDynamicBufferUpdates()
    {
        for (int i = 0; i < s_MaxBuffersPerStage; ++i)
        {
            id<MTLBuffer> mtlBuffer = GetMtlBufferBasedOnSize(m_spBufferResource[i]);
            NSUInteger offset = (m_spBufferResource[i] && m_spBufferResource[i]->m_pMappedData) ? (uint8*)m_spBufferResource[i]->m_pMappedData - (uint8*)mtlBuffer.contents : 0;

            if (m_spBufferResource[i] &&
                ((mtlBuffer != m_Buffers[i]) ||
                 (offset != m_Offsets[i])))
            {
                if (m_Buffers[i])
                {
                    [m_Buffers[i] release];
                }

                m_Buffers[i] = mtlBuffer;
                m_Offsets[i] = offset;

                if (m_Buffers[i])
                {
                    [m_Buffers[i] retain];
                }

                m_MaxBufferUsed = max(m_MaxBufferUsed, i);
                m_MinBufferUsed = min(m_MinBufferUsed, i);
            }
        }
    }
    //  Confetti End: Igor Lobanchikov

    void CContext::SetConstantBuffer(SBuffer* pConstantBuffer, uint32 uStage, uint32 uSlot)
    {
        //  Confetti BEGIN: Igor Lobanchikov
        CRY_ASSERT(uStage < SStageStateCache::eBST_Count);
        CRY_ASSERT(uSlot < SBufferStateStageCache::s_MaxConstantBuffersPerStage);

        m_kStateCache.m_StageCache.m_BufferState[uStage].m_spBufferResource[uSlot].reset(pConstantBuffer);

        if (m_kStateCache.m_StageCache.m_BufferState[uStage].m_Buffers[uSlot])
        {
            [m_kStateCache.m_StageCache.m_BufferState[uStage].m_Buffers[uSlot] release];
        }

        if (pConstantBuffer)
        {
            id<MTLBuffer> mtlBuffer = GetMtlBufferBasedOnSize(pConstantBuffer);
            CRY_ASSERT(mtlBuffer);
            //  Igor: Buffer must be initialized. Either it is static (and initialized at creation time) or dynamic and was mapped
            CRY_ASSERT(pConstantBuffer->m_pMappedData || pConstantBuffer->m_pSystemMemoryCopy || !pConstantBuffer->m_pfMapBufferRange);

            m_kStateCache.m_StageCache.m_BufferState[uStage].m_Buffers[uSlot] = mtlBuffer;
            [m_kStateCache.m_StageCache.m_BufferState[uStage].m_Buffers[uSlot] retain];

            m_kStateCache.m_StageCache.m_BufferState[uStage].m_Offsets[uSlot] = (uint8*)pConstantBuffer->m_pMappedData - (uint8*)pConstantBuffer->m_BufferShared.contents;
            
        }
        else
        {
            m_kStateCache.m_StageCache.m_BufferState[uStage].m_Buffers[uSlot] = 0;
            m_kStateCache.m_StageCache.m_BufferState[uStage].m_Offsets[uSlot] = 0;
        }

        m_kStateCache.m_StageCache.m_BufferState[uStage].m_MaxBufferUsed = max(m_kStateCache.m_StageCache.m_BufferState[uStage].m_MaxBufferUsed, (int32)uSlot);
        m_kStateCache.m_StageCache.m_BufferState[uStage].m_MinBufferUsed = min(m_kStateCache.m_StageCache.m_BufferState[uStage].m_MinBufferUsed, (int32)uSlot);
        //  Confetti End: Igor Lobanchikov
    }

    void CContext::SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY eTopology)
    {
        //  Confetti BEGIN: Igor Lobanchikov
        switch (eTopology)
        {
        case D3D_PRIMITIVE_TOPOLOGY_POINTLIST:
            m_MetalPrimitiveType = MTLPrimitiveTypePoint;
            break;

        case D3D_PRIMITIVE_TOPOLOGY_LINELIST:
            m_MetalPrimitiveType = MTLPrimitiveTypeLine;
            break;

        case D3D_PRIMITIVE_TOPOLOGY_LINESTRIP:
            m_MetalPrimitiveType = MTLPrimitiveTypeLineStrip;
            break;

        case D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
            m_MetalPrimitiveType = MTLPrimitiveTypeTriangle;
            break;

        case D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:
            m_MetalPrimitiveType = MTLPrimitiveTypeTriangleStrip;
            break;

        default:
            DXGL_ERROR("Invalid primitive topology");
            break;
        }
        //  Confetti End: Igor Lobanchikov
    }

    void CContext::SetInputLayout(SInputLayout* pInputLayout)
    {
        m_pInputLayout = pInputLayout;
        m_bInputLayoutDirty = true;
    }

    void CContext::SetVertexBuffer(uint32 uSlot, SBuffer* pVertexBuffer, uint32 uStride, uint32 uOffset)
    {
        SInputAssemblerSlot& kSlot(m_akInputAssemblerSlots[uSlot]);
        kSlot.m_pVertexBuffer = pVertexBuffer;
        kSlot.m_uStride = uStride;
        kSlot.m_uOffset = uOffset;
        m_bInputAssemblerSlotsDirty = true;
    }

    void CContext::SetIndexBuffer(SBuffer* pIndexBuffer, MTLIndexType eIndexType, GLuint uIndexStride, GLuint uOffset)
    {
        m_MetalIndexType = eIndexType;
        m_uIndexStride = uIndexStride;
        m_uIndexOffset = uOffset;

        m_spIndexBufferResource.reset(pIndexBuffer);
    }

    void CContext::DrawIndexed(uint32 uIndexCount, uint32 uStartIndexLocation, uint32 uBaseVertexLocation)
    {
        DXGL_SCOPED_PROFILE("CContext::DrawIndexed")

        SetVertexOffset(uBaseVertexLocation);

        FlushDrawState();        
        CRY_ASSERT(m_spIndexBufferResource.get());
        if (!m_spIndexBufferResource.get())
        {
            return;
        }

        uint32 offset = 0;
        id<MTLBuffer> tmpBuffer = nil;
        m_spIndexBufferResource->GetBufferAndOffset(*this, m_uIndexOffset, uStartIndexLocation, m_uIndexStride, tmpBuffer, offset);

        [m_CurrentEncoder drawIndexedPrimitives: m_MetalPrimitiveType
                                     indexCount: uIndexCount
                                      indexType: m_MetalIndexType
                                    indexBuffer: tmpBuffer
                              indexBufferOffset: offset];

        // assert that all transient mapped data was bound for this draw call
        CRY_ASSERT(m_spIndexBufferResource->m_pTransientMappedData.empty());
    }

    void CContext::Draw(uint32 uVertexCount, uint32 uBaseVertexLocation)
    {
        DXGL_SCOPED_PROFILE("CContext::Draw")
        SetVertexOffset(0); // No need to use uBaseVertexLocation for vertex offset as it is used to indicate which vertex is the starting vertex when calling drawPrimitives below
        
        FlushDrawState();

        [m_CurrentEncoder drawPrimitives: m_MetalPrimitiveType
                             vertexStart: uBaseVertexLocation
                             vertexCount: uVertexCount];
    }

    void CContext::DrawIndexedInstanced(uint32 uIndexCountPerInstance, uint32 uInstanceCount, uint32 uStartIndexLocation, uint32 uBaseVertexLocation, uint32 uStartInstanceLocation)
    {
        DXGL_SCOPED_PROFILE("CContext::DrawIndexedInstanced")
        SetVertexOffset(uBaseVertexLocation);

        FlushDrawState();

        CRY_ASSERT(m_spIndexBufferResource.get());
        if (!m_spIndexBufferResource.get())
        {
            return;
        }

        uint32 offset = 0;
        id<MTLBuffer> tmpBuffer = nil;
        m_spIndexBufferResource->GetBufferAndOffset(*this, m_uIndexOffset, uStartIndexLocation, m_uIndexStride, tmpBuffer, offset);


        id<MTLDevice> mtlDevice = m_pDevice->GetMetalDevice();
        
        bool isBaseVertexInstanceEnabled = true;
#if defined(AZ_PLATFORM_IOS)
        isBaseVertexInstanceEnabled = NCryMetal::s_isIOSGPUFamily3;
#endif
        if(isBaseVertexInstanceEnabled)
        {
            // `-[MTLDebugRenderCommandEncoder drawPrimitives:vertexStart:vertexCount:instanceCount:baseInstance:] is only supported on MTLFeatureSet_iOS_GPUFamily3_v1 and later'... (This is most likely related to the new A9 GPUS.)
            // https://developer.apple.com/library/ios/documentation/Metal/Reference/MTLDevice_Ref/index.html#//apple_ref/c/econst/MTLFeatureSet_iOS_GPUFamily2_v1
            [m_CurrentEncoder drawIndexedPrimitives: m_MetalPrimitiveType
                                         indexCount: uIndexCountPerInstance
                                          indexType: m_MetalIndexType
                                        indexBuffer: tmpBuffer
                                  indexBufferOffset: offset
                                      instanceCount: uInstanceCount
                                        baseVertex : 0
                                       baseInstance: uStartInstanceLocation];
        }
        else if (uStartInstanceLocation == 0)
        {
            [m_CurrentEncoder drawIndexedPrimitives: m_MetalPrimitiveType
                                         indexCount: uIndexCountPerInstance
                                          indexType: m_MetalIndexType
                                        indexBuffer: tmpBuffer
                                  indexBufferOffset: offset
                                      instanceCount: uInstanceCount];
        }
        else
        {
            // Not supported!
            CRY_ASSERT(0);
        }

        // assert that all transient mapped data was bound for this draw call
        CRY_ASSERT(m_spIndexBufferResource->m_pTransientMappedData.empty());
    }

    void CContext::DrawInstanced(uint32 uVertexCountPerInstance, uint32 uInstanceCount, uint32 uStartVertexLocation, uint32 uStartInstanceLocation)
    {
        DXGL_SCOPED_PROFILE("CContext::DrawInstanced")
        SetVertexOffset(0); // No need to use uBaseVertexLocation for vertex offset as it is used to indicate which vertex is the starting vertex when calling drawPrimitives below
        FlushDrawState();


        id<MTLDevice> mtlDevice = m_pDevice->GetMetalDevice();
        
        bool isBaseInstanceEnabled = true;
#if defined(AZ_PLATFORM_IOS)
        isBaseInstanceEnabled = NCryMetal::s_isIOSGPUFamily3;
#endif
        if (isBaseInstanceEnabled)
        {
            // `-[MTLDebugRenderCommandEncoder drawPrimitives:vertexStart:vertexCount:instanceCount:baseInstance:] is only supported on MTLFeatureSet_iOS_GPUFamily3_v1 and later'... (This is most likely related to the new A9 GPUS.)
            // https://developer.apple.com/library/ios/documentation/Metal/Reference/MTLDevice_Ref/index.html#//apple_ref/c/econst/MTLFeatureSet_iOS_GPUFamily2_v1
            [m_CurrentEncoder drawPrimitives: m_MetalPrimitiveType
                                 vertexStart: uStartVertexLocation
                                 vertexCount: uVertexCountPerInstance
                               instanceCount: uInstanceCount
                                baseInstance: uStartInstanceLocation];
        }
        else if (uStartInstanceLocation == 0)
        {
            [m_CurrentEncoder drawPrimitives: m_MetalPrimitiveType
                                 vertexStart: uStartVertexLocation
                                 vertexCount: uVertexCountPerInstance
                               instanceCount: uInstanceCount];
        }
        else
        {
            // Not supported!
            CRY_ASSERT(0);
        }
    }

    void CContext::Flush(id<CAMetalDrawable> drawable, float syncInterval)
    {
        DXGL_SCOPED_PROFILE("CContext::Flush")

        FlushCurrentEncoder();

        if (drawable)
        {
            // call the view's completion handler which is required by the view since it will signal its semaphore and set up the next buffer
            __block dispatch_semaphore_t block_sema = m_FrameQueueSemaphore;
            [m_CurrentCommandBuffer addCompletedHandler: ^ (id < MTLCommandBuffer > buffer) {
                 dispatch_semaphore_signal(block_sema);
             }];
        }

        GetCurrentEventHelper()->bCommandBufferPreSubmitted = true;
        
        if(drawable)
        {
            // presentAfterMinimumDuration only available on ios and appltv 10.3+
#if defined(AZ_PLATFORM_IOS) && defined(__IPHONE_10_3)
            bool hasPresentAfterMinimumDuration = [drawable respondsToSelector:@selector(presentAfterMinimumDuration:)];

            if (hasPresentAfterMinimumDuration && syncInterval > 0.0f)
            {
                [m_CurrentCommandBuffer presentDrawable:drawable afterMinimumDuration:syncInterval];
            }
            else
#endif
            {
                [m_CurrentCommandBuffer presentDrawable:drawable];
            }
        }
        [m_CurrentCommandBuffer commit];
        GetCurrentEventHelper()->bCommandBufferSubmitted = true;

        MTLCommandBufferStatus stat = m_CurrentCommandBuffer.status;
        if (stat == MTLCommandBufferStatusError)
        {
            LogCommandBufferError(m_CurrentCommandBuffer.error.code);
        }

        [m_CurrentCommandBuffer release];
        m_CurrentCommandBuffer = nil;

        if (!drawable)
        {
            NextCommandBuffer();
        }
    }

    void CContext::FlushBlitEncoderAndWait()
    {
        DXGL_SCOPED_PROFILE("CContext::Flush")

        if (m_CurrentBlitEncoder)
        {
            m_GPUEventsHelper.FlushActions(m_CurrentBlitEncoder, CGPUEventsHelper::eFT_FlushEncoder);

            [m_CurrentBlitEncoder endEncoding];
            [m_CurrentBlitEncoder release];
            m_CurrentBlitEncoder = nil;

            GetCurrentEventHelper()->bCommandBufferPreSubmitted = true;
            [m_CurrentCommandBuffer commit];
            GetCurrentEventHelper()->bCommandBufferSubmitted = true;

            [m_CurrentCommandBuffer waitUntilCompleted];
            MTLCommandBufferStatus stat = m_CurrentCommandBuffer.status;

            if (stat == MTLCommandBufferStatusError)
            {
                LogCommandBufferError(m_CurrentCommandBuffer.error.code);
            }

            [m_CurrentCommandBuffer release];
            m_CurrentCommandBuffer = nil;
            NextCommandBuffer();
        }
    }

    SPipelinePtr CContext::AllocatePipeline(const SPipelineConfiguration& kConfiguration)
    {
        DXGL_SCOPED_PROFILE("CContext::AllocatePipeline")

        // First see if there is an equivalent pipeline in the cache
        SPipelineCache::TMap::iterator kFound(m_pPipelineCache->m_kMap.find(kConfiguration));
        if (kFound != m_pPipelineCache->m_kMap.end())
        {
            return kFound->second;
        }

        // Create a new one and cache it
        SPipelinePtr spPipeline(new SPipeline(kConfiguration, this));

        if (!InitializePipeline(spPipeline))
        {
            return NULL;
        }

        m_pPipelineCache->m_kMap.insert(SPipelineCache::TMap::value_type(kConfiguration, spPipeline));
        for (uint32 uShader = 0; uShader < eST_NUM; ++uShader)
        {
            SShader* pShader(spPipeline->m_kConfiguration.m_apShaders[uShader]);
            if (pShader != NULL)
            {
                pShader->AttachPipeline(spPipeline);
            }
        }

        return spPipeline;
    }

    void CContext::RemovePipeline(SPipeline* pPipeline, SShader* pInvalidShader)
    {
        DXGL_SCOPED_PROFILE("CContext::RemovePipeline")

        SPipelineCache::TMap::iterator kFound(m_pPipelineCache->m_kMap.find(pPipeline->m_kConfiguration));
        if (kFound == m_pPipelineCache->m_kMap.end())
        {
            DXGL_ERROR("Pipeline to remove was not found in the cache map");
            return;
        }

        // Remove all references of the pipeline from the attached shaders, except pInvalidShader which is being destroyed
        uint32 uShader;
        for (uShader = 0; uShader < eST_NUM; ++uShader)
        {
            SShader* pAttachedShader(pPipeline->m_kConfiguration.m_apShaders[uShader]);
            if (pAttachedShader != pInvalidShader && pAttachedShader != NULL)
            {
                pAttachedShader->DetachPipeline(pPipeline);
            }
        }

        m_pPipelineCache->m_kMap.erase(kFound);
    }

    bool CContext::InitializePipeline(SPipeline* pPipeline)
    {
        DXGL_SCOPED_PROFILE("CContext::InitializePipeline")

        //  Confetti BEGIN: Igor Lobanchikov

        if (!CompilePipeline(pPipeline, m_pDevice))
        {
            return false;
        }

        //  Confetti End: Igor Lobanchikov

        return true;
    }
    
    void* CContext::AllocateMemoryInRingBuffer(uint32 size, MemRingBufferStorage memAllocMode, size_t& ringBufferOffsetOut)
    {
#if defined(AZ_PLATFORM_MAC)
        if (memAllocMode == MEM_SHARED_RINGBUFFER)
        {
            return m_RingBufferShared.Allocate(m_iCurrentFrameSlot, size, ringBufferOffsetOut);
        }
        else
        {
            return m_RingBufferManaged.Allocate(m_iCurrentFrameSlot, size, ringBufferOffsetOut);
        }
#else
        return m_RingBufferShared.Allocate(m_iCurrentFrameSlot, size, ringBufferOffsetOut);
#endif
    }

    id <MTLBuffer> CContext::GetRingBuffer(MemRingBufferStorage memAllocMode)
    {
#if defined(AZ_PLATFORM_MAC)
        if (memAllocMode == MEM_SHARED_RINGBUFFER)
        {
            return m_RingBufferShared.m_Buffer;
        }
        else
        {
            return m_RingBufferManaged.m_Buffer;
        }
#else
        return m_RingBufferShared.m_Buffer;
#endif
    }
    
    void* CContext::AllocateQueryInRingBuffer()
    {
        size_t ringBufferOffsetOut = 0;
        return m_QueryRingBuffer.Allocate(m_iCurrentFrameEventSlot, QUERY_SIZE, ringBufferOffsetOut);
    }

#undef CACHE_VAR
#undef CACHE_FIELD
}

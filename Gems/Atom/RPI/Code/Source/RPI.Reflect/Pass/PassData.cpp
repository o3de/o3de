/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RPI.Reflect/Pass/ComputePassData.h>
#include <Atom/RPI.Reflect/Pass/CopyPassData.h>
#include <Atom/RPI.Reflect/Pass/DownsampleMipChainPassData.h>
#include <Atom/RPI.Reflect/Pass/EnvironmentCubeMapPassData.h>
#include <Atom/RPI.Reflect/Pass/FullscreenTrianglePassData.h>
#include <Atom/RPI.Reflect/Pass/PassData.h>
#include <Atom/RPI.Reflect/Pass/RasterPassData.h>
#include <Atom/RPI.Reflect/Pass/RenderPassData.h>
#include <Atom/RPI.Reflect/Pass/RenderToTexturePassData.h>
#include <Atom/RPI.Reflect/Pass/SlowClearPassData.h>

namespace AZ
{
    namespace RPI
    {
        void SlowClearPassData::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<SlowClearPassData, RenderPassData>()
                    ->Version(0)
                    ->Field("ClearValue", &SlowClearPassData::m_clearValue)
                    ;
            }
        }

        void RenderToTexturePassData::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<RenderToTexturePassData, PassData>()
                    ->Version(0)
                    ->Field("OutputWidth", &RenderToTexturePassData::m_width)
                    ->Field("OutputHeight", &RenderToTexturePassData::m_height)
                    ->Field("OutputFormat", &RenderToTexturePassData::m_format);
            }
        }
                
        void RenderPassData::Reflect(ReflectContext* context)
        {            
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<RenderPassData, PassData>()
                    ->Version(1)
                    ->Field("DeviceIndex", &RenderPassData::m_deviceIndex)
                    ->Field("BindViewSrg", &RenderPassData::m_bindViewSrg)
                    ->Field("ShaderDataMappings", &RenderPassData::m_mappings);
            }
        }
                
        void RasterPassData::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<RasterPassData, RenderPassData>()
                    ->Version(5) // antonmic: added m_enableDrawItemsByDefault
                    ->Field("DrawListTag", &RasterPassData::m_drawListTag)
                    ->Field("PassSrgShaderAsset", &RasterPassData::m_passSrgShaderReference)
                    ->Field("Viewport", &RasterPassData::m_overrideViewport)
                    ->Field("Scissor", &RasterPassData::m_overrideScissor)
                    ->Field("DrawListSortType", &RasterPassData::m_drawListSortType)
                    ->Field("ViewportScissorTargetOutputIndex", &RasterPassData::m_viewportAndScissorTargetOutputIndex)
                    ->Field("EnableDrawItemsByDefault", &RasterPassData::m_enableDrawItemsByDefault)
                    ;
            }
        }

        void PipelineGlobalConnection::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<PipelineGlobalConnection>()
                    ->Version(1)
                    ->Field("GlobalName", &PipelineGlobalConnection::m_globalName)
                    ->Field("Slot", &PipelineGlobalConnection::m_localBinding);
            }
        }

        void PassData::Reflect(ReflectContext* context)
        {
            PipelineGlobalConnection::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<PassData>()
                    ->Version(1)
                    ->Field("PipelineViewTag", &PassData::m_pipelineViewTag)
                    ->Field("PipelineGlobalConnections", &PassData::m_pipelineGlobalConnections);
            }
        }

        void FullscreenTrianglePassData::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<FullscreenTrianglePassData, RenderPassData>()
                    ->Version(0)
                    ->Field("ShaderAsset", &FullscreenTrianglePassData::m_shaderAsset)
                    ->Field("StencilRef", &FullscreenTrianglePassData::m_stencilRef);
            }
        }

        void EnvironmentCubeMapPassData::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<EnvironmentCubeMapPassData, PassData>()
                    ->Version(0)
                    ->Field("Position", &EnvironmentCubeMapPassData::m_position)
                    ;
            }
        }
        
        void DownsampleMipChainPassData::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<DownsampleMipChainPassData, PassData>()
                    ->Version(0)
                    ->Field("ShaderAsset", &DownsampleMipChainPassData::m_shaderReference)
                    ;
            }
        }

        void CopyPassData::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<CopyPassData, PassData>()
                    ->Version(1)
                    ->Field("BufferSize", &CopyPassData::m_bufferSize)
                    ->Field("BufferSourceOffset", &CopyPassData::m_bufferSourceOffset)
                    ->Field("BufferSourceBytesPerRow", &CopyPassData::m_bufferSourceBytesPerRow)
                    ->Field("BufferSourceBytesPerImage", &CopyPassData::m_bufferSourceBytesPerImage)
                    ->Field("BufferDestinationOffset", &CopyPassData::m_bufferDestinationOffset)
                    ->Field("BufferDestinationBytesPerRow", &CopyPassData::m_bufferDestinationBytesPerRow)
                    ->Field("BufferDestinationBytesPerImage", &CopyPassData::m_bufferDestinationBytesPerImage)
                    ->Field("ImageSourceSize", &CopyPassData::m_sourceSize)
                    ->Field("ImageSourceSubresource", &CopyPassData::m_imageSourceSubresource)
                    ->Field("ImageSourceOrigin", &CopyPassData::m_imageSourceOrigin)
                    ->Field("ImageDestinationSubresource", &CopyPassData::m_imageDestinationSubresource)
                    ->Field("ImageDestinationOrigin", &CopyPassData::m_imageDestinationOrigin)
                    ->Field("CloneInput", &CopyPassData::m_cloneInput)
                    ->Field("UseCopyQueue", &CopyPassData::m_useCopyQueue)
                    ;
            }
        }

        void ComputePassData::Reflect(ReflectContext* context)        
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ComputePassData, RenderPassData>()
                    ->Version(2)
                    ->Field("ShaderAsset", &ComputePassData::m_shaderReference)
                    ->Field("Target Thread Count X", &ComputePassData::m_totalNumberOfThreadsX)
                    ->Field("Target Thread Count Y", &ComputePassData::m_totalNumberOfThreadsY)
                    ->Field("Target Thread Count Z", &ComputePassData::m_totalNumberOfThreadsZ)
                    ->Field("Make Fullscreen Pass", &ComputePassData::m_makeFullscreenPass)
                    ->Field("Use Async Compute", &ComputePassData::m_useAsyncCompute)
                    ;
            }
        }

    } // namespace RPI
} // namespace AZ

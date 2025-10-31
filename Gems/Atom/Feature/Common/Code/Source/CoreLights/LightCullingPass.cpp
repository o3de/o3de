/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreLights/LightCullingPass.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/DevicePipelineState.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/Device.h>

#include <Atom/Feature/Decals/DecalFeatureProcessorInterface.h>
#include <Atom/RHI/ImagePool.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <AzCore/Math/MatrixUtils.h>
#include <AzCore/Math/Plane.h>
#include <AzCore/std/algorithm.h>
#include <CoreLights/CapsuleLightFeatureProcessor.h>
#include <CoreLights/DiskLightFeatureProcessor.h>
#include <CoreLights/LightCullingConstants.h>
#include <CoreLights/PointLightFeatureProcessor.h>
#include <CoreLights/QuadLightFeatureProcessor.h>
#include <CoreLights/SimplePointLightFeatureProcessor.h>
#include <CoreLights/SimpleSpotLightFeatureProcessor.h>
#include <cmath>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<LightCullingPass> LightCullingPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<LightCullingPass> pass = aznew LightCullingPass(descriptor);
            return pass;
        }

        LightCullingPass::LightCullingPass(const RPI::PassDescriptor& descriptor)
            : RPI::ComputePass(descriptor)
        {
            m_lightdata[eLightTypes_SimplePoint].m_lightCountIndex  = Name("m_simplePointLightCount");
            m_lightdata[eLightTypes_SimplePoint].m_lightBufferIndex = Name("m_simplePointLights");
            m_lightdata[eLightTypes_SimpleSpot].m_lightCountIndex   = Name("m_simpleSpotLightCount");
            m_lightdata[eLightTypes_SimpleSpot].m_lightBufferIndex  = Name("m_simpleSpotLights");
            m_lightdata[eLightTypes_Point].m_lightCountIndex        = Name("m_pointLightCount");
            m_lightdata[eLightTypes_Point].m_lightBufferIndex       = Name("m_pointLights");
            m_lightdata[eLightTypes_Disk].m_lightCountIndex         = Name("m_diskLightCount");
            m_lightdata[eLightTypes_Disk].m_lightBufferIndex        = Name("m_diskLights");
            m_lightdata[eLightTypes_Capsule].m_lightCountIndex      = Name("m_capsuleLightCount");
            m_lightdata[eLightTypes_Capsule].m_lightBufferIndex     = Name("m_capsuleLights");
            m_lightdata[eLightTypes_Quad].m_lightCountIndex         = Name("m_quadLightCount");
            m_lightdata[eLightTypes_Quad].m_lightBufferIndex        = Name("m_quadLights");
            m_lightdata[eLightTypes_Decal].m_lightCountIndex        = Name("m_decalCount");
            m_lightdata[eLightTypes_Decal].m_lightBufferIndex       = Name("m_decals");
        }

        void LightCullingPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            AZ_Assert(m_shaderResourceGroup != nullptr, "LightCullingPass %s has a null shader resource group when calling FrameBeginInternal.", GetPathName().GetCStr());

            GetLightDataFromFeatureProcessor();
            SetLightBuffersToSRG();
            SetLightsCountToSRG();
            SetConstantdataToSRG();

            BindPassSrg(context, m_shaderResourceGroup);
            if (RPI::ViewPtr view = GetView())
            {
                BindSrg(view->GetRHIShaderResourceGroup());
            }

            m_shaderResourceGroup->Compile();
        }

        void LightCullingPass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            RHI::CommandList* commandList = context.GetCommandList();

            SetSrgsForDispatch(context);

            RHI::Size res = GetDepthBufferResolution();

            auto arguments{m_dispatchItem.GetArguments()};
            arguments.m_direct.m_totalNumberOfThreadsX = res.m_width;
            arguments.m_direct.m_totalNumberOfThreadsY = res.m_height;
            arguments.m_direct.m_totalNumberOfThreadsZ = 1;
            m_dispatchItem.SetArguments(arguments);

            commandList->Submit(m_dispatchItem.GetDeviceDispatchItem(context.GetDeviceIndex()));
        }

        void LightCullingPass::ResetInternal()
        {
            m_tileDataIndex = std::numeric_limits<uint32_t>::max();
            m_constantDataIndex.Reset();

            for (auto& elem : m_lightdata)
            {
                elem.m_lightBufferIndex.Reset();
                elem.m_lightBuffer = nullptr;
                elem.m_lightCountIndex.Reset();
                elem.m_lightCount = 0;
            }
            m_lightList = nullptr;
        }

        void LightCullingPass::SetLightBuffersToSRG()
        {
            for (auto& elem : m_lightdata)
            {
                m_shaderResourceGroup->SetBuffer(elem.m_lightBufferIndex, elem.m_lightBuffer.get());
                elem.m_lightBufferIndex.AssertValid();
            }
        }

        void LightCullingPass::SetLightsCountToSRG()
        {
            for (auto& elem : m_lightdata)
            {
                m_shaderResourceGroup->SetConstant(elem.m_lightCountIndex, elem.m_lightCount);
                elem.m_lightCountIndex.AssertValid();
            }
        }

        AZ::RHI::Size LightCullingPass::GetDepthBufferResolution()
        {
            const RPI::PassAttachment* tileBuffer = GetInputBinding(m_tileDataIndex).GetAttachment().get();
            // TileData is a texture that is built from taking the depth buffer and dividing it into tiles
            // We will use this attachment to work our way backwards and grab the original depth buffer so we can read the resolution off of it
            // The sizeSource contains the original depth buffer
            const RPI::PassAttachmentBinding* sizeSource = tileBuffer->m_sizeSource;
            const RHI::UnifiedAttachmentDescriptor& depthBufferDescriptor = sizeSource->GetAttachment()->m_descriptor;
            return depthBufferDescriptor.m_image.m_size;
        }

        void LightCullingPass::SetConstantdataToSRG()
        {
            struct LightCullingConstants
            {
                AZStd::array<float, 2> m_gridPixel;
                AZStd::array<float, 2> m_gridHalfPixel;
                uint32_t m_gridWidth;
                uint32_t m_padding[3];
            } cullingConstants{};

            cullingConstants.m_gridPixel = ComputeGridPixelSize();
            cullingConstants.m_gridHalfPixel[0] = cullingConstants.m_gridPixel[0] * 0.5f;
            cullingConstants.m_gridHalfPixel[1] = cullingConstants.m_gridPixel[1] * 0.5f;
            cullingConstants.m_gridWidth = GetTileDataBufferResolution().m_width;

            m_shaderResourceGroup->SetConstant(m_constantDataIndex, cullingConstants);
        }

        uint32_t LightCullingPass::FindInputBinding(const AZ::Name& name)
        {
            for (uint32_t i = 0; i < GetInputCount(); ++i)
            {
                const auto& binding = GetInputBinding(i);
                if (binding.m_name == name)
                {
                    return i;
                }
            }
            return std::numeric_limits<uint32_t>::max();
        }

        AZ::RHI::Size LightCullingPass::GetTileDataBufferResolution()
        {
            auto binding = GetInputBinding(m_tileDataIndex).GetAttachment().get();
            return binding->m_descriptor.m_image.m_size;
        }

        // Used to convert a compute shader threadGroup.xy coordinate into a 0 to 1 screen uv coordinate
        // e.g. if the screen resolution is 1920 x 1080, that means we have 120x68 tiles of size 16x16 pixels. One thread is dispatched to each.
        // each thread group will want to know which normalized (0 to 1) uv coordinate it belongs to, which can be done by multiplying the group ID by these numbers
        AZStd::array<float, 2> LightCullingPass::ComputeGridPixelSize()
        {
            RHI::Size resolution = GetDepthBufferResolution();
            AZStd::array<float, 2> gridPixelSize;
            gridPixelSize[0] = (float(LightCulling::TileDimX) / float(resolution.m_width));
            gridPixelSize[1] = (float(LightCulling::TileDimY) / float(resolution.m_height));
            return gridPixelSize;
        }

        void LightCullingPass::BuildInternal()
        {
            m_tileDataIndex = FindInputBinding(AZ::Name("TileLightData"));
            CreateLightList();
            AttachLightList();
        }

        void LightCullingPass::GetLightDataFromFeatureProcessor()
        {
            const auto simplePointLightFP = m_pipeline->GetScene()->GetFeatureProcessor<SimplePointLightFeatureProcessor>();
            m_lightdata[eLightTypes_SimplePoint].m_lightBuffer = simplePointLightFP->GetLightBuffer();
            m_lightdata[eLightTypes_SimplePoint].m_lightCount = simplePointLightFP->GetLightCount();

            const auto simpleSpotLightFP = m_pipeline->GetScene()->GetFeatureProcessor<SimpleSpotLightFeatureProcessor>();
            m_lightdata[eLightTypes_SimpleSpot].m_lightBuffer = simpleSpotLightFP->GetLightBuffer();
            m_lightdata[eLightTypes_SimpleSpot].m_lightCount = simpleSpotLightFP->GetLightCount();

            const auto pointLightFP = m_pipeline->GetScene()->GetFeatureProcessor<PointLightFeatureProcessor>();
            m_lightdata[eLightTypes_Point].m_lightBuffer = pointLightFP->GetLightBuffer();
            m_lightdata[eLightTypes_Point].m_lightCount = pointLightFP->GetLightCount();

            const auto diskLightFP = m_pipeline->GetScene()->GetFeatureProcessor<DiskLightFeatureProcessor>();
            m_lightdata[eLightTypes_Disk].m_lightBuffer = diskLightFP->GetLightBuffer();
            m_lightdata[eLightTypes_Disk].m_lightCount = diskLightFP->GetLightCount();

            const auto capsuleLightFP = m_pipeline->GetScene()->GetFeatureProcessor<CapsuleLightFeatureProcessor>();
            m_lightdata[eLightTypes_Capsule].m_lightBuffer = capsuleLightFP->GetLightBuffer();
            m_lightdata[eLightTypes_Capsule].m_lightCount = capsuleLightFP->GetLightCount();

            const auto quadLightFP = m_pipeline->GetScene()->GetFeatureProcessor<QuadLightFeatureProcessor>();
            m_lightdata[eLightTypes_Quad].m_lightBuffer = quadLightFP->GetLightBuffer();
            m_lightdata[eLightTypes_Quad].m_lightCount = quadLightFP->GetLightCount();

            const auto decalFP = m_pipeline->GetScene()->GetFeatureProcessor<DecalFeatureProcessorInterface>();
            m_lightdata[eLightTypes_Decal].m_lightBuffer = decalFP->GetDecalBuffer();
            m_lightdata[eLightTypes_Decal].m_lightCount = decalFP->GetDecalCount();
        }

        float LightCullingPass::CreateTraceValues(const AZ::Vector2& unprojection)
        {
            RHI::Size numTiles = GetTileDataBufferResolution();
            float fHalfTileDimMulPixelUnprojectX = 1.0f / (numTiles.m_width * unprojection.GetX());
            float fHalfTileDimMulPixelUnprojectY = 1.0f / (numTiles.m_height * unprojection.GetY());

            return AZStd::max(fHalfTileDimMulPixelUnprojectX, fHalfTileDimMulPixelUnprojectY);
        }

        void LightCullingPass::CreateLightList()
        {
            auto tileBufferResolution = GetTileDataBufferResolution();

            RPI::CommonBufferDescriptor desc;
            desc.m_poolType = RPI::CommonBufferPoolType::ReadWrite;
            desc.m_bufferName = "LightList";
            desc.m_elementSize = sizeof(uint32_t);
            desc.m_byteCount = tileBufferResolution.m_width * tileBufferResolution.m_height * 256 * sizeof(uint32_t);
            m_lightList = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
            AZ_Assert(m_lightList != nullptr, "Unable to allocate buffer for culling light list");
            if (m_lightList != nullptr)
            {
                m_lightList->SetAsStructured<uint32_t>();
            }
        }

        void LightCullingPass::AttachLightList()
        {
            if (m_lightList != nullptr)
            {
                AttachBufferToSlot(Name("LightList"), m_lightList);
            }
        }
    }   // namespace Render
}   // namespace AZ

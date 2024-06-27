/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreLights/LightCullingRemap.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/DevicePipelineState.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/Device.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/Scene.h>
#include <CoreLights/PointLightFeatureProcessor.h>
#include <CoreLights/DiskLightFeatureProcessor.h>
#include <AzCore/Casting/lossy_cast.h>

namespace AZ
{
    namespace Render
    {
        const size_t NumBins = 8;
        const size_t MaxLightsPerTile = 256;
        // TODO convert this to R16_UINT. It just needs RHI support
        // ATOM-3975
        const RHI::Format LightListRemappedFormat = RHI::Format::R32_UINT;

        RPI::Ptr<LightCullingRemap> LightCullingRemap::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<LightCullingRemap> pass = aznew LightCullingRemap(descriptor);
            return pass;
        }

        LightCullingRemap::LightCullingRemap(const RPI::PassDescriptor& descriptor)
            : RPI::ComputePass(descriptor)
        {
        }

        void LightCullingRemap::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            ComputePass::SetupFrameGraphDependencies(frameGraph);
        }

        void LightCullingRemap::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            if (!m_initialized)
            {
                Init();
            }

            AZ_Assert(m_shaderResourceGroup != nullptr, "LightCullingRemap %s has a null shader resource group when calling FrameBeginInternal.", GetPathName().GetCStr());

            if (!m_shaderResourceGroup)
            {
                return;
            }

            m_shaderResourceGroup->SetConstant(m_tileWidthIndex, m_tileDim.m_width);

            BindPassSrg(context, m_shaderResourceGroup);

            m_shaderResourceGroup->Compile();
        }

        void LightCullingRemap::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            RHI::CommandList* commandList = context.GetCommandList();

            SetSrgsForDispatch(context);

            // Each tile gets 32 threads to process indices
            auto arguments{m_dispatchItem.GetArguments()};
            arguments.m_direct.m_totalNumberOfThreadsX = m_tileDim.m_width * arguments.m_direct.m_threadsPerGroupX;
            arguments.m_direct.m_totalNumberOfThreadsY = m_tileDim.m_height;
            arguments.m_direct.m_totalNumberOfThreadsZ = 1;
            m_dispatchItem.SetArguments(arguments);

            commandList->Submit(m_dispatchItem.GetDeviceDispatchItem(context.GetDeviceIndex()));
        }

        void LightCullingRemap::ResetInternal()
        {
            m_lightListRemapped = nullptr;
            m_initialized = false;
            m_tileWidthIndex.Reset();
        }

        uint32_t LightCullingRemap::FindInputOutputBinding(const AZ::Name& name)
        {
            for (uint32_t i = 0; i < GetInputOutputCount(); ++i)
            {
                const auto& binding = GetInputOutputBinding(i);
                if (binding.m_name == name)
                {
                    return i;
                }
            }
            return std::numeric_limits<uint32_t>::max();
        }

        void LightCullingRemap::BuildInternal()
        {
            m_tileDataIndex = FindInputOutputBinding(AZ::Name("TileLightData"));
            m_tileDim = GetTileDataBufferResolution();
            CreateRemappedLightListBuffer();
            AttachBufferToSlot("LightListRemapped", m_lightListRemapped);
        }

        void LightCullingRemap::CreateRemappedLightListBuffer()
        {
            RPI::CommonBufferDescriptor desc;
            desc.m_poolType = RPI::CommonBufferPoolType::ReadWrite;
            desc.m_bufferName = "LightListRemapped";
            desc.m_elementSize = RHI::GetFormatSize(LightListRemappedFormat);
            desc.m_byteCount = m_tileDim.m_width * m_tileDim.m_height * NumBins * MaxLightsPerTile * desc.m_elementSize;
            m_lightListRemapped = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
        }

        AZ::RHI::Size LightCullingRemap::GetTileDataBufferResolution()
        {
            auto binding = GetInputOutputBinding(m_tileDataIndex).GetAttachment().get();
            return binding->m_descriptor.m_image.m_size;
        }

        void LightCullingRemap::Init()
        {
            m_tileWidthIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(AZ::Name("m_tileWidth"));
            AZ_Assert(m_tileWidthIndex.IsValid(), "m_tileWidth not found in shader");
            m_initialized = true;
        }
    }   // namespace Render
}   // namespace AZ

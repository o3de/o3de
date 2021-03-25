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

#include <PostProcessing/LuminanceHistogramGeneratorPass.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/PipelineState.h>

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

#include <PostProcess/PostProcessFeatureProcessor.h>
#include <PostProcess/ExposureControl/ExposureControlSettings.h>

namespace AZ
{
    namespace Render
    {
        // This must match the value in LuminanceHistogramCommon.azsli
        static const size_t NumHistogramBins = 128;

        RPI::Ptr<LuminanceHistogramGeneratorPass> LuminanceHistogramGeneratorPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<LuminanceHistogramGeneratorPass> pass = aznew LuminanceHistogramGeneratorPass(descriptor);
            return AZStd::move(pass);
        }

        LuminanceHistogramGeneratorPass::LuminanceHistogramGeneratorPass(const RPI::PassDescriptor& descriptor)
            : RPI::ComputePass(descriptor)
        {
        }

        void LuminanceHistogramGeneratorPass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            RHI::CommandList* commandList = context.GetCommandList();

            SetSrgsForDispatch(commandList);

            const RHI::Size resolution = GetColorBufferResolution();
            SetTargetThreadCounts(resolution.m_width, resolution.m_height, 1);

            commandList->Submit(m_dispatchItem);
        }

        void LuminanceHistogramGeneratorPass::CreateHistogramBuffer()
        {
            // generate a UUID for the buffer name to keep it unique when there are multiple render pipelines
            AZ::Uuid uuid = AZ::Uuid::CreateRandom();
            AZStd::string uuidString;
            uuid.ToString(uuidString);

            RPI::CommonBufferDescriptor desc;
            desc.m_poolType = RPI::CommonBufferPoolType::ReadWrite;
            desc.m_bufferName = AZStd::string::format("LuminanceHistogramBuffer_%s", uuidString.c_str());
            desc.m_elementSize = sizeof(uint32_t);
            desc.m_byteCount = NumHistogramBins * sizeof(uint32_t);
            desc.m_elementFormat = RHI::Format::R32_UINT;
            m_histogram = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
            AZ_Assert(m_histogram != nullptr, "Unable to allocate buffer");
        }


        AZ::RHI::Size LuminanceHistogramGeneratorPass::GetColorBufferResolution()
        {
            const auto& binding = GetInputBinding(0);
            AZ_Assert(binding.m_name == AZ::Name("ColorInput"), "ColorInput was expected to be the first input");
            const RPI::PassAttachment* colorBuffer = binding.m_attachment.get();
            return colorBuffer->m_descriptor.m_image.m_size;
        }

        void LuminanceHistogramGeneratorPass::BuildAttachmentsInternal()
        {
            CreateHistogramBuffer();
            AttachHistogramBuffer();
        }

        void LuminanceHistogramGeneratorPass::AttachHistogramBuffer()
        {
            if (m_histogram != nullptr)
            {
                AttachBufferToSlot(Name("Output"), m_histogram);
            }
        }
    }   // namespace Render
}   // namespace AZ

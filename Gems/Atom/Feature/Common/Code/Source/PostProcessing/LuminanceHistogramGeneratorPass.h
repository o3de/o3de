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
#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/DrawItem.h>
#include <Atom/RHI/ScopeProducer.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>

#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ
{
    namespace Render
    {
        //! This class generates a histogram of luminance values for the input color buffer.
        class LuminanceHistogramGeneratorPass final
            : public RPI::ComputePass
        {
            AZ_RPI_PASS(LuminanceHistogramGeneratorPass);

        public:
            AZ_RTTI(LuminanceHistogramGeneratorPass, "{062D0696-B159-491C-9ECC-AA02767ED4CF}", RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(LuminanceHistogramGeneratorPass, SystemAllocator, 0);
            ~LuminanceHistogramGeneratorPass() = default;

            static RPI::Ptr<LuminanceHistogramGeneratorPass> Create(const RPI::PassDescriptor& descriptor);
            void BuildCommandList(const RHI::FrameGraphExecuteContext& context, const RPI::PassScopeProducer& producer) override;

        protected:
            LuminanceHistogramGeneratorPass(const RPI::PassDescriptor& descriptor);
            virtual void BuildAttachmentsInternal() override;
            void CreateHistogramBuffer();
            void AttachHistogramBuffer();
            AZ::RHI::Size GetColorBufferResolution();

            Data::Instance<RPI::Buffer> m_histogram;
        };
    }   // namespace Render
}   // namespace AZ

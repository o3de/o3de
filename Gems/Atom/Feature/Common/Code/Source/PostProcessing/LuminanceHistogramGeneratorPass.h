/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/DeviceDrawItem.h>
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
            AZ_CLASS_ALLOCATOR(LuminanceHistogramGeneratorPass, SystemAllocator);
            ~LuminanceHistogramGeneratorPass() = default;

            static RPI::Ptr<LuminanceHistogramGeneratorPass> Create(const RPI::PassDescriptor& descriptor);
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

        protected:
            LuminanceHistogramGeneratorPass(const RPI::PassDescriptor& descriptor);
            virtual void BuildInternal() override;
            void CreateHistogramBuffer();
            void AttachHistogramBuffer();
            AZ::RHI::Size GetColorBufferResolution();

            Data::Instance<RPI::Buffer> m_histogram;
        };
    }   // namespace Render
}   // namespace AZ

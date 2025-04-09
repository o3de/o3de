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
#include <Atom/RHI.Reflect/ShaderInputNameIndex.h>

#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ
{
    namespace Render
    {
        static const char* const EyeAdaptationPassTemplateName = "EyeAdaptationTemplate";
        static const char* const EyeAdaptationDataInputOutputSlotName = "EyeAdaptationDataInputOutput";

        //! The eye adaptation pass.
        //! This pass apply auto exposure control by like eye adaptation for input framebuffer color.
        class EyeAdaptationPass final
            : public RPI::ComputePass
        {
            AZ_RPI_PASS(EyeAdaptationPass);

        public:
            AZ_RTTI(EyeAdaptationPass, "{CC66CFD9-3266-4FD7-A5A8-ACA3753BDF4A}", RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(EyeAdaptationPass, SystemAllocator);
            ~EyeAdaptationPass() = default;

            // Creates a EyeAdaptationPass
            static RPI::Ptr<EyeAdaptationPass> Create(const RPI::PassDescriptor& descriptor);

            // Check if we should enable of disable this pass
            bool IsEnabled() const override;

        protected:
            EyeAdaptationPass(const RPI::PassDescriptor& descriptor);
            void InitBuffer();

            // A StructuredBuffer for exposure calculation on the GPU.
            struct ExposureCalculationData
            {
                float   m_exposureValue = 1.0f;
                float   m_setValueTime = 0;
            };

            void BuildInternal() override;

            void FrameBeginInternal(FramePrepareParams params) override;

            AZ::Data::Instance<RPI::Buffer> m_buffer;

            // SRG binding indices...
            AZ::RHI::ShaderInputNameIndex m_exposureControlBufferInputIndex = "m_exposureControl";
        };
    }   // namespace Render
}   // namespace AZ

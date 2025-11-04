/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/ComputePass.h>

namespace AZ
{
    namespace Render
    {

        // Compute shader that upsamples an input image from half res to full res using depth buffers
        class DepthUpsamplePass final
            : public RPI::ComputePass
        {
            AZ_RPI_PASS(DepthUpsamplePass);
        
        public:
            AZ_RTTI(AZ::Render::DepthUpsamplePass, "{CE22C02E-7F6C-4C70-845C-839A8B51479E}", AZ::RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(DepthUpsamplePass, SystemAllocator);
            virtual ~DepthUpsamplePass() = default;
        
            //! Create function registered with the pass system (see PassSystem::AddPassCreator)
            static RPI::Ptr<DepthUpsamplePass> Create(const RPI::PassDescriptor& descriptor);
        
        protected:
            // Behavior functions override...
            void FrameBeginInternal(FramePrepareParams params) override;
        
        private:
            DepthUpsamplePass(const RPI::PassDescriptor& descriptor);
        
            // SRG binding indices...
            AZ::RHI::ShaderInputNameIndex m_constantsIndex = "m_constants";
        };

    }   // namespace Render
}   // namespace AZ

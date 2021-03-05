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
            AZ_CLASS_ALLOCATOR(DepthUpsamplePass, SystemAllocator, 0);
            virtual ~DepthUpsamplePass() = default;
        
            //! Create function registered with the pass system (see PassSystem::AddPassCreator)
            static RPI::Ptr<DepthUpsamplePass> Create(const RPI::PassDescriptor& descriptor);
        
        protected:
            // Behavior functions override...
            void FrameBeginInternal(FramePrepareParams params) override;
        
        private:
            DepthUpsamplePass(const RPI::PassDescriptor& descriptor);
        
            // SRG binding indices...
            AZ::RHI::ShaderInputConstantIndex m_constantsIndex;
        };

    }   // namespace Render
}   // namespace AZ

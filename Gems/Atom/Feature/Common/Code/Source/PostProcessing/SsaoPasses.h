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
#include <Atom/RPI.Public/Pass/ParentPass.h>

namespace AZ
{
    namespace Render
    {
        // Parent pass for SSAO that contains the SSAO compute pass 
        class SsaoParentPass final
            : public RPI::ParentPass
        {
            AZ_RPI_PASS(SsaoParentPass);

        public:
            AZ_RTTI(AZ::Render::SsaoParentPass, "{A03B5913-B201-4146-AF0A-999E6BF31A1A}", AZ::RPI::ParentPass);
            AZ_CLASS_ALLOCATOR(SsaoParentPass, SystemAllocator, 0);
            virtual ~SsaoParentPass() = default;

            //! Creates an SsaoParentPass
            static RPI::Ptr<SsaoParentPass> Create(const RPI::PassDescriptor& descriptor);

            bool IsEnabled() const override;

        protected:
            // Behavior functions override...
            void OnBuildAttachmentsFinishedInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            SsaoParentPass(const RPI::PassDescriptor& descriptor);

            // Direct pointers to child passes for enable/disable
            ParentPass* m_blurParentPass = nullptr;
            FastDepthAwareBlurHorPass* m_blurHorizontalPass = nullptr;
            FastDepthAwareBlurVerPass* m_blurVerticalPass = nullptr;
            Pass* m_downsamplePass = nullptr;
            Pass* m_upsamplePass = nullptr;
        };

        // Computer shader pass that calculates SSAO from a linear depth buffer
        class SsaoComputePass final
            : public RPI::ComputePass
        {
            AZ_RPI_PASS(SsaoComputePass);

        public:
            AZ_RTTI(AZ::Render::SsaoComputePass, "{0BA5F6F7-15D2-490A-8254-7E61F25B62F9}", AZ::RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(SsaoComputePass, SystemAllocator, 0);
            virtual ~SsaoComputePass() = default;

            //! Creates an SsaoComputePass
            static RPI::Ptr<SsaoComputePass> Create(const RPI::PassDescriptor& descriptor);

        protected:
            // Behavior functions override...
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            SsaoComputePass(const RPI::PassDescriptor& descriptor);

            // SRG binding indices...
            AZ::RHI::ShaderInputConstantIndex m_constantsIndex;
        };
    }   // namespace Render
}   // namespace AZ

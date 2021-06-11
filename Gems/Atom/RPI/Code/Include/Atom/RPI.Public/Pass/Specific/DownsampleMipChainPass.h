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

#include <Atom/RPI.Reflect/Pass/DownsampleMipChainPassData.h>

#include <Atom/RPI.Public/Pass/RenderPass.h>
#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Shader/Shader.h>

namespace AZ
{
    namespace RPI
    {
        using SortedPipelineViewTags = AZStd::set<PipelineViewTag, AZNameSortAscending>;

        //! This pass takes a mip mapped texture as input where the most detailed mip is already written to.
        //! It then recursively downsamples that mip to lower mip levels using the provided Compute Shader.
        //! It does this by recursively creating Compute Passes to write to each mip using the Compute Shader.
        class DownsampleMipChainPass
            : public ParentPass
        {
            AZ_RPI_PASS(DownsampleMipChainPass);

        public:
            AZ_RTTI(DownsampleMipChainPass, "{593B0B69-89E4-4DA5-82D2-745FB2E5FFDC}", ParentPass);
            AZ_CLASS_ALLOCATOR(DownsampleMipChainPass, SystemAllocator, 0);
          
            //! Creates a new pass without a PassTemplate
            static Ptr<DownsampleMipChainPass> Create(const PassDescriptor& descriptor);
            
        protected:
            explicit DownsampleMipChainPass(const PassDescriptor& descriptor);

            // Pass Behaviour Overrides...

            void ResetInternal() override;
            void BuildInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

        private:

            // Gets target height, width and mip levels from the input/output image attachment
            void GetInputInfo();

            // Build child passes to downsample each mip level N and output to mip level N+1
            void BuildChildPasses();

            // Updates various settings on the child passes, such as compute thread count to match the image size
            void UpdateChildren();

            // Data driven values for DownsampleMipChainPass
            DownsampleMipChainPassData m_passData;

            // Height and width of the input mip chain texture
            uint32_t m_inputWidth = 0;
            uint32_t m_inputHeight = 0;

            // Number of mip levels in the input mip chain texture
            uint16_t m_mipLevels = 0;

            // Whether we need to rebuild the passes because the number of mips has changes
            bool m_needToRebuildChildren = true;

            // Whether we need to update the children because the input image size has changed
            bool m_needToUpdateChildren = true;
        };
    }   // namespace RPI
}   // namespace AZ

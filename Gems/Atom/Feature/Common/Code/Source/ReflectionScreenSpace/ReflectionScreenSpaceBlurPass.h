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

#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>

namespace AZ
{
    namespace Render
    {
        //! This pass performs a separable Gaussian blur of the input reflection image
        //! to the lower mip levels of that image.  The blurred mips are used as roughness levels
        //! when applying reflection data to a surface material.
        class ReflectionScreenSpaceBlurPass
            : public RPI::ParentPass
        {
            AZ_RPI_PASS(ReflectionScreenSpaceBlurPass);

        public:
            AZ_RTTI(Render::ReflectionScreenSpaceBlurPass, "{BC3D92C5-E38A-46FE-8EBD-CAD14E505946}", ParentPass);
            AZ_CLASS_ALLOCATOR(Render::ReflectionScreenSpaceBlurPass, SystemAllocator, 0);
          
            //! Creates a new pass without a PassTemplate
            static RPI::Ptr<ReflectionScreenSpaceBlurPass> Create(const RPI::PassDescriptor& descriptor);

            //! Returns the frame buffer image attachment used by the ReflectionFrameBufferCopy pass
            //! to store the previous frame image
            Data::Instance<RPI::AttachmentImage>& GetFrameBufferImageAttachment() { return m_frameBufferImageAttachment; }

        private:
            explicit ReflectionScreenSpaceBlurPass(const RPI::PassDescriptor& descriptor);

            void CreateChildPassesInternal() override;

            // Pass Overrides...
            void ResetInternal() override;
            void BuildInternal() override;

            AZStd::vector<RPI::Ptr<RPI::FullscreenTrianglePass>> m_verticalBlurChildPasses;
            AZStd::vector<RPI::Ptr<RPI::FullscreenTrianglePass>> m_horizontalBlurChildPasses;

            Data::Instance<RPI::AttachmentImage> m_frameBufferImageAttachment;
            uint32_t m_numBlurMips = 0;
        };
    }   // namespace RPI
}   // namespace AZ

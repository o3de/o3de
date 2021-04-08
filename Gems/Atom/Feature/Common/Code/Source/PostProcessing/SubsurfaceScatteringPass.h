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
    namespace RPI
    {
        class ShaderResourceGroup;

        //! A SubsurfaceScatteringPass is a leaf pass (pass with no children) inherited from ComputePass used for subsurface scattering,
        //! due to the requirement of ViewSrg which not available in the original ComputePass template
        class SubsurfaceScatteringPass
            : public ComputePass
        {
            AZ_RPI_PASS(SubsurfaceScatteringPass);

        public:
            AZ_RTTI(SubsurfaceScatteringPass, "{15036827-D18C-4752-B58F-6F17D59D6D9E}", RenderPass);
            AZ_CLASS_ALLOCATOR(SubsurfaceScatteringPass, SystemAllocator, 0);
            virtual ~SubsurfaceScatteringPass() = default;

            //! Creates a SubsurfaceScatteringPass
            static Ptr<SubsurfaceScatteringPass> Create(const PassDescriptor& descriptor);

        protected:
            SubsurfaceScatteringPass(const PassDescriptor& descriptor);

            void FrameBeginInternal(FramePrepareParams params) override;

            // output texture vertical dimension required by compute shader
            AZ::RHI::ShaderInputNameIndex m_screenSizeInputIndex = "m_screenSize";

        };
    }   // namespace RPI
}   // namespace AZ

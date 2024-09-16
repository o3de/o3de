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
            AZ_CLASS_ALLOCATOR(SubsurfaceScatteringPass, SystemAllocator);
            virtual ~SubsurfaceScatteringPass() = default;

            //! Creates a SubsurfaceScatteringPass
            static Ptr<SubsurfaceScatteringPass> Create(const PassDescriptor& descriptor);

        protected:
            SubsurfaceScatteringPass(const PassDescriptor& descriptor);

            void CompileResources(const RHI::FrameGraphCompileContext& context) override;

            // output texture vertical dimension required by compute shader
            AZ::RHI::ShaderInputNameIndex m_screenSizeInputIndex = "m_screenSize";

        };
    }   // namespace RPI
}   // namespace AZ

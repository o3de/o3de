/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Shader/Shader.h>

namespace AZ
{
    namespace Render
    {
        //! This pass traces screenspace reflections from the previous frame image.
        class ReflectionScreenSpaceTracePass
            : public RPI::FullscreenTrianglePass
        {
            AZ_RPI_PASS(DiffuseProbeGridDownsamplePass);

        public:
            AZ_RTTI(Render::ReflectionScreenSpaceTracePass, "{70FD45E9-8363-4AA1-A514-3C24AC975E53}", FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(Render::ReflectionScreenSpaceTracePass, SystemAllocator, 0);

            //! Creates a new pass without a PassTemplate
            static RPI::Ptr<ReflectionScreenSpaceTracePass> Create(const RPI::PassDescriptor& descriptor);

            Data::Instance<RPI::AttachmentImage>& GetPreviousFrameImageAttachment() { return m_previousFrameImageAttachment; }

        private:
            explicit ReflectionScreenSpaceTracePass(const RPI::PassDescriptor& descriptor);

            // Pass behavior overrides...
            virtual void BuildInternal() override;

            Data::Instance<RPI::AttachmentImage> m_previousFrameImageAttachment;
        };
    }   // namespace RPI
}   // namespace AZ

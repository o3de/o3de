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
        //! This pass copies the frame buffer prior to the post-processing pass.
        class ReflectionCopyFrameBufferPass
            : public RPI::FullscreenTrianglePass
        {
            AZ_RPI_PASS(ReflectionScreenSpaceCopyFrameBufferPass);

        public:
            AZ_RTTI(Render::ReflectionCopyFrameBufferPass, "{8B0D4281-0913-4662-81ED-37CB890B5653}", FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(Render::ReflectionCopyFrameBufferPass, SystemAllocator);

            //! Creates a new pass without a PassTemplate
            static RPI::Ptr<ReflectionCopyFrameBufferPass> Create(const RPI::PassDescriptor& descriptor);

        private:
            explicit ReflectionCopyFrameBufferPass(const RPI::PassDescriptor& descriptor);

            // Pass Overrides...
            void BuildInternal() override;
        };
    }   // namespace RPI
}   // namespace AZ

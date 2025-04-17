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
        //! This pass composites the screenspace reflection trace onto the reflection buffer.
        class ReflectionScreenSpaceCompositePass
            : public RPI::FullscreenTrianglePass
        {
            AZ_RPI_PASS(ReflectionScreenSpaceCompositePass);

        public:
            AZ_RTTI(Render::ReflectionScreenSpaceCompositePass, "{88739CC9-C3F1-413A-A527-9916C697D93A}", FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(Render::ReflectionScreenSpaceCompositePass, SystemAllocator);

            //! Creates a new pass without a PassTemplate
            static RPI::Ptr<ReflectionScreenSpaceCompositePass> Create(const RPI::PassDescriptor& descriptor);

        private:
            explicit ReflectionScreenSpaceCompositePass(const RPI::PassDescriptor& descriptor);

            // Pass Overrides...
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;

            RHI::ShaderInputNameIndex m_outputScaleNameIndex = "m_outputScale";
            RHI::ShaderInputNameIndex m_outputWidthNameIndex = "m_outputWidth";
            RHI::ShaderInputNameIndex m_outputHeightNameIndex = "m_outputHeight";
            RHI::ShaderInputNameIndex m_maxRoughnessNameIndex = "m_maxRoughness";
        };
    }   // namespace RPI
}   // namespace AZ

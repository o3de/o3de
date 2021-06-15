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
            AZ_CLASS_ALLOCATOR(Render::ReflectionScreenSpaceCompositePass, SystemAllocator, 0);

            //! Creates a new pass without a PassTemplate
            static RPI::Ptr<ReflectionScreenSpaceCompositePass> Create(const RPI::PassDescriptor& descriptor);

        private:
            explicit ReflectionScreenSpaceCompositePass(const RPI::PassDescriptor& descriptor);

            // Pass Overrides...
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
        };
    }   // namespace RPI
}   // namespace AZ

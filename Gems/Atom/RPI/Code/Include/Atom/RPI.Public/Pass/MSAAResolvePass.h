/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/ScopeProducer.h>

#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Public/Pass/RenderPass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ
{
    namespace RPI
    {
        class ATOM_RPI_PUBLIC_API MSAAResolvePass
            : public RenderPass
        {
            AZ_RPI_PASS(MSAAResolvePass);

        public:
            AZ_RTTI(MSAAResolvePass, "{0234AE92-3EE3-4796-9138-C9B28F9F7AEE}", RenderPass);
            AZ_CLASS_ALLOCATOR(MSAAResolvePass, SystemAllocator);
            virtual ~MSAAResolvePass() = default;

            static Ptr<MSAAResolvePass> Create(const PassDescriptor& descriptor);

        protected:
            MSAAResolvePass(const PassDescriptor& descriptor);

            // Pass behavior overrides...
            void BuildInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;
            bool IsEnabled() const override;

        private:

            // Scope producer functions...
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;
        };
    }   // namespace RPI
}   // namespace AZ

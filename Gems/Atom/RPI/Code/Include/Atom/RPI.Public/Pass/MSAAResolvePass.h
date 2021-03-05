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

#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/ScopeProducer.h>

#include <Atom/RPI.Public/Pass/RenderPass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ
{
    namespace RPI
    {
        class MSAAResolvePass
            : public RenderPass
        {
            AZ_RPI_PASS(MSAAResolvePass);

        public:
            AZ_RTTI(MSAAResolvePass, "{0234AE92-3EE3-4796-9138-C9B28F9F7AEE}", RenderPass);
            AZ_CLASS_ALLOCATOR(MSAAResolvePass, SystemAllocator, 0);
            virtual ~MSAAResolvePass() = default;

            static Ptr<MSAAResolvePass> Create(const PassDescriptor& descriptor);

        protected:
            MSAAResolvePass(const PassDescriptor& descriptor);

            // Pass behavior overrides...
            void BuildAttachmentsInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;


        private:

            virtual void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph, const PassScopeProducer& producer);
            virtual void CompileResources(const RHI::FrameGraphCompileContext& context, const PassScopeProducer& producer);
            virtual void BuildCommandList(const RHI::FrameGraphExecuteContext& context, const PassScopeProducer& producer);
        };
    }   // namespace RPI
}   // namespace AZ

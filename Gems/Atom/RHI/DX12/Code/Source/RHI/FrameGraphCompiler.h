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

#include <Atom/RHI/FrameGraphCompiler.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/optional.h>

namespace AZ
{
    namespace RHI
    {
        class BufferFrameAttachment;
        class ImageFrameAttachment;
        class ScopeAttachment;
    }

    namespace DX12
    {
        class Scope;
        class CommandQueueContext;

        class FrameGraphCompiler final
            : public RHI::FrameGraphCompiler
        {
            using Base = RHI::FrameGraphCompiler;
        public:
            AZ_CLASS_ALLOCATOR(FrameGraphCompiler, AZ::SystemAllocator, 0);

            static RHI::Ptr<FrameGraphCompiler> Create();

        private:
            FrameGraphCompiler() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::FrameGraphCompiler
            RHI::ResultCode InitInternal(RHI::Device& device) override;
            void ShutdownInternal() override;
            RHI::MessageOutcome CompileInternal(const RHI::FrameGraphCompileRequest& request) override;
            //////////////////////////////////////////////////////////////////////////

            static D3D12_RESOURCE_STATES GetResourceState(const RHI::ScopeAttachment& scopeAttachment);

            //Returns pre-discard transition state.
            static AZStd::optional<D3D12_RESOURCE_STATES> GetDiscardResourceState(const RHI::ScopeAttachment& scopeAttachment, D3D12_RESOURCE_FLAGS bindflags);
            
            void CompileResourceBarriers(Scope* rootScope, const RHI::FrameGraphAttachmentDatabase& attachmentDatabase);
            void CompileBufferBarriers(Scope* rootScope, RHI::BufferFrameAttachment& frameGraphAttachment);
            void CompileImageBarriers(RHI::ImageFrameAttachment& frameGraphAttachment);

            void CompileAsyncQueueFences(const RHI::FrameGraph& frameGraph);
        };
    }
}

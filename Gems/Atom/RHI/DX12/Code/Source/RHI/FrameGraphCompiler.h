/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/FrameGraphCompiler.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/optional.h>
#include <RHI/DX12.h>

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
            AZ_CLASS_ALLOCATOR(FrameGraphCompiler, AZ::SystemAllocator);

            static RHI::Ptr<FrameGraphCompiler> Create();

        private:
            FrameGraphCompiler() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::FrameGraphCompiler
            RHI::ResultCode InitInternal() override;
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

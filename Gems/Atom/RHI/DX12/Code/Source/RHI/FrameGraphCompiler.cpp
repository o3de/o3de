/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/FrameGraphCompiler.h>
#include <Atom/RHI/BufferFrameAttachment.h>
#include <Atom/RHI/BufferScopeAttachment.h>
#include <RHI/Buffer.h>
#include <RHI/CommandQueueContext.h>
#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <RHI/Image.h>
#include <RHI/Scope.h>
#include <Atom/RHI/FrameGraph.h>
#include <Atom/RHI/ImageScopeAttachment.h>
#include <Atom/RHI/ScopeAttachment.h>
#include <Atom/RHI/SwapChainFrameAttachment.h>
#include <AzCore/Debug/EventTrace.h>

// #define AZ_DX12_FRAMESCHEDULER_LOG_TRANSITIONS

namespace AZ
{
    namespace DX12
    {
        AZStd::string GetResourceStateDebugString(D3D12_RESOURCE_STATES state)
        {
            AZStd::string result;

            if (!state)
            {
                result = "COMMON|";
            }
            else if (state == (D3D12_RESOURCE_STATES)-1)
            {
                result = "NONE|";
            }
            else
            {
                if ((state & D3D12_RESOURCE_STATE_GENERIC_READ) == D3D12_RESOURCE_STATE_GENERIC_READ)
                {
                    result += "GENERIC_READ|";
                    state &= ~D3D12_RESOURCE_STATE_GENERIC_READ;
                }
                if (state & D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER)
                {
                    result += "VERTEX_AND_CONSTANT_BUFFER|";
                }
                if (state & D3D12_RESOURCE_STATE_INDEX_BUFFER)
                {
                    result += "INDEX_BUFFER|";
                }
                if (state & D3D12_RESOURCE_STATE_RENDER_TARGET)
                {
                    result += "RENDER_TARGET|";
                }
                if (state & D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
                {
                    result += "UNORDERED_ACCESS|";
                }
                if (state & D3D12_RESOURCE_STATE_DEPTH_WRITE)
                {
                    result += "DEPTH_WRITE|";
                }
                if (state & D3D12_RESOURCE_STATE_DEPTH_READ)
                {
                    result += "DEPTH_READ|";
                }
                if (state & D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
                {
                    result += "NON_PIXEL_SHADER_RESOURCE|";
                }
                if (state & D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
                {
                    result += "PIXEL_SHADER_RESOURCE|";
                }
                if (state & D3D12_RESOURCE_STATE_STREAM_OUT)
                {
                    result += "STREAM_OUT|";
                }
                if (state & D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT)
                {
                    result += "INDIRECT_ARGUMENT|";
                }
                if (state & D3D12_RESOURCE_STATE_COPY_DEST)
                {
                    result += "COPY_DEST|";
                }
                if (state & D3D12_RESOURCE_STATE_COPY_SOURCE)
                {
                    result += "COPY_SOURCE|";
                }
                if (state & D3D12_RESOURCE_STATE_RESOLVE_DEST)
                {
                    result += "RESOLVE_DEST|";
                }
                if (state & D3D12_RESOURCE_STATE_RESOLVE_SOURCE)
                {
                    result += "RESOLVE_SOURCE|";
                }
                if (state & (D3D12_RESOURCE_STATES)DX12_RESOURCE_STATE_COPY_QUEUE_BIT)
                {
                    result += "COPY_QUEUE_BIT|";
                }
                if (state & D3D12_RESOURCE_STATE_PREDICATION)
                {
                    result += "PREDICATION|";
                }
                if (state & D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT)
                {
                    result += "INDIRECT_ARGUMENT|";
                }
            }

            if (result.size())
            {
                result.pop_back();
            }
            return result;
        }

        class ResourceTransitionLoggerNull
        {
        public:
            ResourceTransitionLoggerNull(const RHI::AttachmentId&) {}
            void SetStateBefore(D3D12_RESOURCE_STATES) {}
            void SetStateAfter(D3D12_RESOURCE_STATES) {}
            void LogEpilogueTransition(const RHI::Scope&) {}
            void LogPrologueTransition(const RHI::Scope&) {}
            void LogPreDiscardTransition(const RHI::Scope&) {}
        };

        class ResourceTransitionLogger
        {
        public:
            ResourceTransitionLogger(const RHI::AttachmentId& attachmentId)
                : m_attachmentId{attachmentId}
            {
                AZ_Printf("FrameScheduler", "==================================================\n");
                AZ_Printf("FrameScheduler", "ATTACHMENT %s\n", m_attachmentId.GetCStr());
            }

            void SetStateBefore(D3D12_RESOURCE_STATES stateBefore)
            {
                m_stateBeforeString = GetResourceStateDebugString(stateBefore);
            }

            void SetStateAfter(D3D12_RESOURCE_STATES stateAfter)
            {
                m_stateAfterString = GetResourceStateDebugString(stateAfter);
            }

            void LogPrologueTransition(const RHI::Scope& scopeAfter)
            {
                AZ_Printf("FrameScheduler", "\t[%s] Prologue Transition: %s (%s -> %s)\n",
                    scopeAfter.GetId().GetCStr(),
                    m_attachmentId.GetCStr(),
                    m_stateBeforeString.data(),
                    m_stateAfterString.data());
            }

            void LogEpilogueTransition(const RHI::Scope& scopeBefore)
            {
                AZ_Printf("FrameScheduler", "\t[%s] Epilogue Transition: %s (%s -> %s)\n",
                    scopeBefore.GetId().GetCStr(),
                    m_attachmentId.GetCStr(),
                    m_stateBeforeString.data(),
                    m_stateAfterString.data());
            }

            void LogPreDiscardTransition(const RHI::Scope& scopeAfter)
            {
                AZ_Printf("FrameScheduler", "\t[%s] Pre-Discard Transition: %s (%s -> %s)\n",
                    scopeAfter.GetId().GetCStr(),
                    m_attachmentId.GetCStr(),
                    m_stateBeforeString.data(),
                    m_stateAfterString.data());
            }

        private:
            RHI::AttachmentId m_attachmentId;
            AZStd::string m_stateBeforeString;
            AZStd::string m_stateAfterString;
        };

        RHI::Ptr<FrameGraphCompiler> FrameGraphCompiler::Create()
        {
            return aznew FrameGraphCompiler();
        }

        RHI::ResultCode FrameGraphCompiler::InitInternal(RHI::Device&)
        {
            return RHI::ResultCode::Success;
        }

        void FrameGraphCompiler::ShutdownInternal()
        {
        }

        RHI::MessageOutcome FrameGraphCompiler::CompileInternal(const RHI::FrameGraphCompileRequest& request)
        {
            AZ_PROFILE_SCOPE(RHI, "FrameGraphCompiler: CompileInternal(DX12)");

            RHI::FrameGraph& frameGraph = *request.m_frameGraph;

            Scope* rootScope = static_cast<Scope*>(frameGraph.GetRootScope());

            CompileResourceBarriers(rootScope, frameGraph.GetAttachmentDatabase());

            if (RHI::CheckBitsAny(request.m_compileFlags, RHI::FrameSchedulerCompileFlags::DisableAsyncQueues) == false)
            {
                CompileAsyncQueueFences(frameGraph);
            }

            return AZ::Success();
        }

        D3D12_RESOURCE_STATES FrameGraphCompiler::GetResourceState(const RHI::ScopeAttachment& scopeAttachment)
        {
            static const D3D12_RESOURCE_STATES ReadWriteState[RHI::HardwareQueueClassCount] =
            {
                /// Graphics
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,

                /// Compute
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,

                /// Copy
                D3D12_RESOURCE_STATE_COMMON | (D3D12_RESOURCE_STATES)DX12_RESOURCE_STATE_COPY_QUEUE_BIT
            };

            static const D3D12_RESOURCE_STATES ReadState[RHI::HardwareQueueClassCount] =
            {
                /// Graphics
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,

                /// Compute
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,

                /// Copy
                D3D12_RESOURCE_STATE_COMMON | (D3D12_RESOURCE_STATES)DX12_RESOURCE_STATE_COPY_QUEUE_BIT
            };

            static const D3D12_RESOURCE_STATES CopyReadState[RHI::HardwareQueueClassCount] =
            {
                /// Graphics
                D3D12_RESOURCE_STATE_COPY_SOURCE,

                /// Compute
                D3D12_RESOURCE_STATE_COPY_SOURCE,

                /// Copy
                D3D12_RESOURCE_STATE_COPY_SOURCE | (D3D12_RESOURCE_STATES)DX12_RESOURCE_STATE_COPY_QUEUE_BIT
            };

            static const D3D12_RESOURCE_STATES CopyWriteState[RHI::HardwareQueueClassCount] =
            {
                /// Graphics
                D3D12_RESOURCE_STATE_COPY_DEST,

                /// Compute
                D3D12_RESOURCE_STATE_COPY_DEST,

                /// Copy
                D3D12_RESOURCE_STATE_COPY_DEST | (D3D12_RESOURCE_STATES)DX12_RESOURCE_STATE_COPY_QUEUE_BIT
            };

            const RHI::Scope& parentScope = scopeAttachment.GetScope();
            const RHI::HardwareQueueClass hardwareQueueClass = parentScope.GetHardwareQueueClass();
            const uint32_t hardwareQueueClassIdx = static_cast<uint32_t>(hardwareQueueClass);

            const AZStd::vector<RHI::ScopeAttachmentUsageAndAccess>& usagesAndAccesses = scopeAttachment.GetUsageAndAccess();
            D3D12_RESOURCE_STATES mergedResourceState = D3D12_RESOURCE_STATE_COMMON;
            for (const RHI::ScopeAttachmentUsageAndAccess& usageAndAccess : usagesAndAccesses)
            {
                switch (usageAndAccess.m_usage)
                {
                case RHI::ScopeAttachmentUsage::RenderTarget:
                {
                    mergedResourceState |= D3D12_RESOURCE_STATE_RENDER_TARGET;
                    break;
                }

                case RHI::ScopeAttachmentUsage::DepthStencil:
                {
                    mergedResourceState |= RHI::CheckBitsAny(usageAndAccess.m_access, RHI::ScopeAttachmentAccess::Write) ?
                                            D3D12_RESOURCE_STATE_DEPTH_WRITE :
                                            D3D12_RESOURCE_STATE_DEPTH_READ;
                    break;
                }

                case RHI::ScopeAttachmentUsage::Shader:
                {
                    mergedResourceState |= RHI::CheckBitsAny(usageAndAccess.m_access, RHI::ScopeAttachmentAccess::Write) ?
                                            ReadWriteState[hardwareQueueClassIdx] :
                                            ReadState[hardwareQueueClassIdx];
                    break;
                }
                case RHI::ScopeAttachmentUsage::Copy:
                {
                    mergedResourceState |= RHI::CheckBitsAny(usageAndAccess.m_access, RHI::ScopeAttachmentAccess::Write) ?
                                            CopyWriteState[hardwareQueueClassIdx] :
                                            CopyReadState[hardwareQueueClassIdx];
                    break;
                }
                case RHI::ScopeAttachmentUsage::Resolve:
                {
                    mergedResourceState |= D3D12_RESOURCE_STATE_RESOLVE_DEST;
                    break;
                }
                case RHI::ScopeAttachmentUsage::Predication:
                {
                    mergedResourceState |= D3D12_RESOURCE_STATE_PREDICATION;
                    break;
                }
                case RHI::ScopeAttachmentUsage::Indirect:
                {
                    mergedResourceState |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
                    break;
                }
                case RHI::ScopeAttachmentUsage::InputAssembly:
                {
                    mergedResourceState |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_INDEX_BUFFER;
                    break;
                }
                
                case RHI::ScopeAttachmentUsage::Uninitialized:
                default:
                    AZ_Assert(false, "ScopeAttachmentUsage is Uninitialized or not supported");
                    break;
                }
            }
            return mergedResourceState;
        }

        AZStd::optional<D3D12_RESOURCE_STATES> FrameGraphCompiler::GetDiscardResourceState(const RHI::ScopeAttachment& scopeAttachment, D3D12_RESOURCE_FLAGS bindflags)
        {
            //Discard transitions described here https://docs.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-discardresource
            const RHI::Scope& parentScope = scopeAttachment.GetScope();
            const RHI::HardwareQueueClass hardwareQueueClass = parentScope.GetHardwareQueueClass();

            switch(hardwareQueueClass)
            {
            case RHI::HardwareQueueClass::Graphics:
            {
                if (bindflags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
                {
                    return D3D12_RESOURCE_STATE_RENDER_TARGET;
                }
                else if (bindflags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
                {
                    return D3D12_RESOURCE_STATE_DEPTH_WRITE;
                }
                break;
            }
            case RHI::HardwareQueueClass::Compute:
            {
                if (bindflags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
                {
                    return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                }
                break;
            }
            }

            return {};
        }
    
        void FrameGraphCompiler::CompileResourceBarriers(Scope* rootScope, const RHI::FrameGraphAttachmentDatabase& attachmentDatabase)
        {
            AZ_PROFILE_SCOPE(RHI, "FrameGraphCompiler: CompileResourceBarriers(DX12)");

            for (RHI::BufferFrameAttachment* bufferFrameAttachment : attachmentDatabase.GetBufferAttachments())
            {
                CompileBufferBarriers(rootScope, *bufferFrameAttachment);
            }

            for (RHI::ImageFrameAttachment* imageFrameAttachment : attachmentDatabase.GetImageAttachments())
            {
                CompileImageBarriers(*imageFrameAttachment);
            }
        }

        void FrameGraphCompiler::CompileBufferBarriers(Scope* rootScope, RHI::BufferFrameAttachment& bufferFrameAttachment)
        {
#if defined(AZ_DX12_FRAMESCHEDULER_LOG_TRANSITIONS)
            ResourceTransitionLogger logger(bufferFrameAttachment.GetId());
#else
            ResourceTransitionLoggerNull logger(bufferFrameAttachment.GetId());
#endif

            AZ_PROFILE_SCOPE(RHI, "FrameGraphCompiler: CompileBufferBarriers(DX12)");

            Buffer& buffer = static_cast<Buffer&>(*bufferFrameAttachment.GetBuffer());
            RHI::BufferScopeAttachment* scopeAttachment = bufferFrameAttachment.GetFirstScopeAttachment();

            if (scopeAttachment == nullptr)
            {
                AZ_WarningOnce("RHI", false, "Imported BufferFrameAttachment isn't used in any Scope");
                return;
            }

            D3D12_RESOURCE_TRANSITION_BARRIER transition;
            transition.pResource = buffer.GetMemoryView().GetMemory();
            transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            transition.StateBefore = buffer.m_initialAttachmentState;
            logger.SetStateBefore(transition.StateBefore);

            Scope& firstScope = static_cast<Scope&>(scopeAttachment->GetScope());
            if (firstScope.IsResourceDiscarded(*scopeAttachment))
            {
                auto afterState = GetDiscardResourceState(*scopeAttachment, ConvertBufferBindFlags(buffer.GetDescriptor().m_bindFlags));
                if (afterState)
                {
                    transition.StateAfter = afterState.value();
                    if (firstScope.IsStateSupportedByQueue(transition.StateBefore) &&
                        firstScope.IsStateSupportedByQueue(transition.StateAfter))
                    {
                        logger.SetStateAfter(transition.StateAfter);
                        logger.LogPreDiscardTransition(firstScope);
                        firstScope.QueuePreDiscardTransition(transition);

                        transition.StateBefore = transition.StateAfter;
                        logger.SetStateBefore(transition.StateBefore);
                    }
                }
            }

            // Reset the initial state of the buffer back to common since we consumed it.
            buffer.m_initialAttachmentState = D3D12_RESOURCE_STATE_COMMON;

            Scope* scopeBefore = rootScope;
            while (scopeAttachment)
            {
                Scope& scopeAfter = static_cast<Scope&>(scopeAttachment->GetScope());
                transition.StateAfter = GetResourceState(*scopeAttachment);
                logger.SetStateAfter(transition.StateAfter);

                /**
                 * Due to implicit state promotion and decay (which applies to all buffers), we only need
                 * to transition when the before and after scopes are on the same queue. Even then, we can
                 * ignore transitions out of the common state.
                 */
                const bool onSameQueue = scopeBefore->GetHardwareQueueClass() == scopeAfter.GetHardwareQueueClass();
                const bool inCommonState = transition.StateBefore == D3D12_RESOURCE_STATE_COMMON;
                if (onSameQueue && !inCommonState)
                {
                    logger.LogPrologueTransition(scopeAfter);
                    scopeAfter.QueuePrologueTransition(transition);
                }

                scopeAttachment = scopeAttachment->GetNext();
                transition.StateBefore = transition.StateAfter;
                scopeBefore = &scopeAfter;
                logger.SetStateBefore(transition.StateBefore);
            }
        }
    
        void FrameGraphCompiler::CompileImageBarriers(RHI::ImageFrameAttachment& imageFrameAttachment)
        {
#if defined(AZ_DX12_FRAMESCHEDULER_LOG_TRANSITIONS)
            ResourceTransitionLogger logger(imageFrameAttachment.GetId());
#else
            ResourceTransitionLoggerNull logger(imageFrameAttachment.GetId());
#endif

            AZ_PROFILE_SCOPE(RHI, "FrameGraphCompiler: CompileImageBarriers (DX12)");

            Image& image = static_cast<Image&>(*imageFrameAttachment.GetImage());
            RHI::ImageScopeAttachment* scopeAttachment = imageFrameAttachment.GetFirstScopeAttachment();

            if (scopeAttachment == nullptr)
            {
                AZ_WarningOnce("RHI", false, "Imported ImageFrameAttachment isn't used in any Scope");
                return;
            }

            D3D12_RESOURCE_TRANSITION_BARRIER transition;
            memset(&transition, 0, sizeof(D3D12_RESOURCE_TRANSITION_BARRIER)); // C4701 potentially unitialized local variable 'transition' used
            transition.pResource = image.GetMemoryView().GetMemory();

            Scope& firstScope = static_cast<Scope&>(scopeAttachment->GetScope());
            //Apply appropriate pre-discard transition
            if (firstScope.IsResourceDiscarded(*scopeAttachment))
            {
                auto afterState = GetDiscardResourceState(*scopeAttachment, ConvertImageBindFlags(image.GetDescriptor().m_bindFlags));
                if (afterState)
                {
                    transition.StateAfter = afterState.value();
                    for (const auto& subresourceState : image.GetAttachmentStateByIndex())
                    {
                        transition.Subresource = subresourceState.m_subresourceIndex;
                        transition.StateBefore = subresourceState.m_state;
                        logger.SetStateBefore(transition.StateBefore);

                        logger.SetStateAfter(transition.StateAfter);
                        if (firstScope.IsStateSupportedByQueue(transition.StateBefore) &&
                            firstScope.IsStateSupportedByQueue(transition.StateAfter))
                        {
                            logger.LogPreDiscardTransition(firstScope);
                            firstScope.QueuePreDiscardTransition(transition);
                        }
                        else
                        {
                            //Find the previous scope on higher capable queue and do the transition at the end of that scope
                            Scope* previousScope = static_cast<Scope*>(firstScope.FindMoreCapableCrossQueueProducer());
                            AZ_Assert(previousScope, "Coudn't find a scope to transiiton into the correct state for Discard");
                            if (previousScope)
                            {
                                logger.LogEpilogueTransition(*previousScope);
                                previousScope->QueueEpilogueTransition(transition);
                            }
                        }
                        logger.SetStateBefore(transition.StateBefore);
                        transition.StateBefore = transition.StateAfter;
                    }
                    image.SetAttachmentState(transition.StateAfter);
                }
            }
            
            Scope* scopeBefore = nullptr;
            while (scopeAttachment)
            {
                Scope& scopeAfter = static_cast<Scope&>(scopeAttachment->GetScope());
                transition.StateAfter = GetResourceState(*scopeAttachment);
                logger.SetStateAfter(transition.StateAfter);

                RHI::ImageSubresourceRange viewRange = RHI::ImageSubresourceRange(scopeAttachment->GetImageView()->GetDescriptor());
                for (const auto& subresourceState : image.GetAttachmentStateByIndex(&viewRange))
                {
                    transition.StateBefore = subresourceState.m_state;
                    transition.Subresource = subresourceState.m_subresourceIndex;

                    /**
                     * We have all the information we need to transition the attachment. The transition
                     * is occurring between two scopes (excluding the first scope), and each scope could be
                     * on a different queue. The question is whether the transition needs to go at the end
                     * of a (could be any) previous scope, at the beginning of the current scope.
                     *
                     * To answer that question, we need to consider the hardware queues of each scope and the
                     * usage of the attachment. The constraint is that we are not allowed to transition to / from
                     * a state that is incompatible with the queue class. That means, for instance, if we're transitioning
                     * from depth-stencil read-write to non-pixel-shader-read, we must submit that barrier on the graphics
                     * queue.
                     */

                     // Check if we can service the request at the beginning of the current scope. This is the
                     // normal case when transitioning within the same queue family.
                    if (scopeAfter.IsStateSupportedByQueue(transition.StateBefore) &&
                        scopeAfter.IsStateSupportedByQueue(transition.StateAfter))
                    {
                        // We can just queue the transition on the scope directly.
                        logger.LogPrologueTransition(scopeAfter);
                        if (scopeAttachment->HasUsage(RHI::ScopeAttachmentUsage::Resolve))
                        {
                            scopeAfter.QueueResolveTransition(transition);
                        }
                        else
                        {
                            scopeAfter.QueuePrologueTransition(transition);
                        }
                    }

                    // Attempt to find a previous scope to service the request. This will occur when transitioning from a
                    // higher-capability queue family to a lower-capability one.
                    else
                    {
                        if (!scopeBefore)
                        {
                            /**
                             * This attachment is the first usage of the frame, and its before state is incompatible
                             * with the scope it's being used on. This happens if an imported attachment is used on
                             * a lesser-capability queue as the first scope of the frame, but its current state is enabled
                             * for a higher-capability queue.
                             *
                             * What we need to do is search for a scope guaranteed to execute earlier than us, and on a
                             * higher-capability queue. We can do this by searching for a producer on that queue.
                             */
                            scopeBefore = static_cast<Scope*>(scopeAfter.FindMoreCapableCrossQueueProducer());

                            if (AZ::RHI::Validation::IsEnabled())
                            {
                                if (scopeBefore)
                                {
                                    /**
                                     * Edge case for transient resources: if a scope changes its queue type from frame to frame, it may leave the
                                     * resource in a state that is incompatible with the earliest scope in the lifetime of the next frame. This would
                                     * happen if, for example, a resource ended on a graphics job in the PIXEL_SHADER_RESOURCE | NON_PIXEL_SHADER_RESOURCE state,
                                     * but in the next frame, it becomes a compute job. If the resource is now (in the current frame) only used by
                                     * compute jobs, we have no way of knowing that we need to extend the lifetime to accommodate the transition
                                     * change. This only happens when queue classes are changed at runtime, and the effects are unknown at this
                                     * time. It may rear its head as a potential corruption issue, so for now I am queuing a warning to correlate
                                     * this event with potential visual artifacts.
                                     */
                                    if (imageFrameAttachment.GetLifetimeType() == RHI::AttachmentLifetimeType::Transient)
                                    {
                                        AZ_Warning("FrameScheduler", scopeBefore->GetIndex() >= imageFrameAttachment.GetFirstScope()->GetIndex(),
                                            "Warning: Transition barrier search found '%s' as the best scope for '%s', but it is before the aliasing"
                                            "barrier which occurs at '%s'. This may cause corruption.",
                                            scopeBefore->GetId().GetCStr(),
                                            imageFrameAttachment.GetId().GetCStr(),
                                            imageFrameAttachment.GetFirstScope()->GetId().GetCStr());
                                    }
                                }
                                else
                                {
                                    AZ_Error(
                                        "FrameGraphCompiler", false,
                                        "Failed to transition attachment '%s' on scope '%s'. Scope hardware queue does not support resource state "
                                        "and no compatible producer was found to service the transition request. This can be avoided by always having "
                                        "the first scope in the frame be on the graphics queue\n",
                                        imageFrameAttachment.GetId().GetCStr(), scopeAfter.GetId().GetCStr());
                                }
                            }
                        }

                        // Check if we can service the request at the end of the previous scope.
                        if (scopeBefore)
                        {
                            // Moving out of copy queue.
                            if (scopeBefore->GetHardwareQueueClass() == RHI::HardwareQueueClass::Copy)
                            {
                                transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;

                                // Transition from COMMON -> AFTER on graphics / compute scope.
                                logger.SetStateBefore(transition.StateBefore);
                                logger.SetStateAfter(transition.StateAfter);
                                logger.LogPrologueTransition(scopeAfter);
                                scopeAfter.QueuePrologueTransition(transition);
                            }

                            // Moving into copy queue.
                            else if (scopeAfter.GetHardwareQueueClass() == RHI::HardwareQueueClass::Copy)
                            {
                                D3D12_RESOURCE_TRANSITION_BARRIER transitionCopy = transition;
                                transitionCopy.StateAfter = D3D12_RESOURCE_STATE_COMMON;

                                // Transition from BEFORE -> COMMON on before scope. Leave the after state intact
                                // since state promotion will take care of it.
                                logger.SetStateAfter(transitionCopy.StateAfter);
                                logger.LogEpilogueTransition(*scopeBefore);
                                scopeBefore->QueueEpilogueTransition(transitionCopy);
                            }

                            // Moving between compute / graphics queue.
                            else
                            {
                                logger.LogEpilogueTransition(*scopeBefore);
                                scopeBefore->QueueEpilogueTransition(transition);
                            }
                        }
                    }

                    // Add a transition to the resolve_source state if the image is
                    // being resolved
                    if (scopeAttachment->IsBeingResolved())
                    {
                        transition.StateBefore = transition.StateAfter;
                        transition.StateAfter = D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
                        scopeAfter.QueueResolveTransition(transition);
                    }

                }

                scopeAttachment = scopeAttachment->GetNext();
                image.SetAttachmentState(transition.StateAfter, &viewRange);
                scopeBefore = &scopeAfter;
                logger.SetStateBefore(transition.StateBefore);
            }

            /**
             * If this is the last usage of a swap chain, we require that it be in the common state for presentation.
             */
            const bool isSwapChain = (azrtti_cast<const RHI::SwapChainFrameAttachment*>(&imageFrameAttachment) != nullptr);
            if (isSwapChain)
            {
                for (const auto& subresourceState : image.GetAttachmentStateByIndex())
                {
                    transition.StateBefore = subresourceState.m_state;
                    transition.Subresource = subresourceState.m_subresourceIndex;
                    transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
                    logger.SetStateAfter(transition.StateAfter);
                    logger.LogEpilogueTransition(*scopeBefore);
                    scopeBefore->QueueEpilogueTransition(transition);
                    image.SetAttachmentState(transition.StateAfter);
                }
            }
        }

        void FrameGraphCompiler::CompileAsyncQueueFences(const RHI::FrameGraph& frameGraph)
        {
            Device& device = static_cast<Device&>(GetDevice());

            AZ_TRACE_METHOD();
            CommandQueueContext& context = device.GetCommandQueueContext();

            for (RHI::Scope* scopeBase : frameGraph.GetScopes())
            {
                Scope* scope = static_cast<Scope*>(scopeBase);

                bool hasCrossQueueConsumer = false;
                for (uint32_t hardwareQueueClassIdx = 0; hardwareQueueClassIdx < RHI::HardwareQueueClassCount; ++hardwareQueueClassIdx)
                {
                    const RHI::HardwareQueueClass hardwareQueueClass = static_cast<RHI::HardwareQueueClass>(hardwareQueueClassIdx);
                    if (scope->GetHardwareQueueClass() != hardwareQueueClass)
                    {
                        if (const Scope* producer = static_cast<const Scope*>(scope->GetProducerByQueue(hardwareQueueClass)))
                        {
                            scope->SetWaitFenceValueByQueue(hardwareQueueClass, producer->GetSignalFenceValue());
                        }

                        hasCrossQueueConsumer |= (scope->GetConsumerByQueue(hardwareQueueClass) != nullptr);
                    }
                }

                if (hasCrossQueueConsumer)
                {
                    scope->SetSignalFenceValue(context.IncrementFence(scope->GetHardwareQueueClass()));
                }
            }
        }
    }
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/FrameGraphCompiler.h>
#include <Atom/RHI/BufferFrameAttachment.h>
#include <Atom/RHI/BufferScopeAttachment.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameGraph.h>
#include <Atom/RHI/ImageFrameAttachment.h>
#include <Atom/RHI/ImageScopeAttachment.h>
#include <Atom/RHI/RHIUtils.h>
#include <Atom/RHI/Scope.h>
#include <Atom/RHI/SwapChainFrameAttachment.h>
#include <Atom/RHI/TransientAttachmentPool.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/optional.h>

namespace AZ::RHI
{
    ResultCode FrameGraphCompiler::Init()
    {
        const ResultCode resultCode = InitInternal();

        if (resultCode == ResultCode::Success)
        {
            // These are immutable for now. Could be configured per-frame using the compile request.
            const uint32_t BufferViewCapacity = 128;
            m_bufferViewCache.SetCapacity(BufferViewCapacity);

            const uint32_t ImageViewCapacity = 128;
            m_imageViewCache.SetCapacity(ImageViewCapacity);
        }

        return resultCode;
    }

    void FrameGraphCompiler::Shutdown()
    {
        m_imageViewCache.Clear();
        m_bufferViewCache.Clear();
        m_imageReverseLookupHash.clear();
        m_bufferReverseLookupHash.clear();

        ShutdownInternal();
    }

    MessageOutcome FrameGraphCompiler::ValidateCompileRequest(const FrameGraphCompileRequest& request) const
    {
        if (Validation::IsEnabled())
        {
            if (request.m_frameGraph == nullptr)
            {
                return AZ::Failure(AZStd::string("FrameGraph is null. Skipping compilation..."));
            }

            if (request.m_frameGraph->IsCompiled())
            {
                return AZ::Failure(AZStd::string("FrameGraph already compiled. Skipping compilation..."));
            }

            const FrameGraphAttachmentDatabase& attachmentDatabase = request.m_frameGraph->GetAttachmentDatabase();
            const bool hasTransientAttachments = attachmentDatabase.GetTransientBufferAttachments().size() || attachmentDatabase.GetTransientImageAttachments().size();
            if (request.m_transientAttachmentPool == nullptr && hasTransientAttachments)
            {
                return AZ::Failure(AZStd::string("DeviceTransientAttachmentPool is null, but transient attachments are in the graph. Skipping compilation..."));
            }
        }

        (void)request;
        return AZ::Success();
    }

    // The entry point for FrameGraph compilation. Frame Graph compilation is broken into several phases:
    //
    //      1) Queue-Centric Scope Graph Compilation:
    //
    //          This phase takes the scope graph and compiles a queue-centric scope graph. The former is a simple
    //          producer / consumer graph where certain scopes can produce resources for consumer scopes. The queue-centric
    //          graph is split into tracks according to each hardware queue. Scopes are serialized onto each track according
    //          to the topological sort, and cross-track dependencies are generated.
    //
    //      2) Transient Attachment Compilation:
    //
    //          This phase takes the transient attachment set and acquires physical resources from the Transient
    //          Attachment Pool. The resources are assigned to the attachments.
    //
    //      3) Resource View Compilation:
    //
    //          After acquiring all transient resources, the compiler creates and assigns resource views
    //          to each scope attachment. View ownership is managed by an internal cache.
    //
    //      4) Platform-specific Compilation:
    //
    //          The final phase is to compile the platform specific scopes and hand-off compilation to the platform-specific
    //          implementation, which may introduce more phases specific to the platform API.
    MessageOutcome FrameGraphCompiler::Compile(const FrameGraphCompileRequest& request)
    {
        AZ_PROFILE_SCOPE(RHI, "FrameGraphCompiler: Compile");

        MessageOutcome outcome = ValidateCompileRequest(request);
        if (!outcome)
        {
            return outcome;
        }

        FrameGraph& frameGraph = *request.m_frameGraph;

        /// [Phase 1] Compiles the cross-queue scope graph.
        CompileQueueCentricScopeGraph(frameGraph, request.m_compileFlags);

        /// [Phase 2] Compile transient attachments across all scopes.
        CompileTransientAttachments(
            frameGraph,
            *request.m_transientAttachmentPool,
            request.m_compileFlags,
            request.m_statisticsFlags);

        /// [Phase 3] Compiles buffer / image views and assigns them to scope attachments.
        CompileResourceViews(frameGraph.GetAttachmentDatabase());

        /// [Phase 4] Compile platform-specific scope data after all attachments and views have been compiled.
        {
            AZ_PROFILE_SCOPE(RHI, "FrameGraphCompiler: Scope Compile");

            for (Scope* scope : frameGraph.GetScopes())
            {
                scope->Compile();
            }
        }

        /// Perform platform-specific compilation.
        return CompileInternal(request);
    }

    void FrameGraphCompiler::CompileQueueCentricScopeGraph(
        FrameGraph& frameGraph,
        FrameSchedulerCompileFlags compileFlags)
    {
        AZ_PROFILE_SCOPE(RHI, "FrameGraphCompiler: CompileQueueCentricScopeGraph");

        const bool disableAsyncQueues = CheckBitsAll(compileFlags, FrameSchedulerCompileFlags::DisableAsyncQueues);
        if (disableAsyncQueues)
        {
            for (Scope* scope : frameGraph.GetScopes())
            {
                scope->m_hardwareQueueClass = HardwareQueueClass::Graphics;
            }
        }

        // Build the per-queue graph by first linking scopes on the same queue
        // with their neighbors. This is because the queue is going to execute serially.
        {
            Scope* producer[HardwareQueueClassCount] = {};
            for (Scope* consumer : frameGraph.GetScopes())
            {
                const uint32_t hardwareQueueClassIdx = static_cast<uint32_t>(consumer->GetHardwareQueueClass());
                if (producer[hardwareQueueClassIdx])
                {
                    if (producer[hardwareQueueClassIdx]->GetDeviceIndex() == consumer->GetDeviceIndex())
                    {
                        Scope::LinkProducerConsumerByQueues(producer[hardwareQueueClassIdx], consumer);
                    }
                }
                producer[hardwareQueueClassIdx] = consumer;
            }
        }

        /// If async queues are disabled, just return.
        if (disableAsyncQueues)
        {
            return;
        }

        // Build cross-queue edges. This is more complicated because each queue forms a "track" of serialized scopes,
        // but each track is able to mark dependencies on nodes in other tracks. In the final graph, each scope is able to have
        // a single producer / consumer from each queue. We also want to cull out edges that are superfluous.
        //
        // The algorithm first iterates the list of scopes from beginning to end. For consumers of the current scope,
        // we can pick the earliest one for each queue, since all later ones are unnecessary (due to same-queue serialization).
        //
        // When we find the first consumer (for each queue), we need to check that we are the last producer feeding into that consumer on the queue. Otherwise,
        // we are fencing too early. For instance, a later scope on the same queue as us could fence the consumer (or an earlier consumer), which satisfies the constraint
        // making the current edge unnecessary. Once we find the last producer and the first consumer for the current node, we search for a later
        // producer (on the producer's queue) which feeds an earlier consumer (on the consumer's queue). If this test fails, we have found the optimal fencing point.
        for (Scope* currentScope : frameGraph.GetScopes())
        {
            // Grab the last producer on a specific queue that feeds into this scope. Then search to see if a later producer
            // on the producer queue feeds an earlier consumer on the consumer queue. If not, then we have a valid edge.
            for (uint32_t producerHardwareQueueIdx = 0; producerHardwareQueueIdx < HardwareQueueClassCount; ++producerHardwareQueueIdx)
            {
                if (Scope* producerScopeLast = currentScope->m_producersByQueueLast[producerHardwareQueueIdx])
                {
                    bool foundEarlierConsumerOnSameQueue = false;

                    const Scope* nextProducerScope = producerScopeLast->GetConsumerOnSameQueue();
                    while (nextProducerScope)
                    {
                        if (const Scope* sameQueueConsumerScope = nextProducerScope->GetConsumerByQueue(currentScope->GetHardwareQueueClass()))
                        {
                            if (sameQueueConsumerScope->GetIndex() < currentScope->GetIndex())
                            {
                                foundEarlierConsumerOnSameQueue = true;
                            }
                        }

                        nextProducerScope = nextProducerScope->GetConsumerOnSameQueue();
                    }

                    if (foundEarlierConsumerOnSameQueue == false)
                    {
                        if (producerScopeLast->GetDeviceIndex() == currentScope->GetDeviceIndex())
                        {
                            Scope::LinkProducerConsumerByQueues(producerScopeLast, currentScope);
                        }
                    }
                }
            }

            Scope* consumersByQueueFirst[HardwareQueueClassCount] = {};

            // Compute the first consumer for each queue.
            for (Scope* consumer : frameGraph.GetConsumers(*currentScope))
            {
                const bool crossQueueEdge = currentScope->GetHardwareQueueClass() != consumer->GetHardwareQueueClass();
                if (crossQueueEdge)
                {
                    Scope*& consumerScopeFirst = consumersByQueueFirst[static_cast<uint32_t>(consumer->GetHardwareQueueClass())];
                    if (consumerScopeFirst == nullptr || consumerScopeFirst->GetIndex() > consumer->GetIndex())
                    {
                        consumerScopeFirst = consumer;
                    }
                }
            }

            // For each valid first consumer (one per queue), check if we (the producer) are the last (so far) producer to feed into
            // that consumer on our queue. If so, make us the new producer on our queue.
            for (uint32_t consumerHardwareQueueClassIdx = 0; consumerHardwareQueueClassIdx < HardwareQueueClassCount; ++consumerHardwareQueueClassIdx)
            {
                if (Scope* consumerScopeFirst = consumersByQueueFirst[consumerHardwareQueueClassIdx])
                {
                    Scope*& producerScopeLast = consumerScopeFirst->m_producersByQueueLast[consumerHardwareQueueClassIdx];

                    if (producerScopeLast == nullptr || producerScopeLast->GetIndex() < currentScope->GetIndex())
                    {
                        producerScopeLast = currentScope;
                    }
                }
            }
        }
    }

    void FrameGraphCompiler::ExtendTransientAttachmentAsyncQueueLifetimes(
        FrameGraph& frameGraph,
        FrameSchedulerCompileFlags compileFlags)
    {
        /// No need to do this if we have disabled async queues entirely.
        if (CheckBitsAny(compileFlags, FrameSchedulerCompileFlags::DisableAsyncQueues))
        {
            return;
        }

        AZ_PROFILE_FUNCTION(RHI);

        // Each attachment declares which queue classes it can be used on. We require that the first scope be on the most
        // capable queue. This is because we know that we are always able to service transition barrier requests for all
        // frames. NOTE: This only applies to images which have certain restrictions around layout transitions.
        const FrameGraphAttachmentDatabase& attachmentDatabase = frameGraph.GetAttachmentDatabase();
        for (FrameAttachment* transientImage : attachmentDatabase.GetTransientImageAttachments())
        {
            for (int deviceIndex{ 0 }; deviceIndex < RHISystemInterface::Get()->GetDeviceCount(); ++deviceIndex)
            {
                Scope* firstScope = transientImage->GetFirstScope(deviceIndex);
                if (firstScope == nullptr)
                {
                    // If the attachment is owned by a pass that isn't a scope-producer (e.g. Parent-Pass), and is not connected to
                    // anything, the first and last scope will be empty. We will get a warning its unused in ValidateEnd(), but we don't
                    // want to crash here
                    continue;
                }
                const HardwareQueueClass mostCapableQueueUsage = GetMostCapableHardwareQueue(transientImage->GetSupportedQueueMask());

                if (firstScope->GetHardwareQueueClass() != mostCapableQueueUsage)
                {
                    if (Scope* foundScope = firstScope->FindCapableCrossQueueProducer(mostCapableQueueUsage))
                    {
                        transientImage->m_scopeInfos[deviceIndex].m_firstScope = foundScope;
                        continue;
                    }

                    AZ_Warning(
                        "FrameGraphCompiler",
                        false,
                        "Could not find a %s queue producer scope to begin aliasing attachment '%s'. This can be remedied by "
                        "having a %s scope earlier in the frame (or as the root of the frame graph).",
                        GetHardwareQueueClassName(mostCapableQueueUsage),
                        transientImage->GetId().GetCStr(),
                        GetHardwareQueueClassName(mostCapableQueueUsage));
                }
            }
        }

        const auto& scopes = frameGraph.GetScopes();

        // Adjust asynchronous attachment lifetimes. If scopes executing in parallel are utilizing transient
        // attachments, we must extend their lifetimes so that memory is aliased properly. To do this, we first
        // compute the intervals in the sorted scope array where asynchronous activity is occurring. This is
        // done by traversing cross-queue fork / join events.
        struct AsyncInterval
        {
            uint32_t m_indexFirst = 0;
            uint32_t m_indexLast = 0;
            uint32_t m_attachmentCountsByQueue[HardwareQueueClassCount] = {};

            /// This the hardware queue that is allowed to alias memory.
            HardwareQueueClass m_aliasingQueueClass = HardwareQueueClass::Graphics;
        };

        AZStd::vector<AsyncInterval> asyncIntervals;

        const uint32_t ScopeCount = static_cast<uint32_t>(scopes.size());
        for (uint32_t scopeIdx = 0; scopeIdx < ScopeCount; ++scopeIdx)
        {
            Scope* scope = scopes[scopeIdx];
            bool foundInterval = false;

            AsyncInterval interval;
            interval.m_indexFirst = scope->GetIndex();

            for (uint32_t hardwareQueueClassIdx = 0; hardwareQueueClassIdx < HardwareQueueClassCount; ++hardwareQueueClassIdx)
            {
                HardwareQueueClass hardwareQueueClass = static_cast<HardwareQueueClass>(hardwareQueueClassIdx);

                // Skip the queue class matching this scope, we only want cross-queue fork events.
                if (hardwareQueueClass == scope->GetHardwareQueueClass())
                {
                    continue;
                }

                // If this succeeds, we have reached a cross-queue fork. This is the beginning of the async
                // interval. To find the end, we search along the newly forked path (on the other queue) until
                // we join back to the original queue. The interval ends just before the join scope.
                if (const Scope* otherQueueScope = scope->GetConsumerByQueue(hardwareQueueClass))
                {
                    // If the search fails, we fall back to the end of the scope list.
                    uint32_t indexLast = ScopeCount - 1;

                    // Search for a join event.
                    do
                    {
                        if (const Scope* joinScope = otherQueueScope->GetConsumerByQueue(scope->GetHardwareQueueClass()))
                        {
                            // End the interval just before the join scope.
                            indexLast = joinScope->GetIndex() - 1;
                            foundInterval = true;
                            break;
                        }

                        otherQueueScope = otherQueueScope->GetConsumerOnSameQueue();

                    } while (otherQueueScope);

                    // Keep track of the last index. Since we search across all the queues, we may have multiple.
                    interval.m_indexLast = AZStd::max(interval.m_indexLast, indexLast);
                }
            }

            if (foundInterval)
            {
                // Accumulate scope attachments for all scopes in the interval. This will be used to find the best queue to
                // allow aliasing.
                for (uint32_t asyncScopeIdx = interval.m_indexFirst; asyncScopeIdx <= interval.m_indexLast; ++asyncScopeIdx)
                {
                    const Scope* asyncScope = scopes[asyncScopeIdx];
                    interval.m_attachmentCountsByQueue[static_cast<uint32_t>(asyncScope->GetHardwareQueueClass())] += static_cast<uint32_t>(asyncScope->GetTransientAttachments().size());
                }

                asyncIntervals.push_back(interval);
                scopeIdx = interval.m_indexLast;
            }
        }

        const bool disableAsyncQueueAliasing = CheckBitsAny(compileFlags, FrameSchedulerCompileFlags::DisableAttachmentAliasingAsyncQueue);

        // Find the maximum number of transient scope attachments per queue. The one with the most gets to alias memory.
        if (disableAsyncQueueAliasing == false)
        {
            for (AsyncInterval& interval : asyncIntervals)
            {
                uint32_t scopeAttachmentCountMax = 0;
                for (uint32_t i = 0; i < HardwareQueueClassCount; ++i)
                {
                    if (scopeAttachmentCountMax < interval.m_attachmentCountsByQueue[i])
                    {
                        scopeAttachmentCountMax = interval.m_attachmentCountsByQueue[i];
                        interval.m_aliasingQueueClass = (HardwareQueueClass)i;
                    }
                }
            }
        }

        // Finally, for each scope that is within an async interval, we must extend
        // the lifetimes to fill the whole interval. This is because we cannot alias
        // memory between queues on the GPU, as the aliasing system assumes serialized
        // lifetimes. However, we can still allow one queue to alias memory with itself.
        for (uint32_t scopeIdx = 0; scopeIdx < uint32_t(scopes.size()); ++scopeIdx)
        {
            Scope* scope = scopes[scopeIdx];

            for (AsyncInterval interval : asyncIntervals)
            {
                // Only one queue is allowed to alias in async scenarios. In order to alias properly,
                // attachments must have well-defined lifetimes, which is not possible with async execution.
                // However, this is true of a single queue with itself, so one queue is chosen to allow aliasing
                // and the rest will extend lifetimes.
                const bool isAliasingAllowed = !disableAsyncQueueAliasing && interval.m_aliasingQueueClass == scope->GetHardwareQueueClass();

                if (interval.m_indexFirst <= scopeIdx && scopeIdx <= interval.m_indexLast)
                {
                    for (ScopeAttachment* scopeAttachment : scope->GetTransientAttachments())
                    {
                        FrameAttachment& frameAttachment = scopeAttachment->GetFrameAttachment();

                        // If we aren't allowed to alias or we're a cross queue attachment, then extend lifetimes to
                        // the beginning and end of the async interval.
                        if (!isAliasingAllowed)
                        {
                            auto& scopeInfo{ frameAttachment.m_scopeInfos[scope->GetDeviceIndex()] };

                            if (scopeInfo.m_firstScope->GetIndex() > interval.m_indexFirst)
                            {
                                scopeInfo.m_firstScope = scopes[interval.m_indexFirst];
                            }

                            if (scopeInfo.m_lastScope->GetIndex() < interval.m_indexLast)
                            {
                                scopeInfo.m_lastScope = scopes[interval.m_indexLast];
                            }
                        }
                    }
                }
            }
        }
    }

    void FrameGraphCompiler::ExtendTransientAttachmentGroupLifetimes(FrameGraph& frameGraph, FrameSchedulerCompileFlags compileFlags)
    {
        /// No need to do this if intra group aliasing is allowed
        if (CheckBitsAny(compileFlags, FrameSchedulerCompileFlags::IntraGroupAliasing))
        {
            return;
        }

        // Extend the lifetime of a transient attachment to the end of the graph group so we don't have aliasing of the same
        // memory in the same group. Some APIs don't allow synchronization of aliased resource between scopes in the group.
        const FrameGraphAttachmentDatabase& attachmentDatabase = frameGraph.GetAttachmentDatabase();
        const auto& scopes = frameGraph.GetScopes();
        ExtendTransientAttachmentGroupLifetimesHelper(scopes, attachmentDatabase.GetTransientBufferAttachments());
        ExtendTransientAttachmentGroupLifetimesHelper(scopes, attachmentDatabase.GetTransientImageAttachments());
    }

    template<class T>
    void FrameGraphCompiler::ExtendTransientAttachmentGroupLifetimesHelper(
        const AZStd::vector<Scope*>& scopes, const AZStd::vector<T*>& frameAttachments)
    {
        for (T* transientResource : frameAttachments)
        {
            for (int deviceIndex{ 0 }; deviceIndex < RHISystemInterface::Get()->GetDeviceCount(); ++deviceIndex)
            {
                Scope* lastScope = transientResource->GetLastScope(deviceIndex);
                if (!lastScope)
                {
                    continue;
                }

                for (uint32_t scopeIndex = lastScope->GetIndex() + 1;
                     scopeIndex < scopes.size() && lastScope->GetFrameGraphGroupId() == scopes[scopeIndex]->GetFrameGraphGroupId();
                     ++scopeIndex)
                {
                    lastScope = scopes[scopeIndex];
                }
                transientResource->m_scopeInfos[deviceIndex].m_lastScope = lastScope;
            }
        }
    }

    void FrameGraphCompiler::OptimizeTransientLoadStoreActions(FrameGraph& frameGraph, FrameSchedulerCompileFlags compileFlags)
    {
        /// No need to do this if optimization is disabled.
        if (CheckBitsAny(compileFlags, FrameSchedulerCompileFlags::DisableLoadStoreActionOptimization))
        {
            return;
        }

        // Set the load of first usage of transient attachments to DontCare.
        // Set the store of last usage of transient attachments to DontCare.
        const FrameGraphAttachmentDatabase& attachmentDatabase = frameGraph.GetAttachmentDatabase();
        OptimizeTransientLoadStoreActionsHelper(attachmentDatabase.GetTransientBufferAttachments());
        OptimizeTransientLoadStoreActionsHelper(attachmentDatabase.GetTransientImageAttachments());
    }

    template<class T>
    void FrameGraphCompiler::OptimizeTransientLoadStoreActionsHelper(const AZStd::vector<T*>& frameAttachments)
    {
        for (T* transientResource : frameAttachments)
        {
            for (int deviceIndex{ 0 }; deviceIndex < RHISystemInterface::Get()->GetDeviceCount(); ++deviceIndex)
            {
                auto* firstScopeAttachment = transientResource->GetFirstScopeAttachment(deviceIndex);
                if (firstScopeAttachment)
                {
                    // No need to load if it's the first usage of the frame.
                    auto& loadStoreAction = firstScopeAttachment->m_descriptor.m_loadStoreAction;
                    if (loadStoreAction.m_loadAction != AttachmentLoadAction::Clear)
                    {
                        loadStoreAction.m_loadAction = AttachmentLoadAction::DontCare;
                    }
                    if (loadStoreAction.m_loadActionStencil != AttachmentLoadAction::Clear)
                    {
                        loadStoreAction.m_loadActionStencil = AttachmentLoadAction::DontCare;
                    }
                }

                auto* lastScopeAttachment = transientResource->GetLastScopeAttachment(deviceIndex);
                if (lastScopeAttachment)
                {
                    // No need to store if it's the last scope using the transient attachment.
                    lastScopeAttachment->m_descriptor.m_loadStoreAction.m_storeAction = AttachmentStoreAction::DontCare;
                    lastScopeAttachment->m_descriptor.m_loadStoreAction.m_storeActionStencil = AttachmentStoreAction::DontCare;
                }
            }
        }
    }

    void FrameGraphCompiler::CompileTransientAttachments(
        FrameGraph& frameGraph,
        TransientAttachmentPool& transientAttachmentPool,
        FrameSchedulerCompileFlags compileFlags,
        FrameSchedulerStatisticsFlags statisticsFlags)
    {
        const FrameGraphAttachmentDatabase& attachmentDatabase = frameGraph.GetAttachmentDatabase();
        if (attachmentDatabase.GetTransientBufferAttachments().empty() && attachmentDatabase.GetTransientImageAttachments().empty())
        {
            return;
        }

        AZ_PROFILE_SCOPE(RHI, "FrameGraphCompiler: CompileTransientAttachments");

        ExtendTransientAttachmentAsyncQueueLifetimes(frameGraph, compileFlags);
        ExtendTransientAttachmentGroupLifetimes(frameGraph, compileFlags);

        OptimizeTransientLoadStoreActions(frameGraph, compileFlags);

        // Builds a sortable key. It iterates each scope and performs deactivations
        // followed by activations on each attachment.
        const uint32_t ATTACHMENT_BIT_COUNT = 16;
        const uint32_t SCOPE_BIT_COUNT = 14;

        enum class Action
        {
            ActivateImage = 0,
            ActivateBuffer,
            DeactivateImage,
            DeactivateBuffer,
        };

        struct Command
        {
            Command(uint32_t scopeIndex, Action action, uint32_t attachmentIndex)
            {
                m_bits.m_scopeIndex = scopeIndex;
                m_bits.m_action = (uint32_t)action;
                m_bits.m_attachmentIndex = attachmentIndex;
            }

            bool operator < (Command rhs) const
            {
                return m_command < rhs.m_command;
            }

            struct Bits
            {
                /// Sort by attachment index last
                uint32_t m_attachmentIndex : ATTACHMENT_BIT_COUNT;

                /// Sort by the action after the scope. First by deactivations, then by activations.
                uint32_t m_action : 2;

                /// Sort by scope index first.
                uint32_t m_scopeIndex : SCOPE_BIT_COUNT;
            };

            union
            {
                Bits m_bits;

                uint32_t m_command = 0;
            };
        };

        const auto& scopes = frameGraph.GetScopes();
        const auto& transientBufferGraphAttachments = attachmentDatabase.GetTransientBufferAttachments();
        const auto& transientImageGraphAttachments = attachmentDatabase.GetTransientImageAttachments();

        AZ_Assert(scopes.size() < AZ_BIT(SCOPE_BIT_COUNT),
            "Exceeded maximum number of allowed scopes");

        AZ_Assert(
            transientBufferGraphAttachments.size() + transientImageGraphAttachments.size() < AZ_BIT(ATTACHMENT_BIT_COUNT),
            "Exceeded maximum number of allowed attachments");

        AZStd::vector<Buffer*> transientBuffers(transientBufferGraphAttachments.size());
        AZStd::vector<Image*> transientImages(transientImageGraphAttachments.size());
        AZStd::vector<Command> commands;
        commands.reserve((transientBufferGraphAttachments.size() + transientImageGraphAttachments.size()) * 2);
        AZStd::vector<AZStd::pair<int, uint32_t>> removeBuffers;
        AZStd::vector<AZStd::pair<int, uint32_t>> removeImages;

        if (CheckBitsAny(compileFlags, FrameSchedulerCompileFlags::DisableAttachmentAliasing))
        {
            const uint32_t ScopeIndexFirst = 0;
            const uint32_t ScopeIndexLast = static_cast<uint32_t>(scopes.size() - 1);

            // Generate commands for each transient buffer: one for activation, and one for deactivation.
            for (uint32_t attachmentIndex = 0; attachmentIndex < (uint32_t)transientBufferGraphAttachments.size(); ++attachmentIndex)
            {
                commands.emplace_back(ScopeIndexFirst, Action::ActivateBuffer, attachmentIndex);
                commands.emplace_back(ScopeIndexLast, Action::DeactivateBuffer, attachmentIndex);
            }

            // Generate commands for each transient image: one for activation, and one for deactivation.
            for (uint32_t attachmentIndex = 0; attachmentIndex < (uint32_t)transientImageGraphAttachments.size(); ++attachmentIndex)
            {
                commands.emplace_back(ScopeIndexFirst, Action::ActivateImage, attachmentIndex);
                commands.emplace_back(ScopeIndexLast, Action::DeactivateImage, attachmentIndex);
            }
        }
        else
        {
            for (int deviceIndex{ 0 }; deviceIndex < RHISystemInterface::Get()->GetDeviceCount(); ++deviceIndex)
            {
                // Generate commands for each transient buffer: one for activation, and one for deactivation.
                for (uint32_t attachmentIndex = 0; attachmentIndex < (uint32_t)transientBufferGraphAttachments.size(); ++attachmentIndex)
                {
                    BufferFrameAttachment* transientBuffer = transientBufferGraphAttachments[attachmentIndex];
                    const auto* firstScope = transientBuffer->GetFirstScope(deviceIndex);
                    const auto* lastScope = transientBuffer->GetLastScope(deviceIndex);
                    if (firstScope == nullptr || lastScope == nullptr)
                    {
                        removeBuffers.emplace_back(deviceIndex, attachmentIndex);
                        // If the attachment is owned by a pass that isn't a scope-producer (e.g. Parent-Pass), and is not connected to
                        // anything, the first and last scope will be empty. We will get a warning its unused in ValidateEnd(), but we don't
                        // want to crash here
                        continue;
                    }
                    const uint32_t scopeIndexFirst = firstScope->GetIndex();
                    const uint32_t scopeIndexLast = lastScope->GetIndex();
                    commands.emplace_back(scopeIndexFirst, Action::ActivateBuffer, attachmentIndex);
                    commands.emplace_back(scopeIndexLast, Action::DeactivateBuffer, attachmentIndex);
                }

                // Generate commands for each transient image: one for activation, and one for deactivation.
                for (uint32_t attachmentIndex = 0; attachmentIndex < (uint32_t)transientImageGraphAttachments.size(); ++attachmentIndex)
                {
                    ImageFrameAttachment* transientImage = transientImageGraphAttachments[attachmentIndex];
                    const auto* firstScope = transientImage->GetFirstScope(deviceIndex);
                    const auto* lastScope = transientImage->GetLastScope(deviceIndex);
                    if (firstScope == nullptr || lastScope == nullptr)
                    {
                        removeImages.emplace_back(deviceIndex, attachmentIndex);
                        // If the attachment is owned by a pass that isn't a scope-producer (e.g. Parent-Pass), and is not connected to
                        // anything, the first and last scope will be empty. We will get a warning its unused in ValidateEnd(), but we don't
                        // want to crash here
                        continue;
                    }
                    const uint32_t scopeIndexFirst = firstScope->GetIndex();
                    const uint32_t scopeIndexLast = lastScope->GetIndex();
                    commands.emplace_back(scopeIndexFirst, Action::ActivateImage, attachmentIndex);
                    commands.emplace_back(scopeIndexLast, Action::DeactivateImage, attachmentIndex);
                }
            }
        }

        AZStd::sort(commands.begin(), commands.end());

        auto processCommands = [&](int deviceIndex,
                                   TransientAttachmentPoolCompileFlags compileFlags,
                                   TransientAttachmentStatistics::MemoryUsage* memoryHint = nullptr)
        {
            transientAttachmentPool.Begin(compileFlags, memoryHint);

            uint32_t currentScopeIndex = static_cast<uint32_t>(-1);

            bool allocateResources = !CheckBitsAny(compileFlags, TransientAttachmentPoolCompileFlags::DontAllocateResources);
            bool beganScope = false;

            for (Command command : commands)
            {
                const uint32_t scopeIndex = command.m_bits.m_scopeIndex;
                const uint32_t attachmentIndex = command.m_bits.m_attachmentIndex;
                const Action action = (Action)command.m_bits.m_action;

                if (scopes[scopeIndex]->GetDeviceIndex() != deviceIndex)
                {
                    continue;
                }

                // Make sure to walk the full set of scopes, even if a transient resource doesn't
                // exist in it. This is necessary for proper statistics tracking.
                while (currentScopeIndex != scopeIndex)
                {
                    const uint32_t nextScope = ++currentScopeIndex;

                    if (scopes[nextScope]->GetDeviceIndex() != deviceIndex)
                    {
                        continue;
                    }

                    // End the previous scope (if there is one).
                    if (beganScope)
                    {
                        transientAttachmentPool.EndScope();
                    }
                    else
                    {
                        beganScope = true;
                    }

                    transientAttachmentPool.BeginScope(*scopes[nextScope]);
                }

                switch (action)
                {

                case Action::DeactivateBuffer:
                    {
                        AZ_Assert(
                            !allocateResources || transientBuffers[attachmentIndex] || IsNullRHI(),
                            "DeviceBuffer is not active: %s",
                            transientBufferGraphAttachments[attachmentIndex]->GetId().GetCStr());
                        BufferFrameAttachment* bufferFrameAttachment = transientBufferGraphAttachments[attachmentIndex];
                        transientAttachmentPool.DeactivateBuffer(bufferFrameAttachment->GetId());
                        transientBuffers[attachmentIndex] = nullptr;
                        break;
                    }

                case Action::DeactivateImage:
                    {
                        AZ_Assert(
                            !allocateResources || transientImages[attachmentIndex] || IsNullRHI(),
                            "DeviceImage is not active: %s",
                            transientImageGraphAttachments[attachmentIndex]->GetId().GetCStr());
                        ImageFrameAttachment* imageFrameAttachment = transientImageGraphAttachments[attachmentIndex];
                        transientAttachmentPool.DeactivateImage(imageFrameAttachment->GetId());
                        transientImages[attachmentIndex] = nullptr;
                        break;
                    }

                case Action::ActivateBuffer:
                    {
                        BufferFrameAttachment* bufferFrameAttachment = transientBufferGraphAttachments[attachmentIndex];
                        AZ_Assert(
                            transientBuffers[attachmentIndex] == nullptr,
                            "DeviceBuffer has been activated already. %s",
                            bufferFrameAttachment->GetId().GetCStr());

                        TransientBufferDescriptor descriptor;
                        descriptor.m_attachmentId = bufferFrameAttachment->GetId();
                        descriptor.m_bufferDescriptor = bufferFrameAttachment->GetBufferDescriptor();

                        auto buffer = transientAttachmentPool.ActivateBuffer(descriptor);
                        if (allocateResources && buffer)
                        {
                            bufferFrameAttachment->SetResource(buffer, scopes[currentScopeIndex]->GetDeviceIndex());
                            transientBuffers[attachmentIndex] = buffer;
                        }
                        break;
                    }

                case Action::ActivateImage:
                    {
                        ImageFrameAttachment* imageFrameAttachment = transientImageGraphAttachments[attachmentIndex];
                        AZ_Assert(
                            transientImages[attachmentIndex] == nullptr,
                            "DeviceImage has been activated already. %s",
                            imageFrameAttachment->GetId().GetCStr());

                        ClearValue optimizedClearValue;

                        TransientImageDescriptor descriptor;
                        descriptor.m_attachmentId = imageFrameAttachment->GetId();
                        descriptor.m_imageDescriptor = imageFrameAttachment->GetImageDescriptor();
                        descriptor.m_supportedQueueMask = imageFrameAttachment->GetSupportedQueueMask();

                        const bool isOutputMerger = RHI::CheckBitsAny(
                            descriptor.m_imageDescriptor.m_bindFlags, RHI::ImageBindFlags::Color | RHI::ImageBindFlags::DepthStencil);
                        if (isOutputMerger)
                        {
                            optimizedClearValue = imageFrameAttachment->GetOptimizedClearValue(imageFrameAttachment->m_firstDeviceIndex);
                            descriptor.m_optimizedClearValue = &optimizedClearValue;
                        }

                        auto image = transientAttachmentPool.ActivateImage(descriptor);
                        if (allocateResources && image)
                        {
                            imageFrameAttachment->SetResource(image, scopes[currentScopeIndex]->GetDeviceIndex());
                            transientImages[attachmentIndex] = image;
                        }
                        break;
                    }
                }
            }

            if (beganScope)
            {
                transientAttachmentPool.EndScope();
            }

            transientAttachmentPool.End();
        };

        for (auto& [deviceIndex, descriptor] : transientAttachmentPool.GetDescriptor())
        {
            AZStd::optional<TransientAttachmentStatistics::MemoryUsage> memoryUsage;

            // Check if we need to do two passes (one for calculating the size and the second one for allocating the resources)
            if (descriptor.m_heapParameters.m_type == HeapAllocationStrategy::MemoryHint)
            {
                // First pass to calculate size needed.
                processCommands(
                    deviceIndex,
                    TransientAttachmentPoolCompileFlags::GatherStatistics | TransientAttachmentPoolCompileFlags::DontAllocateResources);
                auto statistics = transientAttachmentPool.GetDeviceTransientAttachmentPool(deviceIndex)->GetStatistics();
                memoryUsage = statistics.m_reservedMemory;
            }

            // Second pass uses the information about memory usage
            TransientAttachmentPoolCompileFlags poolCompileFlags = TransientAttachmentPoolCompileFlags::None;
            if (CheckBitsAny(statisticsFlags, FrameSchedulerStatisticsFlags::GatherTransientAttachmentStatistics))
            {
                poolCompileFlags |= TransientAttachmentPoolCompileFlags::GatherStatistics;
            }
            processCommands(deviceIndex, poolCompileFlags, memoryUsage ? &memoryUsage.value() : nullptr);
        }

        for (auto& [deviceIndex, attachmentIndex] : removeImages)
        {
            ImageFrameAttachment* imageFrameAttachment = transientImageGraphAttachments[attachmentIndex];

            auto image = imageFrameAttachment->GetImage();

            if (image)
            {
                transientAttachmentPool.RemoveDeviceImage(deviceIndex, image);
            }
        }

        for (auto& [deviceIndex, attachmentIndex] : removeBuffers)
        {
            BufferFrameAttachment* bufferFrameAttachment = transientBufferGraphAttachments[attachmentIndex];

            auto buffer = bufferFrameAttachment->GetBuffer();

            if (buffer)
            {
                transientAttachmentPool.RemoveDeviceBuffer(deviceIndex, buffer);
            }
        }
    }

    ImageView* FrameGraphCompiler::GetImageViewFromLocalCache(Image* image, const ImageViewDescriptor& imageViewDescriptor)
    {
        const size_t baseHash = AZStd::hash<Image*>()(image);
        // [GFX TODO][ATOM-6289] This should be looked into, combining cityhash with AZStd::hash
        const HashValue64 hash = imageViewDescriptor.GetHash(static_cast<HashValue64>(baseHash));

        // Attempt to find the image view in the cache.
        ImageView* imageView = m_imageViewCache.Find(static_cast<uint64_t>(hash));

        if (!imageView)
        {
            // This is one way of clearing view entries within the cache if we are creating a new view to replace the old one.
            // Normally this can happen for transient resources if their pointer within the heap changes for the current frame
            const ImageResourceViewData imageResourceViewData = ImageResourceViewData{ image->GetName(), imageViewDescriptor };
            RemoveFromCache(imageResourceViewData, m_imageReverseLookupHash, m_imageViewCache);
            // Create a new image view instance and insert it into the cache.
            Ptr<ImageView> imageViewPtr = image->BuildImageView(imageViewDescriptor);
            imageView = imageViewPtr.get();
            m_imageViewCache.Insert(static_cast<uint64_t>(hash), AZStd::move(imageViewPtr));
            if (!image->GetName().IsEmpty())
            {
                m_imageReverseLookupHash.emplace(imageResourceViewData, hash);
            }
        }
        return imageView;
    }

    BufferView* FrameGraphCompiler::GetBufferViewFromLocalCache(Buffer* buffer, const BufferViewDescriptor& bufferViewDescriptor)
    {
        const size_t baseHash = AZStd::hash<Buffer*>()(buffer);
        // [GFX TODO][ATOM-6289] This should be looked into, combining cityhash with AZStd::hash
        const HashValue64 hash = bufferViewDescriptor.GetHash(static_cast<HashValue64>(baseHash));

        // Attempt to find the buffer view in the cache.
        BufferView* bufferView = m_bufferViewCache.Find(static_cast<uint64_t>(hash));

        if (!bufferView)
        {
            // This is one way of clearing view entries within the cache if we are creating a new view to replace the old one.
            // Normally this can happen for transient resources if their pointer within the heap changes for the current frame
            const BufferResourceViewData bufferResourceViewData = BufferResourceViewData {buffer->GetName(), bufferViewDescriptor};
            RemoveFromCache(bufferResourceViewData, m_bufferReverseLookupHash, m_bufferViewCache);
                
            // Create a new buffer view instance and insert it into the cache.
            Ptr<BufferView> bufferViewPtr = buffer->BuildBufferView(bufferViewDescriptor);
            bufferView = bufferViewPtr.get();
            m_bufferViewCache.Insert(static_cast<uint64_t>(hash), AZStd::move(bufferViewPtr));
            if (!buffer->GetName().IsEmpty())
            {
                m_bufferReverseLookupHash.emplace(bufferResourceViewData, hash);
            }
        }
        return bufferView;
    }

    void FrameGraphCompiler::CompileResourceViews(const FrameGraphAttachmentDatabase& attachmentDatabase)
    {
        AZ_PROFILE_SCOPE(RHI, "FrameGraphCompiler: CompileResourceViews");

        for (ImageFrameAttachment* imageAttachment : attachmentDatabase.GetImageAttachments())
        {
            Image* image = imageAttachment->GetImage();

            if (!image)
            {
                continue;
            }
            // Iterates through every usage of the image, pulls image views
            // from image's cache or local cache, and assigns them to the scope attachments.
            for (int deviceIndex{ 0 }; deviceIndex < RHISystemInterface::Get()->GetDeviceCount(); ++deviceIndex)
            {
                for (ImageScopeAttachment* node = imageAttachment->GetFirstScopeAttachment(deviceIndex); node != nullptr;
                     node = node->GetNext())
                {
                    const ImageViewDescriptor& imageViewDescriptor = node->GetDescriptor().m_imageViewDescriptor;

                    // Multi device image views don't have a global cache, so we always cache them
                    ImageView* imageView = GetImageViewFromLocalCache(image, imageViewDescriptor);

                    node->SetImageView(imageView);
                }
            }
        }

        for (BufferFrameAttachment* bufferAttachment : attachmentDatabase.GetBufferAttachments())
        {
            Buffer* buffer = bufferAttachment->GetBuffer();

            if (!buffer)
            {
                continue;
            }

            // Iterates through every usage of the buffer attachment, pulls buffer views
            // from the cache within the buffer, and assigns them to the scope attachments.
            for (int deviceIndex{ 0 }; deviceIndex < RHISystemInterface::Get()->GetDeviceCount(); ++deviceIndex)
            {
                for (BufferScopeAttachment* node = bufferAttachment->GetFirstScopeAttachment(deviceIndex); node != nullptr;
                     node = node->GetNext())
                {
                    const BufferViewDescriptor& bufferViewDescriptor = node->GetDescriptor().m_bufferViewDescriptor;

                    // Multi device buffer views don't have a global cache, so we always cache them
                    BufferView* bufferView = GetBufferViewFromLocalCache(buffer, bufferViewDescriptor);

                    node->SetBufferView(bufferView);
                }
            }
        }
    }
    
    template<typename ReverseLookupObjectType, typename ObjectCacheType>
    void FrameGraphCompiler::RemoveFromCache(ReverseLookupObjectType objectToRemove,
                                                AZStd::unordered_map<ReverseLookupObjectType, HashValue64>& reverseHashLookupMap,
                                                ObjectCache<ObjectCacheType>& objectCache)
    {
        if (objectToRemove.m_name.IsEmpty())
        {
            return;
        }
            
        bool isResourceRegistered = reverseHashLookupMap.contains(objectToRemove);
        if (isResourceRegistered)
        {
            HashValue64 originalHash = reverseHashLookupMap.find(objectToRemove)->second;
            objectCache.EraseItem(aznumeric_cast<uint64_t>(originalHash));
            reverseHashLookupMap.erase(objectToRemove);
        }
    }
}

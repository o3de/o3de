/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/BufferFrameAttachment.h>
#include <Atom/RHI/DeviceBufferPoolBase.h>
#include <Atom/RHI/BufferScopeAttachment.h>
#include <Atom/RHI/FrameGraph.h>
#include <Atom/RHI/ImageFrameAttachment.h>
#include <Atom/RHI/DeviceImagePoolBase.h>
#include <Atom/RHI/ImageScopeAttachment.h>
#include <Atom/RHI/QueryPool.h>
#include <Atom/RHI/ResolveScopeAttachment.h>
#include <Atom/RHI/Scope.h>
#include <Atom/RHI/SwapChain.h>
#include <Atom/RHI/SwapChainFrameAttachment.h>

namespace AZ::RHI
{
    FrameGraph::~FrameGraph()
    {
        Clear();
    }

    bool FrameGraph::IsCompiled() const
    {
        return m_isCompiled;
    }

    void FrameGraph::SetCompiled()
    {
        m_isCompiled = true;
    }

    bool FrameGraph::IsBuilding() const
    {
        return m_isBuilding;
    }

    size_t FrameGraph::GetFrameCount() const
    {
        return m_frameCount;
    }

    const FrameGraphAttachmentDatabase& FrameGraph::GetAttachmentDatabase() const
    {
        return m_attachmentDatabase;
    }

    FrameGraphAttachmentDatabase& FrameGraph::GetAttachmentDatabase()
    {
        return m_attachmentDatabase;
    }

    void FrameGraph::Begin()
    {
        AZ_PROFILE_FUNCTION(RHI);

        AZ_Assert(m_isBuilding == false, "FrameGraph::Begin called, but End was never called on the previous build cycle!");
        AZ_Assert(m_isCompiled == false, "FrameGraph::Clear must be called before reuse.");
        Clear();
        m_isBuilding = true;
        m_frameCount++;
    }

    void FrameGraph::Clear()
    {
        AZ_PROFILE_SCOPE(RHI, "FrameGraph: Clear");
        for (Scope* scope : m_scopes)
        {
            scope->Deactivate();
        }
        m_scopes.clear();
        m_graphNodes.clear();
        m_graphEdges.clear();
        m_scopeLookup.clear();
        m_attachmentDatabase.Clear();
        m_isCompiled = false;
    }

    ResultCode FrameGraph::ValidateEnd()
    {
        if (Validation::IsEnabled())
        {
            if (m_isBuilding == false)
            {
                AZ_Error("FrameGraph", false, "FrameGraph::End called, but Begin was never called");

                Clear();
                return ResultCode::InvalidOperation;
            }

            if (m_currentScope)
            {
                AZ_Error("FrameGraph", false, "We are still building a scope %s!", m_currentScope->GetId().GetCStr());
                Clear();
                return ResultCode::InvalidOperation;
            }

            /// Validate that every attachment was used.
            for (FrameAttachment* attachment : m_attachmentDatabase.GetAttachments())
            {
                if (!attachment->HasScopeAttachments())
                {
                    //We allow the rendering to continue even if an attachment is not used.
                    AZ_WarningOnce(
                        "FrameGraph", false,
                        "Invalid State: attachment '%s' was added but never used!",
                        attachment->GetId().GetCStr());
                }
            }
        }

        return ResultCode::Success;
    }

    void FrameGraph::ValidateOverlappingAttachment(
        AttachmentId attachmentId, ScopeAttachmentUsage usage, [[maybe_unused]] ScopeAttachmentAccess access, const ScopeAttachment& scopeAttachment) const
    {
        // Validation for access type
        AZ_Assert(
            !CheckBitsAll(access, ScopeAttachmentAccess::Write) &&
            !CheckBitsAll(scopeAttachment.GetAccess(), ScopeAttachmentAccess::Write),
            "When adding two overlapping attachments in a scope, neither should have write access,\
            but a write access was detected when adding overlapping attachment %s.",
            attachmentId.GetCStr());

        // Validation for usage type
        switch (usage)
        {
        case ScopeAttachmentUsage::RenderTarget:
            {
                switch (scopeAttachment.GetUsage())
                {
                case ScopeAttachmentUsage::RenderTarget:
                    AZ_Assert(
                        false,
                        "Multiple usages of same type RenderTarget getting added for resource %s",
                        attachmentId.GetCStr());
                    break;
                default:
                    AZ_Assert(
                        false,
                        "ScopeAttachmentUsage::RenderTarget usage mixed with ScopeAttachmentUsage::%s for resource %s",
                        ToString(usage, access),
                        attachmentId.GetCStr());
                    break;
                }
                break;
            }
        case ScopeAttachmentUsage::DepthStencil:
            {
                switch (scopeAttachment.GetUsage())
                {
                case ScopeAttachmentUsage::DepthStencil:
                    AZ_Assert(
                        false,
                        "Multiple usages of same type DepthStencil getting added for resource %s",
                        attachmentId.GetCStr());
                    break;
                case ScopeAttachmentUsage::RenderTarget:
                case ScopeAttachmentUsage::Predication:
                case ScopeAttachmentUsage::Resolve:
                case ScopeAttachmentUsage::InputAssembly:
                case ScopeAttachmentUsage::ShadingRate:
                    AZ_Assert(
                        false,
                        "ScopeAttachmentUsage::DepthStencil usage mixed with ScopeAttachmentUsage::%s for resource %s",
                        ToString(usage, access),
                        attachmentId.GetCStr());
                    break;
                }
                break;
            }
        case ScopeAttachmentUsage::Shader:
            {
                switch (scopeAttachment.GetUsage())
                {
                case ScopeAttachmentUsage::Resolve:
                case ScopeAttachmentUsage::Predication:
                case ScopeAttachmentUsage::InputAssembly:
                    AZ_Assert(
                        false,
                        "ScopeAttachmentUsage::Shader usage mixed with ScopeAttachmentUsage::%s for resource %s",
                        ToString(usage, access),
                        attachmentId.GetCStr());
                    break;
                }
                break;
            }
        case ScopeAttachmentUsage::Resolve:
            {
                switch (scopeAttachment.GetUsage())
                {
                case ScopeAttachmentUsage::Resolve:
                    AZ_Assert(
                        false,
                        "Multiple usages of same type Resolve getting added for resource %s",
                        attachmentId.GetCStr());
                    break;
                case ScopeAttachmentUsage::RenderTarget:
                case ScopeAttachmentUsage::DepthStencil:
                case ScopeAttachmentUsage::Shader:
                case ScopeAttachmentUsage::Predication:
                case ScopeAttachmentUsage::SubpassInput:
                case ScopeAttachmentUsage::InputAssembly:
                case ScopeAttachmentUsage::ShadingRate:
                    AZ_Assert(
                        false,
                        "ScopeAttachmentUsage::Resolve usage mixed with ScopeAttachmentUsage::%s for resource %s",
                        ToString(usage, access),
                        attachmentId.GetCStr());
                    break;
                }
                break;
            }
        case ScopeAttachmentUsage::Predication:
            {
                switch (scopeAttachment.GetUsage())
                {
                case ScopeAttachmentUsage::Predication:
                    AZ_Assert(
                        false,
                        "Multiple usages of same type Predication getting added for resource %s",
                        attachmentId.GetCStr());
                    break;
                case ScopeAttachmentUsage::RenderTarget:
                case ScopeAttachmentUsage::DepthStencil:
                case ScopeAttachmentUsage::Shader:
                case ScopeAttachmentUsage::Resolve:
                case ScopeAttachmentUsage::SubpassInput:
                case ScopeAttachmentUsage::InputAssembly:
                case ScopeAttachmentUsage::ShadingRate:
                    AZ_Assert(
                        false,
                        "ScopeAttachmentUsage::Predication usage mixed with ScopeAttachmentUsage::%s for resource %s",
                        ToString(usage, access),
                        attachmentId.GetCStr());
                    break;
                }
                break;
            }
        case ScopeAttachmentUsage::Indirect:
            {
                break;
            }
        case ScopeAttachmentUsage::SubpassInput:
            {
                switch (scopeAttachment.GetUsage())
                {
                case ScopeAttachmentUsage::Resolve:
                case ScopeAttachmentUsage::Predication:
                case ScopeAttachmentUsage::InputAssembly:
                    AZ_Assert(
                        false,
                        "ScopeAttachmentUsage::SubpassInput usage mixed with ScopeAttachmentUsage::%s for resource %s",
                        ToString(usage, access),
                        attachmentId.GetCStr());
                    break;
                }
                break;
            }
        case ScopeAttachmentUsage::InputAssembly:
            {
                AZ_Assert(
                    false,
                    "ScopeAttachmentUsage::InputAssembly usage mixed with ScopeAttachmentUsage::%s for resource %s",
                    ToString(usage, access),
                    attachmentId.GetCStr());
                break;
            }
        case ScopeAttachmentUsage::ShadingRate:
            {
                switch (scopeAttachment.GetUsage())
                {
                case ScopeAttachmentUsage::Resolve:
                case ScopeAttachmentUsage::Predication:
                case ScopeAttachmentUsage::InputAssembly:
                case ScopeAttachmentUsage::Indirect:
                    AZ_Assert(
                        false,
                        "ScopeAttachmentUsage::ShadingRate usage mixed with ScopeAttachmentUsage::%s for resource %s",
                        ToString(usage, access),
                        attachmentId.GetCStr());
                    break;
                }
                break;
            }
        }
    }

    ResultCode FrameGraph::End()
    {
        AZ_PROFILE_SCOPE(RHI, "FrameGraph: End");
        ResultCode resultCode = ValidateEnd();
        if (resultCode != ResultCode::Success)
        {
            return resultCode;
        }

        /**
            * Swap chains are processed at the end of the last scope they are used on. This requires
            * waiting until all scopes have been added in order to have access to the full lifetime.
            */
        for (SwapChainFrameAttachment* attachment : m_attachmentDatabase.GetSwapChainAttachments())
        {
            auto* swapChain = attachment->GetSwapChain()->GetDeviceSwapChain().get();
            if (auto* lastScope = attachment->GetLastScope(swapChain->GetDevice().GetDeviceIndex()))
            {
                lastScope->m_swapChainsToPresent.push_back(swapChain);
            }
        }

        m_isBuilding = false;

        /// Finally, topologically sort the graph in preparation for compilation.
        resultCode = TopologicalSort();
        if (resultCode != ResultCode::Success)
        {
            Clear();
        }
        return resultCode;
    }

    void FrameGraph::BeginScope(Scope& scope)
    {
        AZ_Assert(
            m_currentScope == nullptr,
            "Cannot begin scope: %s, because scope %s is still recording! Only one scope can be recorded at a time.",
            scope.GetId().GetCStr(), m_currentScope->GetId().GetCStr());

        m_currentScope = &scope;
        m_scopeLookup.emplace(scope.GetId(), &scope);

        scope.m_graphNodeIndex = decltype(scope.m_graphNodeIndex)(m_graphNodes.size());
        m_graphNodes.emplace_back(scope);
    }

    void FrameGraph::EndScope()
    {
        m_currentScope = nullptr;
    }

    void FrameGraph::SetEstimatedItemCount(uint32_t itemCount)
    {
        m_currentScope->m_estimatedItemCount = itemCount;
    }

    void FrameGraph::SetHardwareQueueClass(HardwareQueueClass hardwareQueueClass)
    {
        m_currentScope->m_hardwareQueueClass = hardwareQueueClass;
    }

    void FrameGraph::SetGroupId(const ScopeGroupId& groupId)
    {
        AZ_Assert(m_currentScope, "Current scope is null while setting the group id");
        AZ_Assert(m_currentScope->m_graphNodeIndex.IsValid(), "Current scope doesn't have a valid node graph index");
        m_graphNodes[m_currentScope->m_graphNodeIndex.GetIndex()].m_scopeGroupId = groupId;
    }            

    void FrameGraph::UseAttachmentInternal(
        ImageFrameAttachment& frameAttachment,
        ScopeAttachmentUsage usage,
        ScopeAttachmentAccess access,
        ScopeAttachmentStage stage,
        const ImageScopeAttachmentDescriptor& descriptor)
    {
        AZ_Assert(usage != ScopeAttachmentUsage::Uninitialized, "ScopeAttachmentUsage is Uninitialized");
        AZ_Assert(stage != ScopeAttachmentStage::Uninitialized, "ScopeAttachmentStage is Uninitialized");

        if (Validation::IsEnabled())
        {
            ValidateAttachment(descriptor, usage, access);
        }

        // TODO:[ATOM-1267] Replace with writer / reader dependencies.
        if (Scope* producer = frameAttachment.GetLastScope(m_currentScope->GetDeviceIndex()))
        {
            InsertEdge(*producer, *m_currentScope);
        }

        ImageScopeAttachment* scopeAttachment =
            m_attachmentDatabase.EmplaceScopeAttachment<ImageScopeAttachment>(
            *m_currentScope, frameAttachment, usage, access, stage, descriptor);

        m_currentScope->m_attachments.push_back(scopeAttachment);
        m_currentScope->m_imageAttachments.push_back(scopeAttachment);
        if (frameAttachment.GetLifetimeType() == AttachmentLifetimeType::Transient)
        {
            m_currentScope->m_transientAttachments.push_back(scopeAttachment);
        }
    }

    void FrameGraph::UseAttachmentInternal(
        ImageFrameAttachment& frameAttachment,
        const ResolveScopeAttachmentDescriptor& descriptor)
    {
#if defined(AZ_ENABLE_TRACING)
        if (Validation::IsEnabled())
        {
            auto found = AZStd::find_if(m_currentScope->m_imageAttachments.begin(), m_currentScope->m_imageAttachments.end(), [&descriptor](const auto& scopeAttachment)
            {
                return scopeAttachment->GetFrameAttachment().GetId() == descriptor.m_resolveAttachmentId;
            });

            AZ_Assert(
                found != m_currentScope->m_imageAttachments.end(),
                "Could not find resolve attachment id '%s' when adding a ResolveScopeAttachment '%s'",
                descriptor.m_resolveAttachmentId.GetCStr(),
                descriptor.m_attachmentId.GetCStr());
        }
#endif

        // TODO:[ATOM-1267] Replace with writer / reader dependencies.
        if (Scope* producer = frameAttachment.GetLastScope(m_currentScope->GetDeviceIndex()))
        {
            InsertEdge(*producer, *m_currentScope);
        }

        ResolveScopeAttachment* scopeAttachment =
            m_attachmentDatabase.EmplaceScopeAttachment<ResolveScopeAttachment>(
                *m_currentScope, frameAttachment, descriptor);

        m_currentScope->m_attachments.push_back(scopeAttachment);
        m_currentScope->m_imageAttachments.push_back(scopeAttachment);
        m_currentScope->m_resolveAttachments.push_back(scopeAttachment);
        if (frameAttachment.GetLifetimeType() == AttachmentLifetimeType::Transient)
        {
            m_currentScope->m_transientAttachments.push_back(scopeAttachment);
        }
    }

    void FrameGraph::UseAttachmentInternal(
        BufferFrameAttachment& frameAttachment,
        ScopeAttachmentUsage usage,
        ScopeAttachmentAccess access,
        ScopeAttachmentStage stage,
        const BufferScopeAttachmentDescriptor& descriptor)
    {
        AZ_Assert(usage != ScopeAttachmentUsage::Uninitialized, "ScopeAttachmentUsage is Uninitialized");
        AZ_Assert(stage != ScopeAttachmentStage::Uninitialized, "ScopeAttachmentStage is Uninitialized");

        if (Validation::IsEnabled())
        {
            ValidateAttachment(descriptor, usage, access);
        }

        // TODO:[ATOM-1267] Replace with writer / reader dependencies.
        if (Scope* producer = frameAttachment.GetLastScope(m_currentScope->GetDeviceIndex()))
        {
            InsertEdge(*producer, *m_currentScope);
        }

        BufferScopeAttachment* scopeAttachment =
            m_attachmentDatabase.EmplaceScopeAttachment<BufferScopeAttachment>(
            *m_currentScope, frameAttachment, usage, access, stage, descriptor);

        m_currentScope->m_attachments.push_back(scopeAttachment);
        m_currentScope->m_bufferAttachments.push_back(scopeAttachment);
        if (frameAttachment.GetLifetimeType() == AttachmentLifetimeType::Transient)
        {
            m_currentScope->m_transientAttachments.push_back(scopeAttachment);
        }
    }

    ResultCode FrameGraph::UseAttachment(
        const BufferScopeAttachmentDescriptor& descriptor,
        ScopeAttachmentAccess access,
        ScopeAttachmentUsage usage,
        ScopeAttachmentStage stage)
    {
        AZ_Assert(!descriptor.m_attachmentId.IsEmpty(), "Calling FrameGraph::UseAttachment with an empty attachment ID");

        BufferFrameAttachment* attachment = m_attachmentDatabase.FindAttachment<BufferFrameAttachment>(descriptor.m_attachmentId);
        if (attachment)
        {
            UseAttachmentInternal(*attachment, usage, access, stage, descriptor);
            return ResultCode::Success;
        }

        AZ_Error("FrameGraph", false, "No compatible buffer attachment found for id: '%s'", descriptor.m_attachmentId.GetCStr());
        return ResultCode::InvalidArgument;
    }

    ResultCode FrameGraph::UseAttachment(
        const ImageScopeAttachmentDescriptor& descriptor,
        ScopeAttachmentAccess access,
        ScopeAttachmentUsage usage,
        ScopeAttachmentStage stage)
    {
        AZ_Assert(!descriptor.m_attachmentId.IsEmpty(), "Calling FrameGraph::UseAttachment with an empty attachment ID");

        ImageFrameAttachment* attachment = m_attachmentDatabase.FindAttachment<ImageFrameAttachment>(descriptor.m_attachmentId);
        if (attachment)
        {
            UseAttachmentInternal(*attachment, usage, access, stage, descriptor);
            return ResultCode::Success;
        }

        AZ_Error("FrameGraph", false, "No compatible image attachment found for id: '%s'", descriptor.m_attachmentId.GetCStr());
        return ResultCode::InvalidArgument;
    }

    ResultCode FrameGraph::UseAttachments(
        AZStd::span<const ImageScopeAttachmentDescriptor> descriptors,
        ScopeAttachmentAccess access,
        ScopeAttachmentUsage usage,
        ScopeAttachmentStage stage)
    {
        for (const ImageScopeAttachmentDescriptor& descriptor : descriptors)
        {
            ResultCode resultCode = UseAttachment(descriptor, access, usage, stage);
            if (resultCode != ResultCode::Success)
            {
                AZ_Error("FrameGraph", false, "Error loading image scope attachment array. Attachment that errored is '%s'", descriptor.m_attachmentId.GetCStr());
                return resultCode;
            }
        }
        return ResultCode::Success;
    }

    ResultCode FrameGraph::UseResolveAttachment(const ResolveScopeAttachmentDescriptor& descriptor)
    {
        ImageFrameAttachment* attachment = m_attachmentDatabase.FindAttachment<ImageFrameAttachment>(descriptor.m_attachmentId);
        if (attachment)
        {
            UseAttachmentInternal(*attachment, descriptor);
            return ResultCode::Success;
        }

        AZ_Error("FrameGraph", false, "No compatible image attachment found for id: '%s'", descriptor.m_attachmentId.GetCStr());
        return ResultCode::InvalidArgument;
    }

    ResultCode FrameGraph::UseColorAttachments(AZStd::span<const ImageScopeAttachmentDescriptor> descriptors)
    {
        return UseAttachments(
            descriptors, ScopeAttachmentAccess::Write, ScopeAttachmentUsage::RenderTarget, ScopeAttachmentStage::ColorAttachmentOutput);
    }

    ResultCode FrameGraph::UseDepthStencilAttachment(
        const ImageScopeAttachmentDescriptor& descriptor, ScopeAttachmentAccess access, ScopeAttachmentStage stage)
    {
        return UseAttachment(descriptor, access, ScopeAttachmentUsage::DepthStencil, stage);
    }

    ResultCode FrameGraph::UseSubpassInputAttachments(
        AZStd::span<const ImageScopeAttachmentDescriptor> descriptors, ScopeAttachmentStage stage)
    {
        return UseAttachments(descriptors, ScopeAttachmentAccess::Read, ScopeAttachmentUsage::SubpassInput, stage);
    }

    ResultCode FrameGraph::UseShaderAttachment(
        const BufferScopeAttachmentDescriptor& descriptor, ScopeAttachmentAccess access, ScopeAttachmentStage stage)
    {
        return UseAttachment(descriptor, access, ScopeAttachmentUsage::Shader, stage);
    }

    ResultCode FrameGraph::UseShaderAttachment(
        const ImageScopeAttachmentDescriptor& descriptor, ScopeAttachmentAccess access, ScopeAttachmentStage stage)
    {
        return UseAttachment(descriptor, access, ScopeAttachmentUsage::Shader, stage);
    }

    ResultCode FrameGraph::UseCopyAttachment(const BufferScopeAttachmentDescriptor& descriptor, ScopeAttachmentAccess access)
    {
        return UseAttachment(descriptor, access, ScopeAttachmentUsage::Copy, ScopeAttachmentStage::Copy);
    }

    ResultCode FrameGraph::UseCopyAttachment(const ImageScopeAttachmentDescriptor& descriptor, ScopeAttachmentAccess access)
    {
        return UseAttachment(descriptor, access, ScopeAttachmentUsage::Copy, ScopeAttachmentStage::Copy);
    }

    ResultCode FrameGraph::UseQueryPool(Ptr<QueryPool> queryPool, const RHI::Interval& interval, QueryPoolScopeAttachmentType type, ScopeAttachmentAccess access)
    {
        // We only insert an edge into the graph if the type of attachment is Local (i.e. is going to be accessed by other scopes in the current frame)
        if (type == QueryPoolScopeAttachmentType::Local)
        {
            ScopeId id = m_attachmentDatabase.EmplaceResourcePoolUse(*queryPool, m_currentScope->GetId());
            auto found = m_scopeLookup.find(id);
            if (found != m_scopeLookup.end())
            {
                InsertEdge(*found->second, *m_currentScope);
            }
        }

        m_currentScope->AddQueryPoolUse(queryPool, interval, access);
        return ResultCode::Success;
    }

    void FrameGraph::ExecuteAfter(const ScopeId& producerScopeId)
    {
        if (Scope* producer = FindScope(producerScopeId))
        {
            InsertEdge(*producer, *m_currentScope);
        }
    }

    void FrameGraph::ExecuteBefore(const ScopeId& consumerScopeId)
    {
        if (Scope* consumer = FindScope(consumerScopeId))
        {
            InsertEdge(*m_currentScope, *consumer);
        }
    }

    void FrameGraph::SignalFence(Fence& fence)
    {
        m_currentScope->m_fencesToSignal.push_back(&fence);
    }

    void FrameGraph::WaitFence(Fence &fence)
    {
        m_currentScope->m_fencesToWaitFor.push_back(&fence);
    }

    ResultCode FrameGraph::TopologicalSort()
    {
        struct NodeId
        {
            uint16_t m_nodeIndex;
            uint16_t m_groupId;
        };

        AZStd::vector<NodeId> unblockedNodes;
        unblockedNodes.reserve(m_graphNodes.size());

        // Build a list with the edges for each producer node.
        AZStd::vector<AZStd::vector<uint32_t>> graphEdges(m_graphNodes.size());
        for (uint32_t edgeIndex = 0; edgeIndex < m_graphEdges.size(); ++edgeIndex)
        {
            const GraphEdge& edge = m_graphEdges[edgeIndex];
            AZStd::vector<uint32_t>& edgeList = graphEdges[edge.m_producerIndex];
            edgeList.push_back(edgeIndex);
        }

       
        uint16_t groupCount = 0;
        AZStd::unordered_map<ScopeGroupId, uint16_t> groupIds;
        // Returns a groupId from the ScopeGroupId of the scopes.
        auto getGroupId = [&](const ScopeGroupId& groupId)
        {
            // An empty ScopeGroupId means the scope doesn't belong to a group.
            if (groupId.IsEmpty())
            {
                return groupCount++;
            }

            // It doesn't matter that we incremented groupCount when not inserting since we only
            // care of being a unique increment number.
            auto inserRet = groupIds.insert(AZStd::pair<ScopeGroupId, uint16_t>(groupId, groupCount++));
            return inserRet.first->second;
        };

        // This loop will add all unblocked nodes, i.e. nodes that don't have any producers. This
        // includes the root node.
        for (size_t nodeIndex = 0; nodeIndex < m_graphNodes.size(); ++nodeIndex)
        {
            const GraphNode& graphNode = m_graphNodes[nodeIndex];
            if (graphNode.m_unsortedProducerCount == 0)
            {
                unblockedNodes.push_back({ static_cast<uint16_t>(nodeIndex), getGroupId(graphNode.m_scopeGroupId) });
            }
        }

        AZStd::vector<AZStd::pair<Scope*, uint16_t>> preSortScopes;
        preSortScopes.reserve(m_graphNodes.size());

        // Process nodes that don't have any producers left (they have already been processed).
        // They get added to the unblockedNodes vector in a topological manner.
        while (!unblockedNodes.empty())
        {
            const NodeId& producerNodeId = unblockedNodes.back();
            const uint16_t producerIndex = producerNodeId.m_nodeIndex;
            const uint16_t producerGroupId = producerNodeId.m_groupId;
            unblockedNodes.pop_back();

            Scope* scope = m_graphNodes[producerIndex].m_scope;
            preSortScopes.push_back(AZStd::make_pair(scope, producerGroupId));

            // Go through all the edges of this node, find the consumer nodes that are fully sorted and add them to the unblockedNodes.
            for (const uint32_t edgeIndex : graphEdges[producerIndex])
            {
                const GraphEdge& graphEdge = m_graphEdges[edgeIndex];
                const uint16_t consumerIndex = static_cast<uint16_t>(graphEdge.m_consumerIndex);
                GraphNode& graphNode = m_graphNodes[consumerIndex];
                if (--graphNode.m_unsortedProducerCount == 0)
                {
                    NodeId newNode;
                    newNode.m_nodeIndex = consumerIndex;
                    newNode.m_groupId = getGroupId(graphNode.m_scopeGroupId);
                    unblockedNodes.emplace_back(newNode);
                }
            }
            graphEdges[producerIndex].clear();
        }

        //////////////////////////////////////////////////////////////////
        // This additional sort makes sure that scopes in the same group get grouped consecutively.
        // This is necessary when using subpasses.
        // This is an example on how a Multiview(aka XR) scenario would sort scopes WITHOUT
        // this sort:
        //     [0] "Root"
        //     [1] "XRLeftPipeline_-10.MultiViewForwardPass"
        //     [2] "XRRightPipeline_-10.MultiViewForwardPass"
        //     [3] "XRRightPipeline_-10.MultiViewSkyBoxPass"
        //     [4] "XRLeftPipeline_-10.MultiViewSkyBoxPass"
        // The RHI would crash because the subpasses in the LEFT View are not consecutive.
        // On the other hand, thanks to this sort the order would end like this:
        //     [0] "Root"
        //     [1] "XRLeftPipeline_-10.MultiViewForwardPass"
        //     [2] "XRLeftPipeline_-10.MultiViewSkyBoxPass" 
        //     [3] "XRRightPipeline_-10.MultiViewForwardPass"
        //     [4] "XRRightPipeline_-10.MultiViewSkyBoxPass"
        AZStd::stable_sort(
            preSortScopes.begin(),
            preSortScopes.end(),
            [](const auto& lhs, const auto& rhs)
            {
                // Sort by group id
                return (lhs.second < rhs.second);
            });

        // Activate the scope in topological order.
        m_scopes.resize(preSortScopes.size());
        for (uint32_t scopeIndex = 0; scopeIndex < preSortScopes.size(); ++scopeIndex)
        {
            Scope* scope = preSortScopes[scopeIndex].first;
            m_scopes[scopeIndex] = scope;
            Scope::ActivationFlags activationFlags = m_graphNodes[scope->m_graphNodeIndex.GetIndex()].m_scopeGroupId.IsEmpty()
                ? Scope::ActivationFlags::None
                : Scope::ActivationFlags::Subpass;
            scope->Activate(this, scopeIndex, GraphGroupId(preSortScopes[scopeIndex].second), activationFlags);
        }
        ////////////////////////////////////////////////////////////////

        if (m_graphNodes.size() == m_scopes.size())
        {
            return ResultCode::Success;
        }

        if (Validation::IsEnabled())
        {
            AZStd::string cycleInfoString = "Error, a cycle exists in the graph. Failed to topologically sort. Remaining Edges:\n";
            for (const AZStd::vector<uint32_t>& edgeList : graphEdges)
            {
                for (uint32_t edgeIndex : edgeList)
                {
                    const GraphEdge& edge = m_graphEdges[edgeIndex];
                    cycleInfoString += AZStd::string::format(
                        "\t[Producer: %s], [Consumer: %s]\n",
                        m_graphNodes[edge.m_producerIndex].m_scope->GetId().GetCStr(),
                        m_graphNodes[edge.m_consumerIndex].m_scope->GetId().GetCStr());
                }
            }
            AZ_Error("FrameGraph", false, "%s", cycleInfoString.data());
        }

        return ResultCode::InvalidArgument;
    }

    const Scope* FrameGraph::FindScope(const ScopeId& scopeId) const
    {
        auto findIt = m_scopeLookup.find(scopeId);
        if (findIt != m_scopeLookup.end())
        {
            return findIt->second;
        }
        return nullptr;
    }

    Scope* FrameGraph::FindScope(const ScopeId& scopeId)
    {
        return const_cast<Scope*>(const_cast<const FrameGraph*>(this)->FindScope(scopeId));
    }

    Scope* FrameGraph::GetRootScope() const
    {
        return m_scopes[0];
    }

    const AZStd::vector<Scope*>& FrameGraph::GetScopes() const
    {
        return m_scopes;
    }

    const AZStd::vector<Scope*>& FrameGraph::GetConsumers(const Scope& producer) const
    {
        return m_graphNodes[producer.m_graphNodeIndex.GetIndex()].m_consumers;
    }

    const AZStd::vector<Scope*>& FrameGraph::GetProducers(const Scope& consumer) const
    {
        return m_graphNodes[consumer.m_graphNodeIndex.GetIndex()].m_producers;
    }

    void FrameGraph::InsertEdge(Scope& producer, Scope& consumer)
    {
        // Ignore edges where the read and write are pointing to the same scope
        // This can happen if a scope is reading and writing to different mips of the same attachment
        if (&producer == &consumer)
        {
            return;
        }

        const GraphEdge graphEdge =
        {
            producer.m_graphNodeIndex.GetIndex(),
            consumer.m_graphNodeIndex.GetIndex()
        };

        auto findIter = AZStd::find_if(m_graphEdges.begin(), m_graphEdges.end(), [&graphEdge](const GraphEdge& element)
        {
            return element.m_consumerIndex == graphEdge.m_consumerIndex && element.m_producerIndex == graphEdge.m_producerIndex;
        });

        if (findIter == m_graphEdges.end())
        {
            m_graphEdges.push_back(graphEdge);

            GraphNode& consumerGraphNode = m_graphNodes[graphEdge.m_consumerIndex];
            consumerGraphNode.m_producers.push_back(&producer);
            consumerGraphNode.m_unsortedProducerCount++;

            GraphNode& producerGraphNode = m_graphNodes[graphEdge.m_producerIndex];
            producerGraphNode.m_consumers.push_back(&consumer);
        }
    }
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/BufferFrameAttachment.h>
#include <Atom/RHI/BufferPoolBase.h>
#include <Atom/RHI/BufferScopeAttachment.h>
#include <Atom/RHI/FrameGraph.h>
#include <Atom/RHI/ImageFrameAttachment.h>
#include <Atom/RHI/ImagePoolBase.h>
#include <Atom/RHI/ImageScopeAttachment.h>
#include <Atom/RHI/QueryPool.h>
#include <Atom/RHI/ResolveScopeAttachment.h>
#include <Atom/RHI/Scope.h>
#include <Atom/RHI/SwapChain.h>
#include <Atom/RHI/SwapChainFrameAttachment.h>
#include <AzCore/Debug/EventTrace.h>
#include <AzCore/std/sort.h>

namespace AZ
{
    namespace RHI
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
            AZ_TRACE_METHOD();

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
                    if (attachment->GetFirstScopeAttachment() == nullptr)
                    {
                        //We allow the rendering to continue even if an attachment is not used.
                        AZ_Error(
                            "FrameGraph", false,
                            "Invalid State: attachment '%s' was added but never used!",
                            attachment->GetId().GetCStr());
                    }
                }
            }

            return ResultCode::Success;
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
                if (auto* lastScope = attachment->GetLastScope())
                {
                    lastScope->m_swapChainsToPresent.push_back(attachment->GetSwapChain());
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

        void FrameGraph::UseAttachmentInternal(
            ImageFrameAttachment& frameAttachment,
            ScopeAttachmentUsage usage,
            ScopeAttachmentAccess access,
            const ImageScopeAttachmentDescriptor& descriptor)
        {
            AZ_Assert(usage != ScopeAttachmentUsage::Uninitialized, "ScopeAttachmentUsage is Uninitialized");

            //A scopeattachment can be used in multiple ways within the same scope. Hence, instead of adding duplicate scopeattachments
            //for a scope we add multiple usage/access related data within the same scopeattahcment.
            for (ImageScopeAttachment* imageScopeInnerAttachment : m_currentScope->m_imageAttachments)
            {
                if(imageScopeInnerAttachment->GetFrameAttachment().GetId() == frameAttachment.GetId())
                {
                    //Check if it is the same sub resource as for an imagescopeattachments we may want to read and write into different mips
                    //and in that case we would want multiple scopeattachments. 
                    if(imageScopeInnerAttachment->GetDescriptor().m_imageViewDescriptor.IsSameSubResource(descriptor.m_imageViewDescriptor))
                    {
                        AZ_Assert(imageScopeInnerAttachment->GetDescriptor().m_loadStoreAction == descriptor.m_loadStoreAction, "LoadStore actions for multiple usages need to match");
                        imageScopeInnerAttachment->AddUsageAndAccess(usage, access);
                        return;
                    }
                }
            }
            
            // TODO:[ATOM-1267] Replace with writer / reader dependencies.
            GraphEdgeType edgeType = usage == ScopeAttachmentUsage::SubpassInput ? GraphEdgeType::SameGroup : GraphEdgeType::DifferentGroup;
            if (Scope* producer = frameAttachment.GetLastScope())
            {
                InsertEdge(*producer, *m_currentScope, edgeType);
            }

            ImageScopeAttachment* scopeAttachment =
                m_attachmentDatabase.EmplaceScopeAttachment<ImageScopeAttachment>(
                    *m_currentScope, frameAttachment, usage, access, descriptor);

            
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
            if (Scope* producer = frameAttachment.GetLastScope())
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
            const BufferScopeAttachmentDescriptor& descriptor)
        {
            AZ_Assert(usage != ScopeAttachmentUsage::Uninitialized, "ScopeAttachmentUsage is Uninitialized");

            //A scopeattachment can be used in multiple ways within the same scope. Hence, instead of adding duplicate scopeattachments
            //for a scope we add multiple usage/access related data within the same scopeattahcment.
            for (BufferScopeAttachment* scopeInnerAttachment : m_currentScope->m_bufferAttachments)
            {
                if (scopeInnerAttachment->GetFrameAttachment().GetId() == frameAttachment.GetId())
                {
                    scopeInnerAttachment->AddUsageAndAccess(usage, access);
                    return;
                }
            }

            // TODO:[ATOM-1267] Replace with writer / reader dependencies.
            if (Scope* producer = frameAttachment.GetLastScope())
            {
                InsertEdge(*producer, *m_currentScope);
            }

            BufferScopeAttachment* scopeAttachment =
                m_attachmentDatabase.EmplaceScopeAttachment<BufferScopeAttachment>(
                    *m_currentScope, frameAttachment, usage, access, descriptor);

            m_currentScope->m_attachments.push_back(scopeAttachment);
            m_currentScope->m_bufferAttachments.push_back(scopeAttachment);
            if (frameAttachment.GetLifetimeType() == AttachmentLifetimeType::Transient)
            {
                m_currentScope->m_transientAttachments.push_back(scopeAttachment);
            }
        }

        ResultCode FrameGraph::UseAttachment(const BufferScopeAttachmentDescriptor& descriptor, ScopeAttachmentAccess access, ScopeAttachmentUsage usage)
        {
            BufferFrameAttachment* attachment = m_attachmentDatabase.FindAttachment<BufferFrameAttachment>(descriptor.m_attachmentId);
            if (attachment)
            {
                UseAttachmentInternal(*attachment, usage, access, descriptor);
                return ResultCode::Success;
            }

            AZ_Error("FrameGraph", false, "No compatible buffer attachment found for id: '%s'", descriptor.m_attachmentId.GetCStr());
            return ResultCode::InvalidArgument;
        }

        ResultCode FrameGraph::UseAttachment(const ImageScopeAttachmentDescriptor& descriptor, ScopeAttachmentAccess access, ScopeAttachmentUsage usage)
        {
            ImageFrameAttachment* attachment = m_attachmentDatabase.FindAttachment<ImageFrameAttachment>(descriptor.m_attachmentId);
            if (attachment)
            {
                UseAttachmentInternal(*attachment, usage, access, descriptor);
                return ResultCode::Success;
            }

            AZ_Error("FrameGraph", false, "No compatible image attachment found for id: '%s'", descriptor.m_attachmentId.GetCStr());
            return ResultCode::InvalidArgument;
        }

        ResultCode FrameGraph::UseAttachments(AZStd::array_view<ImageScopeAttachmentDescriptor> descriptors, ScopeAttachmentAccess access, ScopeAttachmentUsage usage)
        {
            for (const ImageScopeAttachmentDescriptor& descriptor : descriptors)
            {
                ResultCode resultCode = UseAttachment(descriptor, access, usage);
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

        ResultCode FrameGraph::UseColorAttachments(AZStd::array_view<ImageScopeAttachmentDescriptor> descriptors)
        {
            return UseAttachments(descriptors, ScopeAttachmentAccess::Write, ScopeAttachmentUsage::RenderTarget);
        }

        ResultCode FrameGraph::UseDepthStencilAttachment(const ImageScopeAttachmentDescriptor& descriptor, ScopeAttachmentAccess access)
        {
            return UseAttachment(descriptor, access, ScopeAttachmentUsage::DepthStencil);
        }

        ResultCode FrameGraph::UseSubpassInputAttachments(AZStd::array_view<ImageScopeAttachmentDescriptor> descriptors)
        {
            return UseAttachments(descriptors, ScopeAttachmentAccess::Read, ScopeAttachmentUsage::SubpassInput);
        }

        ResultCode FrameGraph::UseShaderAttachment(const BufferScopeAttachmentDescriptor& descriptor, ScopeAttachmentAccess access)
        {
            return UseAttachment(descriptor, access, ScopeAttachmentUsage::Shader);
        }

        ResultCode FrameGraph::UseShaderAttachment(const ImageScopeAttachmentDescriptor& descriptor, ScopeAttachmentAccess access)
        {
            return UseAttachment(descriptor, access, ScopeAttachmentUsage::Shader);
        }

        ResultCode FrameGraph::UseCopyAttachment(const BufferScopeAttachmentDescriptor& descriptor, ScopeAttachmentAccess access)
        {
            return UseAttachment(descriptor, access, ScopeAttachmentUsage::Copy);
        }

        ResultCode FrameGraph::UseCopyAttachment(const ImageScopeAttachmentDescriptor& descriptor, ScopeAttachmentAccess access)
        {
            return UseAttachment(descriptor, access, ScopeAttachmentUsage::Copy);
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
            AZStd::vector<AZStd::list<uint32_t>> graphEdges(m_graphNodes.size());
            for (uint32_t edgeIndex = 0; edgeIndex < m_graphEdges.size(); ++edgeIndex)
            {
                const GraphEdge& edge = m_graphEdges[edgeIndex];
                AZStd::list<uint32_t>& edgeList = graphEdges[edge.m_producerIndex];
                // Push group edges at the front so they are processed before the single ones.
                // We need this so nodes in the same group are together.
                switch (edge.m_type)
                {
                case GraphEdgeType::DifferentGroup:
                    edgeList.push_back(edgeIndex);
                    break;
                case GraphEdgeType::SameGroup:
                    edgeList.push_front(edgeIndex);
                    break;
                default:
                    AZ_Assert(false, "Invalid edge type %d", edge.m_type);
                    break;
                }
            }

            uint16_t groupCount = 0;
            // This loop will add all unblocked nodes, i.e. nodes that don't have any producers. This
            // includes the root node.
            for (size_t nodeIndex = 0; nodeIndex < m_graphNodes.size(); ++nodeIndex)
            {
                const GraphNode& graphNode = m_graphNodes[nodeIndex];
                if (graphNode.m_unsortedProducerCount == 0)
                {
                    unblockedNodes.push_back({ static_cast<uint16_t>(nodeIndex), groupCount++ });
                }
            }

            // Process nodes that don't have any producers left (they have already been processed).
            // They get added to the unblockedNodes vector in a topological manner.
            while (!unblockedNodes.empty())
            {
                const NodeId& producerNodeId = unblockedNodes.back();
                const uint16_t producerIndex = producerNodeId.m_nodeIndex;
                const uint16_t producerGroupId = producerNodeId.m_groupId;
                unblockedNodes.pop_back();

                const uint32_t scopeIndexNext = aznumeric_caster(m_scopes.size());

                Scope* scope = m_graphNodes[producerIndex].m_scope;
                // Activate the scope in topological order.
                scope->Activate(this, scopeIndexNext, GraphGroupId(producerGroupId));
                m_scopes.push_back(scope);

                // Go through all the edges of this node, find the consumer nodes that are fully sorted and add them to the unblockedNodes.
                for (const uint32_t edgeIndex : graphEdges[producerIndex])
                {
                    const GraphEdge& graphEdge = m_graphEdges[edgeIndex];
                    const uint16_t consumerIndex = static_cast<uint16_t>(graphEdge.m_consumerIndex);
                    if (--m_graphNodes[consumerIndex].m_unsortedProducerCount == 0)
                    {
                        NodeId newNode;
                        newNode.m_nodeIndex = consumerIndex;
                        newNode.m_groupId = graphEdge.m_type == GraphEdgeType::SameGroup ? producerGroupId : groupCount++;
                        unblockedNodes.emplace_back(newNode);
                    }
                }
                graphEdges[producerIndex].clear();
            }

            if (m_graphNodes.size() == m_scopes.size())
            {
                return ResultCode::Success;
            }

            if (Validation::IsEnabled())
            {
                AZStd::string cycleInfoString = "Error, a cycle exists in the graph. Failed to topologically sort. Remaining Edges:\n";
                for (const AZStd::list<uint32_t>& edgeList : graphEdges)
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

        void FrameGraph::InsertEdge(Scope& producer, Scope& consumer, GraphEdgeType edgeType /*= GraphEdgeType::DifferentGroup*/)
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
                consumer.m_graphNodeIndex.GetIndex(),
                edgeType
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
            else
            {
                // Update the edge type if needed.
                GraphEdge& edge = *findIter;
                edge.m_type = edgeType == GraphEdgeType::SameGroup ? edgeType : edge.m_type;
            }
        }
    }
}

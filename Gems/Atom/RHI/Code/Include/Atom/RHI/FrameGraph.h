/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/span.h>

#include <Atom/RHI.Reflect/BufferScopeAttachmentDescriptor.h>
#include <Atom/RHI.Reflect/ImageScopeAttachmentDescriptor.h>
#include <Atom/RHI.Reflect/ResolveScopeAttachmentDescriptor.h>
#include <Atom/RHI.Reflect/ScopeId.h>

#include <Atom/RHI/DeviceBufferPoolBase.h>
#include <Atom/RHI/FrameGraphAttachmentDatabase.h>
#include <Atom/RHI/Scope.h>

namespace AZ::RHI
{
    class ImageFrameAttachment;
    class SwapChainFrameAttachment;
    class BufferFrameAttachment;
    class ImageScopeAttachment;
    class BufferScopeAttachment;
    class ResolveScopeAttachment;
    class QueryPool;
    class DeviceFence;
    struct Interval;

    //! The frame graph is a graph of scopes, where edges are derived from attachment usage. It can be visualized as a sparse 2D grid.
    //! The first axis is a flat array of all attachments, and the second axis is a flat array of all the scopes in dependency-sorted order.
    //! A scope attachment exists when a frame graph attachment is attached to a specific scope. As a result, each frame graph attachment
    //! builds a doubly linked list of scope attachments, where the head of the list is the first scope attachment on the first scope, and
    //! the tail is the last scope attachment on the last scope. It's possible then to derive lifetimes of each attachment by inspecting the
    //! head and tail of this list, or to traverse the "usage" chain by walking the linked list.
    //!
    //! EXAMPLE:
    //!
    //! [Legend] ATTACHMENTS: Uppercase Letters. SCOPES: Numbers.
    //!
    //!                 0            1           2           3           4            5           6
    //! A           [Color   ->   ImageRead]
    //! B                        [BufferWrite      ->     BufferRead]
    //! C           [DepthWrite-> DepthRead-> DepthRead-> DepthRead-> DepthRead]
    //! D                                    [Color   ->  ImageRead]
    //! E                                                            [ImageWrite-> ImageRead-> ImageRead]
    //!
    //! Lifetimes:
    //! A: [0, 1]
    //! B: [1, 3]
    //! C: [0, 4]
    //! D: [2, 3]
    //! E: [4, 6]
    //!
    //! In this example, (A-E) are frame graph attachments, and (0-6) are scopes. The
    //! entries in the grid are scope attachments where a particular frame graph attachment has been attached to a scope with
    //! a specific usage.
    //!
    //! The graph allows you to walk the sparse grid as a linked list:
    //!     1) You can traverse each "usage" of an attachment, from the first to the last scope (left / right).
    //!     2) You can traverse the list of attachments in a scope (up / down).
    //!     3) You can traverse the list of attachments matching the same type in a scope (up / down).
    class FrameGraph final
    {
        friend class FrameGraphCompiler;
    public:
        AZ_CLASS_ALLOCATOR(FrameGraph, AZ::SystemAllocator);

        FrameGraph() = default;
        ~FrameGraph();

        //! Returns whether the graph has been compiled.
        bool IsCompiled() const;

        //! Returns whether the graph is currently being built.
        bool IsBuilding() const;

        //! Returns number of frame cycles completed since initialization.
        size_t GetFrameCount() const;

        //////////////////////////////////////////////////////////////////////////
        // The following methods are for constructing the graph.

        //! Begins the building phase.
        void Begin();

        //! Begins building a new scope instance associated with scope. The FrameGraphInterface
        //! API is used to construct the scope, and is only valid within the context of these
        //! calls.
        void BeginScope(Scope& scope);

        // See RHI::FrameGraphInterface for detailed comments
        ResultCode UseAttachment(
            const BufferScopeAttachmentDescriptor& descriptor,
            ScopeAttachmentAccess access,
            ScopeAttachmentUsage usage,
            ScopeAttachmentStage stage);
        ResultCode UseAttachment(
            const ImageScopeAttachmentDescriptor& descriptor,
            ScopeAttachmentAccess access,
            ScopeAttachmentUsage usage,
            ScopeAttachmentStage stage);
        ResultCode UseAttachments(
            AZStd::span<const ImageScopeAttachmentDescriptor> descriptors,
            ScopeAttachmentAccess access,
            ScopeAttachmentUsage usage,
            ScopeAttachmentStage stage);
        ResultCode UseResolveAttachment(const ResolveScopeAttachmentDescriptor& descriptor);
        ResultCode UseColorAttachments(AZStd::span<const ImageScopeAttachmentDescriptor> descriptors);
        ResultCode UseDepthStencilAttachment(
            const ImageScopeAttachmentDescriptor& descriptor,
            ScopeAttachmentAccess access,
            ScopeAttachmentStage stage);
        ResultCode UseSubpassInputAttachments(
            AZStd::span<const ImageScopeAttachmentDescriptor> descriptors,
            ScopeAttachmentStage stage);
        ResultCode UseShaderAttachment(
            const BufferScopeAttachmentDescriptor& descriptor,
            ScopeAttachmentAccess access,
            ScopeAttachmentStage stage);
        ResultCode UseShaderAttachment(
            const ImageScopeAttachmentDescriptor& descriptor,
            ScopeAttachmentAccess access,
            ScopeAttachmentStage stage);
        ResultCode UseCopyAttachment(const BufferScopeAttachmentDescriptor& descriptor, ScopeAttachmentAccess access);
        ResultCode UseCopyAttachment(const ImageScopeAttachmentDescriptor& descriptor, ScopeAttachmentAccess access);
        ResultCode UseQueryPool(Ptr<QueryPool> queryPool, const RHI::Interval& interval, QueryPoolScopeAttachmentType type, ScopeAttachmentAccess access);
        void ExecuteAfter(const ScopeId& scopeId);
        void ExecuteBefore(const ScopeId& scopeId);
        void SignalFence(Fence& fence);
        void WaitFence(Fence& fence);
        void SetEstimatedItemCount(uint32_t itemCount);
        void SetHardwareQueueClass(HardwareQueueClass hardwareQueueClass);
        void SetGroupId(const ScopeGroupId& groupId);

        //! Declares a single color attachment for use on the current scope.             
        ResultCode UseColorAttachment(const ImageScopeAttachmentDescriptor& descriptor)
        {
            return UseColorAttachments({ &descriptor, 1 });
        }

        //! Declares a single subpass input attachment for use on the current scope.
        //! Subpass input attachments are image views that can be used for pixel local load operations inside a fragment shader.
        //! This means that framebuffer attachments written in one subpass can be read from at the exact same pixel
        //! in subsequent subpasses. Certain platform have optimization for this type of attachments.             
        ResultCode UseSubpassInputAttachment(const ImageScopeAttachmentDescriptor& descriptor, ScopeAttachmentStage stage)
        {
            return UseSubpassInputAttachments({ &descriptor, 1 }, stage);
        }

        //! Ends building of the current scope.
        void EndScope();

        //! Ends the building phase of the graph.
        ResultCode End();

        //! Clears the graph to an empty state.
        void Clear();

        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // The following methods are for querying the graph.

        /// Returns the attachment database.
        const FrameGraphAttachmentDatabase& GetAttachmentDatabase() const;
        FrameGraphAttachmentDatabase& GetAttachmentDatabase();

        /// Returns the root scope (which is always the first in the list).
        Scope* GetRootScope() const;

        /// Returns the topologically sorted list of scopes.
        const AZStd::vector<Scope*>& GetScopes() const;

        /// Returns the list of consumers for the provided producer scope.
        const AZStd::vector<Scope*>& GetConsumers(const Scope& producer) const;

        /// Returns the list of producers for the provided consumer scope.
        const AZStd::vector<Scope*>& GetProducers(const Scope& consumer) const;

        /// Returns the scope associated with \param scopeId
        Scope* FindScope(const ScopeId& scopeId);
        const Scope* FindScope(const ScopeId& scopeId) const;

        //////////////////////////////////////////////////////////////////////////

    private:
        /// Validates the graph at the end of the building phase.
        ResultCode ValidateEnd();

        /// Validates an attachment before adding it.
        template<class T>
        void ValidateAttachment(
            const T& attachmentDescriptor, ScopeAttachmentUsage usage, ScopeAttachmentAccess access) const;

        /// Validates that an overlapping attachment has the proper access and usage before adding it.
        void ValidateOverlappingAttachment(
            AttachmentId attachmentId,
            ScopeAttachmentUsage usage,
            ScopeAttachmentAccess access,
            const ScopeAttachment& scopeAttachment) const;

        /// Called by the FrameGraphCompiler to mark the graph as compiled.
        void SetCompiled();

        void UseAttachmentInternal(
            ImageFrameAttachment& frameAttachment,
            ScopeAttachmentUsage usage,
            ScopeAttachmentAccess access,
            ScopeAttachmentStage stage,
            const ImageScopeAttachmentDescriptor& descriptor);

        void UseAttachmentInternal(
            ImageFrameAttachment& frameAttachment,
            const ResolveScopeAttachmentDescriptor& descriptor);

        void UseAttachmentInternal(
            BufferFrameAttachment& frameAttachment,
            ScopeAttachmentUsage usage,
            ScopeAttachmentAccess access,
            ScopeAttachmentStage stage,
            const BufferScopeAttachmentDescriptor& descriptor);

        ResultCode TopologicalSort();            
            
        void InsertEdge(Scope& producer, Scope& consumer);

        struct GraphEdge
        {
            bool operator== (const GraphEdge& rhs) const
            {
                return m_producerIndex == rhs.m_producerIndex && m_consumerIndex == rhs.m_consumerIndex;
            }

            uint32_t m_producerIndex;
            uint32_t m_consumerIndex;
        };

        struct GraphNode
        {
            GraphNode(Scope& scope)
                : m_scope{&scope}
            {}

            Scope* m_scope = nullptr;
            AZStd::vector<Scope*> m_producers;
            AZStd::vector<Scope*> m_consumers;
            uint16_t m_unsortedProducerCount = 0;
            ScopeGroupId m_scopeGroupId;
        };

        FrameGraphAttachmentDatabase m_attachmentDatabase;
        AZStd::vector<GraphEdge> m_graphEdges;
        AZStd::vector<GraphNode> m_graphNodes;
        AZStd::vector<Scope*> m_scopes;
        AZStd::unordered_map<ScopeId, Scope*> m_scopeLookup;
        Scope* m_currentScope = nullptr;
        bool m_isCompiled = false;
        bool m_isBuilding = false;
        size_t m_frameCount = 0;
    };

    template<class T>
    void FrameGraph::ValidateAttachment(const T& attachmentDescriptor, ScopeAttachmentUsage usage, ScopeAttachmentAccess access) const
    {
        const ScopeAttachmentPtrList* scopeAttachmentList =
            m_attachmentDatabase.FindScopeAttachmentList(m_currentScope->GetId(), attachmentDescriptor.m_attachmentId);
        if (scopeAttachmentList)
        {
            for (const ScopeAttachment* attachment : *scopeAttachmentList)
            {
                if (attachmentDescriptor.GetViewDescriptor().OverlapsSubResource(
                            static_cast<const T&>(attachment->GetScopeAttachmentDescriptor()).GetViewDescriptor()))
                {
                    ValidateOverlappingAttachment(attachmentDescriptor.m_attachmentId, usage, access, *attachment);
                }
            }
        }
    }
} // namespace AZ::RHI

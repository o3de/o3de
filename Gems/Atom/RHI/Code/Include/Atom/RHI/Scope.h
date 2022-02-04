/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ScopeId.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/Handle.h>
#include <Atom/RHI/ResourcePool.h>
#include <Atom/RHI/QueryPool.h>
#include <Atom/RHI/Fence.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/array.h>

namespace AZ
{
    namespace RHI
    {
        class SwapChain;
        class ScopeAttachment;
        class ImageScopeAttachment;
        class BufferScopeAttachment;
        class ResolveScopeAttachment;
        class BufferPoolBase;
        class ImagePoolBase;
        class ResourcePoolDatabase;
        class FrameGraph;
        class Device;
        class QueryPool;

        using GraphGroupId = Handle<uint32_t>;

        /**
         * A base class for a scope in the current frame. The user is expected to derive from Scope
         * and supply platform-specific scope data. All platform specific data should be built in \ref
         * CompileInternal. At that time, the client will have access to the attachment database, which
         * it can use to compile flat arrays of platform-specific state (fences, barriers, clears, etc).
         */
        class Scope
            : public Object
        {
            friend class FrameGraph;
            friend class FrameGraphCompiler;
        public:
            AZ_RTTI(Scope, "{C9EB500A-EF31-46E2-98DE-62396CDBAFB1}", Object);

            Scope() = default;
            virtual ~Scope() = default;

            /// Returns whether the scope is currently initialized.
            bool IsInitialized() const;

            /// Returns whether the scope is currently active on a frame.
            bool IsActive() const;

            /// Returns the scope id associated with this scope.
            const ScopeId& GetId() const;

            /**
             * Returns the index in the array of scopes in FrameSchedulerBase::GetScopes. The indices
             * are dependency ordered, so a scope with a greater index *may* depend on a scope from a lesser
             * index. Scopes are often independent and even run in parallel (e.g. async compute / copy).
             */
            uint32_t GetIndex() const;

            //! Returns the id of the graph group this scope belongs.
            const GraphGroupId& GetFrameGraphGroupId() const;

            /// Returns the frame graph instance which owns this scope.
            const FrameGraph* GetFrameGraph() const;

            /// Returns the hardware queue class for this scope.
            HardwareQueueClass GetHardwareQueueClass() const;

            /// Sets the hardware queue class for this scope.
            void SetHardwareQueueClass(HardwareQueueClass hardwareQueueClass);

            /**
             * Returns the estimated number of draw / dispatch / copy items that the user will submit
             * while in this scope. This is an estimation intended to be used by the platform-specific
             * load-balancer in the frame scheduler.
             */
            uint32_t GetEstimatedItemCount() const;

            /// Returns the scope for the given hardware queue which must be scheduled immediately prior to this scope (can be null).
            Scope* GetProducerByQueue(HardwareQueueClass hardwareQueueClass) const;

            /// Returns the scope for the producer on the same hardware queue as us.
            Scope* GetProducerOnSameQueue() const;

            /// Returns the scope for the given hardware queue which must be scheduled immediately after this scope (can be null).
            Scope* GetConsumerByQueue(HardwareQueueClass hardwareQueueClass) const;

            /// Returns the scope for the consumer on the same hardware queue as us.
            Scope* GetConsumerOnSameQueue() const;

            /// Returns a list of attachments on this scope.
            const AZStd::vector<ScopeAttachment*>& GetAttachments() const;

            /// Returns a list of attachments which reference transient resources on this scope.
            const AZStd::vector<ScopeAttachment*>& GetTransientAttachments() const;

            /// Returns a list of all image scope attachments.
            const AZStd::vector<ImageScopeAttachment*>& GetImageAttachments() const;

            /// Returns a list of all resolve scope attachments.
            const AZStd::vector<ResolveScopeAttachment*>& GetResolveAttachments() const;

            /// Returns a list of all buffer scope attachments.
            const AZStd::vector<BufferScopeAttachment*>& GetBufferAttachments() const;

            /// Returns a list of resource pools requiring a resolve operation.
            const AZStd::vector<ResourcePoolResolver*>& GetResourcePoolResolves() const;

            /// Returns a list of swap chains which require presentation at the end of the scope.
            const AZStd::vector<SwapChain*>& GetSwapChainsToPresent() const;

            /// Returns a list of fences to signal on completion of the scope.
            const AZStd::vector<Ptr<Fence>>& GetFencesToSignal() const;

            /// Initializes the scope.
            void Init(const ScopeId& scopeId, HardwareQueueClass hardwareQueueClass = HardwareQueueClass::Graphics);

            /// Activates the scope for the current frame.
            void Activate(const FrameGraph* frameGraph, uint32_t index, const GraphGroupId& groupId);

            /// Called when the scope is being compiled at the end of the graph-building phase.
            void Compile(Device& device);

            /// Deactivates the scope for the current frame.
            void Deactivate();

            /// Shuts down the scope.
            void Shutdown() override final;

            /**
             * Queues resource pool resolves for queued upload operations from the resource pool database. operation
             * will pull all of the resource pool resolvers from the database and queue them onto this scope. This
             * should only occur once in the frame on the root scope.
             */
            void QueueResourcePoolResolves(ResourcePoolDatabase& resourcePoolDatabase);

            /// Finds a producer for this scope that is at least as capable as the provided queue class.
            Scope* FindCapableCrossQueueProducer(HardwareQueueClass hardwareQueueClass);

            /// Finds a producer for this scope from a more capable queue.
            Scope* FindMoreCapableCrossQueueProducer();

            /// Finds a producer for this scope from a specific queue class.
            Scope* FindCrossQueueProducer(HardwareQueueClass hardwareQueueClass);

            /// Links the producer and consumer according to their queues.
            static void LinkProducerConsumerByQueues(Scope* producer, Scope* consumer);

            /// Adds a fence that will be signaled at the end of the scope.
            void AddFenceToSignal(Ptr<Fence> fence);

        protected:
            /// Called when the scope will use a query pool during it's execution. Some platforms need this information.
            virtual void AddQueryPoolUse(Ptr<QueryPool> queryPool, const RHI::Interval& interval, RHI::ScopeAttachmentAccess access);

        private:
            //////////////////////////////////////////////////////////////////////////
            // Platform API

            /// Called when the scope is initializing.
            virtual void InitInternal();

            /// Called when the scope is activating on at the beginning of the frame (before building).
            virtual void ActivateInternal();

            /// Called when the scope is being compiled into platform-dependent actions (after graph compilation).
            virtual void CompileInternal(Device& device);

            /// Called when the scope is deactivating at the end of the frame (after execution).
            virtual void DeactivateInternal();

            /// Called when the scope is shutting down.
            virtual void ShutdownInternal();

            //////////////////////////////////////////////////////////////////////////

            ScopeId m_id;

            /// The sorted index is exposed via GetIndex, and maps to the topologically sorted scope list.
            Handle<uint32_t> m_index;

            /// [Internal The unsorted index maps to the intermediate (pre-compiled) graph
            /// node metadata internal to the frame graph.
            Handle<uint32_t> m_graphNodeIndex;

            /// The id of the graph group the scope belongs.
            GraphGroupId m_graphGroupId;

            /// A pointer to the parent frame graph instance.
            const FrameGraph* m_frameGraph = nullptr;

            /// A load balancing factor for command list splitting (platform dependent).
            uint32_t m_estimatedItemCount = 0;

            /// The hardware queue class that this scope is requested to execute on.
            HardwareQueueClass m_hardwareQueueClass = HardwareQueueClass::Graphics;

            /// Tracks whether the scope is initialized, which must occur before activation.
            bool m_isInitialized = false;

            /// Tracks whether the scope is active, which happens once per frame.
            bool m_isActive = false;

            /// The cross-queue producers / consumers, indexed by hardware queue.
            AZStd::array<Scope*, HardwareQueueClassCount> m_producersByQueueLast = {{nullptr}};
            AZStd::array<Scope*, HardwareQueueClassCount> m_producersByQueue     = {{nullptr}};
            AZStd::array<Scope*, HardwareQueueClassCount> m_consumersByQueue     = {{nullptr}};

            /// The union set of of all attachments queued.
            AZStd::vector<ScopeAttachment*>          m_attachments;

            /// The union set of buffer / image transient attachments queued.
            AZStd::vector<ScopeAttachment*>          m_transientAttachments;

            /// The set of image transient attachments queued.
            AZStd::vector<ImageScopeAttachment*>     m_imageAttachments;

            /// The set of resolve image attachments queued.
            AZStd::vector<ResolveScopeAttachment*>   m_resolveAttachments;

            /// The set of buffer transient attachments queued.
            AZStd::vector<BufferScopeAttachment*>    m_bufferAttachments;

            /// The set of pool resolve actions requested for this scope.
            AZStd::vector<ResourcePoolResolver*>     m_resourcePoolResolves;

            /// The set of swap chain present actions requested.
            AZStd::vector<SwapChain*>                m_swapChainsToPresent;

            /// The set of fences to signal on scope completion.
            AZStd::vector<Ptr<Fence>>                m_fencesToSignal;

            /// The set query pools.
            AZStd::vector<Ptr<QueryPool>>                m_queryPools;
        };
    }
}

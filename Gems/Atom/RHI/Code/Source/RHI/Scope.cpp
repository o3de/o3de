/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/Scope.h>
#include <Atom/RHI/ResourcePoolDatabase.h>
#include <Atom/RHI/ImagePoolBase.h>
#include <Atom/RHI/BufferPoolBase.h>

#include <AzCore/Debug/EventTrace.h>

namespace AZ
{
    namespace RHI
    {
        bool Scope::IsInitialized() const
        {
            return m_isInitialized;
        }

        bool Scope::IsActive() const
        {
            return m_isActive;
        }

        void Scope::Init(const ScopeId& scopeId)
        {
            AZ_Assert(!scopeId.IsEmpty(), "Scope id is not valid.");
            AZ_Assert(IsInitialized() == false, "Scope was previously initialized.");
            SetName(scopeId);
            m_id = scopeId;
            InitInternal();
            m_isInitialized = true;
        }

        void Scope::Activate(const FrameGraph* frameGraph, uint32_t index, const GraphGroupId& groupId)
        {
            AZ_Assert(m_isActive == false, "Scope was previously active.");
            m_index = decltype(m_index)(index);
            m_frameGraph = frameGraph;
            m_graphGroupId = groupId;
            ActivateInternal();
            m_isActive = true;
        }

        void Scope::Compile(Device& device)
        {
            AZ_Assert(m_isActive, "Scope being compiled but is not active");
            CompileInternal(device);
        }

        void Scope::Deactivate()
        {
            AZ_Assert(m_isActive, "Scope is not active.");
            DeactivateInternal();
            m_isActive = false;
            m_frameGraph = nullptr;
            m_index.Reset();
            m_graphNodeIndex.Reset();
            m_estimatedItemCount = 0;
            m_hardwareQueueClass = HardwareQueueClass::Graphics;
            m_producersByQueueLast.fill(nullptr);
            m_producersByQueue.fill(nullptr);
            m_consumersByQueue.fill(nullptr);
            m_attachments.clear();
            m_transientAttachments.clear();
            m_imageAttachments.clear();
            m_resolveAttachments.clear();
            m_bufferAttachments.clear();
            m_swapChainsToPresent.clear();
            m_fencesToSignal.clear();
            m_resourcePoolResolves.clear();
            m_queryPools.clear();
        }

        void Scope::Shutdown()
        {
            AZ_Assert(m_isActive == false, "Scope is currently active.");

            /// Multiple shutdown calls are valid behavior with Object.
            if (IsInitialized())
            {
                ShutdownInternal();
                m_isInitialized = false;
            }
        }

        void Scope::QueueResourcePoolResolves(ResourcePoolDatabase& resourcePoolDatabase)
        {
            AZ_TRACE_METHOD();

            const auto queuePoolResolverFunction = [this](ResourcePoolResolver* poolResolver)
            {
                m_resourcePoolResolves.emplace_back(poolResolver);
            };

            resourcePoolDatabase.ForEachPoolResolver<decltype(queuePoolResolverFunction)>(queuePoolResolverFunction);
        }

        void Scope::AddQueryPoolUse(Ptr<QueryPool> queryPool, [[maybe_unused]] const RHI::Interval& interval, [[maybe_unused]] RHI::ScopeAttachmentAccess access)
        {
            m_queryPools.push_back(queryPool);
        }

        void Scope::InitInternal() {}
        void Scope::ActivateInternal() {}
        void Scope::CompileInternal([[maybe_unused]] Device& device) {}
        void Scope::DeactivateInternal() {}
        void Scope::ShutdownInternal() {}

        const ScopeId& Scope::GetId() const
        {
            return m_id;
        }

        uint32_t Scope::GetIndex() const
        {
            return m_index.GetIndex();
        }

        const GraphGroupId& Scope::GetFrameGraphGroupId() const
        {
            return m_graphGroupId;
        }

        const FrameGraph* Scope::GetFrameGraph() const
        {
            return m_frameGraph;
        }

        HardwareQueueClass Scope::GetHardwareQueueClass() const
        {
            return m_hardwareQueueClass;
        }

        uint32_t Scope::GetEstimatedItemCount() const
        {
            return m_estimatedItemCount;
        }

        const AZStd::vector<ScopeAttachment*>& Scope::GetAttachments() const
        {
            return m_attachments;
        }

        const AZStd::vector<ScopeAttachment*>& Scope::GetTransientAttachments() const
        {
            return m_transientAttachments;
        }

        const AZStd::vector<ImageScopeAttachment*>& Scope::GetImageAttachments() const
        {
            return m_imageAttachments;
        }

        const AZStd::vector<ResolveScopeAttachment*>& Scope::GetResolveAttachments() const
        {
            return m_resolveAttachments;
        }

        const AZStd::vector<BufferScopeAttachment*>& Scope::GetBufferAttachments() const
        {
            return m_bufferAttachments;
        }

        const AZStd::vector<ResourcePoolResolver*>& Scope::GetResourcePoolResolves() const
        {
            return m_resourcePoolResolves;
        }

        const AZStd::vector<SwapChain*>& Scope::GetSwapChainsToPresent() const
        {
            return m_swapChainsToPresent;
        }

        const AZStd::vector<Ptr<Fence>>& Scope::GetFencesToSignal() const
        {
            return m_fencesToSignal;
        }

        Scope* Scope::GetProducerByQueue(HardwareQueueClass hardwareQueueClass) const
        {
            return m_producersByQueue[static_cast<size_t>(hardwareQueueClass)];
        }

        Scope* Scope::GetProducerOnSameQueue() const
        {
            return GetProducerByQueue(GetHardwareQueueClass());
        }

        Scope* Scope::GetConsumerByQueue(HardwareQueueClass hardwareQueueClass) const
        {
            return m_consumersByQueue[static_cast<size_t>(hardwareQueueClass)];
        }

        Scope* Scope::GetConsumerOnSameQueue() const
        {
            return GetConsumerByQueue(GetHardwareQueueClass());
        }

        void Scope::LinkProducerConsumerByQueues(Scope* producer, Scope* consumer)
        {
            /// Mark consumer as the consumer for the queue it lives on.
            producer->m_consumersByQueue[static_cast<uint32_t>(consumer->GetHardwareQueueClass())] = consumer;

            /// Mark us as the producer for our queue on consumer.
            consumer->m_producersByQueue[static_cast<uint32_t>(producer->GetHardwareQueueClass())] = producer;
        }

        void Scope::AddFenceToSignal(Ptr<Fence> fence)
        {
            m_fencesToSignal.push_back(fence);
        }

        Scope* Scope::FindMoreCapableCrossQueueProducer()
        {
            Scope* searchScope = this;
            do
            {
                switch (GetHardwareQueueClass())
                {
                case HardwareQueueClass::Copy:
                    if (Scope* foundProducer = searchScope->GetProducerByQueue(HardwareQueueClass::Compute))
                    {
                        return foundProducer;
                    }
                    // Fall through

                case HardwareQueueClass::Compute:
                    if (Scope* foundProducer = searchScope->GetProducerByQueue(HardwareQueueClass::Graphics))
                    {
                        return foundProducer;
                    }
                }

                searchScope = searchScope->GetProducerOnSameQueue();

            } while (searchScope);

            return nullptr;
        }

        Scope* Scope::FindCrossQueueProducer(HardwareQueueClass hardwareQueueClass)
        {
            Scope* searchScope = this;
            do
            {
                if (Scope* foundProducer = searchScope->GetProducerByQueue(hardwareQueueClass))
                {
                    return foundProducer;
                }

                searchScope = searchScope->GetProducerOnSameQueue();

            } while (searchScope);

            return nullptr;
        }

        Scope* Scope::FindCapableCrossQueueProducer(HardwareQueueClass hardwareQueueClass)
        {
            Scope* searchScope = this;
            do
            {
                switch (hardwareQueueClass)
                {
                case HardwareQueueClass::Copy:
                    if (Scope* foundProducer = searchScope->GetProducerByQueue(HardwareQueueClass::Copy))
                    {
                        return foundProducer;
                    }
                    // Fall through

                case HardwareQueueClass::Compute:
                    if (Scope* foundProducer = searchScope->GetProducerByQueue(HardwareQueueClass::Compute))
                    {
                        return foundProducer;
                    }
                    // Fall through

                case HardwareQueueClass::Graphics:
                    if (Scope* foundProducer = searchScope->GetProducerByQueue(HardwareQueueClass::Graphics))
                    {
                        return foundProducer;
                    }
                }

                searchScope = searchScope->GetProducerOnSameQueue();

            } while (searchScope);

            return nullptr;
        }
    }
}

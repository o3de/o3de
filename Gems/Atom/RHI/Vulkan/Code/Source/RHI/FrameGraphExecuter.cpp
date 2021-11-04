/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/FrameGraph.h>
#include <AzCore/std/parallel/thread.h>
#include <RHI/FrameGraphExecuteGroupHandler.h>
#include <RHI/FrameGraphExecuteGroupMergedHandler.h>
#include <RHI/FrameGraphExecuteGroup.h>
#include <RHI/FrameGraphExecuteGroupMerged.h>
#include <RHI/FrameGraphExecuter.h>
#include <RHI/Device.h>
#include <RHI/SwapChain.h>
#include <RHI/Scope.h>
#include <RHI/CommandQueueContext.h>


namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<FrameGraphExecuter> FrameGraphExecuter::Create()
        {
            return aznew FrameGraphExecuter();
        }

        Device& FrameGraphExecuter::GetDevice() const
        {
            return static_cast<Device&>(Base::GetDevice());
        }
        
        FrameGraphExecuter::FrameGraphExecuter()
        {
            SetJobPolicy(RHI::JobPolicy::Parallel);
        }
        
        RHI::ResultCode FrameGraphExecuter::InitInternal(const RHI::FrameGraphExecuterDescriptor& descriptor)
        {
            const RHI::ConstPtr<RHI::PlatformLimitsDescriptor> rhiPlatformLimitsDescriptor = descriptor.m_platformLimitsDescriptor;
            if (RHI::ConstPtr<PlatformLimitsDescriptor> vkPlatformLimitsDesc = azrtti_cast<const PlatformLimitsDescriptor*>(rhiPlatformLimitsDescriptor))
            {
                m_frameGraphExecuterData = vkPlatformLimitsDesc->m_frameGraphExecuterData;
            }
            return RHI::ResultCode::Success;
        }

        void FrameGraphExecuter::ShutdownInternal()
        {
            // do nothing
        }

        void FrameGraphExecuter::BeginInternal(const RHI::FrameGraph& frameGraph)
        {
            Device& device = GetDevice();

            RHI::HardwareQueueClass mergedHardwareQueueClass = RHI::HardwareQueueClass::Graphics;
            uint32_t mergedGroupCost = 0;
            uint32_t mergedSwapchainCount = 0;
            AZStd::vector<const Scope*> mergedScopes;

            const Scope* scopePrev = nullptr;
            const Scope* scopeNext = nullptr;
            const AZStd::vector<RHI::Scope*>& scopes = frameGraph.GetScopes();
            for (auto it = scopes.begin(); it != scopes.end(); ++it)
            {
                const Scope& scope = *static_cast<const Scope*>(*it);
                auto nextIter = it + 1;
                scopeNext = nextIter != scopes.end() ? static_cast<const Scope*>(*nextIter) : nullptr;

                // Reset merged hardware queue class to match current scope if empty.
                if (mergedGroupCost == 0)
                {
                    mergedHardwareQueueClass = scope.GetHardwareQueueClass();
                }

                const uint32_t estimatedItemCount = scope.GetEstimatedItemCount();

                const uint32_t CommandListCostThreshold =
                    AZStd::max(
                        m_frameGraphExecuterData.m_commandListCostThresholdMin,
                        RHI::DivideByMultiple(estimatedItemCount, m_frameGraphExecuterData.m_commandListsPerScopeMax));

                /**
                    * Computes a cost heuristic based on the number of items and number of attachments in
                    * the scope. This cost is used to partition command list generation.
                    */
                const uint32_t totalScopeCost =
                    estimatedItemCount * m_frameGraphExecuterData.m_itemCost +
                    static_cast<uint32_t>(scope.GetAttachments().size()) * m_frameGraphExecuterData.m_attachmentCost;

                // Check if we are in a middle of a framegraph group.
                const bool subpassGroup =
                    (scopeNext && scopeNext->GetFrameGraphGroupId() == scope.GetFrameGraphGroupId()) ||
                    (scopePrev && scopePrev->GetFrameGraphGroupId() == scope.GetFrameGraphGroupId());

                const uint32_t swapchainCount = static_cast<uint32_t>(scope.GetSwapChainsToPresent().size());

                // Detect if we are able to continue merging.
                {
                    // Check if the group fits into the current running merge queue. If not, we have to flush the queue.
                    const bool exceededCommandCost = (mergedGroupCost + totalScopeCost) > CommandListCostThreshold;

                    // Check if the swap chains fit into this group.
                    const bool exceededSwapChainLimit = (mergedSwapchainCount + swapchainCount) > m_frameGraphExecuterData.m_swapChainsPerCommandList;

                    // Check if the hardware queue classes match.
                    const bool hardwareQueueMismatch = scope.GetHardwareQueueClass() != mergedHardwareQueueClass;

                    // Check if we are straddling the boundary of a fence/semaphore.
                    const bool onSyncBoundaries = !scope.GetWaitSemaphores().empty() || (scopePrev && (!scopePrev->GetSignalSemaphores().empty() || !scopePrev->GetSignalFences().empty()));

                    // If we exceeded limits, then flush the group.
                    const bool flushMergedScopes = exceededCommandCost || exceededSwapChainLimit || hardwareQueueMismatch || onSyncBoundaries || subpassGroup;

                    if (flushMergedScopes && mergedScopes.size())
                    {
                        mergedGroupCost = 0;
                        mergedSwapchainCount = 0;
                        mergedHardwareQueueClass = scope.GetHardwareQueueClass();
                        FrameGraphExecuteGroupMerged* multiScopeContextGroup = AddGroup<FrameGraphExecuteGroupMerged>();
                        multiScopeContextGroup->Init(device, AZStd::move(mergedScopes));                    
                    }
                }

                // Attempt to merge the current scope.
                if (!subpassGroup && totalScopeCost < CommandListCostThreshold)
                {
                    mergedScopes.push_back(&scope);
                    mergedGroupCost += totalScopeCost;
                    mergedSwapchainCount += swapchainCount;
                }
                // Not mergeable, create a dedicated context group for it.
                else
                {
                    // And then create a new group for the current scope with dedicated [1, N] command lists.
                    const uint32_t commandListCount = AZStd::max(RHI::DivideByMultiple(totalScopeCost, CommandListCostThreshold), 1u);

                    FrameGraphExecuteGroup* scopeContextGroup = AddGroup<FrameGraphExecuteGroup>();
                    scopeContextGroup->Init(device, scope, commandListCount, GetJobPolicy());
                }
                scopePrev = &scope;
            }

            if (mergedScopes.size())
            {
                mergedGroupCost = 0;
                mergedSwapchainCount = 0;
                FrameGraphExecuteGroupMerged* multiScopeContextGroup = AddGroup<FrameGraphExecuteGroupMerged>();
                multiScopeContextGroup->Init(device, AZStd::move(mergedScopes));
            }

            // Create the handlers to manage the execute groups.
            auto groups = GetGroups();
            RHI::GraphGroupId groupId;
            uint32_t initGroupIndex = 0;
            for (uint32_t i = 0; i < groups.size(); ++i)
            {
                const FrameGraphExecuteGroupBase* group = static_cast<const FrameGraphExecuteGroupBase*>(groups[i].get());
                if (groupId != group->GetGroupId())
                {
                    AddExecuteGroupHandler(groupId, { groups.begin() + initGroupIndex, groups.begin() + i });
                    groupId = group->GetGroupId();
                    initGroupIndex = i;
                }
            }

            // Add the final handler for the remaining groups.
            AddExecuteGroupHandler(groupId, { groups.begin() + initGroupIndex, groups.end() });
        }

        void FrameGraphExecuter::ExecuteGroupInternal(RHI::FrameGraphExecuteGroup& groupBase)
        {
            FrameGraphExecuteGroupBase& group = static_cast<FrameGraphExecuteGroupBase&>(groupBase);
            auto findIter = m_groupHandlers.find(group.GetGroupId());
            AZ_Assert(findIter != m_groupHandlers.end(), "Could not find group handler for groupId %d", group.GetGroupId().GetIndex());
            FrameGraphExecuteGroupHandlerBase* handler = findIter->second.get();
            // Wait until all execute groups of the handler has finished and also make sure that the handler itself hasn't executed already (which is possible for parallel encoding).
            if (!handler->IsExecuted() && handler->IsComplete())
            {
                // This will execute the recorded work into the queue.
                handler->End();
            }
        }

        void FrameGraphExecuter::EndInternal()
        {
            m_groupHandlers.clear();
        }

        void FrameGraphExecuter::AddExecuteGroupHandler(const RHI::GraphGroupId& groupId, const AZStd::vector<RHI::FrameGraphExecuteGroup*>& groups)
        {
            if (groups.empty())
            {
                return;
            }

            // Add the handler depending on the number of execute groups.
            AZStd::unique_ptr<FrameGraphExecuteGroupHandlerBase> handler(groups.size() == 1 && azrtti_cast<FrameGraphExecuteGroupMerged*>(groups.front()) ?
                static_cast<FrameGraphExecuteGroupHandlerBase*>(aznew FrameGraphExecuteGroupMergedHandler) :
                static_cast<FrameGraphExecuteGroupHandlerBase*>(aznew FrameGraphExecuteGroupHandler));

            handler->Init(GetDevice(), groups);
            m_groupHandlers.insert({ groupId, AZStd::move(handler) });
        }
    }
}

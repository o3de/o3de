/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/FrameGraph.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzCore/std/parallel/thread.h>
#include <RHI/CommandQueueContext.h>
#include <RHI/Device.h>
#include <RHI/FrameGraphExecuteGroupPrimary.h>
#include <RHI/FrameGraphExecuteGroupPrimaryHandler.h>
#include <RHI/FrameGraphExecuteGroupSecondary.h>
#include <RHI/FrameGraphExecuteGroupSecondaryHandler.h>
#include <RHI/FrameGraphExecuter.h>
#include <RHI/Scope.h>
#include <RHI/SwapChain.h>

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
            RHI::JobPolicy graphJobPolicy = RHI::JobPolicy::Parallel;
#if defined(AZ_FORCE_CPU_GPU_INSYNC)
            graphJobPolicy = RHI::JobPolicy::Serial;
#endif
            SetJobPolicy(graphJobPolicy);
        }

        RHI::ResultCode FrameGraphExecuter::InitInternal(const RHI::FrameGraphExecuterDescriptor& descriptor)
        {
            const RHI::ConstPtr<RHI::PlatformLimitsDescriptor> rhiPlatformLimitsDescriptor = descriptor.m_platformLimitsDescriptor;
            if (RHI::ConstPtr<PlatformLimitsDescriptor> vkPlatformLimitsDesc =
                    azrtti_cast<const PlatformLimitsDescriptor*>(rhiPlatformLimitsDescriptor))
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
            AZStd::vector<const Scope*> mergedScopes;
            const Scope* scopePrev = nullptr;
            const Scope* scopeNext = nullptr;
            const AZStd::vector<RHI::Scope*>& scopes = frameGraph.GetScopes();

            // The following semaphore trackers are there to count how many semaphores are present before each swapchain
            // Swapchains need to use a binary semaphore, which needs all dependent semaphores to be signalled before submitted to the queue
            // We do this by counting the number of semaphores that the scopes are waiting for
            // Not needed when AZ_FORCE_CPU_GPU_INSYNC because of synchronization after every scope
            AZStd::intrusive_ptr<SemaphoreTrackerCollection> semaphoreTrackers;
            // Tracker that will be used for the next swapchain in the framegraph
            AZStd::shared_ptr<SemaphoreTrackerHandle> currentSemaphoreHandle;
            // Some semaphores (Fences) might be waited-for by the use in a scope
            // We remember which semaphores are signalled and assume that the ones that are never waited-for are waited-for by the user
            AZStd::unordered_map<Fence*, bool> userFencesSignalledMap;
            [[maybe_unused]] int numUnwaitedFences = 0;
            bool useSemaphoreTrackers = device.GetFeatures().m_signalFenceFromCPU;
            if (useSemaphoreTrackers)
            {
                semaphoreTrackers = new SemaphoreTrackerCollection;
                currentSemaphoreHandle = semaphoreTrackers->CreateHandle();
            }

#if defined(AZ_FORCE_CPU_GPU_INSYNC)
            // Forces all scopes to issue a dedicated merged scope group with one command list.
            // This will ensure that the Execute is done on only one scope and if an error happens
            // we can be sure about the work gpu was working on before the crash.
            for (auto it = scopes.begin(); it != scopes.end(); ++it)
            {
                const Scope& scope = *static_cast<const Scope*>(*it);
                auto nextIter = it + 1;
                scopeNext = nextIter != scopes.end() ? static_cast<const Scope*>(*nextIter) : nullptr;
                const bool subpassGroup = (scopeNext && scopeNext->GetFrameGraphGroupId() == scope.GetFrameGraphGroupId()) ||
                    (scopePrev && scopePrev->GetFrameGraphGroupId() == scope.GetFrameGraphGroupId());

                if (subpassGroup)
                {
                    FrameGraphExecuteGroupSecondary* scopeContextGroup = AddGroup<FrameGraphExecuteGroupSecondary>();
                    scopeContextGroup->Init(device, scope, 1, GetJobPolicy(), nullptr);
                }
                else
                {
                    mergedScopes.push_back(&scope);
                    FrameGraphExecuteGroupPrimary* multiScopeContextGroup = AddGroup<FrameGraphExecuteGroupPrimary>();
                    multiScopeContextGroup->SetName(scope.GetName());
                    multiScopeContextGroup->Init(device, AZStd::move(mergedScopes), nullptr);
                }
                scopePrev = &scope;
            }
#else

            RHI::HardwareQueueClass mergedHardwareQueueClass = RHI::HardwareQueueClass::Graphics;
            uint32_t mergedGroupCost = 0;
            uint32_t mergedSwapchainCount = 0;
            bool needNewSemaphoreHandle = false;

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

                const uint32_t CommandListCostThreshold = AZStd::max(
                    m_frameGraphExecuterData.m_commandListCostThresholdMin,
                    AZ::DivideAndRoundUp(estimatedItemCount, m_frameGraphExecuterData.m_commandListsPerScopeMax));

                /**
                 * Computes a cost heuristic based on the number of items and number of attachments in
                 * the scope. This cost is used to partition command list generation.
                 */
                const uint32_t totalScopeCost = estimatedItemCount * m_frameGraphExecuterData.m_itemCost +
                    static_cast<uint32_t>(scope.GetAttachments().size()) * m_frameGraphExecuterData.m_attachmentCost;

                // Check if we are in a middle of a framegraph group.
                const bool subpassGroup = (scopeNext && scopeNext->GetFrameGraphGroupId() == scope.GetFrameGraphGroupId()) ||
                    (scopePrev && scopePrev->GetFrameGraphGroupId() == scope.GetFrameGraphGroupId());

                const uint32_t swapchainCount = static_cast<uint32_t>(scope.GetSwapChainsToPresent().size());

                // Detect if we are able to continue merging.
                {
                    // Check if the group fits into the current running merge queue. If not, we have to flush the queue.
                    const bool exceededCommandCost = (mergedGroupCost + totalScopeCost) > CommandListCostThreshold;

                    // Check if the swap chains fit into this group.
                    const bool exceededSwapChainLimit =
                        (mergedSwapchainCount + swapchainCount) > m_frameGraphExecuterData.m_swapChainsPerCommandList;

                    // Check if the hardware queue classes match.
                    const bool hardwareQueueMismatch = scope.GetHardwareQueueClass() != mergedHardwareQueueClass;

                    // Check if we are straddling the boundary of a fence/semaphore.
                    const bool onSyncBoundaries = !scope.GetWaitSemaphores().empty() || !scope.GetWaitFences().empty() ||
                        (scopePrev && (!scopePrev->GetSignalSemaphores().empty() || !scopePrev->GetSignalFences().empty()));

                    // If we exceeded limits, then flush the group.
                    const bool flushMergedScopes =
                        exceededCommandCost || exceededSwapChainLimit || hardwareQueueMismatch || onSyncBoundaries || subpassGroup;

                    if (flushMergedScopes && mergedScopes.size())
                    {
                        // All merged scopes use a single primary command list
                        mergedGroupCost = 0;
                        mergedSwapchainCount = 0;
                        mergedHardwareQueueClass = scope.GetHardwareQueueClass();
                        FrameGraphExecuteGroupPrimary* multiScopeContextGroup = AddGroup<FrameGraphExecuteGroupPrimary>();
                        multiScopeContextGroup->Init(device, AZStd::move(mergedScopes));
                    }
                }

                if (useSemaphoreTrackers)
                {
                    if (needNewSemaphoreHandle)
                    {
                        currentSemaphoreHandle = semaphoreTrackers->CreateHandle();
                    }
                    for (auto& fence : scope.GetSignalFences())
                    {
                        auto it = userFencesSignalledMap.find(fence.get());
                        if (it == userFencesSignalledMap.end())
                        {
                            userFencesSignalledMap[fence.get()] = false;
                            numUnwaitedFences++;
                        }
                    }
                    for (auto& fence : scope.GetWaitFences())
                    {
                        auto it = userFencesSignalledMap.find(fence.get());
                        if (it != userFencesSignalledMap.end())
                        {
                            if (!it->second)
                            {
                                numUnwaitedFences--;
                            }
                        }
                        userFencesSignalledMap[fence.get()] = true;
                        fence->SetSemaphoreHandle(currentSemaphoreHandle);
                        if (fence->GetFenceType() == FenceType::TimelineSemaphore)
                        {
                            semaphoreTrackers->AddSemaphores(1);
                        }
                    }
                    for (auto& semaphore : scope.GetWaitSemaphores())
                    {
                        // We want to set the Handle when we wait for the semaphore, not when we signal it
                        // We could signal the semaphore before a swapchain and wait for it afterwards
                        // Then the waiting for the signal would be too early
                        // This would not lead to any deadlocks, but maybe to too much waits in the swapchain
                        semaphore.second->SetSemaphoreHandle(currentSemaphoreHandle);
                        if (semaphore.second->GetType() == SemaphoreType::Binary)
                        {
                            for (auto [fence, waitedFor] : userFencesSignalledMap)
                            {
                                if (!waitedFor)
                                {
                                    // Here we assume that the fence is waited for in the same scope as we signal it, but on the CPU
                                    fence->SetSemaphoreHandle(currentSemaphoreHandle);
                                    if (fence->GetFenceType() == FenceType::TimelineSemaphore)
                                    {
                                        semaphoreTrackers->AddSemaphores(1);
                                    }
                                }
                            }
                            numUnwaitedFences = 0;
                            userFencesSignalledMap.clear();
                            needNewSemaphoreHandle = true;
                        }
                        else
                        {
                            semaphoreTrackers->AddSemaphores(1);
                        }
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
                    // And then create a new group for the current scope with dedicated [1, N] secondary command lists
                    const uint32_t commandListCount = AZStd::max(AZ::DivideAndRoundUp(totalScopeCost, CommandListCostThreshold), 1u);
                    FrameGraphExecuteGroupSecondary* scopeContextGroup = AddGroup<FrameGraphExecuteGroupSecondary>();
                    scopeContextGroup->Init(device, scope, commandListCount, GetJobPolicy());
                }
                scopePrev = &scope;
            }

            // Merge all pending scopes
            if (mergedScopes.size())
            {
                mergedGroupCost = 0;
                mergedSwapchainCount = 0;
                FrameGraphExecuteGroupPrimary* multiScopeContextGroup = AddGroup<FrameGraphExecuteGroupPrimary>();
                multiScopeContextGroup->Init(device, AZStd::move(mergedScopes));
            }
#endif
            // Create the handlers to manage the execute groups.
            // Handlers manage one or multiple execute groups by creating a shared renderpass/framebuffer
            // or advancing the subpass if needed.
            auto groups = GetGroups();
            AZStd::vector<RHI::FrameGraphExecuteGroup*> groupRefs;
            groupRefs.reserve(groups.size());
            RHI::GraphGroupId groupId;
            uint32_t initGroupIndex = 0;
            for (uint32_t i = 0; i < groups.size(); ++i)
            {
                const FrameGraphExecuteGroup* group = static_cast<const FrameGraphExecuteGroup*>(groups[i].get());
                if (groupId != group->GetGroupId())
                {
                    groupRefs.clear();
                    for (size_t groupRefIndex = initGroupIndex; groupRefIndex < i; ++groupRefIndex)
                    {
                        groupRefs.push_back(groups[groupRefIndex].get());
                    }
                    AddExecuteGroupHandler(groupId, groupRefs);
                    groupId = group->GetGroupId();
                    initGroupIndex = i;
                }
            }

            // Add the final handler for the remaining groups.
            groupRefs.clear();
            for (size_t groupRefIndex = initGroupIndex; groupRefIndex < groups.size(); ++groupRefIndex)
            {
                groupRefs.push_back(groups[groupRefIndex].get());
            }
            AddExecuteGroupHandler(groupId, groupRefs);
        }

        void FrameGraphExecuter::ExecuteGroupInternal(RHI::FrameGraphExecuteGroup& groupBase)
        {
            FrameGraphExecuteGroup& group = static_cast<FrameGraphExecuteGroup&>(groupBase);
            auto findIter = m_groupHandlers.find(group.GetGroupId());
            AZ_Assert(findIter != m_groupHandlers.end(), "Could not find group handler for groupId %d", group.GetGroupId().GetIndex());
            FrameGraphExecuteGroupHandler* handler = findIter->second.get();
            // Wait until all execute groups of the handler has finished and also make sure that the handler itself hasn't executed already
            // (which is possible for parallel encoding).
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

        void FrameGraphExecuter::AddExecuteGroupHandler(
            const RHI::GraphGroupId& groupId, const AZStd::vector<RHI::FrameGraphExecuteGroup*>& groups)
        {
            if (groups.empty())
            {
                return;
            }

            // Add the handler depending on the number of execute groups.
            AZStd::unique_ptr<FrameGraphExecuteGroupHandler> handler(
                groups.size() == 1 && azrtti_cast<FrameGraphExecuteGroupPrimary*>(groups.front())
                    ? static_cast<FrameGraphExecuteGroupHandler*>(aznew FrameGraphExecuteGroupPrimaryHandler)
                    : static_cast<FrameGraphExecuteGroupHandler*>(aznew FrameGraphExecuteGroupSecondaryHandler));

            handler->Init(GetDevice(), groups);
            m_groupHandlers.insert({ groupId, AZStd::move(handler) });
        }
    } // namespace Vulkan
} // namespace AZ

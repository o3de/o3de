/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/FrameGraph.h>
#include <RHI/Device.h>
#include <RHI/FrameGraphExecuter.h>
#include <RHI/FrameGraphExecuteGroupMerged.h>
#include <RHI/FrameGraphExecuteGroup.h>
#include <Atom/RHI.Reflect/Metal/PlatformLimitsDescriptor.h>


namespace AZ
{
    namespace Metal
    {
        RHI::Ptr<FrameGraphExecuter> FrameGraphExecuter::Create()
        {
            return aznew FrameGraphExecuter();
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
            for (auto& [deviceIndex, platformLimitsDescriptor] : descriptor.m_platformLimitsDescriptors)
            {
                const RHI::ConstPtr<RHI::PlatformLimitsDescriptor> rhiPlatformLimitsDescriptor = platformLimitsDescriptor;
                if (RHI::ConstPtr<PlatformLimitsDescriptor> metalPlatformLimitsDesc = azrtti_cast<const PlatformLimitsDescriptor*>(rhiPlatformLimitsDescriptor))
                {
                    m_frameGraphExecuterData[deviceIndex] = metalPlatformLimitsDesc->m_frameGraphExecuterData;
                }
            }

            return RHI::ResultCode::Success;
        }
        
        void FrameGraphExecuter::ShutdownInternal()
        {
        }
        
        void FrameGraphExecuter::BeginInternal(const RHI::FrameGraph& frameGraph)
        {
#if defined(AZ_FORCE_CPU_GPU_INSYNC)
            // Forces all scopes to issue a dedicated merged scope group with one command list.
            // This will ensure that the Commit is done on only one scope and if an error happens
            // we can be sure about the work gpu was working on before the crash.
            AZStd::vector<const Scope*> mergedScopes;
            for (const RHI::Scope* scopeBase : frameGraph.GetScopes())
            {
                mergedScopes.push_back(static_cast<const Scope*>(scopeBase));
                FrameGraphExecuteGroupMerged* scopeContextGroup = AddGroup<FrameGraphExecuteGroupMerged>();
                scopeContextGroup->Init(static_cast<Device&>(scopeBase->GetDevice()), AZStd::move(mergedScopes), GetGroupCount());
            }
#else
     
            bool hasUserFencesToSignal = false;
            RHI::HardwareQueueClass mergedHardwareQueueClass = RHI::HardwareQueueClass::Graphics;
            int mergedDeviceIndex = RHI::MultiDevice::InvalidDeviceIndex;
            AZ::u32 mergedGroupCost = 0;
            AZ::u32 mergedSwapchainCount = 0;
            AZStd::vector<const Scope*> mergedScopes;

            const Scope* scopePrev = nullptr;
            for (const RHI::Scope* scopeBase : frameGraph.GetScopes())
            {
                const Scope& scope = *static_cast<const Scope*>(scopeBase);

                // Reset merged hardware queue class to match current scope if empty.
                if (mergedGroupCost == 0)
                {
                    mergedHardwareQueueClass = scope.GetHardwareQueueClass();
                }

                const AZ::u32 estimatedItemCount = scope.GetEstimatedItemCount();

                const uint32_t CommandListCostThreshold =
                    AZStd::max(
                        m_frameGraphExecuterData[scope.GetDeviceIndex()].m_commandListCostThresholdMin,
                        RHI::DivideByMultiple(estimatedItemCount, m_frameGraphExecuterData[scope.GetDeviceIndex()].m_commandListsPerScopeMax));

                /**
                 * Computes a cost heuristic based on the number of items and number of attachments in
                 * the scope. This cost is used to partition command list generation.
                 */
                const AZ::u32 totalScopeCost =
                    estimatedItemCount * m_frameGraphExecuterData[scope.GetDeviceIndex()].m_itemCost +
                    static_cast<AZ::u32>(scope.GetAttachments().size()) * m_frameGraphExecuterData[scope.GetDeviceIndex()].m_attachmentCost;

                const AZ::u32 swapchainCount = static_cast<AZ::u32>(scope.GetSwapChainsToPresent().size());

                // Detect if we are able to continue merging.
                
                // Check if the group fits into the current running merge queue. If not, we have to flush the queue.
                const bool exceededCommandCost = (mergedGroupCost + totalScopeCost) > CommandListCostThreshold;

                // Check if the swap chains fit into this group.
                const bool exceededSwapChainLimit = (mergedSwapchainCount + swapchainCount) > m_frameGraphExecuterData[scope.GetDeviceIndex()].m_swapChainsPerCommandList;

                // Check if the hardware queue classes match.
                const bool hardwareQueueMismatch = scope.GetHardwareQueueClass() != mergedHardwareQueueClass;

                // Check if we are straddling the boundary of a fence.
                const bool onFenceBoundaries = (scope.HasWaitFences() || (scopePrev && scopePrev->HasSignalFence())) || hasUserFencesToSignal;

                       // Check if the devices match.
                const bool deviceMismatch = mergedDeviceIndex != scope.GetDeviceIndex();

                // If we exceeded limits, then flush the group.
                const bool flushMergedScopes = exceededCommandCost || exceededSwapChainLimit || hardwareQueueMismatch || onFenceBoundaries || deviceMismatch;

                if (flushMergedScopes && mergedScopes.size())
                {
                    hasUserFencesToSignal = false;
                    mergedGroupCost = 0;
                    mergedSwapchainCount = 0;
                    mergedHardwareQueueClass = scope.GetHardwareQueueClass();
                    mergedDeviceIndex = scope.GetDeviceIndex();
                    FrameGraphExecuteGroupMerged* multiScopeContextGroup = AddGroup<FrameGraphExecuteGroupMerged>();
                    multiScopeContextGroup->Init(static_cast<Device&>(scope.GetDevice()), AZStd::move(mergedScopes), GetGroupCount());
                }
                
                // Attempt to merge the current scope.
                if (totalScopeCost < CommandListCostThreshold)
                {
                    mergedScopes.push_back(&scope);
                    mergedGroupCost += totalScopeCost;
                    mergedSwapchainCount += swapchainCount;
                    hasUserFencesToSignal = !scope.GetFencesToSignal().empty();
                }
                // Not mergeable, create a dedicated context group for it.
                else
                {
                    // And then create a new group for the current scope with dedicated [1, N] command lists.
                    const AZ::u32 commandListCount = AZStd::max(RHI::DivideByMultiple(totalScopeCost, CommandListCostThreshold), 1u);

                    FrameGraphExecuteGroup* scopeContextGroup = AddGroup<FrameGraphExecuteGroup>();
                    scopeContextGroup->Init(static_cast<Device&>(scope.GetDevice()), scope, commandListCount, GetJobPolicy(), GetGroupCount());
                }

                scopePrev = &scope;
            }

            if (mergedScopes.size())
            {
                mergedGroupCost = 0;
                mergedSwapchainCount = 0;
                FrameGraphExecuteGroupMerged* multiScopeContextGroup = AddGroup<FrameGraphExecuteGroupMerged>();
                multiScopeContextGroup->Init(static_cast<Device&>(mergedScopes.front()->GetDevice()), AZStd::move(mergedScopes), GetGroupCount());
            }
#endif
        }

        void FrameGraphExecuter::ExecuteGroupInternal(RHI::FrameGraphExecuteGroup& groupBase)
        {
            FrameGraphExecuteGroupBase& group = static_cast<FrameGraphExecuteGroupBase&>(groupBase);
            &static_cast<Device*>(groupBase->GetDevice())->GetCommandQueueContext()->ExecuteWork(group.GetHardwareQueueClass(), group.AcquireWorkRequest());
        }
    }
}

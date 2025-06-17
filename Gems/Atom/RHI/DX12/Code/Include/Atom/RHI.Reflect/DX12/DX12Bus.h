/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

#include <AzCore/PlatformIncl.h>
#include <d3d12.h>

namespace AZ::DX12
{
    //! Ebus for collecting external handle requirements for creating memory/semaphores
    class DX12RequirementsRequest : public AZ::EBusTraits
    {
    public:
        virtual ~DX12RequirementsRequest() = default;

        //! Collects requirements for external memory when allocating memory
        virtual void CollectTransientAttachmentPoolHeapFlags([[maybe_unused]] D3D12_HEAP_FLAGS& flags){};
        virtual void CollectAllocatorExtraHeapFlags([[maybe_unused]] D3D12_HEAP_FLAGS& flags, [[maybe_unused]] D3D12_HEAP_TYPE heapType){};
        virtual void CollectFenceFlags([[maybe_unused]] D3D12_FENCE_FLAGS& flags){};

        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        using MutexType = AZStd::mutex;
        static constexpr bool LocklessDispatch = true;
    };

    using DX12RequirementBus = AZ::EBus<DX12RequirementsRequest>;
} // namespace AZ::DX12

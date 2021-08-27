/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

// NOTE: We are careful to include platform headers *before* we include AzCore/Debug/Profiler.h to ensure that d3d12 symbols
// are defined prior to the inclusion of the pix3 runtime.
#include <RHI/DX12.h>

#include <Atom/RHI/DeviceObject.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AZ
{
    namespace DX12
    {
        class Device;
        class ImageView;
        class BufferView;

        class CommandListBase
            : public RHI::DeviceObject
        {
            using Base = RHI::DeviceObject;
        public:
            virtual ~CommandListBase() = 0;

            CommandListBase(const CommandListBase&) = delete;

            virtual void Reset(ID3D12CommandAllocator* commandAllocator);
            virtual void Close();

            bool IsRecording() const;

            void QueueUAVBarrier(ID3D12Resource* resource);
            void QueueTransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter);
            void QueueTransitionBarrier(const D3D12_RESOURCE_TRANSITION_BARRIER& barrier);
            void QueueAliasingBarrier(const D3D12_RESOURCE_ALIASING_BARRIER& barrier);

            void FlushBarriers();

            ID3D12GraphicsCommandList* GetCommandList();

            const ID3D12GraphicsCommandList* GetCommandList() const;

            RHI::HardwareQueueClass GetHardwareQueueClass() const;

            void SetAftermathEventMarker(const AZStd::string& markerData);

        protected:
            void Init(Device& device, RHI::HardwareQueueClass hardwareQueueClass, ID3D12CommandAllocator* commandAllocator);

            CommandListBase() = default;

        private:
            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////

            RHI::HardwareQueueClass m_hardwareQueueClass;
            Microsoft::WRL::ComPtr<ID3D12GraphicsCommandListX> m_commandList;
            AZStd::vector<D3D12_RESOURCE_BARRIER> m_queuedBarriers;
            bool m_isRecording = false;

            // Nsight Aftermath related command list context
            void* m_aftermathCommandListContext = nullptr;
        };
    }
}

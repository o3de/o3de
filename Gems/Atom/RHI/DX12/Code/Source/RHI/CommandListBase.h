/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <Atom/RHI/DeviceObject.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>
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
        };
    }
}

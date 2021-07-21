/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "RHI/Atom_RHI_DX12_precompiled.h"
#include <RHI/Conversions.h>
#include <RHI/CommandQueue.h>

namespace AZ
{
    namespace DX12
    {
        namespace Platform
        {
            HRESULT CreateCommandQueue(ID3D12DeviceX* device, RHI::HardwareQueueClass hardwareQueueClass, [[maybe_unused]] HardwareQueueSubclass hardwareQueueSubclass, ID3D12CommandQueueX** commandQueue)
            {
                D3D12_COMMAND_QUEUE_DESC QueueDesc = {};
                QueueDesc.Type = ConvertHardwareQueueClass(hardwareQueueClass);
                QueueDesc.NodeMask = 1;

                return device->CreateCommandQueue(&QueueDesc, IID_GRAPHICS_PPV_ARGS(commandQueue));
            }
        }
    }
}


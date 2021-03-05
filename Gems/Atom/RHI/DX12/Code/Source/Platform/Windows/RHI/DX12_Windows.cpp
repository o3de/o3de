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


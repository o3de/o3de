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

#include <RHI/CommandList.h>
#include <RHI/CommandQueue.h>
#include <RHI/Scope.h>
#include <Atom/RHI/FrameGraphExecuteGroup.h>

namespace AZ
{
    namespace DX12
    {
        class FrameGraphExecuteGroupBase
            : public RHI::FrameGraphExecuteGroup
        {
            using Base = RHI::FrameGraphExecuteGroup;
        public:
            FrameGraphExecuteGroupBase() = default;

            void SetDevice(Device& device);

            ExecuteWorkRequest&& MakeWorkRequest();

            RHI::HardwareQueueClass GetHardwareQueueClass() const;

        protected:
            CommandList* AcquireCommandList() const;

            Device* m_device = nullptr;
            RHI::HardwareQueueClass m_hardwareQueueClass = RHI::HardwareQueueClass::Graphics;
            ExecuteWorkRequest m_workRequest;
        };
    }
}
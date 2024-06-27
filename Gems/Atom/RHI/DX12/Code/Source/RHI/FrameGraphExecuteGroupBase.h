/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

            Device& GetDevice() const;

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

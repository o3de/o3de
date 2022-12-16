/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceFence.h>
#include <Atom/RHI/MultiDeviceObject.h>

namespace AZ
{
    namespace RHI
    {
        class Fence : public MultiDeviceObject
        {
        public:
            AZ_CLASS_ALLOCATOR(Fence, AZ::SystemAllocator, 0);
            AZ_RTTI(Fence, "{D8C31560-7342-46C4-BC6A-8B2A19FE6DEA}", Object);

            Fence() = default;
            ~Fence();

            /// Initializes the fence using the provided device and initial state.
            ResultCode Init(DeviceMask deviceMask, FenceState initialState);

            /// Shuts down the fence.
            void Shutdown() override final;

            /// Signals the fence from the calling thread.
            RHI::ResultCode SignalOnCpu();

            /// Waits (blocks) for the fence on the calling thread.
            RHI::ResultCode WaitOnCpu() const;

            /// Resets the fence.
            RHI::ResultCode Reset();

            using SignalCallback = AZStd::function<void()>;

            /// Spawns a dedicated thread to wait on the fence. The provided callback
            /// is invoked when the fence completes.
            ResultCode WaitOnCpuAsync(SignalCallback callback);

            const Ptr<DeviceFence>& GetDeviceFence(int deviceIndex);

        protected:
            bool ValidateIsInitialized() const;

            AZStd::thread m_waitThread;

            AZStd::unordered_map<int, Ptr<DeviceFence>> m_deviceFences;
        };
    }
}

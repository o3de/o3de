/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/MultiDeviceObject.h>
#include <Atom/RHI/Fence.h>

namespace AZ
{
    namespace RHI
    {
        class MultiDeviceFence : public MultiDeviceObject
        {
        public:
            AZ_CLASS_ALLOCATOR(MultiDeviceFence, AZ::SystemAllocator, 0);
            AZ_RTTI(MultiDeviceFence, "{5FF150A4-2C1E-4EC6-AE36-8EBD1CE22C31}", MultiDeviceObject);

            MultiDeviceFence() = default;
            virtual ~MultiDeviceFence() = default;

            /// Initializes the fence using the provided device and initial state.
            ResultCode Init(MultiDevice::DeviceMask deviceMask, FenceState initialState);

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

            const Ptr<Fence>& GetDeviceFence(int deviceIndex)
            {
                AZ_Error(
                    "MultiDeviceFence",
                    m_deviceFences.find(deviceIndex) != m_deviceFences.end(),
                    "No Fence found for device index %d\n",
                    deviceIndex);
                return m_deviceFences.at(deviceIndex);
            }

        protected:
            bool ValidateIsInitialized() const;

            AZStd::thread m_waitThread;

            AZStd::unordered_map<int, Ptr<Fence>> m_deviceFences;
        };
    } // namespace RHI
} // namespace AZ

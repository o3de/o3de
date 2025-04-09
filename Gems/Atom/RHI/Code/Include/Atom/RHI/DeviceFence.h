/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceObject.h>

namespace AZ::RHI
{
    enum class FenceState : uint32_t
    {
        Reset = 0,
        Signaled
    };

    class DeviceFence
        : public DeviceObject
    {
    public:
        AZ_RTTI(DeviceFence, "{D66C8B8F-226A-4018-89C1-F190A730CBC3}", Object);
        virtual ~DeviceFence() = 0;

        /// Initializes the fence using the provided device and initial state.
        /// Set usedForWaitingOnDevice to true if the Fence shoud be signaled on the CPU and waited for on the device.
        ResultCode Init(Device& device, FenceState initialState, bool usedForWaitingOnDevice = false);

        /// Shuts down the fence.
        void Shutdown() override final;

        /// Signals the fence from the calling thread.
        RHI::ResultCode SignalOnCpu();

        /// Waits (blocks) for the fence on the calling thread.
        RHI::ResultCode WaitOnCpu() const;

        /// Resets the fence.
        RHI::ResultCode Reset();

        /// Returns whether the fence is signaled or not.
        FenceState GetFenceState() const;

        using SignalCallback = AZStd::function<void()>;

        /// Spawns a dedicated thread to wait on the fence. The provided callback
        /// is invoked when the fence completes.
        ResultCode WaitOnCpuAsync(SignalCallback callback);

        /// BinaryFences in Vulkan need their dependent TimelineSemaphore Fences to be
        /// signalled. This is currently only implemented in Vulkan
        virtual void SetExternallySignalled(){};

    protected:
        bool ValidateIsInitialized() const;

        //////////////////////////////////////////////////////////////////////////
        // Platform API

        /// Called when the fence is being initialized.
        virtual ResultCode InitInternal(Device& device, FenceState initialState, bool usedForWaitingOnDevice) = 0;

        /// Called when the PSO is being shutdown.
        virtual void ShutdownInternal() = 0;

        /// Called when the fence is being signaled on the CPU.
        virtual void SignalOnCpuInternal() = 0;

        /// Called when the fence is waiting on the CPU.
        virtual void WaitOnCpuInternal() const = 0;

        /// Called when the fence is being reset.
        virtual void ResetInternal() = 0;

        /// Called to retrieve the current fence state.
        virtual FenceState GetFenceStateInternal() const = 0;

        //////////////////////////////////////////////////////////////////////////

        AZStd::thread m_waitThread;
    };
}

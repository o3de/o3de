/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/SingleDeviceFence.h>
#include <Atom/RHI/MultiDeviceObject.h>

namespace AZ::RHI
{
    //! A multi-device synchronization primitive, holding device-specific Fences, than can be used to insert
    //! dependencies between a queue and a host
    class MultiDeviceFence : public MultiDeviceObject
    {
    public:
        AZ_CLASS_ALLOCATOR(MultiDeviceFence, AZ::SystemAllocator, 0);
        AZ_RTTI(MultiDeviceFence, "{5FF150A4-2C1E-4EC6-AE36-8EBD1CE22C31}", MultiDeviceObject);
        AZ_RHI_MULTI_DEVICE_OBJECT_GETTER(Fence);

        MultiDeviceFence() = default;
        virtual ~MultiDeviceFence() = default;

        //! Initializes the multi-device fence using the provided deviceMask.
        //! It creates on device-specific fence for each bit set in the deviceMask and
        //! passes on the initial FenceState to each SingleDeviceFence
        ResultCode Init(MultiDevice::DeviceMask deviceMask, FenceState initialState);

        //! Waits on m_waitThread and shuts down all device-specific fences.
        void Shutdown() override final;

        //! Signals the device-specific fences managed by this class
        RHI::ResultCode SignalOnCpu();

        //! Waits (blocks) for all device-specific fences managed by this class
        RHI::ResultCode WaitOnCpu() const;

        //! Resets the device-specific fences.
        RHI::ResultCode Reset();

        using SignalCallback = AZStd::function<void()>;

        //! Spawns a dedicated thread to wait on all device-specific fences. The provided callback
        //! is invoked when the fences complete.
        ResultCode WaitOnCpuAsync(SignalCallback callback);

    protected:
        bool ValidateIsInitialized() const;

        //! This can be used to asynchronously wait on all fences by calling WaitOnCpuAsync
        AZStd::thread m_waitThread;
    };
} // namespace AZ::RHI

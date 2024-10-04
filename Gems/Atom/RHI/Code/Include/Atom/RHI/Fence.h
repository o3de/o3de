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

namespace AZ::RHI
{
    //! A multi-device synchronization primitive, holding device-specific Fences, than can be used to insert
    //! dependencies between a queue and a host
    class Fence : public MultiDeviceObject
    {
    public:
        AZ_CLASS_ALLOCATOR(Fence, AZ::SystemAllocator, 0);
        AZ_RTTI(Fence, "{5FF150A4-2C1E-4EC6-AE36-8EBD1CE22C31}", MultiDeviceObject);
        AZ_RHI_MULTI_DEVICE_OBJECT_GETTER(Fence);

        Fence() = default;
        virtual ~Fence() = default;

        //! Initializes the multi-device fence using the provided deviceMask.
        //! It creates on device-specific fence for each bit set in the deviceMask and
        //! passes on the initial FenceState to each DeviceFence.
        //! Set usedForWaitingOnDevice to true if the Fence shoud be signaled on the CPU and waited for on the device.
        ResultCode Init(MultiDevice::DeviceMask deviceMask, FenceState initialState, bool usedForWaitingOnDevice = false);

        //! Shuts down all device-specific fences.
        void Shutdown() override final;

        //! Signals the device-specific fences managed by this class
        RHI::ResultCode SignalOnCpu();

        //! Resets the device-specific fences.
        RHI::ResultCode Reset();

        using SignalCallback = AZStd::function<void()>;

    protected:
        bool ValidateIsInitialized() const;
    };
} // namespace AZ::RHI

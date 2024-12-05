/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/DeviceFence.h>

#include <AzCore/Debug/Profiler.h>

namespace AZ::RHI
{
    DeviceFence::~DeviceFence() {}

    bool DeviceFence::ValidateIsInitialized() const
    {
        if (Validation::IsEnabled())
        {
            if (!IsInitialized())
            {
                AZ_Error("DeviceFence", false, "DeviceFence is not initialized!");
                return false;
            }
        }

        return true;
    }

    ResultCode DeviceFence::Init(Device& device, FenceState initialState, bool usedForWaitingOnDevice)
    {
        if (Validation::IsEnabled())
        {
            if (IsInitialized())
            {
                AZ_Error("DeviceFence", false, "DeviceFence is already initialized!");
                return ResultCode::InvalidOperation;
            }
        }

        const ResultCode resultCode = InitInternal(device, initialState, usedForWaitingOnDevice);

        if (resultCode == ResultCode::Success)
        {
            DeviceObject::Init(device);
        }
        else
        {
            AZ_Assert(false, "Failed to create a fence");
        }

        return resultCode;
    }

    void DeviceFence::Shutdown()
    {
        if (IsInitialized())
        {
            if (m_waitThread.joinable())
            {
                m_waitThread.join();
            }

            ShutdownInternal();
            DeviceObject::Shutdown();
        }
    }

    ResultCode DeviceFence::SignalOnCpu()
    {
        if (!ValidateIsInitialized())
        {
            return ResultCode::InvalidOperation;
        }

        SignalOnCpuInternal();
        return ResultCode::Success;
    }

    ResultCode DeviceFence::WaitOnCpu() const
    {
        if (!ValidateIsInitialized())
        {
            return ResultCode::InvalidOperation;
        }

        AZ_PROFILE_SCOPE(RHI, "DeviceFence: WaitOnCpu");
        WaitOnCpuInternal();
        return ResultCode::Success;
    }

    ResultCode DeviceFence::WaitOnCpuAsync(SignalCallback callback)
    {
        if (!ValidateIsInitialized())
        {
            return ResultCode::InvalidOperation;
        }

        if (!callback)
        {
            AZ_Error("DeviceFence", false, "Callback is null.");
            return ResultCode::InvalidOperation;
        }

        if (m_waitThread.joinable())
        {
            m_waitThread.join();
        }

        AZStd::thread_desc threadDesc{ "DeviceFence WaitOnCpu Thread" };

        m_waitThread = AZStd::thread(threadDesc, [this, callback]()
        {
            ResultCode resultCode = WaitOnCpu();
            if (resultCode != ResultCode::Success)
            {
                AZ_Error("DeviceFence", false, "Failed to call WaitOnCpu in async thread.");
            }
            callback();
        });

        return ResultCode::Success;
    }

    ResultCode DeviceFence::Reset()
    {
        if (!ValidateIsInitialized())
        {
            return ResultCode::InvalidOperation;
        }

        ResetInternal();
        return ResultCode::Success;
    }

    FenceState DeviceFence::GetFenceState() const
    {
        if (!ValidateIsInitialized())
        {
            return FenceState::Reset;
        }

        return GetFenceStateInternal();
    }
}

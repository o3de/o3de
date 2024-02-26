/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/SingleDeviceFence.h>

#include <AzCore/Debug/Profiler.h>

namespace AZ::RHI
{
    SingleDeviceFence::~SingleDeviceFence() {}

    bool SingleDeviceFence::ValidateIsInitialized() const
    {
        if (Validation::IsEnabled())
        {
            if (!IsInitialized())
            {
                AZ_Error("SingleDeviceFence", false, "SingleDeviceFence is not initialized!");
                return false;
            }
        }

        return true;
    }

    ResultCode SingleDeviceFence::Init(Device& device, FenceState initialState)
    {
        if (Validation::IsEnabled())
        {
            if (IsInitialized())
            {
                AZ_Error("SingleDeviceFence", false, "SingleDeviceFence is already initialized!");
                return ResultCode::InvalidOperation;
            }
        }

        const ResultCode resultCode = InitInternal(device, initialState);

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

    void SingleDeviceFence::Shutdown()
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

    ResultCode SingleDeviceFence::SignalOnCpu()
    {
        if (!ValidateIsInitialized())
        {
            return ResultCode::InvalidOperation;
        }

        SignalOnCpuInternal();
        return ResultCode::Success;
    }

    ResultCode SingleDeviceFence::WaitOnCpu() const
    {
        if (!ValidateIsInitialized())
        {
            return ResultCode::InvalidOperation;
        }

        AZ_PROFILE_SCOPE(RHI, "SingleDeviceFence: WaitOnCpu");
        WaitOnCpuInternal();
        return ResultCode::Success;
    }

    ResultCode SingleDeviceFence::WaitOnCpuAsync(SignalCallback callback)
    {
        if (!ValidateIsInitialized())
        {
            return ResultCode::InvalidOperation;
        }

        if (!callback)
        {
            AZ_Error("SingleDeviceFence", false, "Callback is null.");
            return ResultCode::InvalidOperation;
        }

        if (m_waitThread.joinable())
        {
            m_waitThread.join();
        }

        AZStd::thread_desc threadDesc{ "SingleDeviceFence WaitOnCpu Thread" };

        m_waitThread = AZStd::thread(threadDesc, [this, callback]()
        {
            ResultCode resultCode = WaitOnCpu();
            if (resultCode != ResultCode::Success)
            {
                AZ_Error("SingleDeviceFence", false, "Failed to call WaitOnCpu in async thread.");
            }
            callback();
        });

        return ResultCode::Success;
    }

    ResultCode SingleDeviceFence::Reset()
    {
        if (!ValidateIsInitialized())
        {
            return ResultCode::InvalidOperation;
        }

        ResetInternal();
        return ResultCode::Success;
    }

    FenceState SingleDeviceFence::GetFenceState() const
    {
        if (!ValidateIsInitialized())
        {
            return FenceState::Reset;
        }

        return GetFenceStateInternal();
    }
}

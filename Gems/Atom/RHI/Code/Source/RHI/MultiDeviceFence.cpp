/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/MultiDeviceFence.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHISystemInterface.h>

#include <AzCore/Debug/Profiler.h>

namespace AZ::RHI
{
    bool MultiDeviceFence::ValidateIsInitialized() const
    {
        if (Validation::IsEnabled())
        {
            if (!IsInitialized())
            {
                AZ_Error("MultiDeviceFence", false, "MultiDeviceFence is not initialized!");
                return false;
            }
        }

        return true;
    }

    ResultCode MultiDeviceFence::Init(MultiDevice::DeviceMask deviceMask, FenceState initialState)
    {
        if (Validation::IsEnabled())
        {
            if (IsInitialized())
            {
                AZ_Error("MultiDeviceFence", false, "MultiDeviceFence is already initialized!");
                return ResultCode::InvalidOperation;
            }
        }

        MultiDeviceObject::Init(deviceMask);

        ResultCode resultCode = ResultCode::Success;

        IterateDevices(
            [this, initialState, &resultCode](int deviceIndex)
            {
                auto* device = RHISystemInterface::Get()->GetDevice(deviceIndex);

                m_deviceFences[deviceIndex] = Factory::Get().CreateFence();
                resultCode = m_deviceFences[deviceIndex]->Init(*device, initialState);

                return resultCode == ResultCode::Success;
            });

        if (resultCode != ResultCode::Success)
        {
            // Reset already initialized device-specific Fences and set deviceMask to 0
            m_deviceFences.clear();
            MultiDeviceObject::Init(static_cast<MultiDevice::DeviceMask>(0u));
        }

        return resultCode;
    }

    void MultiDeviceFence::Shutdown()
    {
        if (IsInitialized())
        {
            if (m_waitThread.joinable())
            {
                m_waitThread.join();
            }

            for (auto& [deviceIndex, deviceFence] : m_deviceFences)
            {
                deviceFence->Shutdown();
            }

            MultiDeviceObject::Shutdown();
        }
    }

    ResultCode MultiDeviceFence::SignalOnCpu()
    {
        if (!ValidateIsInitialized())
        {
            return ResultCode::InvalidOperation;
        }

        ResultCode resultCode;

        for (auto& [deviceIndex, deviceFence] : m_deviceFences)
        {
            resultCode = deviceFence->SignalOnCpu();

            if (resultCode != ResultCode::Success)
            {
                return resultCode;
            }
        }

        return ResultCode::Success;
    }

    ResultCode MultiDeviceFence::WaitOnCpu() const
    {
        if (!ValidateIsInitialized())
        {
            return ResultCode::InvalidOperation;
        }

        ResultCode resultCode;

        for (auto& [deviceIndex, deviceFence] : m_deviceFences)
        {
            resultCode = deviceFence->WaitOnCpu();

            if (resultCode != ResultCode::Success)
            {
                return resultCode;
            }
        }

        return ResultCode::Success;
    }

    ResultCode MultiDeviceFence::WaitOnCpuAsync(SignalCallback callback)
    {
        if (!ValidateIsInitialized())
        {
            return ResultCode::InvalidOperation;
        }

        if (!callback)
        {
            AZ_Error("MultiDeviceFence", false, "Callback is null.");
            return ResultCode::InvalidOperation;
        }

        if (m_waitThread.joinable())
        {
            m_waitThread.join();
        }

        AZStd::thread_desc threadDesc{ "MultiDeviceFence WaitOnCpu Thread" };

        m_waitThread = AZStd::thread(
            threadDesc,
            [this, callback]()
            {
                ResultCode resultCode = WaitOnCpu();
                if (resultCode != ResultCode::Success)
                {
                    AZ_Error("MultiDeviceFence", false, "Failed to call WaitOnCpu in async thread.");
                }
                callback();
            });

        return ResultCode::Success;
    }

    ResultCode MultiDeviceFence::Reset()
    {
        if (!ValidateIsInitialized())
        {
            return ResultCode::InvalidOperation;
        }

        ResultCode resultCode;

        for (auto& [deviceIndex, deviceFence] : m_deviceFences)
        {
            resultCode = deviceFence->Reset();

            if (resultCode != ResultCode::Success)
            {
                return resultCode;
            }
        }

        return ResultCode::Success;
    }
} // namespace AZ::RHI
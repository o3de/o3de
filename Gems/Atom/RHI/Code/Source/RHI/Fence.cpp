/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Fence.h>

#include <Atom/RHI/DeviceFence.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHISystemInterface.h>

#include <AzCore/Debug/Profiler.h>

namespace AZ
{
    namespace RHI
    {
        Fence::~Fence()
        {
        }

        bool Fence::ValidateIsInitialized() const
        {
            if (Validation::IsEnabled())
            {
                if (!IsInitialized())
                {
                    AZ_Error("Fence", false, "Fence is not initialized!");
                    return false;
                }
            }

            return true;
        }

        ResultCode Fence::Init(DeviceMask deviceMask, FenceState initialState)
        {
            if (Validation::IsEnabled())
            {
                if (IsInitialized())
                {
                    AZ_Error("Fence", false, "Fence is already initialized!");
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
                AZ_Assert(false, "Failed to create a fence");
            }

            return resultCode;
        }

        void Fence::Shutdown()
        {
            if (IsInitialized())
            {
                if (m_waitThread.joinable())
                {
                    m_waitThread.join();
                }

                for (auto& [deviceIndex, deviceFence] : m_deviceFences)
                    deviceFence->Shutdown();

                MultiDeviceObject::Shutdown();
            }
        }

        ResultCode Fence::SignalOnCpu()
        {
            ResultCode resultCode;

            for (auto& [deviceIndex, deviceFence] : m_deviceFences)
            {
                resultCode = deviceFence->SignalOnCpu();

                if (resultCode != ResultCode::Success)
                    return resultCode;
            }

            return ResultCode::Success;
        }

        ResultCode Fence::WaitOnCpu() const
        {
            ResultCode resultCode;

            for (auto& [deviceIndex, deviceFence] : m_deviceFences)
            {
                resultCode = deviceFence->WaitOnCpu();

                if (resultCode != ResultCode::Success)
                    return resultCode;
            }

            return ResultCode::Success;
        }

        ResultCode Fence::WaitOnCpuAsync(SignalCallback callback)
        {
            if (!ValidateIsInitialized())
            {
                return ResultCode::InvalidOperation;
            }

            if (!callback)
            {
                AZ_Error("Fence", false, "Callback is null.");
                return ResultCode::InvalidOperation;
            }

            if (m_waitThread.joinable())
            {
                m_waitThread.join();
            }

            AZStd::thread_desc threadDesc{ "Fence WaitOnCpu Thread" };

            m_waitThread = AZStd::thread(
                threadDesc,
                [this, callback]()
                {
                    ResultCode resultCode = WaitOnCpu();
                    if (resultCode != ResultCode::Success)
                    {
                        AZ_Error("Fence", false, "Failed to call WaitOnCpu in async thread.");
                    }
                    callback();
                });

            return ResultCode::Success;
        }

        const Ptr<DeviceFence>& Fence::GetDeviceFence(int deviceIndex)
        {
            return m_deviceFences.at(deviceIndex);
        }

        ResultCode Fence::Reset()
        {
            ResultCode resultCode;

            for (auto& [deviceIndex, deviceFence] : m_deviceFences)
            {
                resultCode = deviceFence->Reset();

                if (resultCode != ResultCode::Success)
                    return resultCode;
            }

            return ResultCode::Success;
        }
    }
}

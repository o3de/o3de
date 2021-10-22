/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Fence.h>

#include <AzCore/Debug/Profiler.h>

namespace AZ
{
    namespace RHI
    {
        Fence::~Fence() {}

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

        ResultCode Fence::Init(Device& device, FenceState initialState)
        {
            if (Validation::IsEnabled())
            {
                if (IsInitialized())
                {
                    AZ_Error("Fence", false, "Fence is already initialized!");
                    return ResultCode::InvalidOperation;
                }
            }

            const ResultCode resultCode = InitInternal(device, initialState);

            if (resultCode == ResultCode::Success)
            {
                DeviceObject::Init(device);
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

                ShutdownInternal();
                DeviceObject::Shutdown();
            }
        }

        ResultCode Fence::SignalOnCpu()
        {
            if (!ValidateIsInitialized())
            {
                return ResultCode::InvalidOperation;
            }

            SignalOnCpuInternal();
            return ResultCode::Success;
        }

        ResultCode Fence::WaitOnCpu() const
        {
            if (!ValidateIsInitialized())
            {
                return ResultCode::InvalidOperation;
            }

            AZ_PROFILE_SCOPE(RHI, "Fence: WaitOnCpu");
            WaitOnCpuInternal();
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

            m_waitThread = AZStd::thread(threadDesc, [this, callback]()
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

        ResultCode Fence::Reset()
        {
            if (!ValidateIsInitialized())
            {
                return ResultCode::InvalidOperation;
            }

            ResetInternal();
            return ResultCode::Success;
        }

        FenceState Fence::GetFenceState() const
        {
            if (!ValidateIsInitialized())
            {
                return FenceState::Reset;
            }

            return GetFenceStateInternal();
        }
    }
}

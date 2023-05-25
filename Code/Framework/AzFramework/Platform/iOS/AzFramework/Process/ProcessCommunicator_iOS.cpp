/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzFramework/Process/ProcessCommunicator.h>

namespace AzFramework
{
    AZ::u32 StdInOutCommunication::PeekHandle([[maybe_unused]] StdProcessCommunicatorHandle& handle)
    {
        return 0;
    }

    AZ::u32 StdInOutCommunication::ReadDataFromHandle([[maybe_unused]] StdProcessCommunicatorHandle& handle, [[maybe_unused]] void* readBuffer, [[maybe_unused]] AZ::u32 bufferSize)
    {
        return 0;
    }

    AZ::u32 StdInOutCommunication::WriteDataToHandle([[maybe_unused]] StdProcessCommunicatorHandle& handle, [[maybe_unused]] const void* writeBuffer, [[maybe_unused]] AZ::u32 bytesToWrite)
    {
        return 0;
    }

    bool StdInOutProcessCommunicator::CreatePipesForProcess([[maybe_unused]] ProcessData* processData)
    {
        return false;
    }

    void StdInOutProcessCommunicator::WaitForReadyOutputs([[maybe_unused]] OutputStatus& status)
    {
        status.outputDeviceReady = false;
        status.errorsDeviceReady = false;
        status.shouldReadOutput = status.shouldReadErrors = false;
    }

    bool StdInOutProcessCommunicatorForChildProcess::AttachToExistingPipes()
    {
        return false;
    }
} // namespace AzFramework


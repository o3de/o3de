/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

    void StdInOutProcessCommunicator::WaitForReadyOutputs([[maybe_unused]] OutputStatus& status) const
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


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

#include <AzCore/Platform.h>
#include <AzCore/Math/Sha1.h>

#include <sys/types.h>
#include <unistd.h>

namespace AZ
{
    namespace Platform
    {
        ProcessId GetCurrentProcessId()
        {
            return static_cast<ProcessId>(::getpid());
        }

        MachineId GetLocalMachineId()
        {
            if (s_machineId == 0)
            {
                // Fetch a machine guid from the OS. Apple says this is guaranteed to be unique enough for
                // licensing purposes (so that should be good enough for us here), but that
                // the hardware ids which generate this can be modified (it most likely includes MAC addr)
                uuid_t hostId = { 0 };
                timespec wait;
                wait.tv_sec = 0;
                wait.tv_nsec = 5000;
                int result = ::gethostuuid(hostId, &wait);
                AZ_Error("System", result == 0, "gethostuuid() failed with code %d", result);  
                Sha1 hash;
                AZ::u32 digest[5] = { 0 };
                hash.ProcessBytes(&hostId, sizeof(hostId));
                hash.GetDigest(digest);
                s_machineId = digest[0];

                if (s_machineId == 0)
                {
                    s_machineId = 1;
                    AZ_Warning("System", false, "0 machine ID is reserved!");
                }
            }
            return s_machineId;
        }
    } // namespace Platform
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>

namespace AZ
{
    namespace Platform
    {
        size_t GetHeapCapacity()
        {
            SYSTEM_INFO si;
            GetSystemInfo(&si);
            return (char*)si.lpMaximumApplicationAddress - (char*)si.lpMinimumApplicationAddress;
        }
    }
}

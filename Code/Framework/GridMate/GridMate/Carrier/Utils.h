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
#ifndef GM_CARRIER_UTILS_H
#define GM_CARRIER_UTILS_H

#include <GridMate/String/string.h>

namespace GridMate
{
    namespace Utils
    {
        ///< Returns the machines address(ip) in a string. familyType is platform dependent.
        string GetMachineAddress(int familyType = 0);
        ///< Returns a broadcast address based on a family type. On ipv6 we emulate broadbast using the multicast on the all nodes address (FF02::1). familyType is platform dependent.
        const char* GetBroadcastAddress(int familyType = 0);
    }
}

#endif // GM_DEFAULT_TRAFFIC_CONTROL_H

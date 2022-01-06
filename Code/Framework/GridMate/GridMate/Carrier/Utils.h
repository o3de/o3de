/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_CARRIER_UTILS_H
#define GM_CARRIER_UTILS_H

#include <AzCore/std/string/string.h>

namespace GridMate
{
    namespace Utils
    {
        ///< Returns the machines address(ip) in a string. familyType is platform dependent.
        AZStd::string GetMachineAddress(int familyType = 0);
        ///< Returns a broadcast address based on a family type. On ipv6 we emulate broadbast using the multicast on the all nodes address (FF02::1). familyType is platform dependent.
        const char* GetBroadcastAddress(int familyType = 0);
    }
}

#endif // GM_DEFAULT_TRAFFIC_CONTROL_H

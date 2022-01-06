/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <unistd.h>

#include <AzCore/std/string/string.h>

namespace GridMate
{
    namespace Platform
    {
        void AssignExtendedName(AZStd::string& extendedName)
        {
            char hostName[64];
            gethostname(hostName, AZ_ARRAY_SIZE(hostName));

            extendedName = AZStd::string::format("%s", hostName);
        }
    }
}

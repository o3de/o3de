/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/FileIO.h>

namespace AZ::IO
{
    // On posix compatible systems, file times are in time_t, that is, 
    // number of seconds since posix epoch.  Converting this just means converting from
    // seconds to milliseconds
    AZ::u64 FileTimeToMSecsSincePosixEpoch(AZ::u64 fileTime)
    { 
        return fileTime * 1000;
    }
}

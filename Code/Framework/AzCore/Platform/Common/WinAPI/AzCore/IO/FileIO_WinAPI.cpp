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
    // There are no leap seconds from the start of the win32 file epoch
    // which begins Jan 1 1601, until the posix epoch begins.  So to convert
    // it can just be offset.
    // see Code\Legacy\CrySystem\LocalizedStringManager.cpp for example of where these constants
    // came from.
    // it represents the number of 100-nanosecond intervals between the two epocs.
    // since win32 file time is in 100-nanosecond intervals.
    AZ::u64 FileTimeToMSecsSincePosixEpoch(AZ::u64 fileTime)
    { 
        static constexpr const AZ::u64 differenceBetweenEpochs = 116444736000000000;
        if (fileTime <= differenceBetweenEpochs)
        {
            return 0; // before the epoch
        }
        
        fileTime = fileTime - differenceBetweenEpochs; 
        // convert second      to millisecond = * 1,000
        // convert millisecond to millisecond = * 1
        // convert microsecond to millisecond = / 1,000
        // convert nanosecond  to millisecond = / 1,000,000 , therefore
        // convert 100 nanosec to millisecond = /    10,000   (1,000,000 / 100)
        fileTime = fileTime / 10000; // convert from 100-nanosecond interval to seconds
        return fileTime;
    }
}

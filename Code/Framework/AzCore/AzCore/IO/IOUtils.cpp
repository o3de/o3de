/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/base.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/std/parallel/thread.h> /// this_thread sleep_for.

namespace AZ::IO
{
    bool RetryOpenStream(FileIOStream& stream, int numRetries, int delayBetweenRetry)
    {
        while ((!stream.IsOpen()) && (numRetries > 0))
        {
            numRetries--;
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(delayBetweenRetry));
            stream.ReOpen();
        }
        return stream.IsOpen();
    }
} // namespace AZ::IO

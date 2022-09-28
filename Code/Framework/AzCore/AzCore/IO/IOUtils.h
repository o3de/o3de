/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace AZ
{
    namespace IO
    {        
        class FileIOStream;

        /**
        * ReOpen a stream until it opens - this can help avoid transient problems where the OS has a brief
        * lock on a file stream.
        * returns false if the stream is not open at the end of the retries.
        */
        bool RetryOpenStream(FileIOStream& stream, int numRetries = 10, int delayBetweenRetry = 250);
    }   // namespace IO
   
}   // namespace AZ

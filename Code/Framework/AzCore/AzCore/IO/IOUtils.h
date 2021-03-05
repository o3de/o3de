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
#pragma once

namespace AZ
{
    namespace IO
    {        
        class FileIOStream;

        int TranslateOpenModeToSystemFileMode(const char* path, OpenMode mode);

        /**
        * ReOpen a stream until it opens - this can help avoid transient problems where the OS has a brief
        * lock on a file stream.
        * returns false if the stream is not open at the end of the retries.
        */
        bool RetryOpenStream(FileIOStream& stream, int numRetries = 10, int delayBetweenRetry = 250);
    }   // namespace IO
   
}   // namespace AZ

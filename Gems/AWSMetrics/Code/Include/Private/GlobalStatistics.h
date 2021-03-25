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

#include <stdint.h>

#include <AzCore/std/parallel/atomic.h>

namespace AWSMetrics
{
    //! GlobalStatistics is used to store the statistics for sending metrics to the backend or local file.
    struct GlobalStatistics
    {
        //! Returns the total number of bytes sent to the backend or local file.
        AZStd::atomic<uint32_t> m_sendSizeInBytes = 0;

        //! Returns the total number of metrics events to the backend or local file.
        AZStd::atomic<uint32_t> m_numEvents = 0;

        //! Return the total number of metrics events failed to be sent to the backend or local file.
        AZStd::atomic<uint32_t> m_numErrors = 0;

        //! Returns the total number of metrics events sent successfully to the backend or local file.
        AZStd::atomic<uint32_t> m_numSuccesses = 0;

        //! Returns the total number of metrics events which failed the JSON schema validation or reached the maximum number of retries.
        AZStd::atomic<uint32_t> m_numDropped = 0;
    };
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

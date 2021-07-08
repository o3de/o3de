/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "ThreadHelper.h"

static AZ_THREAD_LOCAL AZ::s64 s_currentJobId = 0;

namespace AssetProcessor
{
    AZ::s64 GetThreadLocalJobId()
    {
        return s_currentJobId;
    }

    void SetThreadLocalJobId(AZ::s64 jobId)
    {
        s_currentJobId = jobId;
    }
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/O3DEKernelConfiguration.h>
#include <AzCore/Debug/Trace.h>

namespace AZ::Debug
{
    ITrace::ITrace()
    {
        s_tracer = this;
    }

    ITrace::~ITrace()
    {
        s_tracer = nullptr;
    }

    ITrace& ITrace::Instance()
    {
        if (!s_tracer)
        {
            static ITrace defaultTracer;
        }
        return *s_tracer;
    }
} // namespace AZ::Debug

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/PlatformDef.h>
#include <AzCore/Debug/Profiler.h>

namespace AZStd
{
    struct thread_id;
}

namespace AZ
{
    namespace Debug
    {
        namespace EventTrace
        {
            class ScopedSlice
            {
            public:
                ScopedSlice(const char* name, const char* category);
                ~ScopedSlice();

            private:
                const char* m_Name;
                const char* m_Category;
                u64 m_Time;
            };
        }
    }
}

#define AZ_TRACE_METHOD_NAME_CATEGORY(name, category)
#define AZ_TRACE_METHOD_NAME(name) AZ_TRACE_METHOD_NAME_CATEGORY(name, "")
#define AZ_TRACE_METHOD() AZ_TRACE_METHOD_NAME(AZ_FUNCTION_SIGNATURE)

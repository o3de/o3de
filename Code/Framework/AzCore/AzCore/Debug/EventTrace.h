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

#define AZ_TRACE_METHOD_NAME_CATEGORY(name, category) AZ::Debug::EventTrace::ScopedSlice AZ_JOIN(ScopedSlice__, __LINE__)(name, category);

#ifdef AZ_PROFILE_TELEMETRY
#   define AZ_TRACE_METHOD_NAME(name) \
        AZ_TRACE_METHOD_NAME_CATEGORY(name, "") \
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzTrace, name)

#   define AZ_TRACE_METHOD() \
        AZ_TRACE_METHOD_NAME_CATEGORY(AZ_FUNCTION_SIGNATURE, "") \
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzTrace)
#else
#   define AZ_TRACE_METHOD_NAME(name) AZ_TRACE_METHOD_NAME_CATEGORY(name, "")
#   define AZ_TRACE_METHOD() AZ_TRACE_METHOD_NAME(AZ_FUNCTION_SIGNATURE)
#endif

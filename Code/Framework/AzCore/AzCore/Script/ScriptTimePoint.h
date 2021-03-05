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
#ifndef AZCORE_TIMEPOINT_H_
#define AZCORE_TIMEPOINT_H_

#include <AzCore/PlatformDef.h>
#include <AzCore/std/chrono/clocks.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    class ReflectContext;

    AZ_TYPE_INFO_SPECIALIZE(AZStd::chrono::system_clock::time_point, "{5C48FD59-7267-405D-9C06-1EA31379FE82}");


    /**
    * Wrapper that reflects a AZStd::chrono::system_clock::time_point to script.
    */
    class ScriptTimePoint
    {
    public:
        AZ_TYPE_INFO(ScriptTimePoint, "{4c0f6ad4-0d4f-4354-ad4a-0c01e948245c}");
        AZ_CLASS_ALLOCATOR(ScriptTimePoint, SystemAllocator, 0);

        ScriptTimePoint()
            : m_timePoint(AZStd::chrono::system_clock::now()) {}
        explicit ScriptTimePoint(AZStd::chrono::system_clock::time_point timePoint)
            : m_timePoint(timePoint) {}

        AZStd::string ToString() const
        {
            return AZStd::string::format("Time %llu", m_timePoint.time_since_epoch().count());
        }

        const AZStd::chrono::system_clock::time_point& Get() { return m_timePoint; }

        // Returns the time point in seconds
        double GetSeconds()
        {
            typedef AZStd::chrono::duration<double> double_seconds;
            return AZStd::chrono::duration_cast<double_seconds>(m_timePoint.time_since_epoch()).count();
        }

        // Returns the time point in milliseconds
        double GetMilliseconds()
        {
            typedef AZStd::chrono::duration<double, AZStd::milli> double_ms;
            return AZStd::chrono::duration_cast<double_ms>(m_timePoint.time_since_epoch()).count();
        }

        static void Reflect(ReflectContext* reflection);

    protected:

        AZStd::chrono::system_clock::time_point m_timePoint;
    };
}


#endif // AZCORE_TIMEPOINT_H_

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_TIMEPOINT_H_
#define AZCORE_TIMEPOINT_H_

#include <AzCore/PlatformDef.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    class ReflectContext;

    AZ_TYPE_INFO_SPECIALIZE(AZStd::chrono::steady_clock::time_point, "{5C48FD59-7267-405D-9C06-1EA31379FE82}");


    //! Wrapper that reflects a AZStd::chrono::steady_clock::time_point to script.
    class ScriptTimePoint
    {
    public:
        AZ_TYPE_INFO(ScriptTimePoint, "{4c0f6ad4-0d4f-4354-ad4a-0c01e948245c}");
        AZ_CLASS_ALLOCATOR(ScriptTimePoint, SystemAllocator);

        ScriptTimePoint()
            : m_timePoint(AZStd::chrono::steady_clock::now()) {}
        explicit ScriptTimePoint(AZStd::chrono::steady_clock::time_point timePoint)
            : m_timePoint(timePoint) {}

        //! Formats the time point in a string formatted as: "Time <seconds since epoch>".
        AZStd::string ToString() const;

        //! Returns the time point.
        const AZStd::chrono::steady_clock::time_point& Get() const;

        //! Returns the time point in seconds
        double GetSeconds() const;

        //! Returns the time point in milliseconds
        double GetMilliseconds() const;

        static void Reflect(ReflectContext* reflection);

    protected:
        AZStd::chrono::steady_clock::time_point m_timePoint;
    };

    inline AZStd::string ScriptTimePoint::ToString() const
    {
        return AZStd::string::format("Time %lld", static_cast<long long>(m_timePoint.time_since_epoch().count()));
    }

    inline const AZStd::chrono::steady_clock::time_point& ScriptTimePoint::Get() const
    {
        return m_timePoint;
    }

    inline double ScriptTimePoint::GetSeconds() const
    {
        typedef AZStd::chrono::duration<double> double_seconds;
        return AZStd::chrono::duration_cast<double_seconds>(m_timePoint.time_since_epoch()).count();
    }

    inline double ScriptTimePoint::GetMilliseconds() const
    {
        typedef AZStd::chrono::duration<double, AZStd::milli> double_ms;
        return AZStd::chrono::duration_cast<double_ms>(m_timePoint.time_since_epoch()).count();
    }
}


#endif // AZCORE_TIMEPOINT_H_

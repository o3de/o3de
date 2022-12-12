/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/chrono/chrono.h>
#include <AzCore/RTTI/TypeInfo.h>

namespace AZ
{
    class ReflectContext;

    //! OnDemand reflection for AZStd::chrono::duration specializations
    //! Currently supported specializations are nanoseconds,m milliseconds microseconds and seconds
    //!
    //!
    template<class T>
    struct OnDemandReflection;

    template<class Rep, class Period>
    struct OnDemandReflection<AZStd::chrono::duration<Rep, Period>>
    {
        using DurationType = AZStd::chrono::duration<Rep, Period>;

        static void Reflect(AZ::ReflectContext* context);
    };

    // extern common second duration types
    extern template struct OnDemandReflection<AZStd::chrono::nanoseconds>;
    extern template struct OnDemandReflection<AZStd::chrono::milliseconds>;
    extern template struct OnDemandReflection<AZStd::chrono::microseconds>;
    extern template struct OnDemandReflection<AZStd::chrono::seconds>;


    // Specialize AzTypeInfo for the second duration types, so that they can be reflected to the BehaviorContext
    AZ_TYPE_INFO_SPECIALIZE(AZStd::chrono::nanoseconds, "{5CEA0194-A74C-448A-AC10-3F7A6A8EEFB4}");
    AZ_TYPE_INFO_SPECIALIZE(AZStd::chrono::microseconds, "{1BF1728B-E0EB-4AA1-9937-7D1D7869E398}");
    AZ_TYPE_INFO_SPECIALIZE(AZStd::chrono::milliseconds, "{414B61DC-8042-44DC-82A5-C70DFE04CB65}");
    AZ_TYPE_INFO_SPECIALIZE(AZStd::chrono::seconds, "{63D98FB5-0802-4850-96F6-71046DF38C94}");
}

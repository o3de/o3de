/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/ChronoReflection.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    //! OnDemand reflection for AZStd::chrono::duration

    template<class Rep, class Period>
    void OnDemandReflection<AZStd::chrono::duration<Rep, Period>>::Reflect(AZ::ReflectContext* context)
    {
        if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context);
            behaviorContext != nullptr)
        {
            auto GetDisplayName = []() -> const char*
            {
                if constexpr (AZStd::same_as<DurationType, AZStd::chrono::milliseconds>)
                {
                    return "nanoseconds";
                }
                else if constexpr (AZStd::same_as<DurationType, AZStd::chrono::microseconds>)
                {
                    return "microseconds";
                }
                else if constexpr (AZStd::same_as<DurationType, AZStd::chrono::milliseconds>)
                {
                    return "milliseconds";
                }
                else if constexpr (AZStd::same_as<DurationType, AZStd::chrono::seconds>)
                {
                    return "seconds";
                }
                else
                {
                    return "";
                }
            };
            auto GetTooltip = []() -> const char*
            {
                if constexpr (AZStd::same_as<DurationType, AZStd::chrono::milliseconds>)
                {
                    return "Represents a time interval that counts nanosecond amount of ticks";
                }
                else if constexpr (AZStd::same_as<DurationType, AZStd::chrono::microseconds>)
                {
                    return "Represents a time interval that counts microsecond amount of ticks";
                }
                else if constexpr (AZStd::same_as<DurationType, AZStd::chrono::milliseconds>)
                {
                    return "Represents a time interval that counts millisecond amount of ticks";
                }
                else if constexpr (AZStd::same_as<DurationType, AZStd::chrono::seconds>)
                {
                    return "Represents a time interval that counts second amount of ticks";
                }
                else
                {
                    return "Represents a time interval that counts the chrono::duration<DurationType> amount of ticks";
                }
            };

            behaviorContext->Class<DurationType>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "time")
                ->Attribute(AZ::Script::Attributes::Category, "Time")
                ->Attribute(AZ::ScriptCanvasAttributes::PrettyName, GetDisplayName)
                ->Attribute(AZ::Script::Attributes::ToolTip, GetTooltip)
                ->template Constructor<typename DurationType::rep>()
                ->Method("count", &DurationType::count)
                ->Attribute(AZ::Script::Attributes::ToolTip, "Return the tick count in the representation of the duration type")
                ;
        }
    }

    // Explicit instantations of common second chrono duration types
    template struct OnDemandReflection<AZStd::chrono::nanoseconds>;
    template struct OnDemandReflection<AZStd::chrono::microseconds>;
    template struct OnDemandReflection<AZStd::chrono::milliseconds>;
    template struct OnDemandReflection<AZStd::chrono::seconds>;
}


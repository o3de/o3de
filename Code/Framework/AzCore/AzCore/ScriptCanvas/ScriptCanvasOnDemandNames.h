/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/RTTI/RTTI.h>

namespace AZ
{
    class BehaviorContext;

    constexpr const char* k_accessElementNameUnchecked = "AtUnchecked";
    constexpr const char* k_accessElementName = "At";
    constexpr const char* k_sizeName = "Size";

    constexpr const char* k_iteratorConstructorName = "Iterate_VM";
    constexpr const char* k_iteratorGetKeyName = "GetKeyUnchecked";
    constexpr const char* k_iteratorIsNotAtEndName = "IsNotAtEnd";
    constexpr const char* k_iteratorModValueName = "ModValueUnchecked";
    constexpr const char* k_iteratorNextName = "Next";
    
    namespace ScriptCanvasOnDemandReflection
    {
        AZStd::string GetPrettyNameForAZTypeId(AZ::BehaviorContext& context, AZ::Uuid typeId);

        template<typename T>
        struct OnDemandPrettyName
        {
            static AZStd::string Get(AZ::BehaviorContext& context)
            {
                return GetPrettyNameForAZTypeId(context, azrtti_typeid<AZStd::remove_pointer_t<AZStd::decay_t<T>>>());
            }
        };

        template<typename T>
        struct OnDemandToolTip
        {
            static AZStd::string Get(AZ::BehaviorContext& context)
            {
                return OnDemandPrettyName<T>::Get(context);
            }
        };

        template<typename T>
        struct OnDemandCategoryName
        {
            static AZStd::string Get(AZ::BehaviorContext& context)
            {
                AZStd::string prettyName = OnDemandPrettyName<T>::Get(context);
                AZStd::size_t stringPos = prettyName.find_first_of("<");

                if (stringPos != AZStd::string::npos)
                {
                    prettyName = prettyName.substr(0, stringPos);
                }

                return prettyName;
            }
        };
    } // namespace ScriptCanvasOnDemandReflection
} // namespace AZ


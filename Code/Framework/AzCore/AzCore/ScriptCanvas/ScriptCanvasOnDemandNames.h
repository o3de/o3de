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


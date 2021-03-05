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

#include <AzCore/Preprocessor/Enum.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/string/fixed_string.h>


//! Call this macro in the same namespace where the AZ_ENUM was defined to generate ReflectEnumType() utility functions.
//! @param EnumTypeName an enum that was defined using one of the AZ_ENUM macros.
#define AZ_ENUM_DEFINE_REFLECT_UTILITIES(EnumTypeName)                                                                                                  \
                                                                                                                                                        \
    static void AZ_JOIN(EnumTypeName, Reflect)(AZ::SerializeContext& context)                                                                           \
    {                                                                                                                                                   \
        auto enumMaker = context.Enum<EnumTypeName>();                                                                                                  \
        for (auto&& member : AzEnumTraits<EnumTypeName>::Members)                                                                                       \
        {                                                                                                                                               \
            enumMaker.Value(member.m_string.data(), member.m_value);                                                                                    \
        }                                                                                                                                               \
    }                                                                                                                                                   \
                                                                                                                                                        \
    template <size_t Index>                                                                                                                             \
    void AZ_JOIN(EnumTypeName, ReflectValue)(AZ::BehaviorContext& context, AZStd::string_view typeName = {})                                            \
    {                                                                                                                                                   \
        if (typeName.empty())                                                                                                                           \
        {                                                                                                                                               \
            typeName = AzEnumTraits<EnumTypeName>::EnumName;                                                                                            \
        }                                                                                                                                               \
        constexpr auto& enumValueStringPair = AzEnumTraits<EnumTypeName>::Members[Index];                                                               \
        auto qualifiedEnumName = AZStd::fixed_string<256>::format("%.*s_%.*s", AZ_STRING_ARG(typeName), AZ_STRING_ARG(enumValueStringPair.m_string));   \
        context.Enum<aznumeric_cast<int>(enumValueStringPair.m_value)>(qualifiedEnumName.c_str());                                                      \
    }                                                                                                                                                   \
                                                                                                                                                        \
    template <size_t... Indices>                                                                                                                        \
    void AZ_JOIN(EnumTypeName, ReflectValues)(AZ::BehaviorContext& context, AZStd::string_view typeName, AZStd::index_sequence<Indices...>)             \
    {                                                                                                                                                   \
        (AZ_JOIN(EnumTypeName, ReflectValue)<Indices>(context, typeName), ...);                                                                         \
    }                                                                                                                                                   \
                                                                                                                                                        \
    void AZ_JOIN(EnumTypeName, Reflect)(AZ::BehaviorContext& context, AZStd::string_view typeName = {})                                                 \
    {                                                                                                                                                   \
        AZ_JOIN(EnumTypeName, ReflectValues)(context, typeName, AZStd::make_index_sequence<AzEnumTraits<EnumTypeName>::Members.size()>());              \
    }                                                                                                                                                   


/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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


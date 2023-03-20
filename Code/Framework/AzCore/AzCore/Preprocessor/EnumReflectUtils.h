/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
#define AZ_ENUM_DEFINE_REFLECT_UTILITIES(EnumTypeName)                                                                                             \
                                                                                                                                                   \
    inline void AZ_JOIN(EnumTypeName, Reflect)(AZ::SerializeContext& context)                                                                      \
    {                                                                                                                                              \
        auto enumMaker = context.Enum<EnumTypeName>();                                                                                             \
        for (auto&& member : AZ::AzEnumTraits<EnumTypeName>::Members)                                                                              \
        {                                                                                                                                          \
            enumMaker.Value(member.m_string.data(), member.m_value);                                                                               \
        }                                                                                                                                          \
    }                                                                                                                                              \
                                                                                                                                                   \
    template <size_t Index>                                                                                                                        \
    void AZ_JOIN(EnumTypeName, ReflectValue)(AZ::BehaviorContext& context, AZ::BehaviorContext::ClassBuilder<EnumTypeName>& enumTypeBuilder,       \
        AZStd::string_view typeName)                                                                                                               \
    {                                                                                                                                              \
        constexpr auto&& enumValue = AZ::AzEnumTraits<EnumTypeName>::Members[Index].m_value;                                                       \
        constexpr auto&& enumOptionName = AZ::AzEnumTraits<EnumTypeName>::Members[Index].m_string;                                                 \
        auto qualifiedEnumName = AZStd::fixed_string<256>::format("%.*s_%.*s", AZ_STRING_ARG(typeName), AZ_STRING_ARG(enumOptionName));            \
        /* Reflect the enum option as a global Behavior Context property of the form "<enum-type-name>_<enum-option-name>" */                      \
        context.Enum<enumValue>(qualifiedEnumName.c_str());                                                                                        \
        /* Reflect the enum option as a property on the Behavior Class <enum-type-name>. This allows <dot> syntax to be used to access the enum */ \
        /* i.e <enum-type-name>.<enum-option-name> */                                                                                              \
        enumTypeBuilder.Enum<enumValue>(enumOptionName.data());                                                                                    \
    }                                                                                                                                              \
                                                                                                                                                   \
    template <size_t... Indices>                                                                                                                   \
    void AZ_JOIN(EnumTypeName, ReflectValues)(AZ::BehaviorContext& context, AZ::BehaviorContext::ClassBuilder<EnumTypeName>& enumTypeBuilder,      \
        AZStd::string_view typeName, AZStd::index_sequence<Indices...>)                                                                            \
    {                                                                                                                                              \
        (AZ_JOIN(EnumTypeName, ReflectValue)<Indices>(context, enumTypeBuilder, typeName), ...);                                                   \
    }                                                                                                                                              \
                                                                                                                                                   \
    inline void AZ_JOIN(EnumTypeName, Reflect)(AZ::BehaviorContext& context, AZStd::string_view typeName = {})                                            \
    {                                                                                                                                              \
        if (typeName.empty())                                                                                                                      \
        {                                                                                                                                          \
            typeName = AZ::AzEnumTraits<EnumTypeName>::EnumName;                                                                                   \
        }                                                                                                                                          \
        /* Reflect the enum type as behavior class for storing constants of the enum option */                                                     \
        auto enumTypeBuilder = context.Class<EnumTypeName>();                                                                                      \
        AZ_JOIN(EnumTypeName, ReflectValues)(context, enumTypeBuilder, typeName,                                                                   \
        AZStd::make_index_sequence<AZ::AzEnumTraits<EnumTypeName>::Members.size()>());                                                             \
    }

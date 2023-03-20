/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <stdint.h>
#include <AzCore/base.h>
#include <AzCore/std/typetraits/is_integral.h>
#include <AzCore/std/utility/to_underlying.h>

namespace AZStd
{
    template <typename TYPE>
    struct is_type_safe_integral
    {
        static constexpr bool value = false;
    };
}

//! This implements a type-safe integral class.
//! The usage of an enum class prevents any potential negative performance impact that might occur with a wrapping struct.
//! Usage: AZ_TYPE_SAFE_INTEGRAL(TypeSafeClassName, int8_t/uint8_t/int16_t/uint16_t/int32_t/uint32_t/int64_t/uint64_t);
#define AZ_TYPE_SAFE_INTEGRAL(TYPE_NAME, BASE_TYPE)                                                                                                 \
    static_assert(AZStd::is_integral<BASE_TYPE>::value, "Underlying type must be an integral");                                                     \
    enum class TYPE_NAME : BASE_TYPE {};                                                                                                            \
    AZ_DEFINE_ENUM_ARITHMETIC_OPERATORS(TYPE_NAME)                                                                                                  \
    AZ_DEFINE_ENUM_RELATIONAL_OPERATORS(TYPE_NAME)                                                                                                  \
    AZ_DEFINE_ENUM_BITWISE_OPERATORS(TYPE_NAME)

//! This implements AZStd to_string methods for a type safe integral alias.
//! This must be placed in global scope (not in a namespace) and the TYPE_NAME should be fully qualified.
//! Usage: AZ_TYPE_SAFE_INTEGRAL_TOSTRING(TypeSafeClassName);
#define AZ_TYPE_SAFE_INTEGRAL_TOSTRING(TYPE_NAME)                                                                               \
    namespace AZStd                                                                                                                        \
    {                                                                                                                                      \
        template<class Str>                                                                                                                \
        void to_string(Str& str, const TYPE_NAME value)                                                                                    \
        {                                                                                                                                  \
            to_string(str, AZStd::to_underlying(value));                                                                                   \
        }                                                                                                                                  \
        inline AZStd::string to_string(const TYPE_NAME& val)                                                                               \
        {                                                                                                                                  \
            AZStd::string str;                                                                                                             \
            to_string(str, val);                                                                                                           \
            return str;                                                                                                                    \
        }                                                                                                                                  \
    }

//! This implements cvar binding methods for a type safe integral alias.
//! This must be placed in global scope (not in a namespace) and the TYPE_NAME should be fully qualified.
//! O3DE_DEPRECATED This macro no longer needs to be used
//! The ConsoleTypeHelpers automatically support cvar bindings for Enums
#define AZ_TYPE_SAFE_INTEGRAL_CVARBINDING(TYPE_NAME)

#define AZ_TYPE_SAFE_INTEGRAL_SERIALIZEBINDING(TYPE_NAME) \
    namespace AZStd \
    { \
        template <> \
        struct is_type_safe_integral<TYPE_NAME> \
        { \
            static constexpr bool value = true; \
        }; \
    }

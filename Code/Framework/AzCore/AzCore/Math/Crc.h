/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/std/hash.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/RTTI/TypeInfoSimple.h>

// Implementation with only 1 param (by default it should be string)
#define AZ_CRC_1(_1)    AZ::Crc32(_1)

// Deprecated (use AZ_CRC_CE instead): Implementation with 2 params (only the second parameter is used)
#define AZ_CRC_2(_1, _2) AZ::Crc32(AZ::u32(_2))

//! Generates a Crc32 value from a string representation.
//! This macro is @b not guaranteed to be evaluated at compile time if the version with one parameter is used, so it should only be used if
//! the parameter is not a constexpr value. If the parameter is a constexpr value, the AZ_CRC_CE macro should be used instead. The version
//! with two parameters is deprecated, also use AZ_CRC_CE instead.
//! @example
//! <ul>
//!   <li>AZ::Crc32 testValue = AZ_CRC("My string");</li>
//!   <li>AZ::Crc32 testValue = AZ_CRC(0x18fbd270);</li>
//!   <li>AZ::Crc32 testValue = AZ_CRC("My string", 0x18fbd270); // Deprecated, use AZ_CRC_CE("My string") instead</li>
//! </ul>
#define AZ_CRC(...) AZ_MACRO_SPECIALIZE(AZ_CRC_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

//! Generates a Crc32 value from a string representation at compile time.
//! This macro @b is guaranteed to be evaluated at compile time, so it should be used if the parameter is a constexpr value.
//! The "CE" postfix stands for consteval, which is the C++20 term used to mark immediate functions.
//! It can be used for any parameter that is convertible to either the uint32_t or string_view Crc constructor.
//! It works by using the Crc32 constructor to create a temp Crc32 and then using operator uint32_t to convert the parameter into uint32_t.
//! Since this code isn't using C++20 yet only integral types can be used as non-type-template parameters.
//! @example
//! <ul>
//!   <li>AZ::Crc32 testValue = AZ_CRC_CE("My string");</li>
//!   <li>constexpr AZ::Crc32 testValue = AZ_CRC_CE("My string");</li>
//!   <li>AZ::Crc32 testValue = AZ_CRC_CE(0x18fbd270);</li>
//!   <li>case AZ_CRC_CE("My string"): {} break;</li>
//! </ul>
#define AZ_CRC_CE(literal_) AZ::Internal::CompileTimeCrc32<static_cast<AZ::u32>(AZ::Crc32{ literal_ })>

namespace AZ
{
    class SerializeContext;

    /**
     * Class for all of our crc32 types, better than just using ints everywhere.
     */
    class Crc32
    {
    public:
        AZ_TYPE_INFO_WITH_NAME_DECL(Crc32)

        /**
         * Initializes to 0.
         */
        constexpr Crc32()
            : m_value(0)    {  }

        /**
         * Initializes from an int.
         */
        constexpr Crc32(AZ::u32 value) : m_value{ value } {}

        /**
         * Calculates the value from a string.
         */
        explicit constexpr Crc32(AZStd::string_view view);

        /**
         * Calculates the value from a block of raw data.
         */
        Crc32(const void* data, size_t size, bool forceLowerCase = false);
        template<class ByteType, class = AZStd::enable_if_t<sizeof(ByteType) == 1>>
        constexpr Crc32(const ByteType* data, size_t size, bool forceLowerCase = false);
        constexpr Crc32(AZStd::span<const AZStd::byte> inputSpan);

        constexpr void Add(AZStd::string_view view);
        void Add(const void* data, size_t size, bool forceLowerCase = false);
        template<class ByteType>
        constexpr auto Add(const ByteType* data, size_t size, bool forceLowerCase = false)
            -> AZStd::enable_if_t<sizeof(ByteType) == 1>;
        constexpr void Add(AZStd::span<const AZStd::byte> inputSpan);

        constexpr operator u32() const               { return m_value; }

        constexpr bool operator==(Crc32 rhs) const   { return (m_value == rhs.m_value); }
        constexpr bool operator!=(Crc32 rhs) const   { return (m_value != rhs.m_value); }

        constexpr bool operator!() const             { return (m_value == 0); }

        static void Reflect(AZ::SerializeContext& context);

    protected:
        // A constant expression cannot contain a conversion from const-volatile void to any pointer to object type
        // nor can it contain a reinterpret_cast, therefore overloads for const char* and uint8_t are added
        void Set(const void* data, size_t size, bool forceLowerCase = false);
        template<class ByteType>
        constexpr auto Set(const ByteType* data, size_t size, bool forceLowerCase = false)
            -> AZStd::enable_if_t<sizeof(ByteType) == 1>;
        constexpr void Combine(u32 crc, size_t len);

        u32 m_value;
    };
}

namespace AZStd
{
    template<>
    struct hash<AZ::Crc32>
    {
        size_t operator()(const AZ::Crc32& id) const
        {
            AZStd::hash<AZ::u32> hasher;
            return hasher(static_cast<AZ::u32>(id));
        }
    };
}

#include <AzCore/Math/Crc.inl>

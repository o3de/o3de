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
#include <AzCore/std/ranges/ranges.h>
#include <AzCore/std/string/fixed_string.h>

#if AZ_TRAIT_UUID_SUPPORTS_GUID_CONVERSION
struct  _GUID;
typedef _GUID GUID;
#endif


namespace AZ
{
    struct Uuid
    {
        enum Variant
        {
            VAR_UNKNOWN = -1,
            VAR_NCS = 0, // 0 - -
            VAR_RFC_4122 = 2, // 1 0 -
            VAR_MICROSOFT = 6, // 1 1 0
            VAR_RESERVED = 7  // 1 1 1
        };
        enum Version
        {
            VER_UNKNOWN = -1,
            VER_TIME = 1, // 0 0 0 1
            VER_DCE = 2, // 0 0 1 0
            VER_NAME_MD5 = 3, // 0 0 1 1
            VER_RANDOM = 4, // 0 1 0 0
            VER_NAME_SHA1 = 5, // 0 1 0 1
            // AZ Custom version, same as VER_RANDOM except we store a CRC32 in the first 4 bytes
            // you can add and combine in this 32 bytes.
            //VER_AZ_RANDOM_CRC32 = 6, // 0 1 1 0
        };

        static constexpr int ValidUuidStringLength = 32; /// Number of characters (data only, no extra formatting) in a valid UUID string
        static constexpr size_t MaxStringBuffer = 39; /// 32 Uuid + 4 dashes + 2 brackets + 1 terminate
        using FixedString = AZStd::fixed_string<MaxStringBuffer>;
        constexpr Uuid() = default;
        constexpr explicit Uuid(AZStd::string_view uuidString);
        constexpr Uuid(const char* string, size_t stringLength);

        static constexpr Uuid CreateNull();
        // In some cases it may be preferred to use a non-null, but clearly invalid UUID. For example, a material with a texture reference tracked
        // via UUID would use the null UUID for when no texture is set, but the Invalid UUID when the texture is set but does not resolve to an asset.
        static constexpr Uuid CreateInvalid();
        /// Create a Uuid (VAR_RFC_4122,VER_RANDOM)
        static Uuid Create();
        /**
        * This function accepts the following formats, if format is invalid it returns a NULL UUID.
        * 0123456789abcdef0123456789abcdef
        * 01234567-89ab-cdef-0123-456789abcdef
        * {01234567-89ab-cdef-0123-456789abcdef}
        * {0123456789abcdef0123456789abcdef}
        * \param string pointer to a string buffer
        * \param stringLength length of the string in the buffer
        */
        static constexpr Uuid CreateString(const char* string, size_t stringLength);
        static constexpr Uuid CreateString(AZStd::string_view uuidString);

        // Performs a first pass to handle more uuid string formats - allows and removes spaces, 0x, {, }
        static constexpr Uuid CreateStringPermissive(const char* string, size_t stringLength, bool skipWarnings = true);
        static constexpr Uuid CreateStringPermissive(AZStd::string_view uuidString, bool skipWarnings = true);

        static Uuid CreateRandom();
        /// Create a UUID based on a string name (sha1)
        static constexpr Uuid CreateName(AZStd::string_view name);
        /// Create a UUID based on a byte stream (sha1)
        static constexpr Uuid CreateData(const AZStd::byte* data, size_t dataSize);
        // Accepts a range whose value type is implicitly or explicitly convertible to an AZStd::byte
        template<class R>
        static constexpr auto CreateData(R&& dataSpan)
            -> AZStd::enable_if_t<AZStd::convertible_to<AZStd::ranges::range_value_t<R>, AZStd::byte>
            || decltype(static_cast<AZStd::byte>(AZStd::declval<AZStd::ranges::range_value_t<R>>()), AZStd::true_type())::value, Uuid>;

        constexpr bool IsNull() const;
        constexpr Variant GetVariant() const;
        constexpr Version GetVersion() const;
        /**
         * Outputs to a string in one of the following formats
         * 0123456789abcdef0123456789abcdef
         * 01234567-89ab-cdef-0123-456789abcdef
         * {01234567-89ab-cdef-0123-456789abcdef}
         * {0123456789abcdef0123456789abcdef}
         * \returns if positive number of characters written to the buffer (including terminate)
         * if negative the the number of characters required for output (nothing is writen to the output),
         * including terminating character.
         */
        constexpr int ToString(char* output, int outputSize, bool isBrackets = true, bool isDashes = true) const;

        /**
         * Outputs to a string in one of the following formats
         * 0123456789abcdef0123456789abcdef
         * 01234567-89ab-cdef-0123-456789abcdef
         * {01234567-89ab-cdef-0123-456789abcdef}
         * {0123456789abcdef0123456789abcdef}
         * \returns if positive number of characters written to the buffer (including terminate)
         * if negative the the number of characters required for output (nothing is writen to the output),
         * including terminating character.
         */
        template<size_t SizeT>
        constexpr int ToString(char(&output)[SizeT], bool isBrackets = true, bool isDashes = true) const;

        /// The only requirements is that StringType can be constructed from char* and it can copied.
        template<class StringType>
        constexpr StringType ToString(bool isBrackets = true, bool isDashes = true) const;

        /// For inplace version we require resize, data and size members.
        template<class StringType>
        constexpr void ToString(StringType& result, bool isBrackets = true, bool isDashes = true) const;

        constexpr FixedString ToFixedString(bool isBrackets = true, bool isDashes = true) const;

        constexpr bool operator==(const Uuid& rhs) const;
        constexpr bool operator!=(const Uuid& rhs) const;
        constexpr bool operator<(const Uuid& rhs) const;
        constexpr bool operator>(const Uuid& rhs) const;
        constexpr bool operator<=(const Uuid& rhs) const;
        constexpr bool operator>=(const Uuid& rhs) const;

#if AZ_TRAIT_UUID_SUPPORTS_GUID_CONVERSION
        // Add some conversion to from windows
        Uuid(const GUID& guid);
        operator GUID() const;
        Uuid& operator=(const GUID& guid);
        bool operator==(const GUID& guid) const;
        bool operator!=(const GUID& guid) const;
        friend bool operator==(const GUID& guid, const Uuid& uuid);
        friend bool operator!=(const GUID& guid, const Uuid& uuid);
#endif

        /// Adding two UUID generates SHA1 Uuid based on the data of both uuids
        friend constexpr Uuid operator+(const Uuid& lhs, const Uuid& rhs);
        constexpr Uuid& operator+=(const Uuid& rhs);

        //////////////////////////////////////////////////////////////////////////
        // AZStd interface
        using iterator = AZStd::byte*;
        using const_iterator = const AZStd::byte*;

        constexpr iterator begin();
        constexpr iterator end();
        constexpr const_iterator begin() const;
        constexpr const_iterator end() const;
        constexpr size_t size() const;
        //////////////////////////////////////////////////////////////////////////

        //! Returns the first 8 bytes of the Uuid interpreted as a little endian
        //! size_t value as the hash value
        constexpr size_t GetHash() const;

    private:
        static constexpr Uuid CreateStringSkipWarnings(AZStd::string_view uuidString, bool skipWarnings);

        //! Converts the next 8 bytes starting from startIndex into a size_t
        //! The startIndex represents the low byte of a little endian integer
        constexpr size_t GetSectionAsSizeT(size_t startIndex) const;
        //! Retrieves the first 8 bytes of the UUID(m_data[7] ... m_data[0]) as a size_t
        //! in little endian format
        constexpr size_t GetFirst8BytesAsSizeT() const;
        //! Retrieves the last 8 bytes of the UUID(m_data[15] ... m_data[8]) as a size_t
        //! in little endian format
        constexpr size_t GetLast8BytesAsSizeT() const;

        alignas(16) AZStd::byte m_data[16]{};
    };

//! O3DE_UUID_TO_NONTYPE_PARAM macro is for associating a Uuid with a non-type template parameter
//! It does this by taking the Hash Value of the TypeId which is the top 8 bytes of the Uuid
//! and converting using that as the template parameter.
//! The entire 16 byte Uuid can't be used until C++20 compiler support is available as literal class types
//! are not available for use as non-type parameters until then
//! An example of the use is as below
//! template<auto Uuid>
//! static bool VersionConverter(...)
//! template<>
//! static bool VersionConverter<O3DE_UUID_TO_NONTYPE_PARAM(AZ::Uuid("01234567-89ab-cdef-0123-456789abcdef"))>(...)
#define O3DE_UUID_TO_NONTYPE_PARAM(literal_) AZ::Uuid(literal_).GetHash()
} // namespace AZ

namespace AZStd
{
    // extern the fixed_string<39> class required to hold a Uuid string
    extern template class basic_fixed_string<char, AZ::Uuid::MaxStringBuffer>;

    // hash specialization
    template <>
    struct hash<AZ::Uuid>
    {
        constexpr size_t operator()(const AZ::Uuid& id) const
        {
            return id.GetHash();
        }
    };
}

#include <AzCore/Math/Uuid.inl>

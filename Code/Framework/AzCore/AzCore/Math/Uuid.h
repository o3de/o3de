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
#include <AzCore/Math/MathUtils.h>

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
            VAR_UNKNOWN         = -1,
            VAR_NCS             = 0, // 0 - -
            VAR_RFC_4122        = 2, // 1 0 -
            VAR_MICROSOFT       = 6, // 1 1 0
            VAR_RESERVED        = 7  // 1 1 1
        };
        enum Version
        {
            VER_UNKNOWN         = -1,
            VER_TIME            = 1, // 0 0 0 1
            VER_DCE             = 2, // 0 0 1 0
            VER_NAME_MD5        = 3, // 0 0 1 1
            VER_RANDOM          = 4, // 0 1 0 0
            VER_NAME_SHA1       = 5, // 0 1 0 1
            // AZ Custom version, same as VER_RANDOM except we store a CRC32 in the first 4 bytes
            // you can add and combine in this 32 bytes.
            //VER_AZ_RANDOM_CRC32 = 6, // 0 1 1 0
        };

        static constexpr int ValidUuidStringLength = 32; /// Number of characters (data only, no extra formatting) in a valid UUID string
        static const size_t MaxStringBuffer = 39; /// 32 Uuid + 4 dashes + 2 brackets + 1 terminate
        
        Uuid()  {}
        Uuid(const char* string, size_t stringLength = 0) { *this = CreateString(string, stringLength); }

        static Uuid CreateNull();
        /// Create a Uuid (VAR_RFC_4122,VER_RANDOM)
        static Uuid Create()        { return CreateRandom(); }
        /**
        * This function accepts the following formats, if format is invalid it returns a NULL UUID.
        * 0123456789abcdef0123456789abcdef
        * 01234567-89ab-cdef-0123-456789abcdef
        * {01234567-89ab-cdef-0123-456789abcdef}
        * {0123456789abcdef0123456789abcdef}
        * \param string pointer to a string buffer
        * \param stringLength if zero 'string' pointer must be null terminated so we can compute it's length, otherwise you can provide the length of the buffer.
        */
        static Uuid CreateString(const char* string, size_t stringLength = 0);
        static Uuid CreateStringSkipWarnings(const char* string, size_t stringLength, bool skipWarnings);

        // Performs a first pass to handle more uuid string formats - allows and removes spaces, 0x, {, }
        static Uuid CreateStringPermissive(const char* string, size_t stringLength = 0, bool skipWarnings = true);

        static Uuid CreateRandom();
        /// Create a UUID based on a string name (sha1)
        static Uuid CreateName(const char* name);
        /// Create a UUID based on a byte stream (sha1)
        static Uuid CreateData(const void* data, size_t dataSize);

        bool    IsNull() const;
        Variant GetVariant() const;
        Version GetVersion() const;
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
        int ToString(char* output, int outputSize, bool isBrackets = true, bool isDashes = true) const;

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
        int ToString(char(&output)[SizeT], bool isBrackets = true, bool isDashes = true) const
        {
            return ToString(output, SizeT, isBrackets, isDashes);
        }

        /// The only requirements is that StringType can be constructed from char* and it can copied.
        template<class StringType>
        inline StringType ToString(bool isBrackets = true, bool isDashes = true) const
        {
            char output[MaxStringBuffer];
            ToString(output, AZ_ARRAY_SIZE(output), isBrackets, isDashes);
            return StringType(output);
        }

        /// For inplace version we require resize, data and size members.
        template<class StringType>
        inline void ToString(StringType& result, bool isBrackets = true, bool isDashes = true) const
        {
            result.resize(-ToString(nullptr, 0, isBrackets, isDashes) - 1); // remove the terminating string
            ToString(&result[0], static_cast<int>(result.size()) + 1, isBrackets, isDashes);
        }

        AZ_MATH_INLINE bool operator==(const Uuid& rhs) const
        {
            const AZ::u64* lhs64 = reinterpret_cast<const AZ::u64*>(data);
            const AZ::u64* rhs64 = reinterpret_cast<const AZ::u64*>(rhs.data);
            if (lhs64[0] != rhs64[0] || lhs64[1] != rhs64[1])
            {
                return false;
            }
            return true;
        }
        AZ_MATH_INLINE bool operator!=(const Uuid& rhs) const { return !(*this == rhs); }
        bool operator<(const Uuid& rhs) const;
        bool operator>(const Uuid& rhs) const;
        bool operator<=(const Uuid& rhs) const { return !(*this > rhs); }
        bool operator>=(const Uuid& rhs) const { return !(*this < rhs); }

    #if AZ_TRAIT_UUID_SUPPORTS_GUID_CONVERSION
        // Add some conversion to from windows
        Uuid(const GUID& guid);
        operator GUID() const;
        AZ_MATH_INLINE Uuid& operator=(const GUID& guid)          { *this = Uuid(guid); return *this; }
        AZ_MATH_INLINE bool operator==(const GUID& guid) const    { return *this == Uuid(guid); }
        AZ_MATH_INLINE bool operator!=(const GUID& guid) const    { return !(*this == guid);  }
    #endif

        /// Adding two UUID generates SHA1 Uuid based on the data of both uuids
        Uuid operator+(const Uuid& rhs) const;
        AZ_MATH_INLINE Uuid& operator+=(const Uuid& rhs)      { *this = *this + rhs; return *this; }

        //////////////////////////////////////////////////////////////////////////
        // AZStd interface
        typedef unsigned char*          iterator;
        typedef const unsigned char*    const_iterator;

        AZ_MATH_INLINE iterator begin()               { return data; }
        AZ_MATH_INLINE iterator end()                 { return data + AZ_ARRAY_SIZE(data); }
        AZ_MATH_INLINE const_iterator begin() const   { return data; }
        AZ_MATH_INLINE const_iterator end() const     { return data + AZ_ARRAY_SIZE(data); }
        //////////////////////////////////////////////////////////////////////////

        size_t GetHash() const
        {
            // Returning first few bytes as a size_t. This is faster than
            // hashing every byte and the chance of collision is still very low.
            union
            {
                const size_t* hash;
                const unsigned char* data;
            } convert;

            convert.data = data;
            return *convert.hash;
        }

        // or _m128i and VMX ???
        alignas(16) unsigned char data[16];
    };
} // namespace AZ

namespace AZStd
{
    template<class T>
    struct hash;

    // hash specialization
    template <>
    struct hash<AZ::Uuid>
    {
        typedef AZ::Uuid    argument_type;
        typedef size_t      result_type;
        AZ_FORCE_INLINE size_t operator()(const AZ::Uuid& id) const
        {
            return id.GetHash();
        }
    };
}

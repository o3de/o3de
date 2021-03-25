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

#include <AzCore/RTTI/TypeSafeIntegral.h>

namespace AZ
{
    AZ_TYPE_SAFE_INTEGRAL(HashValue32, u32);
    AZ_TYPE_SAFE_INTEGRAL(HashValue64, u64);

    //! Hashes a contiguous array of bytes starting at buffer and ending at buffer + length
    //! @param[in] buffer pointer to the memory to be hashed
    //! @param[in] length the length in bytes of memory to read for generating the hash
    //! @return 32 bit resulting hash
    HashValue32 TypeHash32(const uint8_t* buffer, uint64_t length);

    //! Hashes a contiguous array of bytes starting at buffer and ending at buffer + length
    //! @param[in] buffer pointer to the memory to be hashed
    //! @param[in] length the length in bytes of memory to read for generating the hash
    //! @return 64 bit resulting hash
    HashValue64 TypeHash64(const uint8_t* buffer, uint64_t length);

    //! Hashes a contiguous array of bytes starting at buffer and ending at buffer + length
    //! @param[in] buffer pointer to the memory to be hashed
    //! @param[in] length the length in bytes of memory to read for generating the hash
    //! @param[in] seed   a seed that is hashed into the result
    //! @return 64 bit resulting hash
    HashValue64 TypeHash64(const uint8_t* buffer, uint64_t length, HashValue64 seed);

    //! Hashes a contiguous array of bytes starting at buffer and ending at buffer + length
    //! @param[in] buffer pointer to the memory to be hashed
    //! @param[in] length the length in bytes of memory to read for generating the hash
    //! @param[in] seed1  first seed that is hashed into the result
    //! @param[in] seed2  second seed that is also hashed into the result
    //! @return 64 bit resulting hash
    HashValue64 TypeHash64(const uint8_t* buffer, uint64_t length, HashValue64 seed1, HashValue64 seed2);

    template <typename T>
    HashValue32 TypeHash32(const T& t)
    {
        return TypeHash32(reinterpret_cast<const uint8_t*>(&t), sizeof(T));
    }

    template <typename T>
    HashValue64 TypeHash64(const T& t)
    {
        return TypeHash64(reinterpret_cast<const uint8_t*>(&t), sizeof(T));
    }

    template <typename T>
    HashValue64 TypeHash64(const T& t, HashValue64 seed)
    {
        return TypeHash64(reinterpret_cast<const uint8_t*>(&t), sizeof(T), seed);
    }

    template <typename T>
    HashValue64 TypeHash64(const T& t, HashValue64 seed1, HashValue64 seed2)
    {
        return TypeHash64(reinterpret_cast<const uint8_t*>(&t), sizeof(T), seed1, seed2);
    }

    inline HashValue32 TypeHash32(const char* value)
    {
        return TypeHash32(reinterpret_cast<const uint8_t*>(value), strlen(value));
    }

    inline HashValue64 TypeHash64(const char* value)
    {
        return TypeHash64(reinterpret_cast<const uint8_t*>(value), strlen(value));
    }

    inline HashValue64 TypeHash64(const char* value, HashValue64 seed)
    {
        return TypeHash64(reinterpret_cast<const uint8_t*>(value), strlen(value), seed);
    }

    inline HashValue64 TypeHash64(const char* value, HashValue64 seed1, HashValue64 seed2)
    {
        return TypeHash64(reinterpret_cast<const uint8_t*>(value), strlen(value), seed1, seed2);
    }
}

AZ_TYPE_SAFE_INTEGRAL_SERIALIZEBINDING(AZ::HashValue32);
AZ_TYPE_SAFE_INTEGRAL_SERIALIZEBINDING(AZ::HashValue64);

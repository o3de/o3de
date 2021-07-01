/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include required headers
#include "StandardHeaders.h"
#include "Vector.h"
#include <AzCore/std/string/string.h>


namespace MCore
{
    /**
     * The hash function.
     * The hash function must return an non-negative (so positive) integer, based on a key value.
     * Use partial template specialization to implement hashing functions for different data types.
     */
    template <typename Key>
    MCORE_INLINE uint32 Hash(const Key& key)
    {
        MCORE_ASSERT(false); // you should implement this function
        MCORE_UNUSED(key);
        //#pragma message (MCORE_ERROR "You should implement the Hash function for some specific Key type that you used")
        return 0;
    }


    template<>
    MCORE_INLINE uint32 Hash<AZStd::string>(const AZStd::string& key)
    {
        uint32 result = 0;
        const size_t length = key.size();
        for (size_t i = 0; i < length; ++i)
        {
            result = (result << 4) + key[i];
            const uint32 g = result & 0xf0000000L;
            if (g != 0)
            {
                result ^= g >> 24;
            }
            result &= ~g;
        }

        return result;
    }
    

    template<>
    MCORE_INLINE uint32 Hash<int32>(const int32& key)
    {
        return (uint32)Math::Abs(static_cast<float>(key));
    }


    template<>
    MCORE_INLINE uint32 Hash<uint32>(const uint32& key)
    {
        return key;
    }


    template<>
    MCORE_INLINE uint32 Hash<float>(const float& key)
    {
        return (uint32)Math::Abs(key * 12345.0f);
    }


    template<>
    MCORE_INLINE uint32 Hash<AZ::Vector3>(const AZ::Vector3& key)
    {
        return (uint32)Math::Abs(key.GetX() * 101.0f + key.GetY() * 1002.0f + key.GetZ() * 10003.0f);
    }
}   // namespace MCore

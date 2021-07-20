/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Crc.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <string.h>

namespace AZ
{
    //=========================================================================
    //
    // Crc32 constructor
    //
    //=========================================================================
    Crc32::Crc32(const void* data, size_t size, bool forceLowerCase)
        : Crc32{ reinterpret_cast<const uint8_t*>(data), size, forceLowerCase }
    {
    }

    void Crc32::Set(const void* data, size_t size, bool forceLowerCase)
    {
        Internal::Crc32Set(reinterpret_cast<const uint8_t*>(data), size, forceLowerCase, m_value);
    }

    //=========================================================================
    // Crc32 - Add
    //=========================================================================
    void Crc32::Add(const void* data, size_t size, bool forceLowerCase)
    {
        Combine(Crc32{ data, size, forceLowerCase }, size);
    }

    //=========================================================================
    // Crc32 - Reflect
    //=========================================================================
    void Crc32::Reflect(AZ::SerializeContext& context)
    {
        context.Class<Crc32>()
            ->Field("Value", &Crc32::m_value);
    }
}

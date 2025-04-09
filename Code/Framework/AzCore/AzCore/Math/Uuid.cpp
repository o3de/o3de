/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Uuid.h>
#include <AzCore/Math/Guid.h>
#include <AzCore/Math/Sfmt.h>

#include <AzCore/std/algorithm.h>

namespace AZStd
{
    template class AZStd::basic_fixed_string<char, AZ::Uuid::MaxStringBuffer>;
}

namespace AZ
{
    Uuid Uuid::Create()
    {
        return CreateRandom();
    }

    Uuid Uuid::CreateRandom()
    {
        Uuid id;
        Sfmt& smft = Sfmt::GetInstance();

        AZ::u64* id64 = reinterpret_cast<AZ::u64*>(&id);
        id64[0] = smft.Rand64();
        id64[1] = smft.Rand64();

        // variant VAR_RFC_4122
        id.m_data[8] &= AZStd::byte(0xBF);
        id.m_data[8] |= AZStd::byte(0x80);

        // version VER_RANDOM
        id.m_data[6] &= AZStd::byte(0x4F);
        id.m_data[6] |= AZStd::byte(0x40);

        return id;
    }

#if AZ_TRAIT_UUID_SUPPORTS_GUID_CONVERSION
    Uuid::Uuid(const GUID& guid)
    {
        memcpy(m_data, &guid, 16);
        // make big endian (internal storage type), all supported platforms are little endian
        AZStd::endian_swap(*reinterpret_cast<AZ::u32*>(&m_data[0]));
        AZStd::endian_swap(*reinterpret_cast<AZ::u16*>(&m_data[4]));
        AZStd::endian_swap(*reinterpret_cast<AZ::u16*>(&m_data[6]));
    }

    Uuid::operator GUID() const
    {
        GUID guid;
        memcpy(&guid, m_data, 16);
        // make big endian (internal storage type), all supported platforms are little endian
        AZStd::endian_swap(guid.Data1);
        AZStd::endian_swap(guid.Data2);
        AZStd::endian_swap(guid.Data3);
        return guid;
    }

    Uuid& Uuid::operator=(const GUID& guid)
    {
        return operator=(Uuid(guid));
    }
    bool Uuid::operator==(const GUID& guid) const
    {
        return operator==(Uuid(guid));
    }
    bool Uuid::operator!=(const GUID& guid) const
    {
        return operator!=(Uuid(guid));
    }

    bool operator==(const GUID& guid, const Uuid& uuid)
    {
        return Uuid(guid) == uuid;
    }
    bool operator!=(const GUID& guid, const Uuid& uuid)
    {
        return Uuid(guid) != uuid;
    }

#endif
} // namespace AZ

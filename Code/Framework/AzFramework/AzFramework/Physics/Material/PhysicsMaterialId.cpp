/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/Material/PhysicsMaterialId.h>

namespace Physics
{
    MaterialId MaterialId::CreateFromAssetId(const AZ::Data::AssetId& assetId)
    {
        return MaterialId(assetId.m_guid, assetId.m_subId);
    }

    MaterialId MaterialId::CreateName(const char* name)
    {
        return MaterialId(AZ::Uuid::CreateName(name));
    }

    MaterialId MaterialId::CreateData(const void* data, size_t dataSize)
    {
        return MaterialId(AZ::Uuid::CreateData(reinterpret_cast<const AZStd::byte*>(data), dataSize));
    }

    MaterialId MaterialId::CreateRandom()
    {
        return MaterialId(AZ::Uuid::CreateRandom());
    }

    MaterialId::MaterialId(const AZ::Uuid& guid)
        : m_guid{ guid }
    {
    }

    MaterialId::MaterialId(const AZ::Uuid& guid, uint32_t subId)
        : m_guid{ guid }
        , m_subId{ subId }
    {
    }

    bool MaterialId::IsValid() const
    {
        return m_guid != AZ::Uuid::CreateNull();
    }

    bool MaterialId::operator==(const MaterialId& rhs) const
    {
        return m_guid == rhs.m_guid && m_subId == rhs.m_subId;
    }

    bool MaterialId::operator!=(const MaterialId& rhs) const
    {
        return m_guid != rhs.m_guid || m_subId != rhs.m_subId;
    }
} // namespace Physics

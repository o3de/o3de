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
    MaterialId2 MaterialId2::CreateFromAssetId(const AZ::Data::AssetId& assetId)
    {
        return MaterialId2(assetId.m_guid, assetId.m_subId);
    }

    MaterialId2 MaterialId2::CreateName(const char* name)
    {
        return MaterialId2(AZ::Uuid::CreateName(name));
    }

    MaterialId2 MaterialId2::CreateData(const void* data, size_t dataSize)
    {
        return MaterialId2(AZ::Uuid::CreateData(data, dataSize));
    }

    MaterialId2 MaterialId2::CreateRandom()
    {
        return MaterialId2(AZ::Uuid::CreateRandom());
    }

    MaterialId2::MaterialId2(const AZ::Uuid& guid)
        : m_guid{ guid }
    {
    }

    MaterialId2::MaterialId2(const AZ::Uuid& guid, uint32_t subId)
        : m_guid{ guid }
        , m_subId{ subId }
    {
    }

    bool MaterialId2::IsValid() const
    {
        return m_guid != AZ::Uuid::CreateNull();
    }

    bool MaterialId2::operator==(const MaterialId2& rhs) const
    {
        return m_guid == rhs.m_guid && m_subId == rhs.m_subId;
    }

    bool MaterialId2::operator!=(const MaterialId2& rhs) const
    {
        return m_guid != rhs.m_guid || m_subId != rhs.m_subId;
    }
} // namespace Physics

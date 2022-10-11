/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomCore/Instance/InstanceId.h>

namespace AZ
{
    namespace Data
    {
        InstanceId InstanceId::CreateFromAsset(const Asset<AssetData>& asset)
        {
            // Passing the asset data pointer in as a unique identifier for this version of the asset. Ideally this would use the asset
            // creation token instead of the asset pointer but that requires the asset pointer to be valid beforehand. If the asset pointer
            // is null this will be the same as CreateFromAssetId.
            return InstanceId(asset.GetId().m_guid, asset.GetId().m_subId, asset.Get());
        }

        InstanceId InstanceId::CreateFromAssetId(const AssetId& assetId)
        {
            return InstanceId(assetId.m_guid, assetId.m_subId);
        }

        InstanceId InstanceId::CreateName(const char* name)
        {
            return InstanceId(Uuid::CreateName(name));
        }

        InstanceId InstanceId::CreateData(const void* data, size_t dataSize)
        {
            return InstanceId(Uuid::CreateData(data, dataSize));
        }

        InstanceId InstanceId::CreateRandom()
        {
            return InstanceId(Uuid::CreateRandom());
        }

        InstanceId::InstanceId(const Uuid& guid)
            : m_guid{ guid }
        {
        }

        InstanceId::InstanceId(const Uuid& guid, uint32_t subId)
            : m_guid{ guid }
            , m_subId{ subId }
        {
        }

        InstanceId::InstanceId(const Uuid& guid, uint32_t subId, void* versionId)
            : m_guid{ guid }
            , m_subId{ subId }
            , m_versionId{ versionId }
        {
        }

        bool InstanceId::IsValid() const
        {
            return m_guid != AZ::Uuid::CreateNull();
        }

        bool InstanceId::operator==(const InstanceId& rhs) const
        {
            return m_guid == rhs.m_guid && m_subId == rhs.m_subId && m_versionId == rhs.m_versionId;
        }

        bool InstanceId::operator!=(const InstanceId& rhs) const
        {
            return m_guid != rhs.m_guid || m_subId != rhs.m_subId || m_versionId != rhs.m_versionId;
        }
    } // namespace Data
} // namespace AZ

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
        InstanceId InstanceId::CreateFromAsset(const Asset<AssetData>& asset, const SubIdContainer& subIds)
        {
            const Data::AssetId assetId{ asset.GetId() };
            const Data::AssetData* assetPtr{ asset.GetData() };

            SubIdContainer combinedSubIds;
            combinedSubIds.push_back(assetId.m_subId);
            // The creation token is a unique id assigned by the asset manager to differentiate between unique versions of (re)loaded
            // assets. Including it in the instance id allows creating new, unique instances from (re)loaded assets with previously used
            // asset IDs.
            combinedSubIds.push_back(assetPtr ? assetPtr->GetCreationToken() : 0);
            combinedSubIds.insert(combinedSubIds.end(), subIds.begin(), subIds.end());
            return InstanceId(assetId.m_guid, combinedSubIds);
        }

        InstanceId InstanceId::CreateFromAssetId(const AssetId& assetId, const SubIdContainer& subIds)
        {
            SubIdContainer combinedSubIds;
            combinedSubIds.push_back(assetId.m_subId);
            // Injecting 0 as a placeholder creation token to keep instance IDs uniform and interchangeable whether created from an asset or
            // asset id.
            combinedSubIds.push_back(0);
            combinedSubIds.insert(combinedSubIds.end(), subIds.begin(), subIds.end());
            return InstanceId(assetId.m_guid, combinedSubIds);
        }

        InstanceId InstanceId::CreateName(const char* name, const SubIdContainer& subIds)
        {
            return InstanceId(Uuid::CreateName(name), subIds);
        }

        InstanceId InstanceId::CreateData(const void* data, size_t dataSize, const SubIdContainer& subIds)
        {
            return InstanceId(Uuid::CreateData(reinterpret_cast<const AZStd::byte*>(data), dataSize), subIds);
        }

        InstanceId InstanceId::CreateRandom()
        {
            return InstanceId(Uuid::CreateRandom());
        }

        InstanceId::InstanceId(const Uuid& guid)
            : InstanceId(guid, {})
        {
        }

        InstanceId::InstanceId(const Uuid& guid, uint32_t subId)
            : m_guid(guid)
            , m_subIds({ subId })
        {
        }

        InstanceId::InstanceId(const Uuid& guid, const SubIdContainer& subIds)
            : m_guid(guid)
            , m_subIds(subIds)
        {
        }

        bool InstanceId::IsValid() const
        {
            return !m_guid.IsNull();
        }

        bool InstanceId::operator<(const InstanceId& rhs) const
        {
            return m_guid < rhs.m_guid ||
                (m_guid == rhs.m_guid &&
                 AZStd::lexicographical_compare(m_subIds.begin(), m_subIds.end(), rhs.m_subIds.begin(), rhs.m_subIds.end()));
        }

        bool InstanceId::operator==(const InstanceId& rhs) const
        {
            return m_guid == rhs.m_guid && AZStd::equal(m_subIds.begin(), m_subIds.end(), rhs.m_subIds.begin(), rhs.m_subIds.end());
        }

        bool InstanceId::operator!=(const InstanceId& rhs) const
        {
            return !operator==(rhs);
        }

        const Uuid& InstanceId::GetGuid() const
        {
            return m_guid;
        }
    } // namespace Data
} // namespace AZ

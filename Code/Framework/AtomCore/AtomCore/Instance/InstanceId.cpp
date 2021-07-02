/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomCore/Instance/InstanceId.h>

namespace AZ
{
    namespace Data
    {
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
            : m_guid{guid}
        {}

        InstanceId::InstanceId(const Uuid& guid, uint32_t subId)
            : m_guid{guid}
            , m_subId{subId}
        {}

        bool InstanceId::IsValid() const
        {
            return m_guid != AZ::Uuid::CreateNull();
        }

        bool InstanceId::operator == (const InstanceId& rhs) const
        {
            return m_guid == rhs.m_guid && m_subId == rhs.m_subId;
        }

        bool InstanceId::operator != (const InstanceId& rhs) const
        {
            return m_guid != rhs.m_guid || m_subId != rhs.m_subId;
        }
    }
}

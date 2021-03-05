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
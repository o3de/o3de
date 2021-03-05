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

#include <AtomCore/Instance/InstanceData.h>
#include <AtomCore/Instance/InstanceDatabase.h>

namespace AZ
{
    namespace Data
    {
        const InstanceId& InstanceData::GetId() const
        {
            return m_id;
        }

        const AssetId& InstanceData::GetAssetId() const
        {
            return m_assetId;
        }

        const AssetType& InstanceData::GetAssetType() const
        {
            return m_assetType;
        }

        void InstanceData::add_ref()
        {
            AZ_Assert(m_useCount >= 0, "m_useCount is negative");
            ++m_useCount;
        }

        void InstanceData::release()
        {
            // It is possible that some other thread, not us, will delete this InstanceData after we
            // decrement m_useCount. For example, another thread could create and release an instance
            // immediately after we decrement. So we copy the necessary data to the callstack before
            // decrementing. This ensures the call to ReleaseInstance() below won't crash even if this
            // InstanceData gets deleted by another thread first.
            InstanceDatabaseInterface* parentDatabase = m_parentDatabase;
            InstanceId instanceId = GetId();

            const int prevUseCount = m_useCount.fetch_sub(1);

            AZ_Assert(prevUseCount >= 1, "m_useCount is negative");

            if (prevUseCount == 1)
            {
                if (parentDatabase)
                {
                    parentDatabase->ReleaseInstance(this, instanceId);
                }
                else
                {
                    // This is a standalone object not created through the InstanceDatabase so
                    // we can just delete it.
                    delete this;
                }
            }
        }
    }
}

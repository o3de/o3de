/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            // If @m_parentDatabase is valid we can't just simply decrement the ref count
            // InstanceDatabase also supports the case of Orphaned instances. The only
            // way to guarantee correcteness is to delegate ref count subtraction to the
            // instanceDatabase under its database mutex.
            // TODO: Ideally we should call `m_useCount.fetch_sub(1)` first and if it reaches the value
            //       0, then we'd ask @m_parentDatabase to release. Investigate why Mesh Model Loading
            //       needs to Orphan instances. Once Orphaned instance API is removed then we can call
            //       m_useCount.fetch_sub(1) before having to lock the instanceDatabase mutex.
            if (m_parentDatabase)
            {
                m_parentDatabase->ReleaseInstance(this);
            }
            else
            {
                const int prevUseCount = m_useCount.fetch_sub(1);
                AZ_Assert(prevUseCount >= 1, "m_useCount is negative");
                if (prevUseCount == 1)
                {
                    // This is a standalone object not created through the InstanceDatabase so
                    // we can just delete it.
                    delete this;
                }
            }
        }
    }
}

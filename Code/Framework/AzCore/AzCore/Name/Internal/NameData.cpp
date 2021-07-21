/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Name/Internal/NameData.h>
#include <AzCore/Name/NameDictionary.h>

namespace AZ
{
    namespace Internal
    {
        NameData::NameData(AZStd::string&& name, Hash hash)
            : m_name{AZStd::move(name)}
            , m_hash{hash}
        {}

        AZStd::string_view NameData::GetName() const
        {
            return m_name;
        }

        NameData::Hash NameData::GetHash() const
        {
            return m_hash;
        }

        void NameData::add_ref()
        {
            AZ_Assert(m_useCount >= 0, "NameData has been deleted");
            ++m_useCount;
        }

        void NameData::release()
        {
            AZ_Assert(m_useCount > 0, "m_useCount is already 0!");
            if (m_useCount.fetch_sub(1) == 1)
            {
                AZ::NameDictionary::Instance().TryReleaseName(this);
            }
        }
    }
}


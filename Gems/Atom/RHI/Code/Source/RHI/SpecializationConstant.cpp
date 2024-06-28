/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/SpecializationConstant.h>

namespace AZ::RHI
{
    bool SpecializationConstant::operator==(const SpecializationConstant& rhs) const
    {
        return
            m_value == rhs.m_value &&
            m_name == rhs.m_name &&
            m_id == rhs.m_id &&
            m_type == rhs.m_type;
    }

    HashValue64 SpecializationConstant::GetHash() const
    {
        AZ::HashValue64 seed = AZ::HashValue64{ 0 };
        seed = TypeHash64(m_value.GetIndex(), seed);
        seed = TypeHash64(m_name.GetHash(), seed);
        seed = TypeHash64(m_id, seed);
        seed = TypeHash64(m_type, seed);
        return seed;
    }    
}

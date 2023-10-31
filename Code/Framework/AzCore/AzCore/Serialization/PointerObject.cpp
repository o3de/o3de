/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/PointerObject.h>

namespace AZ
{
    AZ_TYPE_INFO_WITH_NAME_IMPL(PointerObject, "PointerObject", "{256581B7-8512-41FD-9A2C-6AF76B3E7BD6}");

    bool PointerObject::IsValid() const
    {
        return m_address != nullptr && !m_typeId.IsNull();
    }

    bool operator==(const PointerObject& lhs, const PointerObject& rhs)
    {
        return lhs.m_address == rhs.m_address && lhs.m_typeId == rhs.m_typeId;
    }

    bool operator!=(const PointerObject& lhs, const PointerObject& rhs)
    {
        return !operator==(lhs, rhs);
    }
} // namespace AZ

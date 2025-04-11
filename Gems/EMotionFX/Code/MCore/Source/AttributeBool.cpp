/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AttributeFloat.h"
#include "AttributeInt32.h"
#include "AttributeBool.h"
#include <MCore/Source/AttributeAllocator.h>

namespace MCore
{
    AZ_CLASS_ALLOCATOR_IMPL(AttributeBool, AttributeAllocator)

    bool AttributeBool::InitFrom(const Attribute* other)
    {
        switch (other->GetType())
        {
        case TYPE_ID:
            m_value = static_cast<const AttributeBool*>(other)->GetValue();
            return true;
        case MCore::AttributeFloat::TYPE_ID:
            m_value = !MCore::Math::IsFloatZero(static_cast<const AttributeFloat*>(other)->GetValue());
            return true;
        case MCore::AttributeInt32::TYPE_ID:
            m_value = static_cast<const AttributeInt32*>(other)->GetValue() != 0;
            return true;
        default:
            return false;
        }
    }
}   // namespace MCore

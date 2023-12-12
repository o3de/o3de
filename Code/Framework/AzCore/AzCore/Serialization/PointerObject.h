/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/TypeInfoSimple.h>

namespace AZ
{
    //! Wraps a pointer for serializing the pointer address to JSON
    //! This is not meant for serialization to filesystem
    struct PointerObject
    {
        AZ_TYPE_INFO_WITH_NAME_DECL(PointerObject);
        void* m_address{};
        AZ::TypeId m_typeId;

        bool IsValid() const;

        friend bool operator==(const PointerObject& lhs, const PointerObject& rhs);
        friend bool operator!=(const PointerObject& lhs, const PointerObject& rhs);
    };

} // namespace AZ

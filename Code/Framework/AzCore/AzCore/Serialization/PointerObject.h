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
    struct AZCORE_API PointerObject
    {
        AZ_TYPE_INFO_WITH_NAME_DECL_API(AZCORE_API, PointerObject);
        void* m_address{};
        AZ::TypeId m_typeId;

        bool IsValid() const;

        friend AZCORE_API bool operator==(const PointerObject& lhs, const PointerObject& rhs);
        friend AZCORE_API bool operator!=(const PointerObject& lhs, const PointerObject& rhs);
    };
    AZ_TYPE_INFO_WITH_NAME_DECL_EXT_API(AZCORE_API, PointerObject);

    AZCORE_API bool operator==(const PointerObject& lhs, const PointerObject& rhs);
    AZCORE_API bool operator!=(const PointerObject& lhs, const PointerObject& rhs);
} // namespace AZ

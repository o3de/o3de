/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Data/DataTrait.h>
#include <ScriptCanvas/Data/PropertyTraits.h>

namespace ScriptCanvas
{
    namespace Data
    {
        struct TypeErasedTraits
        {
            AZ_CLASS_ALLOCATOR(TypeErasedTraits, AZ::SystemAllocator);

            TypeErasedTraits() = default;

            template<eType scTypeValue>
            explicit TypeErasedTraits(AZStd::integral_constant<eType, scTypeValue>)
            {
                m_dataTraits = MakeTypeErasedDataTraits<scTypeValue>();
                m_propertyTraits = ScriptCanvas::Data::Properties::MakeTypeErasedPropertyTraits<scTypeValue>();
            }

            TypeErasedDataTraits m_dataTraits;
            ScriptCanvas::Data::Properties::TypeErasedPropertyTraits m_propertyTraits;
        };

        template<eType scTypeValue>
        static TypeErasedTraits MakeTypeErasedTraits()
        {
            return TypeErasedTraits(AZStd::integral_constant<eType, scTypeValue>{});
        }
    } 
}

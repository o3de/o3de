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

#pragma once

#include <ScriptCanvas/Data/DataTrait.h>
#include <ScriptCanvas/Data/PropertyTraits.h>

namespace ScriptCanvas
{
    namespace Data
    {
        struct TypeErasedTraits
        {
            AZ_CLASS_ALLOCATOR(TypeErasedTraits, AZ::SystemAllocator, 0);

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
    } // namespace Data
} // namespace ScriptCanvas
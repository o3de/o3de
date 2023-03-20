/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "ValueParameter.h"
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/Allocators.h>

namespace EMotionFX
{
    template <class ValueType, class Derived>
    class DefaultValueParameter
        : public ValueParameter
    {
    public:
        AZ_RTTI((DefaultValueParameter, "{AE70C43D-6BAE-4EDF-A1CF-FC18B9F92ABB}", ValueType, Derived), ValueParameter);
        AZ_CLASS_ALLOCATOR(DefaultValueParameter, AnimGraphAllocator)

        explicit DefaultValueParameter(const ValueType& defaultValue, AZStd::string name = {}, AZStd::string description = {})
            : ValueParameter(AZStd::move(name), AZStd::move(description))
            , m_defaultValue(defaultValue)
        {
        }

        static void Reflect(AZ::ReflectContext* context);

        ValueType GetDefaultValue() const { return m_defaultValue; }
        void SetDefaultValue(const ValueType& newValue) { m_defaultValue = newValue; }

    protected:
        ValueType m_defaultValue;
    };

    template <class ValueType, class Derived>
    void DefaultValueParameter<ValueType, Derived>::Reflect(AZ::ReflectContext* context)
    {
        using ThisType = DefaultValueParameter<ValueType, Derived>;

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<ThisType, ValueParameter>()
            ->Version(1)
            ->Field("defaultValue", &ThisType::m_defaultValue)
        ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<ThisType>("Non-range value parameter", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::Default, &ThisType::m_defaultValue, "Default", "Parameter's default value")
        ;
    }
}

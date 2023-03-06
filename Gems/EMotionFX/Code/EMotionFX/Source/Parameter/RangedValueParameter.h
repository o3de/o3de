/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "DefaultValueParameter.h"

namespace EMotionFX
{
    template <class ValueType, class Derived>
    class RangedValueParameter;

    AZ_TYPE_INFO_TEMPLATE(RangedValueParameter, "{83572845-AFBD-4685-AACD-0D15CF79006A}", AZ_TYPE_INFO_CLASS, AZ_TYPE_INFO_CLASS);

    template <class ValueType, class Derived>
    class RangedValueParameter
        : public DefaultValueParameter<ValueType, RangedValueParameter<ValueType, Derived>>
    {
        using BaseType = DefaultValueParameter<ValueType, RangedValueParameter<ValueType, Derived>>;
    public:
        // AZ_RTTI can't be used when a CRTP is used when the base class receives a template parameter for this class
        // and that template parameters participates in the base class TypeInfo calculation
        // So use AZ_TYPE_INFO paired with AZ_RTTI_NO_DECL
        AZ_RTTI_NO_TYPE_INFO_DECL()
        AZ_CLASS_ALLOCATOR(RangedValueParameter, AnimGraphAllocator)

        RangedValueParameter(const ValueType& defaultValue, const ValueType& minValue, const ValueType& maxValue, bool hasMinValue = true, bool hasMaxValue = true, AZStd::string name = {}, AZStd::string description = {})
            : BaseType(defaultValue, AZStd::move(name), AZStd::move(description))
            , m_minValue(minValue)
            , m_maxValue(maxValue)
            , m_hasMinValue(hasMinValue)
            , m_hasMaxValue(hasMaxValue)
        {
        }

        static void Reflect(AZ::ReflectContext* context);

        ValueType GetMinValue() const
        {
            if (m_hasMinValue)
            {
                return m_minValue;
            }
            return Derived::GetUnboundedMinValue();
        }

        void SetMinValue(const ValueType& newValue)
        {
            m_minValue = newValue;
            m_hasMinValue = true;
        }

        ValueType GetMaxValue() const
        {
            if (m_hasMaxValue)
            {
                return m_maxValue;
            }
            return Derived::GetUnboundedMaxValue();
        }

        void SetMaxValue(const ValueType& newValue)
        {
            m_maxValue = newValue;
            m_hasMaxValue = true;
        }

        bool GetHasMinValue() const
        {
            return m_hasMinValue;
        }

        void SetHasMinValue(const bool newValue)
        {
            m_hasMinValue = newValue;
        }

        bool GetHasMaxValue() const
        {
            return m_hasMaxValue;
        }

        void SetHasMaxValue(const bool newValue)
        {
            m_hasMaxValue = newValue;
        }

    protected:
        AZ::Crc32 GetMinValueVisibility() const
        {
            return m_hasMinValue ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
        }

        AZ::Crc32 GetMaxValueVisibility() const
        {
            return m_hasMaxValue ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
        }

        ValueType m_minValue;
        ValueType m_maxValue;
        bool m_hasMinValue = true;
        bool m_hasMaxValue = true;
    };

    AZ_RTTI_NO_TYPE_INFO_IMPL_INLINE((RangedValueParameter, AZ_TYPE_INFO_CLASS, AZ_TYPE_INFO_CLASS), BaseType);

    template <class ValueType, class Derived>
    void RangedValueParameter<ValueType, Derived>::Reflect(AZ::ReflectContext* context)
    {
        using ThisType = RangedValueParameter<ValueType, Derived>;

        // This method calls Reflect() on it's parent class, which is uncommon
        // in the LY reflection framework.  This is because the parent class is
        // a template, and is unique to each type that subclasses it, as it
        // uses the Curiously Recursive Template Pattern.
        BaseType::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<ThisType, BaseType>()
            ->Version(1)
            ->Field("hasMinValue", &RangedValueParameter<ValueType, Derived>::m_hasMinValue)
            ->Field("minValue", &RangedValueParameter<ValueType, Derived>::m_minValue)
            ->Field("hasMaxValue", &RangedValueParameter<ValueType, Derived>::m_hasMaxValue)
            ->Field("maxValue", &RangedValueParameter<ValueType, Derived>::m_maxValue);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<ThisType>("Range value parameter", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::Default, &ThisType::m_hasMinValue, "Has minimum", "Parameter has a minimum value")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ::Edit::UIHandlers::Default, &ThisType::m_minValue, "Minimum", "Parameter's minimum value")
                ->Attribute(AZ::Edit::Attributes::Visibility, &ThisType::GetMinValueVisibility)
            ->DataElement(AZ::Edit::UIHandlers::Default, &ThisType::m_hasMaxValue, "Has maximum", "Parameter has a maximum value")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ::Edit::UIHandlers::Default, &ThisType::m_maxValue, "Maximum", "Parameter's maximum value")
                ->Attribute(AZ::Edit::Attributes::Visibility, &ThisType::GetMaxValueVisibility)
            ;
    }
}

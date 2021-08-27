/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "ValueParameterEditor.h"

namespace EMStudio
{
    class FloatSliderParameterEditor
        : public ValueParameterEditor
    {
    public:
        AZ_RTTI(FloatSliderParameterEditor, "{44C45ECB-E9D0-4B20-B384-3A3636DC318C}", ValueParameterEditor)
        AZ_CLASS_ALLOCATOR_DECL

        // Required for serialization
        FloatSliderParameterEditor()
            : ValueParameterEditor(nullptr, nullptr, AZStd::vector<MCore::Attribute*>())
            , m_currentValue(0.0f)
        {}

        FloatSliderParameterEditor(EMotionFX::AnimGraph* animGraph, const EMotionFX::ValueParameter* valueParameter, const AZStd::vector<MCore::Attribute*> attributes);

        ~FloatSliderParameterEditor() override = default;

        static void Reflect(AZ::ReflectContext* context);

        void UpdateValue() override;

    private:
        void OnValueChanged();

        float GetMinValue() const;
        float GetMaxValue() const;

        float m_currentValue;
    };
} 

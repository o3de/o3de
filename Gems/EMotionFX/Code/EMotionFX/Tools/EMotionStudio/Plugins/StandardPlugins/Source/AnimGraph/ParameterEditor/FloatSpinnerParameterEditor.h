/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "ValueParameterEditor.h"

namespace EMStudio
{
    class FloatSpinnerParameterEditor
        : public ValueParameterEditor
    {
    public:
        AZ_RTTI(FloatSpinnerParameterEditor, "{10E9BC54-7A0F-4DE6-870B-8C5C38D44E6C}", ValueParameterEditor)
        AZ_CLASS_ALLOCATOR_DECL

        // Required for serialization
        FloatSpinnerParameterEditor()
            : ValueParameterEditor(nullptr, nullptr, AZStd::vector<MCore::Attribute*>())
            , m_currentValue(0.0f)
        {}

        FloatSpinnerParameterEditor(EMotionFX::AnimGraph* animGraph, const EMotionFX::ValueParameter* valueParameter, const AZStd::vector<MCore::Attribute*> attributes);

        ~FloatSpinnerParameterEditor() override = default;

        static void Reflect(AZ::ReflectContext* context);

        void UpdateValue() override;

    private:
        void OnValueChanged();

        float GetMinValue() const;
        float GetMaxValue() const;

        float m_currentValue;
    };
} 

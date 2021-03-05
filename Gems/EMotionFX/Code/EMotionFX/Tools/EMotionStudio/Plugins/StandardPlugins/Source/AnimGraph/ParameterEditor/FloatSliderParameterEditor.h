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

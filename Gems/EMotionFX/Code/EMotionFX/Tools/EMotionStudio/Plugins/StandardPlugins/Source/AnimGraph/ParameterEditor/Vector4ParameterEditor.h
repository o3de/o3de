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
    class Vector4ParameterEditor
        : public ValueParameterEditor
    {
    public:
        AZ_RTTI(Vector4ParameterEditor, "{45D399D5-1871-47EE-8159-BA7D52B13893}", ValueParameterEditor)
        AZ_CLASS_ALLOCATOR_DECL

        // Required for serialization
        Vector4ParameterEditor()
            : Vector4ParameterEditor(nullptr, nullptr, AZStd::vector<MCore::Attribute*>())
        {}

        Vector4ParameterEditor(EMotionFX::AnimGraph* animGraph, const EMotionFX::ValueParameter* valueParameter, const AZStd::vector<MCore::Attribute*>& attributes);

        ~Vector4ParameterEditor() override = default;

        static void Reflect(AZ::ReflectContext* context);

        void UpdateValue() override;

    private:
        void OnValueChanged();

        AZ::Vector4 GetMinValue() const;
        AZ::Vector4 GetMaxValue() const;

        AZ::Vector4 m_currentValue;
    };
}

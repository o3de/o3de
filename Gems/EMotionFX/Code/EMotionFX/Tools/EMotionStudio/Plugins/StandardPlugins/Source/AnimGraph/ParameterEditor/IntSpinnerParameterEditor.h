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
    class IntSpinnerParameterEditor
        : public ValueParameterEditor
    {
    public:
        AZ_RTTI(IntSpinnerParameterEditor, "{4DA44E56-AA78-4BEF-88E6-81AA4E65729F}", ValueParameterEditor);
        AZ_CLASS_ALLOCATOR_DECL

        // Required for serialization
        IntSpinnerParameterEditor()
            : ValueParameterEditor(nullptr, nullptr, AZStd::vector<MCore::Attribute*>())
            , m_currentValue(0)
        {}

        IntSpinnerParameterEditor(EMotionFX::AnimGraph* animGraph, const EMotionFX::ValueParameter* valueParameter, const AZStd::vector<MCore::Attribute*> attributes);

        ~IntSpinnerParameterEditor() override = default;

        static void Reflect(AZ::ReflectContext* context);

        void UpdateValue() override;

    private:
        void OnValueChanged();

        int GetMinValue() const;
        int GetMaxValue() const;

        int m_currentValue;
    };
} 

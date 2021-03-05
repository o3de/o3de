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
    class IntSliderParameterEditor
        : public ValueParameterEditor
    {
    public:
        AZ_RTTI(IntSliderParameterEditor, "{95295E4B-2D0F-4462-B107-22D124A8B845}", ValueParameterEditor)
        AZ_CLASS_ALLOCATOR_DECL

        // Required for serialization
        IntSliderParameterEditor()
            : ValueParameterEditor(nullptr, nullptr, AZStd::vector<MCore::Attribute*>())
            , m_currentValue(0)
        {}

        IntSliderParameterEditor(EMotionFX::AnimGraph* animGraph, const EMotionFX::ValueParameter* valueParameter, const AZStd::vector<MCore::Attribute*> attributes);

        ~IntSliderParameterEditor() override = default;

        static void Reflect(AZ::ReflectContext* context);

        void UpdateValue() override;

    private:
        void OnValueChanged();

        int GetMinValue() const;
        int GetMaxValue() const;

        int m_currentValue;
    };
} 

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
    class Vector3ParameterEditor
        : public ValueParameterEditor
    {
    public:
        AZ_RTTI(Vector3ParameterEditor, "{4BA2214A-EB3B-4E98-BB36-9996E0B4B7E1}", ValueParameterEditor)
        AZ_CLASS_ALLOCATOR_DECL

        // Required for serialization
        Vector3ParameterEditor()
            : Vector3ParameterEditor(nullptr, nullptr, AZStd::vector<MCore::Attribute*>())
        {}

        Vector3ParameterEditor(EMotionFX::AnimGraph* animGraph, const EMotionFX::ValueParameter* valueParameter, const AZStd::vector<MCore::Attribute*>& attributes);

        ~Vector3ParameterEditor() override = default;

        static void Reflect(AZ::ReflectContext* context);

        void setIsReadOnly(bool isReadOnly) override;
       
        void UpdateValue() override;

    private:
        void OnValueChanged();

        AZ::Vector3 GetMinValue() const;
        AZ::Vector3 GetMaxValue() const;

        AZ::Vector3 m_currentValue;
    };
}

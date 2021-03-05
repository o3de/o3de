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
#include <AzCore/Math/Quaternion.h>
#include <QPushButton>

namespace MCommon
{
    class RotateManipulator;
}

namespace EMStudio
{
    class RotationParameterEditor
        : public ValueParameterEditor
    {
    public:
        AZ_RTTI(RotationParameterEditor, "{55C122A9-AA80-49FB-8663-2113C7AC97C0}", ValueParameterEditor)
        AZ_CLASS_ALLOCATOR_DECL

        // Required for serialization
        RotationParameterEditor()
            : RotationParameterEditor(nullptr, nullptr, AZStd::vector<MCore::Attribute*>())
        {}

        RotationParameterEditor(EMotionFX::AnimGraph* animGraph, const EMotionFX::ValueParameter* valueParameter, const AZStd::vector<MCore::Attribute*>& attributes);

        ~RotationParameterEditor() override;

        static void Reflect(AZ::ReflectContext* context);

        void setIsReadOnly(bool isReadOnly) override;

        QWidget* CreateGizmoWidget(const AZStd::function<void()>& manipulatorCallback) override;

        void SetValue(const AZ::Quaternion& value);
        
        void UpdateValue() override;

        AZ::Quaternion GetCurrentValue() const { return m_currentValue; }

    private:
        void OnValueChanged();

        AZ::Quaternion GetMinValue() const;
        AZ::Quaternion GetMaxValue() const;

        void ToggleTranslationGizmo();

        AZ::Quaternion m_currentValue;

        QPushButton* m_gizmoButton;
        MCommon::RotateManipulator* m_transformationGizmo;
        AZStd::function<void()> m_manipulatorCallback;
    };
}

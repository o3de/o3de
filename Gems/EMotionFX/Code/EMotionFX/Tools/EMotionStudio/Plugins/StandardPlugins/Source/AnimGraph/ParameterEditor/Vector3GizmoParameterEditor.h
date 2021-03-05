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

#include <QPushButton>

namespace MCommon
{
    class TranslateManipulator;
}

namespace EMStudio
{
    class Vector3GizmoParameterEditor
        : public ValueParameterEditor
    {
    public:
        AZ_RTTI(Vector3GizmoParameterEditor, "{9603AE76-2E84-4BAD-8351-FDED3B880C65}", ValueParameterEditor)
        AZ_CLASS_ALLOCATOR_DECL

        // Required for serialization
        Vector3GizmoParameterEditor()
            : Vector3GizmoParameterEditor(nullptr, nullptr, AZStd::vector<MCore::Attribute*>())
        {
        }

        Vector3GizmoParameterEditor(EMotionFX::AnimGraph* animGraph, const EMotionFX::ValueParameter* valueParameter, const AZStd::vector<MCore::Attribute*>& attributes);
        ~Vector3GizmoParameterEditor() override;

        static void Reflect(AZ::ReflectContext* context);

        QWidget* CreateGizmoWidget(const AZStd::function<void()>& manipulatorCallback) override;
        void SetValue(const AZ::Vector3& value);      
        void UpdateValue() override;
        void setIsReadOnly(bool isReadOnly) override;

    private:
        void OnValueChanged();
        void UpdateAnimGraphInstanceAttributes();
        void ToggleTranslationGizmo();

        AZ::Vector3 GetMinValue() const;
        AZ::Vector3 GetMaxValue() const;

    private:
        AZ::Vector3 m_currentValue = AZ::Vector3::CreateZero();
        QPushButton* m_gizmoButton = nullptr;
        MCommon::TranslateManipulator* m_transformationGizmo = nullptr;
        AZStd::function<void()> m_manipulatorCallback;
    };
} // namespace EMStudio

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterEditor/ValueParameterEditor.h>
#include <AzToolsFramework/Manipulators/TranslationManipulators.h>

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

        void OnManipulatorMoved(const AZ::Vector3& position);

    private:
        AZ::Vector3 m_currentValue = AZ::Vector3::CreateZero();
        QPushButton* m_gizmoButton = nullptr;

        // TODO: Remove this when we remove the opengl widget
        MCommon::TranslateManipulator* m_transformationGizmo = nullptr;

        AzToolsFramework::TranslationManipulators m_translationManipulators;
        AZStd::function<void()> m_manipulatorCallback;
    };
} // namespace EMStudio

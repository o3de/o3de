/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>
#include <AzCore/Math/Quaternion.h>
#include <AzToolsFramework/Manipulators/RotationManipulators.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterEditor/ValueParameterEditor.h>

class QPushButton;

namespace EMStudio
{
    class RotationParameterEditor
        : public ValueParameterEditor
        , private AZ::TickBus::Handler
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
        // AZ::TickBus::Handler overrides ...
        void OnTick(float delta, AZ::ScriptTimePoint timePoint) override;

        void OnValueChanged();

        AZ::Quaternion GetMinValue() const;
        AZ::Quaternion GetMaxValue() const;

        void ToggleTranslationGizmo();

        AZ::Quaternion m_currentValue;

        QPushButton* m_gizmoButton;
        AzToolsFramework::RotationManipulators m_rotationManipulator;
        AZStd::function<void()> m_manipulatorCallback;
    };
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterEditor/RotationParameterEditor.h>

#include <AzCore/Math/Transform.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Viewport/ViewportColors.h>
#include <AzToolsFramework/Viewport/ViewportSettings.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/Parameter/RotationParameter.h>
#include <EMotionStudio/EMStudioSDK/Source/Allocators.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <MCore/Source/AttributeQuaternion.h>

#include <QPushButton>

namespace EMStudio
{
    AZ_CLASS_ALLOCATOR_IMPL(RotationParameterEditor, EMStudio::UIAllocator)

    RotationParameterEditor::RotationParameterEditor(
        EMotionFX::AnimGraph* animGraph,
        const EMotionFX::ValueParameter* valueParameter,
        const AZStd::vector<MCore::Attribute*>& attributes)
        : ValueParameterEditor(animGraph, valueParameter, attributes)
        , m_currentValue(AZ::Quaternion::CreateIdentity())
        , m_gizmoButton(nullptr)
        , m_rotationManipulator(AZ::Transform::Identity())
    {
        UpdateValue();
    }

    RotationParameterEditor::~RotationParameterEditor()
    {
        if (m_rotationManipulator.Registered())
        {
            m_rotationManipulator.Unregister();
            AZ::TickBus::Handler::BusDisconnect();
        }
    }

    void RotationParameterEditor::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<RotationParameterEditor, ValueParameterEditor>()->Version(1)->Field(
            "value", &RotationParameterEditor::m_currentValue);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<RotationParameterEditor>("Rotation parameter editor", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RotationParameterEditor::m_currentValue, "", "")
            ->Attribute(AZ::Edit::Attributes::DescriptionTextOverride, &ValueParameterEditor::GetDescription)
            ->Attribute(AZ::Edit::Attributes::Min, &RotationParameterEditor::GetMinValue)
            ->Attribute(AZ::Edit::Attributes::Max, &RotationParameterEditor::GetMaxValue)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RotationParameterEditor::OnValueChanged)
            ->Attribute(AZ::Edit::Attributes::ReadOnly, &ValueParameterEditor::IsReadOnly);
    }

    void RotationParameterEditor::UpdateValue()
    {
        // Use the value from the first attribute, they all should match since they all are the same parameter in different graph instances
        if (!m_attributes.empty())
        {
            MCore::AttributeQuaternion* attribute = static_cast<MCore::AttributeQuaternion*>(m_attributes[0]);
            m_currentValue = attribute->GetValue();
        }
        else
        {
            const EMotionFX::RotationParameter* parameter = static_cast<const EMotionFX::RotationParameter*>(m_valueParameter);
            m_currentValue = parameter->GetDefaultValue();
        }
        m_rotationManipulator.SetLocalOrientation(m_currentValue);
    }

    void RotationParameterEditor::setIsReadOnly(bool isReadOnly)
    {
        ValueParameterEditor::setIsReadOnly(isReadOnly);
        if (m_gizmoButton)
        {
            m_gizmoButton->setEnabled(!IsReadOnly());
        }
    }

    QWidget* RotationParameterEditor::CreateGizmoWidget(const AZStd::function<void()>& manipulatorCallback)
    {
        m_gizmoButton = new QPushButton();
        EMStudioManager::MakeTransparentButton(
            m_gizmoButton, "Images/Icons/Vector3GizmoDisabled.png", "Show/Hide translation gizmo for visual manipulation");
        QObject::connect(
            m_gizmoButton,
            &QPushButton::clicked,
            [this]()
            {
                ToggleTranslationGizmo();
            });
        m_gizmoButton->setCheckable(true);
        m_gizmoButton->setEnabled(!IsReadOnly());
        m_manipulatorCallback = manipulatorCallback;

        m_rotationManipulator.SetCircleBoundWidth(AzToolsFramework::ManipulatorCicleBoundWidth());
        m_rotationManipulator.SetLocalAxes(AZ::Vector3::CreateAxisX(), AZ::Vector3::CreateAxisY(), AZ::Vector3::CreateAxisZ());
        m_rotationManipulator.ConfigureView(
            AzToolsFramework::RotationManipulatorRadius(),
            AzFramework::ViewportColors::XAxisColor,
            AzFramework::ViewportColors::YAxisColor,
            AzFramework::ViewportColors::ZAxisColor);
        m_rotationManipulator.InstallMouseMoveCallback(
            [this](const AzToolsFramework::AngularManipulator::Action& action)
            {
                SetValue(action.LocalOrientation());
                if (m_manipulatorCallback)
                {
                    m_manipulatorCallback();
                }
            });

        return m_gizmoButton;
    }

    void RotationParameterEditor::SetValue(const AZ::Quaternion& value)
    {
        m_currentValue = value;
        OnValueChanged();
    }

    AZ::Quaternion RotationParameterEditor::GetMinValue() const
    {
        const EMotionFX::RotationParameter* parameter = static_cast<const EMotionFX::RotationParameter*>(m_valueParameter);
        return parameter->GetMinValue();
    }

    AZ::Quaternion RotationParameterEditor::GetMaxValue() const
    {
        const EMotionFX::RotationParameter* parameter = static_cast<const EMotionFX::RotationParameter*>(m_valueParameter);
        return parameter->GetMaxValue();
    }

    void RotationParameterEditor::OnTick([[maybe_unused]] float delta, [[maybe_unused]] AZ::ScriptTimePoint timePoint)
    {
        EMotionFX::ActorInstance* selectedActorInstance = EMotionFX::GetActorManager().GetFirstEditorActorInstance();
        if (selectedActorInstance)
        {
            AZ::Transform transform = AZ::Transform::CreateIdentity();
            transform.SetTranslation(selectedActorInstance->GetAabb().GetCenter());
            m_rotationManipulator.SetSpace(transform);
        }
        else
        {
            m_rotationManipulator.SetSpace(AZ::Transform::CreateIdentity());
        }
    }

    void RotationParameterEditor::OnValueChanged()
    {
        for (MCore::Attribute* attribute : m_attributes)
        {
            MCore::AttributeQuaternion* typedAttribute = static_cast<MCore::AttributeQuaternion*>(attribute);
            typedAttribute->SetValue(m_currentValue);
        }
        m_rotationManipulator.SetLocalOrientation(m_currentValue);
    }

    void RotationParameterEditor::ToggleTranslationGizmo()
    {
        if (m_gizmoButton->isChecked())
        {
            EMStudioManager::MakeTransparentButton(
                m_gizmoButton, "Images/Icons/Vector3Gizmo.svg", "Show/Hide translation gizmo for visual manipulation");
        }
        else
        {
            EMStudioManager::MakeTransparentButton(
                m_gizmoButton, "Images/Icons/Vector3GizmoDisabled.png", "Show/Hide translation gizmo for visual manipulation");
        }

        if (m_rotationManipulator.Registered())
        {
            AZ::TickBus::Handler::BusDisconnect();
            m_rotationManipulator.Unregister();
        }
        else
        {
            AZ::TickBus::Handler::BusConnect();
            m_rotationManipulator.Register(g_animManipulatorManagerId);
        }
    }
} // namespace EMStudio

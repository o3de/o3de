/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterEditor/Vector3GizmoParameterEditor.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/Parameter/Vector3Parameter.h>
#include <EMotionStudio/EMStudioSDK/Source/Allocators.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <MCore/Source/AttributeVector3.h>
#include <QPushButton>

namespace EMStudio
{
    AZ_CLASS_ALLOCATOR_IMPL(Vector3GizmoParameterEditor, EMStudio::UIAllocator)

    Vector3GizmoParameterEditor::Vector3GizmoParameterEditor(EMotionFX::AnimGraph* animGraph, const EMotionFX::ValueParameter* valueParameter, const AZStd::vector<MCore::Attribute*>& attributes)
        : ValueParameterEditor(animGraph, valueParameter, attributes)
        , m_currentValue(0.0f, 0.0f, 0.0f)
        , m_gizmoButton()
        , m_translationManipulators(
              AzToolsFramework::TranslationManipulators::Dimensions::Three, AZ::Transform::Identity(), AZ::Vector3::CreateOne())
    {
        UpdateValue();
    }

    Vector3GizmoParameterEditor::~Vector3GizmoParameterEditor()
    {
        if (m_translationManipulators.Registered())
        {
            m_translationManipulators.Unregister();
        }
    }

    void Vector3GizmoParameterEditor::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<Vector3GizmoParameterEditor, ValueParameterEditor>()
            ->Version(1)
            ->Field("value", &Vector3GizmoParameterEditor::m_currentValue)
        ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<Vector3GizmoParameterEditor>("Vector3 gizmo parameter editor", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::Default, &Vector3GizmoParameterEditor::m_currentValue, "", "")
                ->Attribute(AZ::Edit::Attributes::DescriptionTextOverride, &ValueParameterEditor::GetDescription)
                ->Attribute(AZ::Edit::Attributes::Min, &Vector3GizmoParameterEditor::GetMinValue)
                ->Attribute(AZ::Edit::Attributes::Max, &Vector3GizmoParameterEditor::GetMaxValue)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &Vector3GizmoParameterEditor::OnValueChanged)
                ->Attribute(AZ::Edit::Attributes::ReadOnly, &ValueParameterEditor::IsReadOnly)
        ;
    }

    void Vector3GizmoParameterEditor::UpdateValue()
    {
        // Use the value from the first attribute, they all should match since they all are the same parameter in different graph instances
        if (!m_attributes.empty())
        {
            MCore::AttributeVector3* attribute = static_cast<MCore::AttributeVector3*>(m_attributes[0]);
            m_currentValue = attribute->GetValue();
        }
        else
        {
            const EMotionFX::Vector3Parameter* parameter = static_cast<const EMotionFX::Vector3Parameter*>(m_valueParameter);
            m_currentValue = parameter->GetDefaultValue();
        }
        m_translationManipulators.SetLocalPosition(m_currentValue);
    }

    void Vector3GizmoParameterEditor::setIsReadOnly(bool isReadOnly)
    {
        ValueParameterEditor::setIsReadOnly(isReadOnly);
        if (m_gizmoButton)
        {
            m_gizmoButton->setEnabled(!IsReadOnly());
        }
    }

    QWidget* Vector3GizmoParameterEditor::CreateGizmoWidget(const AZStd::function<void()>& manipulatorCallback)
    {
        m_gizmoButton = new QPushButton();
        EMStudioManager::MakeTransparentButton(m_gizmoButton, "Images/Icons/Vector3GizmoDisabled.png", "Show/Hide translation gizmo for visual manipulation");
        QObject::connect(m_gizmoButton, &QPushButton::clicked, [this]() { ToggleTranslationGizmo(); });
        m_gizmoButton->setCheckable(true);
        m_gizmoButton->setEnabled(!IsReadOnly());
        m_manipulatorCallback = manipulatorCallback;

        // Setup the translation manipulator
        AzToolsFramework::ConfigureTranslationManipulatorAppearance3d(&m_translationManipulators);
        auto mouseMoveHandlerFn = [this](const auto& action) {
            SetValue(action.LocalPosition());
            if (m_manipulatorCallback)
            {
                m_manipulatorCallback();
            }
        };
        
        m_translationManipulators.InstallLinearManipulatorMouseMoveCallback(mouseMoveHandlerFn);
        m_translationManipulators.InstallPlanarManipulatorMouseMoveCallback(mouseMoveHandlerFn);
        m_translationManipulators.InstallSurfaceManipulatorMouseMoveCallback(mouseMoveHandlerFn);

        return m_gizmoButton;
    }

    void Vector3GizmoParameterEditor::SetValue(const AZ::Vector3& value)
    {
        m_currentValue = value;
        OnValueChanged();
    }

    AZ::Vector3 Vector3GizmoParameterEditor::GetMinValue() const
    {
        const EMotionFX::Vector3Parameter* parameter = static_cast<const EMotionFX::Vector3Parameter*>(m_valueParameter);
        return parameter->GetMinValue();
    }

    AZ::Vector3 Vector3GizmoParameterEditor::GetMaxValue() const
    {
        const EMotionFX::Vector3Parameter* parameter = static_cast<const EMotionFX::Vector3Parameter*>(m_valueParameter);
        return parameter->GetMaxValue();
    }

    void Vector3GizmoParameterEditor::OnValueChanged()
    {
        for (MCore::Attribute* attribute : m_attributes)
        {
            MCore::AttributeVector3* typedAttribute = static_cast<MCore::AttributeVector3*>(attribute);
            typedAttribute->SetValue(m_currentValue);
        }
        m_translationManipulators.SetLocalPosition(m_currentValue);
    }

    void Vector3GizmoParameterEditor::ToggleTranslationGizmo()
    {
        if (m_gizmoButton->isChecked())
        {
            EMStudioManager::MakeTransparentButton(m_gizmoButton, "Images/Icons/Vector3Gizmo.svg", "Show/Hide translation gizmo for visual manipulation");
        }
        else
        {
            EMStudioManager::MakeTransparentButton(m_gizmoButton, "Images/Icons/Vector3GizmoDisabled.png", "Show/Hide translation gizmo for visual manipulation");
        }

        // These will enable/disable the translation manipulator for atom render viewport.
        if (m_translationManipulators.Registered())
        {
            m_translationManipulators.Unregister();
        }
        else
        {
            m_translationManipulators.Register(g_animManipulatorManagerId);
        }

    }

}

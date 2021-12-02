/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RotationParameterEditor.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Rendering/Common/RotateManipulator.h>
#include <EMotionFX/Source/Parameter/RotationParameter.h>
#include <EMotionStudio/EMStudioSDK/Source/Allocators.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <MCore/Source/AttributeQuaternion.h>

namespace EMStudio
{
    AZ_CLASS_ALLOCATOR_IMPL(RotationParameterEditor, EMStudio::UIAllocator, 0)

    RotationParameterEditor::RotationParameterEditor(EMotionFX::AnimGraph* animGraph, const EMotionFX::ValueParameter* valueParameter, const AZStd::vector<MCore::Attribute*>& attributes)
        : ValueParameterEditor(animGraph, valueParameter, attributes)
        , m_currentValue(AZ::Quaternion::CreateIdentity())
        , m_gizmoButton(nullptr)
        , m_transformationGizmo(nullptr)
    {
        UpdateValue();
    }

    RotationParameterEditor::~RotationParameterEditor()
    {
        if (m_transformationGizmo)
        {
            GetManager()->RemoveTransformationManipulator(m_transformationGizmo);
            delete m_transformationGizmo;
        }
    }

    void RotationParameterEditor::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<RotationParameterEditor, ValueParameterEditor>()
            ->Version(1)
            ->Field("value", &RotationParameterEditor::m_currentValue)
        ;

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
                ->Attribute(AZ::Edit::Attributes::ReadOnly, &ValueParameterEditor::IsReadOnly)
        ;
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
        EMStudioManager::MakeTransparentButton(m_gizmoButton, "Images/Icons/Vector3GizmoDisabled.png", "Show/Hide translation gizmo for visual manipulation");
        QObject::connect(m_gizmoButton, &QPushButton::clicked, [this]() { ToggleTranslationGizmo(); });
        m_gizmoButton->setCheckable(true);
        m_gizmoButton->setEnabled(!IsReadOnly());
        m_manipulatorCallback = manipulatorCallback;
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

    void RotationParameterEditor::OnValueChanged()
    {
        for (MCore::Attribute* attribute : m_attributes)
        {
            MCore::AttributeQuaternion* typedAttribute = static_cast<MCore::AttributeQuaternion*>(attribute);
            typedAttribute->SetValue(m_currentValue);
        }
    }

    class Callback
        : public MCommon::ManipulatorCallback
    {
    public:
        Callback(const AZStd::function<void()>& manipulatorCallback, const AZ::Quaternion& oldValue, RotationParameterEditor* parentEditor = nullptr)
            : MCommon::ManipulatorCallback(nullptr, oldValue)
            , m_parentEditor(parentEditor)
            , m_manipulatorCallback(manipulatorCallback)
        {}

        using MCommon::ManipulatorCallback::Update;
        void Update(const AZ::Quaternion& value) override
        {
            // call the base class update function
            MCommon::ManipulatorCallback::Update(value);

            // update the value of the attribute
            m_parentEditor->SetValue(value);

            if (m_manipulatorCallback)
            {
                m_manipulatorCallback();
            }
        }

    private:
        RotationParameterEditor* m_parentEditor;
        const AZStd::function<void()>& m_manipulatorCallback;
    };

    void RotationParameterEditor::ToggleTranslationGizmo()
    {
        if (m_gizmoButton->isChecked())
        {
            EMStudioManager::MakeTransparentButton(m_gizmoButton, "Images/Icons/Vector3Gizmo.svg", "Show/Hide translation gizmo for visual manipulation");
        }
        else
        {
            EMStudioManager::MakeTransparentButton(m_gizmoButton, "Images/Icons/Vector3GizmoDisabled.png", "Show/Hide translation gizmo for visual manipulation");
        }

        if (!m_transformationGizmo)
        {
            m_transformationGizmo = static_cast<MCommon::RotateManipulator*>(GetManager()->AddTransformationManipulator(new MCommon::RotateManipulator(70.0f, true)));
            m_transformationGizmo->Init(AZ::Vector3::CreateZero());
            m_transformationGizmo->SetCallback(new Callback(m_manipulatorCallback, m_currentValue, this));
            m_transformationGizmo->SetName(m_valueParameter->GetName());
        }
        else
        {
            GetManager()->RemoveTransformationManipulator(m_transformationGizmo);
            delete m_transformationGizmo;
            m_transformationGizmo = nullptr;
        }
    }
}

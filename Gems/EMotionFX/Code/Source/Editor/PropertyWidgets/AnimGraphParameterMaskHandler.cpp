/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/EventHandler.h>
#include <Editor/PropertyWidgets/AnimGraphParameterMaskHandler.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterSelectionWindow.h>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QTimer>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphParameterMaskHandler, EditorAllocator)

    AnimGraphParameterMaskHandler::AnimGraphParameterMaskHandler()
        : QObject()
        , AzToolsFramework::PropertyHandler<AZStd::vector<AZStd::string>, AnimGraphParameterPicker>()
        , m_object(nullptr)
    {
    }

    AZ::u32 AnimGraphParameterMaskHandler::GetHandlerName() const
    {
        return AZ_CRC_CE("AnimGraphParameterMask");
    }

    QWidget* AnimGraphParameterMaskHandler::CreateGUI(QWidget* parent)
    {
        AnimGraphParameterPicker* picker = aznew AnimGraphParameterPicker(parent, false, true);

        connect(picker, &AnimGraphParameterPicker::ParametersChanged, this, [picker]([[maybe_unused]] const AZStd::vector<AZStd::string>& newParameters)
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, picker);
        });
        return picker;
    }

    void AnimGraphParameterMaskHandler::ConsumeAttribute(AnimGraphParameterPicker* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
    {
        if (attrValue)
        {
            AnimGraphNode* node = static_cast<AnimGraphNode*>(attrValue->GetInstance());
            m_object = azdynamic_cast<ObjectAffectedByParameterChanges*>(node);
            GUI->SetObjectAffectedByParameterChanges(m_object);
        }

        if (attrib == AZ::Edit::Attributes::ReadOnly)
        {
            bool value;
            if (attrValue->Read<bool>(value))
            {
                GUI->setEnabled(!value);
            }
        }
    }

    void AnimGraphParameterMaskHandler::WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, [[maybe_unused]] AnimGraphParameterPicker* GUI, property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        // Don't update the parameter names yet, we still need the information for constructing the command group.
        instance = m_object->GetParameters();
    }

    bool AnimGraphParameterMaskHandler::ReadValuesIntoGUI([[maybe_unused]] size_t index, AnimGraphParameterPicker* GUI, const property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker signalBlocker(GUI);
        GUI->InitializeParameterNames(instance);
        return true;
    }
} // namespace EMotionFX

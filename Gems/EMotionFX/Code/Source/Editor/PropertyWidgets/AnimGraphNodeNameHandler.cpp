/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <Editor/PropertyWidgets/AnimGraphNodeNameHandler.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphNodeNameLineEdit, EditorAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphNodeNameHandler, EditorAllocator, 0)

    AnimGraphNodeNameLineEdit::AnimGraphNodeNameLineEdit(QWidget* parent)
        : LineEditValidatable(parent)
        , m_node(nullptr)
    {
        SetValidatorFunc([this]()
        {
            if (m_node)
            {
                const AnimGraph* animGraph = m_node->GetAnimGraph();
                return animGraph->IsNodeNameUnique(text().toUtf8().data(), m_node);
            }

            return false;
        });
    }

    void AnimGraphNodeNameLineEdit::SetNode(AnimGraphNode* node)
    {
        m_node = node;
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    AnimGraphNodeNameHandler::AnimGraphNodeNameHandler()
        : QObject()
        , AzToolsFramework::PropertyHandler<AZStd::string, AnimGraphNodeNameLineEdit>()
    {
    }


    AZ::u32 AnimGraphNodeNameHandler::GetHandlerName() const
    {
        return AZ_CRC("AnimGraphNodeName", 0x15120d7d);
    }


    QWidget* AnimGraphNodeNameHandler::CreateGUI(QWidget* parent)
    {
        AnimGraphNodeNameLineEdit* lineEdit = aznew AnimGraphNodeNameLineEdit(parent);

        connect(lineEdit, &AnimGraphNodeNameLineEdit::TextEditingFinished, this, [lineEdit]()
        {
            EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, lineEdit);
        });

        return lineEdit;
    }


    void AnimGraphNodeNameHandler::ConsumeAttribute(AnimGraphNodeNameLineEdit* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
    {
        if (attrValue)
        {
            m_node = static_cast<AnimGraphNode*>(attrValue->GetInstancePointer());
            GUI->SetNode(m_node);
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


    void AnimGraphNodeNameHandler::WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, AnimGraphNodeNameLineEdit* GUI, [[maybe_unused]] property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        if (!m_node)
        {
            AZ_Error("EMotionFX", false, "Cannot set new name (%s) to anim graph node named %s. Node is not valid.", GUI->text().toUtf8().data(), GUI->GetPreviousText().toUtf8().data());
            return;
        }

        const AnimGraph* animGraph = m_node->GetAnimGraph();
        const AZStd::string command = AZStd::string::format("AnimGraphAdjustNode -animGraphID %d -name \"%s\" -newName \"%s\"", animGraph->GetID(), GUI->GetPreviousText().toUtf8().data(), GUI->text().toUtf8().data());

        AZStd::string result;
        if (!CommandSystem::GetCommandManager()->ExecuteCommand(command, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
            GUI->setText(GUI->GetPreviousText());
        }
        else
        {
            GUI->SetPreviousText(GUI->text());
        }
    }


    bool AnimGraphNodeNameHandler::ReadValuesIntoGUI([[maybe_unused]] size_t index, AnimGraphNodeNameLineEdit* GUI, const property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker signalBlocker(GUI);
        GUI->SetPreviousText(instance.c_str());
        GUI->setText(instance.c_str());
        return true;
    }
} // namespace EMotionFX

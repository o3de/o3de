/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/SimulatedObjectSetup.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <Editor/PropertyWidgets/SimulatedObjectNameHandler.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(SimulatedObjectNameLineEdit, AZ::SystemAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(SimulatedObjectNameHandler, AZ::SystemAllocator)

    SimulatedObjectNameLineEdit::SimulatedObjectNameLineEdit(QWidget* parent)
        : LineEditValidatable(parent)
        , m_simulatedObject(nullptr)
    {
        SetValidatorFunc([this]() {
            if (m_simulatedObject)
            {
                const SimulatedObjectSetup* setup = m_simulatedObject->GetSimulatedObjectSetup();
                AZ_Assert(setup, "Simulated object %s does not belong to a simulated object setup.", m_simulatedObject->GetName().c_str());
                return setup->IsSimulatedObjectNameUnique(text().toUtf8().data(), m_simulatedObject);
            }

            return false;
        });

        connect(this, &LineEditValidatable::TextEditingFinished, this, [this]() {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, this);
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, this);
        });
    }

    void SimulatedObjectNameLineEdit::SetSimulatedObject(SimulatedObject* simulatedObject)
    {
        m_simulatedObject = simulatedObject;
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    SimulatedObjectNameHandler::SimulatedObjectNameHandler()
        : QObject()
        , AzToolsFramework::PropertyHandler<AZStd::string, SimulatedObjectNameLineEdit>()
    {
    }

    AZ::u32 SimulatedObjectNameHandler::GetHandlerName() const
    {
        return AZ_CRC_CE("SimulatedObjectName");
    }

    QWidget* SimulatedObjectNameHandler::CreateGUI(QWidget* parent)
    {
        SimulatedObjectNameLineEdit* lineEdit = aznew SimulatedObjectNameLineEdit(parent);
        return lineEdit;
    }

    void SimulatedObjectNameHandler::ConsumeAttribute(SimulatedObjectNameLineEdit* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
    {
        if (attrib == AZ::Edit::Attributes::ReadOnly)
        {
            bool value;
            if (attrValue->Read<bool>(value))
            {
                GUI->setEnabled(!value);
            }
        }
    }

    void SimulatedObjectNameHandler::WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, SimulatedObjectNameLineEdit* GUI, property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->text().toUtf8().data();
    }

    bool SimulatedObjectNameHandler::ReadValuesIntoGUI([[maybe_unused]] size_t index, SimulatedObjectNameLineEdit* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker signalBlocker(GUI);
        GUI->SetPreviousText(instance.c_str());
        GUI->setText(instance.c_str());

        if (node && node->GetParent())
        {
            AzToolsFramework::InstanceDataNode* parentNode = node->GetParent();
            m_simulatedObject = static_cast<SimulatedObject*>(parentNode->FirstInstance());
            GUI->SetSimulatedObject(m_simulatedObject);
        }
        else
        {
            GUI->SetSimulatedObject(nullptr);
        }

        return true;
    }
} // namespace EMotionFX

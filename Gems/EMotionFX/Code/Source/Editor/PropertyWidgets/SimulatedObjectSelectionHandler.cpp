/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/PropertyWidgets/SimulatedObjectSelectionHandler.h>
#include <Editor/AnimGraphEditorBus.h>
#include <Editor/Plugins/SimulatedObject/SimulatedObjectSelectionWindow.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <QHBoxLayout>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(SimulatedObjectPicker, AZ::SystemAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(SimulatedObjectSelectionHandler, AZ::SystemAllocator)

    //---------------------------------------------------------------------------------------------------------------------------------------------------------
    SimulatedObjectPicker::SimulatedObjectPicker(QWidget* parent)
        : QWidget(parent)
    {
        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setMargin(0);

        m_pickButton = new QPushButton(this);
        connect(m_pickButton, &QPushButton::clicked, this, &SimulatedObjectPicker::OnPickClicked);
        hLayout->addWidget(m_pickButton);

        m_resetButton = new QPushButton(this);
        EMStudio::EMStudioManager::MakeTransparentButton(m_resetButton, "Images/Icons/Clear.svg", "Reset selection");
        connect(m_resetButton, &QPushButton::clicked, this, &SimulatedObjectPicker::OnResetClicked);
        hLayout->addWidget(m_resetButton);

        setLayout(hLayout);
    }

    void SimulatedObjectPicker::SetSimulatedObjects(const AZStd::vector<AZStd::string>& simulatedObjectNames)
    {
        m_simulatedObjectNames = simulatedObjectNames;
        UpdateInterface();
    }

    const AZStd::vector<AZStd::string>& SimulatedObjectPicker::GetSimulatedObjects() const
    {
        return m_simulatedObjectNames;
    }

    void SimulatedObjectPicker::UpdateSimulatedObjects(const AZStd::vector<AZStd::string>& simulatedObjectNames)
    {
        if (m_simulatedObjectNames != simulatedObjectNames)
        {
            m_simulatedObjectNames = simulatedObjectNames;
            emit SelectionChanged(m_simulatedObjectNames);

            UpdateInterface();
        }
    }

    void SimulatedObjectPicker::UpdateInterface()
    {
        // Set the picker button name based on the number of objects.
        const size_t numObjects = m_simulatedObjectNames.size();
        if (numObjects == 0)
        {
            m_pickButton->setText("Select simulated objects");
        }
        else if (numObjects == 1)
        {
            m_pickButton->setText(m_simulatedObjectNames[0].c_str());
        }
        else
        {
            m_pickButton->setText(QString("%1 simulated objects").arg(numObjects));
        }
    }

    void SimulatedObjectPicker::OnPickClicked()
    {
        Actor* selectedActor = nullptr;
        ActorInstance* selectedActorInstance = nullptr;
        ActorEditorRequestBus::BroadcastResult(selectedActorInstance, &ActorEditorRequests::GetSelectedActorInstance);

        if (selectedActorInstance)
        {
            selectedActor = selectedActorInstance->GetActor();
        }
        else
        {
            ActorEditorRequestBus::BroadcastResult(selectedActor, &ActorEditorRequests::GetSelectedActor);
        }

        if (!selectedActor)
        {
            return;
        }

        // Create and show the simulated object picker window
        EMStudio::SimulatedObjectSelectionWindow selectionWindow(this);
        selectionWindow.Update(selectedActor, m_simulatedObjectNames);
        selectionWindow.setModal(true);

        if (selectionWindow.exec() != QDialog::Rejected)
        {
            UpdateSimulatedObjects(selectionWindow.GetSimulatedObjectSelectionWidget()->GetSelectedSimulatedObjectNames());
        }
    }

    void SimulatedObjectPicker::OnResetClicked()
    {
        if (m_simulatedObjectNames.empty())
        {
            return;
        }
        UpdateSimulatedObjects({});
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------
    AZ::u32 SimulatedObjectSelectionHandler::GetHandlerName() const
    {
        return AZ_CRC_CE("SimulatedObjectSelection");
    }

    QWidget* SimulatedObjectSelectionHandler::CreateGUI(QWidget* parent)
    {
        SimulatedObjectPicker* picker = aznew SimulatedObjectPicker(parent);

        connect(picker, &SimulatedObjectPicker::SelectionChanged, this, [picker]([[maybe_unused]] const AZStd::vector<AZStd::string>& newSimulatedObjects)
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, picker);
        });

        return picker;
    }

    void SimulatedObjectSelectionHandler::ConsumeAttribute(SimulatedObjectPicker* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
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

    void SimulatedObjectSelectionHandler::WriteGUIValuesIntoProperty(size_t index, SimulatedObjectPicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);
        instance = GUI->GetSimulatedObjects();
    }

    bool SimulatedObjectSelectionHandler::ReadValuesIntoGUI(size_t index, SimulatedObjectPicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);
        QSignalBlocker signalBlocker(GUI);
        GUI->SetSimulatedObjects(instance);
        return true;
    }
} // namespace EMotionFX

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/PropertyWidgets/TransitionStateFilterLocalHandler.h>
#include <Editor/ActorEditorBus.h>
#include <EMotionFX/Source/Attachment.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/NodeSelectionWindow.h>
#include <Editor/AnimGraphEditorBus.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/StateFilterSelectionWindow.h>
#include <QHBoxLayout>
#include <QMessageBox>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(TransitionStateFilterPicker, EditorAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(TransitionStateFilterLocalHandler, EditorAllocator)

    TransitionStateFilterPicker::TransitionStateFilterPicker(AnimGraphStateMachine* stateMachine, QWidget* parent)
        : QWidget(parent)
        , m_stateMachine(stateMachine)
    {
        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setMargin(0);

        m_pickButton = new QPushButton(this);
        connect(m_pickButton, &QPushButton::clicked, this, &TransitionStateFilterPicker::OnPickClicked);
        hLayout->addWidget(m_pickButton);

        m_resetButton = new QPushButton(this);
        EMStudio::EMStudioManager::MakeTransparentButton(m_resetButton, "Images/Icons/Clear.svg", "Reset selection");
        connect(m_resetButton, &QPushButton::clicked, this, &TransitionStateFilterPicker::OnResetClicked);
        hLayout->addWidget(m_resetButton);

        setLayout(hLayout);
    }


    void TransitionStateFilterPicker::SetStateMachine(AnimGraphStateMachine* stateMachine)
    {
        m_stateMachine = stateMachine;
    }


    void TransitionStateFilterPicker::OnResetClicked()
    {
        if (m_stateFilter.IsEmpty())
        {
            return;
        }

        m_stateFilter.Clear();
        emit SelectionChanged();
    }


    void TransitionStateFilterPicker::UpdateInterface()
    {
        AZStd::vector<AnimGraphNodeId> stateIds = m_stateFilter.CollectStates(m_stateMachine);
        const size_t numStates = stateIds.size();

        if (numStates == 0 || !m_stateMachine)
        {
            m_pickButton->setText("Select states");
            m_resetButton->setVisible(false);
        }
        else if (numStates == 1)
        {
            const AnimGraphNode* state = m_stateMachine->FindChildNodeById(stateIds[0]);
            if (state)
            {
                m_pickButton->setText(state->GetName());
                m_resetButton->setVisible(true);
            }
        }
        else
        {
            m_pickButton->setText(AZStd::string::format("%zu states", numStates).c_str());
            m_resetButton->setVisible(true);
        }
    }


    void TransitionStateFilterPicker::SetStateFilter(const AnimGraphStateTransition::StateFilterLocal& stateFilter)
    {
        m_stateFilter = stateFilter;
        UpdateInterface();
    }


    AnimGraphStateTransition::StateFilterLocal TransitionStateFilterPicker::GetStateFilter() const
    {
        return m_stateFilter;
    }


    void TransitionStateFilterPicker::OnPickClicked()
    {
        if (!m_stateMachine)
        {
            AZ_Error("EMotionFX", false, "Cannot open local state filter selection window. No valid state machine.");
            return;
        }

        // create the dialog
        EMStudio::StateFilterSelectionWindow dialog(this);
        dialog.ReInit(m_stateMachine, m_stateFilter.CollectStateIds(), m_stateFilter.GetGroups());
        if (dialog.exec() != QDialog::Rejected)
        {
            m_stateFilter.SetStateIds(dialog.GetSelectedNodeIds());
            m_stateFilter.SetGroups(dialog.GetSelectedGroupNames());

            UpdateInterface();
            emit SelectionChanged();
        }
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    TransitionStateFilterLocalHandler::TransitionStateFilterLocalHandler()
        : QObject()
        , AzToolsFramework::PropertyHandler<AnimGraphStateTransition::StateFilterLocal, TransitionStateFilterPicker>()
        , m_stateMachine(nullptr)
    {
    }


    AZ::u32 TransitionStateFilterLocalHandler::GetHandlerName() const
    {
        return AZ_CRC_CE("TransitionStateFilterLocal");
    }


    QWidget* TransitionStateFilterLocalHandler::CreateGUI(QWidget* parent)
    {
        TransitionStateFilterPicker* picker = aznew TransitionStateFilterPicker(m_stateMachine, parent);

        connect(picker, &TransitionStateFilterPicker::SelectionChanged, this, [picker]()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, picker);
        });

        return picker;
    }


    void TransitionStateFilterLocalHandler::ConsumeAttribute(TransitionStateFilterPicker* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
    {
        if (attrib == AZ::Edit::Attributes::ReadOnly)
        {
            bool value;
            if (attrValue->Read<bool>(value))
            {
                GUI->setEnabled(!value);
            }
        }

        if (attrib == AZ_CRC_CE("StateMachine"))
        {
            attrValue->Read<AnimGraphStateMachine*>(m_stateMachine);
            GUI->SetStateMachine(m_stateMachine);
        }
    }


    void TransitionStateFilterLocalHandler::WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, TransitionStateFilterPicker* GUI, property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetStateFilter();
    }


    bool TransitionStateFilterLocalHandler::ReadValuesIntoGUI([[maybe_unused]] size_t index, TransitionStateFilterPicker* GUI, const property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker signalBlocker(GUI);
        GUI->SetStateFilter(instance);
        return true;
    }
} // namespace EMotionFX

#include <Source/Editor/PropertyWidgets/moc_TransitionStateFilterLocalHandler.cpp>

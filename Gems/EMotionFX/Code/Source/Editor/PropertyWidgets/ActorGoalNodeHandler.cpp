/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/PropertyWidgets/ActorGoalNodeHandler.h>
#include <Editor/ActorEditorBus.h>
#include <EMotionFX/Source/Attachment.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/NodeSelectionWindow.h>
#include <Editor/AnimGraphEditorBus.h>
#include <QHBoxLayout>
#include <QMessageBox>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(ActorGoalNodePicker, EditorAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(ActorGoalNodeHandler, EditorAllocator)

    ActorGoalNodePicker::ActorGoalNodePicker(QWidget* parent)
        : QWidget(parent)
    {
        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setMargin(0);

        m_pickButton = new QPushButton(this);
        connect(m_pickButton, &QPushButton::clicked, this, &ActorGoalNodePicker::OnPickClicked);
        hLayout->addWidget(m_pickButton);

        m_resetButton = new QPushButton(this);
        EMStudio::EMStudioManager::MakeTransparentButton(m_resetButton, "Images/Icons/Clear.svg", "Reset selection");
        connect(m_resetButton, &QPushButton::clicked, this, &ActorGoalNodePicker::OnResetClicked);
        hLayout->addWidget(m_resetButton);

        setLayout(hLayout);
    }


    void ActorGoalNodePicker::OnResetClicked()
    {
        if (m_goalNode.first.empty() && m_goalNode.second == 0)
        {
            return;
        }

        SetGoalNode(AZStd::make_pair(AZStd::string(), 0));
        emit SelectionChanged();
    }


    void ActorGoalNodePicker::UpdateInterface()
    {
        if (m_goalNode.first.empty())
        {
            m_pickButton->setText("Select node");
            m_resetButton->setVisible(false);
        }
        else
        {
            m_pickButton->setText(m_goalNode.first.c_str());
            m_resetButton->setVisible(true);
        }
    }


    void ActorGoalNodePicker::SetGoalNode(const AZStd::pair<AZStd::string, int>& goalNode)
    {
        m_goalNode = goalNode;
        UpdateInterface();
    }


    AZStd::pair<AZStd::string, int> ActorGoalNodePicker::GetGoalNode() const
    {
        return m_goalNode;
    }


    void ActorGoalNodePicker::OnPickClicked()
    {
        EMotionFX::ActorInstance* actorInstance = nullptr;
        ActorEditorRequestBus::BroadcastResult(actorInstance, &ActorEditorRequests::GetSelectedActorInstance);
        if (!actorInstance)
        {
            QMessageBox::warning(this, "No Actor Instance", "Cannot open node selection window. No valid actor instance selected.");
            return;
        }
        EMotionFX::Actor* actor = actorInstance->GetActor();

        // Create and show the node picker window
        EMStudio::NodeSelectionWindow nodeSelectionWindow(this, true);
        nodeSelectionWindow.GetNodeHierarchyWidget()->SetSelectionMode(true);

        CommandSystem::SelectionList prevSelection;
        EMotionFX::Node* node = actor->GetSkeleton()->FindNodeByName(m_goalNode.first.c_str());
        if (node)
        {
            prevSelection.AddNode(node);
        }


        AZStd::vector<uint32> actorInstanceIDs;

        // Add the current actor instance and all the ones it is attached to
        EMotionFX::ActorInstance* currentInstance = actorInstance;
        while (currentInstance)
        {
            actorInstanceIDs.emplace_back(currentInstance->GetID());
            EMotionFX::Attachment* attachment = currentInstance->GetSelfAttachment();
            if (attachment)
            {
                currentInstance = attachment->GetAttachToActorInstance();
            }
            else
            {
                currentInstance = nullptr;
            }
        }


        nodeSelectionWindow.Update(actorInstanceIDs, &prevSelection);
        nodeSelectionWindow.setModal(true);

        if (nodeSelectionWindow.exec() != QDialog::Rejected)
        {
            AZStd::vector<SelectionItem>& newSelection = nodeSelectionWindow.GetNodeHierarchyWidget()->GetSelectedItems();
            if (newSelection.size() == 1)
            {
                AZStd::string selectedNodeName = newSelection[0].GetNodeName();
                AZ::u32 selectedActorInstanceId = newSelection[0].m_actorInstanceId;

                const auto parentDepth = AZStd::find(begin(actorInstanceIDs), end(actorInstanceIDs), selectedActorInstanceId);
                AZ_Assert(parentDepth != end(actorInstanceIDs), "Cannot get parent depth. The selected actor instance was not shown in the selection window.");

                m_goalNode = {AZStd::move(selectedNodeName), static_cast<int>(AZStd::distance(begin(actorInstanceIDs), parentDepth))};

                UpdateInterface();
                emit SelectionChanged();
            }
        }
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    AZ::u32 ActorGoalNodeHandler::GetHandlerName() const
    {
        return AZ_CRC_CE("ActorGoalNode");
    }


    QWidget* ActorGoalNodeHandler::CreateGUI(QWidget* parent)
    {
        ActorGoalNodePicker* picker = aznew ActorGoalNodePicker(parent);

        connect(picker, &ActorGoalNodePicker::SelectionChanged, this, [picker]()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, picker);
        });

        return picker;
    }


    void ActorGoalNodeHandler::ConsumeAttribute(ActorGoalNodePicker* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
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


    void ActorGoalNodeHandler::WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, ActorGoalNodePicker* GUI, property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetGoalNode();
    }


    bool ActorGoalNodeHandler::ReadValuesIntoGUI([[maybe_unused]] size_t index, ActorGoalNodePicker* GUI, const property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker signalBlocker(GUI);
        GUI->SetGoalNode(instance);
        return true;
    }
} // namespace EMotionFX

#include <Source/Editor/PropertyWidgets/moc_ActorGoalNodeHandler.cpp>

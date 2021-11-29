/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <Editor/ActorJointBrowseEdit.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/NodeSelectionWindow.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/ActorManager.h>
#include <QTreeWidget>

namespace EMStudio
{
    AZ_CLASS_ALLOCATOR_IMPL(ActorJointBrowseEdit, AZ::SystemAllocator, 0)

    ActorJointBrowseEdit::ActorJointBrowseEdit(QWidget* parent)
        : AzQtComponents::BrowseEdit(parent)
    {
        connect(this, &AzQtComponents::BrowseEdit::attachedButtonTriggered, this, &ActorJointBrowseEdit::OnBrowseButtonClicked);
        connect(this, &AzQtComponents::BrowseEdit::textEdited, this, &ActorJointBrowseEdit::OnTextEdited);

        SetSingleJointSelection(m_singleJointSelection);
        setClearButtonEnabled(true);
        setLineEditReadOnly(true);
    }

    void ActorJointBrowseEdit::SetSingleJointSelection(bool singleSelectionEnabled)
    {
        m_singleJointSelection = singleSelectionEnabled;
        UpdatePlaceholderText();
    }

    void ActorJointBrowseEdit::UpdatePlaceholderText()
    {
        const QString placeholderText = QString("Select joint%1").arg(m_singleJointSelection ? "" : "s");
        setPlaceholderText(placeholderText);
    }

    void ActorJointBrowseEdit::OnBrowseButtonClicked()
    {
        const EMotionFX::ActorInstance* actorInstance = GetActorInstance();
        if (!actorInstance)
        {
            AZ_Warning("EMotionFX", false, "Cannot open joint selection window. Please select an actor instance first.");
            return;
        }

        CommandSystem::SelectionList selectionList;
        for (const SelectionItem& selectedJoint : m_selectedJoints)
        {
            EMotionFX::Node* node = selectedJoint.GetNode();
            if (node)
            {
                selectionList.AddNode(node);
            }
        }
        AZ_Warning("EMotionFX", m_singleJointSelection && selectionList.GetNumSelectedNodes() > 1,
            "Single selection actor joint window has multiple pre-selected joints.");

        m_previouslySelectedJoints = m_selectedJoints;

        m_jointSelectionWindow = new NodeSelectionWindow(this, m_singleJointSelection);
        connect(m_jointSelectionWindow, &NodeSelectionWindow::rejected, this, &ActorJointBrowseEdit::OnSelectionRejected);
        connect(m_jointSelectionWindow->GetNodeHierarchyWidget()->GetTreeWidget(), &QTreeWidget::itemSelectionChanged, this, &ActorJointBrowseEdit::OnSelectionChanged);
        connect(m_jointSelectionWindow->GetNodeHierarchyWidget(), &NodeHierarchyWidget::OnSelectionDone, this, &ActorJointBrowseEdit::OnSelectionDone);

        NodeSelectionWindow::connect(m_jointSelectionWindow, &QDialog::finished, [=]([[maybe_unused]] int resultCode)
            {
                m_jointSelectionWindow->deleteLater();
                m_jointSelectionWindow = nullptr;
            });

        m_jointSelectionWindow->open();
        m_jointSelectionWindow->Update(actorInstance->GetID(), &selectionList);
    }

    void ActorJointBrowseEdit::SetSelectedJoints(const AZStd::vector<SelectionItem>& selectedJoints)
    {
        if (m_singleJointSelection && selectedJoints.size() > 1)
        {
            AZ_Warning("EMotionFX", true, "Cannot select multiple joints for single selection actor joint browse edit. Only the first joint will be selected.");
            m_selectedJoints = {selectedJoints[0]};
        }
        else
        {
            m_selectedJoints = selectedJoints;
        }

        QString text;
        const size_t numJoints = m_selectedJoints.size();
        switch (numJoints)
        {
            case 0:
            {
                // Leave text empty (shows placeholder text).
                break;
            }
            case 1:
            {
                text = m_selectedJoints[0].GetNodeName();
                break;
            }
            default:
            {
                text = QString("%1 joints").arg(numJoints);
            }
        }
        setText(text);
    }

    void ActorJointBrowseEdit::OnSelectionDone(const AZStd::vector<SelectionItem>& selectedJoints)
    {
        SetSelectedJoints(selectedJoints);
        emit SelectionDone(selectedJoints);
    }

    void ActorJointBrowseEdit::OnSelectionChanged()
    {
        if (m_jointSelectionWindow)
        {
            NodeHierarchyWidget* hierarchyWidget = m_jointSelectionWindow->GetNodeHierarchyWidget();
            hierarchyWidget->UpdateSelection();
            AZStd::vector<SelectionItem>& selectedJoints = hierarchyWidget->GetSelectedItems();

            emit SelectionChanged(selectedJoints);
        }
    }

    void ActorJointBrowseEdit::OnSelectionRejected()
    {
        emit SelectionRejected(m_previouslySelectedJoints);
    }

    void ActorJointBrowseEdit::OnTextEdited(const QString& text)
    {
        if (text.isEmpty())
        {
            OnSelectionDone(/*selectedJoints=*/{});
        }
    }

    EMotionFX::ActorInstance* ActorJointBrowseEdit::GetActorInstance() const
    {
        const CommandSystem::SelectionList& selectionList = CommandSystem::GetCommandManager()->GetCurrentSelection();
        EMotionFX::ActorInstance* actorInstance = selectionList.GetSingleActorInstance();
        if (actorInstance)
        {
            return actorInstance;
        }

        EMotionFX::Actor* actor = selectionList.GetSingleActor();
        if (actor)
        {
            const size_t numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
            for (size_t i = 0; i < numActorInstances; ++i)
            {
                EMotionFX::ActorInstance* actorInstance2 = EMotionFX::GetActorManager().GetActorInstance(i);
                if (actorInstance2->GetActor() == actor)
                {
                    return actorInstance2;
                }
            }
        }

        return nullptr;
    }

} // namespace EMStudio

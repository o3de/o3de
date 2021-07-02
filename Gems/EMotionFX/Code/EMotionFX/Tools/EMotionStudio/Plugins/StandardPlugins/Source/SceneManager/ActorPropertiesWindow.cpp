/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ActorPropertiesWindow.h"
#include "SceneManagerPlugin.h"
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QWindow>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/MotionManager.h>
#include "MirrorSetupWindow.h"

namespace EMStudio
{
    ActorPropertiesWindow::ActorPropertiesWindow(QWidget* parent, SceneManagerPlugin* plugin)
        : QWidget(parent)
    {
        mPlugin = plugin;
    }

    // init after the parent dock window has been created
    void ActorPropertiesWindow::Init()
    {
        QVBoxLayout* mainVerticalLayout = new QVBoxLayout();
        mainVerticalLayout->setMargin(0);
        setLayout(mainVerticalLayout);

        QGridLayout* layout = new QGridLayout();
        uint32 rowNr = 0;
        layout->setMargin(0);
        layout->setVerticalSpacing(0);
        layout->setAlignment(Qt::AlignLeft);
        mainVerticalLayout->addLayout(layout);

        // actor name
        rowNr = 0;
        layout->addWidget(new QLabel("Actor name"), rowNr, 0);
        mNameEdit = new QLineEdit();
        connect(mNameEdit, &QLineEdit::editingFinished, this, &ActorPropertiesWindow::NameEditChanged);
        layout->addWidget(mNameEdit, rowNr, 1);

        // Motion extraction joint.
        rowNr++;
        QHBoxLayout* extractNodeLayout = new QHBoxLayout();
        extractNodeLayout->setDirection(QBoxLayout::LeftToRight);
        extractNodeLayout->setMargin(0);

        m_motionExtractionJointBrowseEdit = aznew ActorJointBrowseEdit(this);
        m_motionExtractionJointBrowseEdit->setToolTip("The joint used to drive the character's movement and rotation.");
        m_motionExtractionJointBrowseEdit->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        layout->addWidget(new QLabel("Motion extraction joint"), rowNr, 0);
        extractNodeLayout->addWidget(m_motionExtractionJointBrowseEdit);
        connect(m_motionExtractionJointBrowseEdit, &ActorJointBrowseEdit::SelectionDone, this, &ActorPropertiesWindow::OnMotionExtractionJointSelected);

        // Find best match for motion extraction joint.
        m_findBestMatchButton = new QPushButton("Find best match");
        m_findBestMatchButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        extractNodeLayout->addWidget(m_findBestMatchButton);
        connect(m_findBestMatchButton, &QPushButton::clicked, this, &ActorPropertiesWindow::OnFindBestMatchingNode);

        layout->addLayout(extractNodeLayout, rowNr, 1);

        // Retarget root joint.
        rowNr++;
        m_retargetRootJointBrowseEdit = aznew ActorJointBrowseEdit(this);
        m_retargetRootJointBrowseEdit->setToolTip("The root joint that will use special handling when retargeting. Z must point up.");
        m_retargetRootJointBrowseEdit->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        connect(m_retargetRootJointBrowseEdit, &ActorJointBrowseEdit::SelectionDone, this, &ActorPropertiesWindow::OnRetargetRootJointSelected);
        layout->addWidget(new QLabel("Retarget root joint"), rowNr, 0);
        layout->addWidget(m_retargetRootJointBrowseEdit, rowNr, 1);

        // bounding box nodes
        rowNr++;
        m_excludeFromBoundsBrowseEdit = aznew ActorJointBrowseEdit(this);
        m_excludeFromBoundsBrowseEdit->setToolTip("Joints that are excluded from bounding volume calculations.");
        m_excludeFromBoundsBrowseEdit->SetSingleJointSelection(false);
        m_excludeFromBoundsBrowseEdit->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

        connect(m_excludeFromBoundsBrowseEdit, &ActorJointBrowseEdit::SelectionDone, this, &ActorPropertiesWindow::OnExcludedJointsFromBoundsSelectionDone);
        connect(m_excludeFromBoundsBrowseEdit, &ActorJointBrowseEdit::SelectionChanged, this, &ActorPropertiesWindow::OnExcludedJointsFromBoundsSelectionChanged);
        connect(m_excludeFromBoundsBrowseEdit, &ActorJointBrowseEdit::SelectionRejected, this, &ActorPropertiesWindow::OnExcludedJointsFromBoundsSelectionChanged);

        layout->addWidget(new QLabel("Excluded from bounds"), rowNr, 0);
        layout->addWidget(m_excludeFromBoundsBrowseEdit, rowNr, 1);

        // mirror setup
        rowNr++;
        mMirrorSetupWindow = new MirrorSetupWindow(mPlugin->GetDockWidget(), mPlugin);
        mMirrorSetupLink = new AzQtComponents::BrowseEdit();
        mMirrorSetupLink->setClearButtonEnabled(true);
        mMirrorSetupLink->setLineEditReadOnly(true);
        mMirrorSetupLink->setPlaceholderText("Click folder to setup");
        layout->addWidget(new QLabel("Mirror setup"), rowNr, 0);
        layout->addWidget(mMirrorSetupLink, rowNr, 1);
        connect(mMirrorSetupLink, &AzQtComponents::BrowseEdit::attachedButtonTriggered, this, &ActorPropertiesWindow::OnMirrorSetup);

        UpdateInterface();
    }


    void ActorPropertiesWindow::UpdateInterface()
    {
        mActor         = nullptr;
        mActorInstance = nullptr;

        EMotionFX::ActorInstance*   actorInstance   = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
        EMotionFX::Actor*           actor           = GetCommandManager()->GetCurrentSelection().GetSingleActor();

        // in case we have selected a single actor instance
        if (actorInstance)
        {
            mActorInstance  = actorInstance;
            mActor          = actorInstance->GetActor();
        }
        // in case we have selected a single actor
        else if (actor)
        {
            mActor = actor;

            const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
            for (uint32 i = 0; i < numActorInstances; ++i)
            {
                EMotionFX::ActorInstance* currentInstance = EMotionFX::GetActorManager().GetActorInstance(i);
                if (currentInstance->GetActor() == actor)
                {
                    mActorInstance = currentInstance;
                    break;
                }
            }
        }

        mMirrorSetupWindow->Reinit();

        if (mActorInstance == nullptr || mActor == nullptr)
        {
            // reset data and disable interface elements
            m_motionExtractionJointBrowseEdit->setEnabled(false);
            m_motionExtractionJointBrowseEdit->SetSelectedJoints({});

            m_findBestMatchButton->setVisible(false);
            m_retargetRootJointBrowseEdit->setEnabled(false);
            m_retargetRootJointBrowseEdit->SetSelectedJoints({});

            // nodes excluded from bounding volume calculations
            m_excludeFromBoundsBrowseEdit->setEnabled(false);
            m_excludeFromBoundsBrowseEdit->SetSelectedJoints({});

            // actor name
            mNameEdit->setEnabled(false);
            mNameEdit->setText("");

            mMirrorSetupLink->setEnabled(false);

            return;
        }

        mMirrorSetupLink->setEnabled(true);

        // Motion extraction node
        EMotionFX::Node* extractionNode = mActor->GetMotionExtractionNode();
        m_motionExtractionJointBrowseEdit->setEnabled(true);
        if (extractionNode)
        {
            m_motionExtractionJointBrowseEdit->SetSelectedJoints({SelectionItem(mActorInstance->GetID(), extractionNode->GetName())});
        }
        else
        {
            m_motionExtractionJointBrowseEdit->SetSelectedJoints({});
        }

        EMotionFX::Node* bestMatchingNode = mActor->FindBestMotionExtractionNode();
        m_findBestMatchButton->setVisible(bestMatchingNode && mActor->GetMotionExtractionNode() != bestMatchingNode);

        // Retarget root node
        EMotionFX::Node* retargetRootNode = mActor->GetRetargetRootNode();
        m_retargetRootJointBrowseEdit->setEnabled(true);
        if (retargetRootNode)
        {
            m_retargetRootJointBrowseEdit->SetSelectedJoints({SelectionItem(mActorInstance->GetID(), retargetRootNode->GetName())});
        }
        else
        {
            m_retargetRootJointBrowseEdit->SetSelectedJoints({});
        }

        // nodes excluded from bounding volume calculations
        m_excludeFromBoundsBrowseEdit->setEnabled(true);

        AZStd::vector<SelectionItem> jointsExcludedFromBounds;
        if (mActorInstance)
        {
            const uint32 numNodes = mActor->GetNumNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                EMotionFX::Node* node = mActor->GetSkeleton()->GetNode(i);
                if (!node->GetIncludeInBoundsCalc())
                {
                    jointsExcludedFromBounds.emplace_back(mActorInstance->GetID(), node->GetName());
                }
            }
        }
        m_excludeFromBoundsBrowseEdit->SetSelectedJoints(jointsExcludedFromBounds);

        // actor name
        mNameEdit->setEnabled(true);
        mNameEdit->setText(mActor->GetName());
    }

    void ActorPropertiesWindow::GetNodeName(const MCore::Array<SelectionItem>& selection, AZStd::string* outNodeName, uint32* outActorID)
    {
        outNodeName->clear();
        *outActorID = MCORE_INVALIDINDEX32;

        if (selection.GetLength() != 1 || selection[0].GetNodeNameString().empty())
        {
            AZ_Warning("EMotionFX", false, "Cannot adjust motion extraction node. No valid node selected.");
            return;
        }

        const uint32                actorInstanceID = selection[0].mActorInstanceID;
        const char*                 nodeName        = selection[0].GetNodeName();
        EMotionFX::ActorInstance*   actorInstance   = EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID);
        if (actorInstance == nullptr)
        {
            return;
        }

        EMotionFX::Actor* actor = actorInstance->GetActor();
        *outActorID     = actor->GetID();
        *outNodeName    = nodeName;
    }

    void ActorPropertiesWindow::GetNodeName(const AZStd::vector<SelectionItem>& joints, AZStd::string* outNodeName, uint32* outActorID)
    {
        outNodeName->clear();
        *outActorID = MCORE_INVALIDINDEX32;

        if (joints.size() != 1 || joints[0].GetNodeNameString().empty())
        {
            AZ_Warning("EMotionFX", false, "Cannot get node name. No valid node selected.");
            return;
        }

        const uint32 actorInstanceID = joints[0].mActorInstanceID;
        const char* nodeName = joints[0].GetNodeName();
        EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID);
        if (!actorInstance)
        {
            return;
        }

        EMotionFX::Actor* actor = actorInstance->GetActor();
        *outActorID = actor->GetID();
        *outNodeName = nodeName;
    }

    void ActorPropertiesWindow::OnMotionExtractionJointSelected(const AZStd::vector<SelectionItem>& selectedJoints)
    {
        uint32 actorID;
        AZStd::string nodeName;

        if (selectedJoints.empty())
        {
            EMotionFX::ActorInstance* actorInstance = m_motionExtractionJointBrowseEdit->GetActorInstance();
            actorID = actorInstance->GetActor()->GetID();
            nodeName = "$NULL$";
        }
        else
        {
            GetNodeName(selectedJoints, &nodeName, &actorID);
        }

        MCore::CommandGroup commandGroup("Adjust motion extraction node");

        AZStd::string command = AZStd::string::format("AdjustActor -actorID %i -motionExtractionNodeName \"%s\"", actorID, nodeName.c_str());
        commandGroup.AddCommandString(command);

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }

    void ActorPropertiesWindow::OnRetargetRootJointSelected(const AZStd::vector<SelectionItem>& selectedJoints)
    {
        uint32 actorID;
        AZStd::string nodeName;

        if (selectedJoints.empty())
        {
            EMotionFX::ActorInstance* actorInstance = m_retargetRootJointBrowseEdit->GetActorInstance();
            actorID = actorInstance->GetActor()->GetID();
            nodeName = "$NULL$";
        }
        else
        {
            GetNodeName(selectedJoints, &nodeName, &actorID);
        }

        MCore::CommandGroup commandGroup("Adjust retarget root node");
        AZStd::string command = AZStd::string::format("AdjustActor -actorID %i -retargetRootNodeName \"%s\"", actorID, nodeName.c_str());
        commandGroup.AddCommandString(command);

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    // automatically find the best matching motion extraction node and set it
    void ActorPropertiesWindow::OnFindBestMatchingNode()
    {
        // check if the actor is invalid
        if (mActor == nullptr)
        {
            return;
        }

        // find the best motion extraction node
        EMotionFX::Node* bestMatchingNode = mActor->FindBestMotionExtractionNode();
        if (bestMatchingNode == nullptr)
        {
            return;
        }

        // create the command group
        MCore::CommandGroup commandGroup("Adjust motion extraction node");

        // adjust the actor
        const AZStd::string command = AZStd::string::format("AdjustActor -actorID %i -motionExtractionNodeName \"%s\"", mActor->GetID(), bestMatchingNode->GetName());
        commandGroup.AddCommandString(command);

        // execute the command group
        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }

    // actor name changed
    void ActorPropertiesWindow::NameEditChanged()
    {
        if (mActor == nullptr)
        {
            return;
        }

        // execute the command
        const AZStd::string command = AZStd::string::format("AdjustActor -actorID %i -name \"%s\"", mActor->GetID(), mNameEdit->text().toUtf8().data());

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommand(command, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }

    // select the nodes that should be excluded from the bounding volume calculations
    void ActorPropertiesWindow::OnExcludedJointsFromBoundsSelectionDone(const AZStd::vector<SelectionItem>& selectedJoints)
    {
        EMotionFX::ActorInstance* actorInstance = m_excludeFromBoundsBrowseEdit->GetActorInstance();
        if (!actorInstance)
        {
            return;
        }

        AZStd::string command = AZStd::string::format("AdjustActor -actorID %i -nodesExcludedFromBounds \"", mActor->GetID());

        // prepare the nodes excluded from bounds string
        size_t addedItems = 0;
        for (const SelectionItem& selectedJoint : selectedJoints)
        {
            EMotionFX::Node* node = mActor->GetSkeleton()->FindNodeByName(selectedJoint.GetNodeName());
            if (node)
            {
                if (addedItems > 0)
                {
                    command += ';';
                }

                command += node->GetName();
                addedItems++;
            }
        }

        command += "\" -nodeAction \"select\"";

        // Reset the changes we did so that the undo data can be stored correctly.
        OnExcludedJointsFromBoundsSelectionChanged(m_excludeFromBoundsBrowseEdit->GetPreviouslySelectedJoints());

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommand(command, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }
    
    // called when the selection got changed while the window is still opened
    void ActorPropertiesWindow::OnExcludedJointsFromBoundsSelectionChanged(const AZStd::vector<SelectionItem>& selectedJoints)
    {
        EMotionFX::ActorInstance* actorInstance = m_excludeFromBoundsBrowseEdit->GetActorInstance();
        if (!actorInstance)
        {
            return;
        }

        EMotionFX::Actor* actor = actorInstance->GetActor();
        EMotionFX::Skeleton* skeleton = actor->GetSkeleton();
        const uint32 numJoints = mActor->GetNumNodes();

        // Include all joints first.
        for (uint32 i = 0; i < numJoints; ++i)
        {
            skeleton->GetNode(i)->SetIncludeInBoundsCalc(true);
        }

        // Exclude the selected joints.
        for (const SelectionItem& selectedJoint : selectedJoints)
        {
            EMotionFX::Node* node = skeleton->FindNodeByName(selectedJoint.GetNodeName());
            if (node)
            {
                node->SetIncludeInBoundsCalc(false);
            }
        }
    }

    // open the mirror setup
    void ActorPropertiesWindow::OnMirrorSetup()
    {
        if (mMirrorSetupWindow)
        {
            mMirrorSetupWindow->exec();
        }
    }
} // namespace EMStudio

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MotionExtractionWindow.h"
#include "MotionWindowPlugin.h"
#include "../SceneManager/ActorPropertiesWindow.h"
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QIcon>
#include <QCheckBox>
#include <MCore/Source/LogManager.h>
#include <EMotionFX/CommandSystem/Source/MotionCommands.h>
#include <EMotionFX/Source/MotionSystem.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/MotionManager.h>
#include <AzQtComponents/Components/Widgets/CheckBox.h>

namespace EMStudio
{
    MotionExtractionWindow::MotionExtractionWindow(QWidget* parent, MotionWindowPlugin* motionWindowPlugin)
        : QWidget(parent)
    {
        m_motionWindowPlugin                 = motionWindowPlugin;
        m_warningWidget                      = nullptr;
        m_mainVerticalLayout                 = nullptr;
        m_childVerticalLayout                = nullptr;
        m_motionExtractionNodeSelectionWindow= nullptr;
        m_warningSelectNodeLink              = nullptr;
        m_captureHeight                      = nullptr;
        m_warningShowed                      = false;
    }

    MotionExtractionWindow::~MotionExtractionWindow()
    {
        for (MCore::Command::Callback* callback : m_commandCallbacks)
        {
            GetCommandManager()->RemoveCommandCallback(callback, false);
            delete callback;
        }
    }

#define MOTIONEXTRACTIONWINDOW_HEIGHT 54

    // Create the flags widget.
    void MotionExtractionWindow::CreateFlagsWidget()
    {
        m_flagsWidget = new QWidget();

        m_captureHeight = new QCheckBox();
        AzQtComponents::CheckBox::applyToggleSwitchStyle(m_captureHeight);
        connect(m_captureHeight, &QCheckBox::clicked, this, &MotionExtractionWindow::OnMotionExtractionFlagsUpdated);

        QGridLayout* layout = new QGridLayout();
        layout->setAlignment(Qt::AlignTop);
        layout->setSpacing(3);
        layout->addWidget(new QLabel(tr("Capture Height Changes")), 0, 0);
        layout->addWidget(m_captureHeight, 0, 1);
        layout->setContentsMargins(0, 0, 0, 0);
        m_flagsWidget->setLayout(layout);

        m_childVerticalLayout->addWidget(m_flagsWidget);
    }


    // create the warning widget
    void MotionExtractionWindow::CreateWarningWidget()
    {
        // create the warning widget
        m_warningWidget = new QWidget();
        m_warningWidget->setMinimumHeight(MOTIONEXTRACTIONWINDOW_HEIGHT);
        m_warningWidget->setMaximumHeight(MOTIONEXTRACTIONWINDOW_HEIGHT);

        QLabel* warningLabel = new QLabel("<qt>No node has been selected yet to enable Motion Extraction.</qt>");
        warningLabel->setWordWrap(true);
        warningLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

        m_warningSelectNodeLink = new AzQtComponents::BrowseEdit(m_warningWidget);
        m_warningSelectNodeLink->setPlaceholderText("Click here to setup the Motion Extraction node");
        connect(m_warningSelectNodeLink, &AzQtComponents::BrowseEdit::attachedButtonTriggered, this, &MotionExtractionWindow::OnSelectMotionExtractionNode);

        // create and fill the layout
        QVBoxLayout* layout = new QVBoxLayout();

        layout->setAlignment(Qt::AlignTop);
        layout->setContentsMargins(0, 0, 0, 0);

        layout->addWidget(warningLabel);
        layout->addWidget(m_warningSelectNodeLink);

        m_warningWidget->setLayout(layout);

        // add it to our main layout
        m_childVerticalLayout->addWidget(m_warningWidget);
    }

    // init after the parent dock window has been created
    void MotionExtractionWindow::Init()
    {
        m_commandCallbacks.emplace_back(new SelectActorCallback(this));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("Select", m_commandCallbacks.back());
        m_commandCallbacks.emplace_back(new SelectActorCallback(this));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("Unselect", m_commandCallbacks.back());
        m_commandCallbacks.emplace_back(new UpdateMotionExtractionWindowCallback(this));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("ClearSelection", m_commandCallbacks.back());
        m_commandCallbacks.emplace_back(new UpdateMotionExtractionWindowCallback(this));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("AdjustActor", m_commandCallbacks.back());

        // create the node selection windows
        m_motionExtractionNodeSelectionWindow = new NodeSelectionWindow(this, true);
        connect(m_motionExtractionNodeSelectionWindow->GetNodeHierarchyWidget(), &NodeHierarchyWidget::OnSelectionDone, this, &MotionExtractionWindow::OnMotionExtractionNodeSelected);

        // set some layout for our window
        m_mainVerticalLayout = new QVBoxLayout();
        m_mainVerticalLayout->setSpacing(0);
        setLayout(m_mainVerticalLayout);

        QCheckBox* checkBox = new QCheckBox(tr("Motion extraction"));
        checkBox->setChecked(true);
        checkBox->setStyleSheet("QCheckBox::indicator\
                {\
                    width: 16px;\
                    height: 16px;\
                    border: none;\
                    margin: 0px;\
                }\
                \
                QCheckBox::indicator:checked,\
                QCheckBox::indicator:checked:disabled,\
                QCheckBox::indicator:checked:focus\
                {\
                    image: url(:/Cards/img/UI20/Cards/caret-down.svg);\
                }\
                \
                QCheckBox::indicator:unchecked,\
                QCheckBox::indicator:unchecked:disabled,\
                QCheckBox::indicator:unchecked:focus\
                {\
                    image: url(:/Cards/img/UI20/Cards/caret-right.svg);\
                }");
        m_mainVerticalLayout->addWidget(checkBox);
        QWidget* childWidget = new QWidget(this);
        m_mainVerticalLayout->addWidget(childWidget);

        m_childVerticalLayout = new QVBoxLayout(childWidget);
        m_childVerticalLayout->setContentsMargins(28,0,0,0);
        connect(checkBox, &QCheckBox::toggled, childWidget, &QWidget::setVisible);

        // default create the warning widget (this is needed else we're getting a crash when switching layouts as the widget and the flag might be out of sync)
        CreateWarningWidget();
        m_warningShowed = true;

        // update interface
        UpdateInterface();
    }


    void MotionExtractionWindow::UpdateInterface()
    {
        const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();

        // Check if there actually is any motion selected.
        const size_t numSelectedMotions = selectionList.GetNumSelectedMotions();
        const bool isEnabled = (numSelectedMotions != 0);

        EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();

        // Get the motion extraction and the trajectory node.
        EMotionFX::Actor*   actor          = nullptr;
        EMotionFX::Node*    extractionNode = nullptr;

        if (m_captureHeight)
        {
            m_captureHeight->setEnabled( isEnabled );
        }

        if (actorInstance)
        {
            actor           = actorInstance->GetActor();
            extractionNode  = actor->GetMotionExtractionNode();
        }

        if (extractionNode == nullptr)
        {
            // Check if we already show the warning widget, if yes, do nothing.
            if (m_warningShowed == false)
            {
                CreateWarningWidget();

                if (m_flagsWidget)
                {
                    m_flagsWidget->hide();
                    m_flagsWidget->deleteLater();
                    m_flagsWidget = nullptr;
                    m_captureHeight = nullptr;
                }
            }

            // Disable the link in case no actor is selected.
            if (actorInstance == nullptr)
            {
                m_warningSelectNodeLink->setEnabled(false);
            }
            else
            {
                m_warningSelectNodeLink->setEnabled(true);
            }

            // Return directly in case we show the warning widget.
            m_warningShowed = true;
            return;
        }
        else
        {
            // Check if we already show the motion extraction flags widget, if yes, do nothing.
            if (m_warningShowed)
            {
                if (m_warningWidget)
                {
                    m_warningWidget->hide();
                    m_warningWidget->deleteLater();
                    m_warningWidget = nullptr;
                }

                CreateFlagsWidget();
            }

            if (m_captureHeight)
            {
                m_captureHeight->setEnabled( isEnabled );
            }

            // Figure out if all selected motions use the same settings.
            bool allCaptureHeightEqual = true;
            size_t numCaptureHeight = 0;
            const size_t numMotions = selectionList.GetNumSelectedMotions();
            bool curCaptureHeight = false;

            for (size_t i = 0; i < numMotions; ++i)
            {
                EMotionFX::Motion* curMotion = selectionList.GetMotion(i);
                EMotionFX::Motion* prevMotion = (i>0) ? selectionList.GetMotion(i-1) : nullptr;
                curCaptureHeight = (curMotion->GetMotionExtractionFlags() & EMotionFX::MOTIONEXTRACT_CAPTURE_Z);

                if (curCaptureHeight)
                {
                    numCaptureHeight++;
                }

                if (curMotion && prevMotion)
                {
                    if (curCaptureHeight != (prevMotion->GetMotionExtractionFlags() & EMotionFX::MOTIONEXTRACT_CAPTURE_Z))
                    {
                        allCaptureHeightEqual = false;
                    }
                }
            }

            // Adjust the height capture checkbox, based on the selected motions.
            const bool triState = (numMotions > 1) && !allCaptureHeightEqual;
            m_captureHeight->setTristate( triState );
            if (numMotions > 1)
            {
                if (!allCaptureHeightEqual)
                {
                    m_captureHeight->setCheckState( Qt::CheckState::PartiallyChecked );
                }
                else
                {
                    m_captureHeight->setChecked( curCaptureHeight );
                }
            }
            else
            {
                if (numCaptureHeight > 0)
                {
                    m_captureHeight->setCheckState( Qt::CheckState::Checked );
                }
                else
                {
                    m_captureHeight->setCheckState( Qt::CheckState::Unchecked );
                }
            }

            m_warningShowed = false;
        }
    }

    
    // The the currently set motion extraction flags from the interface.
    EMotionFX::EMotionExtractionFlags MotionExtractionWindow::GetMotionExtractionFlags() const
    {
        int flags = 0;
        
        if (m_captureHeight->checkState() == Qt::CheckState::Checked)
            flags |= EMotionFX::MOTIONEXTRACT_CAPTURE_Z;

        return static_cast<EMotionFX::EMotionExtractionFlags>(flags);
    }
    
    
    // Called when any of the motion extraction flags buttons got pressed.
    void MotionExtractionWindow::OnMotionExtractionFlagsUpdated()
    {
        const CommandSystem::SelectionList& selectionList       = GetCommandManager()->GetCurrentSelection();
        const size_t                        numSelectedMotions  = selectionList.GetNumSelectedMotions();
        EMotionFX::ActorInstance*           actorInstance       = selectionList.GetSingleActorInstance();

        // Check if there is at least one motion selected and exactly one actor instance.
        if (!actorInstance || numSelectedMotions == 0)
        {
            return;
        }

        EMotionFX::Actor* actor = actorInstance->GetActor();

        EMotionFX::Node* motionExtractionNode = actor->GetMotionExtractionNode();
        if (!motionExtractionNode)
        {
            MCore::LogWarning("Motion extraction node not set.");
            return;
        }

        // The the currently set motion extraction flags from the interface.
        EMotionFX::EMotionExtractionFlags extractionFlags = GetMotionExtractionFlags();

        MCore::CommandGroup commandGroup("Adjust motion extraction settings", numSelectedMotions);

        // First of all stop all running motions.
        commandGroup.AddCommandString("StopAllMotionInstances");

        // Iterate through all selected motions.
        AZStd::string command;
        for (size_t i = 0; i < numSelectedMotions; ++i)
        {
            // Get the current selected motion, check if it is a skeletal motion, skip directly elsewise.
            EMotionFX::Motion* motion = selectionList.GetMotion(i);

            // Prepare the command and add it to the command group.
            command = AZStd::string::format("AdjustMotion -motionID %i -motionExtractionFlags %i", motion->GetID(), static_cast<uint8>(extractionFlags));
            commandGroup.AddCommandString(command.c_str());
        }

        // In case the command group is not empty, execute it.
        if (commandGroup.GetNumCommands() > 0)
        {
            AZStd::string outResult;
            if (GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult) == false)
            {
                if (outResult.empty() == false)
                {
                    MCore::LogError(outResult.c_str());
                }
            }
        }
    }
    

    // open node selection window so that we can select a motion extraction node
    void MotionExtractionWindow::OnSelectMotionExtractionNode()
    {
        EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
        if (actorInstance == nullptr)
        {
            MCore::LogWarning("Cannot open node selection window. Please select an actor instance first.");
            return;
        }

        m_motionExtractionNodeSelectionWindow->Update(actorInstance->GetID());
        m_motionExtractionNodeSelectionWindow->show();
    }


    void MotionExtractionWindow::OnMotionExtractionNodeSelected(AZStd::vector<SelectionItem> selection)
    {
        // get the selected node name
        uint32 actorID;
        AZStd::string nodeName;
        ActorPropertiesWindow::GetNodeName(selection, &nodeName, &actorID);

        // create the command group
        MCore::CommandGroup commandGroup("Adjust motion extraction node");

        // adjust the actor
        commandGroup.AddCommandString(AZStd::string::format("AdjustActor -actorID %i -motionExtractionNodeName \"%s\"", actorID, nodeName.c_str()).c_str());

        // execute the command group
        AZStd::string outResult;
        if (GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult) == false)
        {
            MCore::LogError(outResult.c_str());
        }
    }

    //-----------------------------------------------------------------------------------------
    // command callbacks
    //-----------------------------------------------------------------------------------------

    MotionExtractionWindow::SelectActorCallback::SelectActorCallback(MotionExtractionWindow* motionExtractionWindow)
        : MCore::Command::Callback(false)
        , m_motionExtractionWindow(motionExtractionWindow)
    {
    }

    bool MotionExtractionWindow::SelectActorCallback::Execute([[maybe_unused]] MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        if (CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        m_motionExtractionWindow->UpdateInterface();
        return true;
    }

    bool MotionExtractionWindow::SelectActorCallback::Undo([[maybe_unused]] MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        if (CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        m_motionExtractionWindow->UpdateInterface();
        return true;
    }

    MotionExtractionWindow::UpdateMotionExtractionWindowCallback::UpdateMotionExtractionWindowCallback(MotionExtractionWindow* motionExtractionWindow)
        : MCore::Command::Callback(false)
        , m_motionExtractionWindow(motionExtractionWindow)
    {
    }

    bool MotionExtractionWindow::UpdateMotionExtractionWindowCallback::Execute([[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        m_motionExtractionWindow->UpdateInterface();
        return true;
    }

    bool MotionExtractionWindow::UpdateMotionExtractionWindowCallback::Undo([[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        m_motionExtractionWindow->UpdateInterface();
        return true;
    }
} // namespace EMStudio

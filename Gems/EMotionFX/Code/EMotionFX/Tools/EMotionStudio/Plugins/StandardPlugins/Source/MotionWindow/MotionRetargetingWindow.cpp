/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "MotionRetargetingWindow.h"
#include "MotionWindowPlugin.h"
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QGridLayout>
#include <QIcon>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <EMotionFX/CommandSystem/Source/MotionCommands.h>
#include <MCore/Source/Compare.h>
#include <MCore/Source/LogManager.h>
#include <AzQtComponents/Components/Widgets/CheckBox.h>

namespace EMStudio
{
    // constructor
    MotionRetargetingWindow::MotionRetargetingWindow(QWidget* parent, MotionWindowPlugin* motionWindowPlugin)
        : QWidget(parent)
    {
        m_motionWindowPlugin         = motionWindowPlugin;
        m_motionRetargetingButton    = nullptr;
    }


    // destructor
    MotionRetargetingWindow::~MotionRetargetingWindow()
    {
    }


    // init after the parent dock window has been created
    void MotionRetargetingWindow::Init()
    {
        QGridLayout* layout = new QGridLayout();
        setLayout(layout);

        m_motionRetargetingButton = new QCheckBox();
        AzQtComponents::CheckBox::applyToggleSwitchStyle(m_motionRetargetingButton);
        layout->addWidget(new QLabel(tr("Use Motion Retargeting")), 0, 0);
        layout->addWidget(m_motionRetargetingButton, 0, 1);
        connect(m_motionRetargetingButton, &QCheckBox::clicked, this, &MotionRetargetingWindow::UpdateMotions);
    }


    void MotionRetargetingWindow::UpdateMotions()
    {
        const CommandSystem::SelectionList& selection = CommandSystem::GetCommandManager()->GetCurrentSelection();

        // create our command group
        MCore::CommandGroup commandGroup("Adjust default motion instances");

        // get the number of selected motions and iterate through them
        const size_t numMotions = selection.GetNumSelectedMotions();
        for (size_t i = 0; i < numMotions; ++i)
        {
            MotionWindowPlugin::MotionTableEntry* entry = m_motionWindowPlugin->FindMotionEntryByID(selection.GetMotion(i)->GetID());
            if (entry == nullptr)
            {
                MCore::LogError("Cannot find motion table entry for the given motion.");
                continue;
            }

            EMotionFX::Motion*          motion              = entry->m_motion;
            EMotionFX::PlayBackInfo*    playbackInfo        = motion->GetDefaultPlayBackInfo();

            AZStd::string commandParameters;

            if (playbackInfo->m_retarget != m_motionRetargetingButton->isChecked())
            {
                commandParameters += AZStd::string::format("-retarget %s ", AZStd::to_string(m_motionRetargetingButton->isChecked()).c_str());
            }

            // in case the command parameters are empty it means nothing changed, so we can skip this command
            if (commandParameters.empty() == false)
            {
                commandGroup.AddCommandString(AZStd::string::format("AdjustDefaultPlayBackInfo -filename \"%s\" %s", motion->GetFileName(), commandParameters.c_str()).c_str());
            }
        }

        // execute the group command
        AZStd::string outResult;
        if (CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult) == false)
        {
            MCore::LogError(outResult.c_str());
        }
    }


    void MotionRetargetingWindow::UpdateInterface()
    {
        const CommandSystem::SelectionList& selection = CommandSystem::GetCommandManager()->GetCurrentSelection();

        // check if there actually is any motion selected
        const size_t numSelectedMotions = selection.GetNumSelectedMotions();
        const bool isEnabled = (numSelectedMotions != 0);

        m_motionRetargetingButton->setEnabled(isEnabled);

        if (isEnabled == false)
        {
            return;
        }

        // iterate through the selected motions
        for (size_t i = 0; i < numSelectedMotions; ++i)
        {
            MotionWindowPlugin::MotionTableEntry* entry = m_motionWindowPlugin->FindMotionEntryByID(selection.GetMotion(i)->GetID());
            if (entry == nullptr)
            {
                MCore::LogWarning("Cannot find motion table entry for the given motion.");
                continue;
            }

            EMotionFX::Motion*          motion              = entry->m_motion;
            EMotionFX::PlayBackInfo*    defaultPlayBackInfo = motion->GetDefaultPlayBackInfo();

            m_motionRetargetingButton->setChecked(defaultPlayBackInfo->m_retarget);
        }
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/MotionWindow/moc_MotionRetargetingWindow.cpp>

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MotionEventsPlugin.h"
#include "../TimeView/TrackDataWidget.h"
#include "../TimeView/TrackHeaderWidget.h"
#include <MCore/Source/LogManager.h>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include <EMotionFX/Source/MotionEventTrack.h>
#include <EMotionFX/Source/MotionEventTable.h>
#include <EMotionFX/CommandSystem/Source/MotionEventCommands.h>
#include <QLabel>
#include <QShortcut>
#include <QPushButton>
#include <QVBoxLayout>

namespace EMStudio
{
    MotionEventsPlugin::MotionEventsPlugin()
        : EMStudio::DockWidgetPlugin()
        , m_dialogStack(nullptr)
        , m_motionEventPresetsWidget(nullptr)
        , m_timeViewPlugin(nullptr)
    {
    }

    void MotionEventsPlugin::Reflect(AZ::ReflectContext* context)
    {
        MotionEventPreset::Reflect(context);
        MotionEventPresetManager::Reflect(context);
    }

    // on before remove plugin
    void MotionEventsPlugin::OnBeforeRemovePlugin(uint32 classID)
    {
        if (classID == TimeViewPlugin::CLASS_ID)
        {
            m_timeViewPlugin = nullptr;
        }
    }


    // init after the parent dock window has been created
    bool MotionEventsPlugin::Init()
    {
        GetEventPresetManager()->LoadFromSettings();
        GetEventPresetManager()->Load();

        // create the dialog stack
        assert(m_dialogStack == nullptr);
        m_dialogStack = new MysticQt::DialogStack(m_dock);
        m_dock->setWidget(m_dialogStack);

        // create the motion event presets widget
        m_motionEventPresetsWidget = new MotionEventPresetsWidget(m_dialogStack, this);
        m_dialogStack->Add(m_motionEventPresetsWidget, "Motion Event Presets", false, true);

        ValidatePluginLinks();

        return true;
    }

    void MotionEventsPlugin::ValidatePluginLinks()
    {
        if (!m_timeViewPlugin)
        {
            EMStudioPlugin* timeViewBasePlugin = EMStudio::GetPluginManager()->FindActivePlugin(TimeViewPlugin::CLASS_ID);
            if (timeViewBasePlugin)
            {
                m_timeViewPlugin = static_cast<TimeViewPlugin*>(timeViewBasePlugin);
                connect(this, &MotionEventsPlugin::OnColorChanged, m_timeViewPlugin, &TimeViewPlugin::ReInit);

                TrackDataWidget* trackDataWidget = m_timeViewPlugin->GetTrackDataWidget();
                connect(trackDataWidget, &TrackDataWidget::MotionEventPresetsDropped, this, &MotionEventsPlugin::OnEventPresetDropped);
            }
        }
    }

    bool MotionEventsPlugin::CheckIfIsPresetReadyToDrop()
    {
        ValidatePluginLinks();

        // get the motion event presets table
        QTableWidget* eventPresetsTable = m_motionEventPresetsWidget->GetMotionEventPresetsTable();
        if (eventPresetsTable == nullptr)
        {
            return false;
        }

        // get the number of motion event presets and iterate through them
        const uint32 numRows = eventPresetsTable->rowCount();
        for (uint32 i = 0; i < numRows; ++i)
        {
            QTableWidgetItem* itemType = eventPresetsTable->item(i, 1);

            if (itemType->isSelected())
            {
                return true;
            }
        }

        return false;
    }


    void MotionEventsPlugin::OnEventPresetDropped(QPoint position)
    {
        ValidatePluginLinks();

        EMotionFX::Motion* motion = GetCommandManager()->GetCurrentSelection().GetSingleMotion();
        if (!motion)
        {
            return;
        }

        // calculate the start time for the motion event
        double dropTimeInSeconds = m_timeViewPlugin->PixelToTime(position.x());

        // get the time track on which we dropped the preset
        TimeTrack* timeTrack = m_timeViewPlugin->GetTrackAt(position.y());
        if (!timeTrack)
        {
            return;
        }

        // get the corresponding motion event track
        EMotionFX::MotionEventTable* eventTable = motion->GetEventTable();
        EMotionFX::MotionEventTrack* eventTrack = eventTable->FindTrackByName(timeTrack->GetName());
        if (eventTrack == nullptr)
        {
            return;
        }

        // get the motion event presets table
        QTableWidget* eventPresetsTable = m_motionEventPresetsWidget->GetMotionEventPresetsTable();
        if (eventPresetsTable == nullptr)
        {
            return;
        }

        // get the number of motion event presets and iterate through them
        const size_t numRows = EMStudio::GetEventPresetManager()->GetNumPresets();
        AZStd::string result;
        for (size_t i = 0; i < numRows; ++i)
        {
            const MotionEventPreset* preset = EMStudio::GetEventPresetManager()->GetPreset(i);
            QTableWidgetItem* itemName = eventPresetsTable->item(static_cast<int>(i), 1);

            if (itemName->isSelected())
            {
                CommandSystem::CommandCreateMotionEvent* createMotionEventCommand = aznew CommandSystem::CommandCreateMotionEvent();
                createMotionEventCommand->SetMotionID(motion->GetID());
                createMotionEventCommand->SetEventTrackName(eventTrack->GetName());
                createMotionEventCommand->SetStartTime(aznumeric_cast<float>(dropTimeInSeconds));
                createMotionEventCommand->SetEndTime(aznumeric_cast<float>(dropTimeInSeconds));
                createMotionEventCommand->SetEventDatas(preset->GetEventDatas());

                if (!EMStudio::GetCommandManager()->ExecuteCommand(createMotionEventCommand, result))
                {
                    AZ_Error("EMotionFX", false, result.c_str());
                }
            }
        }
    }
} // namespace EMStudio

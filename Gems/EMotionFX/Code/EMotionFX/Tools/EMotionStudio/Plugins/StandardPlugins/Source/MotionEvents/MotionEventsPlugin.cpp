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
        , m_adjustMotionCallback(nullptr)
        , m_selectCallback(nullptr)
        , m_unselectCallback(nullptr)
        , m_clearSelectionCallback(nullptr)
        , m_dialogStack(nullptr)
        , m_motionEventPresetsWidget(nullptr)
        , m_timeViewPlugin(nullptr)
        , m_trackHeaderWidget(nullptr)
        , m_trackDataWidget(nullptr)
        , m_motionWindowPlugin(nullptr)
        , m_motionListWindow(nullptr)
        , m_motion(nullptr)
    {
    }

    MotionEventsPlugin::~MotionEventsPlugin()
    {
        GetCommandManager()->RemoveCommandCallback(m_adjustMotionCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_selectCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_unselectCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_clearSelectionCallback, false);
        delete m_adjustMotionCallback;
        delete m_selectCallback;
        delete m_unselectCallback;
        delete m_clearSelectionCallback;
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

        if (classID == MotionWindowPlugin::CLASS_ID)
        {
            m_motionWindowPlugin = nullptr;
        }
    }


    // init after the parent dock window has been created
    bool MotionEventsPlugin::Init()
    {
        GetEventPresetManager()->LoadFromSettings();
        GetEventPresetManager()->Load();

        // create callbacks
        m_adjustMotionCallback = new CommandAdjustMotionCallback(false);
        m_selectCallback = new CommandSelectCallback(false);
        m_unselectCallback = new CommandUnselectCallback(false);
        m_clearSelectionCallback = new CommandClearSelectionCallback(false);
        GetCommandManager()->RegisterCommandCallback("AdjustMotion", m_adjustMotionCallback);
        GetCommandManager()->RegisterCommandCallback("Select", m_selectCallback);
        GetCommandManager()->RegisterCommandCallback("Unselect", m_unselectCallback);
        GetCommandManager()->RegisterCommandCallback("ClearSelection", m_clearSelectionCallback);

        // create the dialog stack
        assert(m_dialogStack == nullptr);
        m_dialogStack = new MysticQt::DialogStack(m_dock);
        m_dock->setWidget(m_dialogStack);

        // create the motion event presets widget
        m_motionEventPresetsWidget = new MotionEventPresetsWidget(m_dialogStack, this);
        m_dialogStack->Add(m_motionEventPresetsWidget, "Motion Event Presets", false, true);
        connect(m_dock, &QDockWidget::visibilityChanged, this, &MotionEventsPlugin::WindowReInit);

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
                m_timeViewPlugin     = (TimeViewPlugin*)timeViewBasePlugin;
                m_trackDataWidget    = m_timeViewPlugin->GetTrackDataWidget();
                m_trackHeaderWidget  = m_timeViewPlugin->GetTrackHeaderWidget();

                connect(m_trackDataWidget, &TrackDataWidget::MotionEventPresetsDropped, this, &MotionEventsPlugin::OnEventPresetDropped);
                connect(this, &MotionEventsPlugin::OnColorChanged, m_timeViewPlugin, &TimeViewPlugin::ReInit);
            }
        }

        if (!m_motionWindowPlugin)
        {
            EMStudioPlugin* motionBasePlugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionWindowPlugin::CLASS_ID);
            if (motionBasePlugin)
            {
                m_motionWindowPlugin = (MotionWindowPlugin*)motionBasePlugin;
                m_motionListWindow   = m_motionWindowPlugin->GetMotionListWindow();

                connect(m_motionListWindow, &MotionListWindow::MotionSelectionChanged, this, &MotionEventsPlugin::MotionSelectionChanged);
            }
        }
    }


    void MotionEventsPlugin::MotionSelectionChanged()
    {
        EMotionFX::Motion* motion = GetCommandManager()->GetCurrentSelection().GetSingleMotion();
        if (m_motion != motion)
        {
            m_motion = motion;
            ReInit();
        }
    }


    void MotionEventsPlugin::ReInit()
    {
        ValidatePluginLinks();
    }


    // reinit the window when it gets activated
    void MotionEventsPlugin::WindowReInit(bool visible)
    {
        if (visible)
        {
            MotionSelectionChanged();
        }
    }


    bool MotionEventsPlugin::CheckIfIsPresetReadyToDrop()
    {
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
        // calculate the start time for the motion event
        double dropTimeInSeconds = m_timeViewPlugin->PixelToTime(position.x());

        // get the time track on which we dropped the preset
        TimeTrack* timeTrack = m_timeViewPlugin->GetTrackAt(position.y());
        if (!timeTrack || !m_motion)
        {
            return;
        }

        // get the corresponding motion event track
        EMotionFX::MotionEventTable* eventTable = m_motion->GetEventTable();
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
                createMotionEventCommand->SetMotionID(m_motion->GetID());
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

    // callbacks
    bool ReInitMotionEventsPlugin()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionEventsPlugin::CLASS_ID);
        if (!plugin)
        {
            return false;
        }

        MotionEventsPlugin* motionEventsPlugin = (MotionEventsPlugin*)plugin;
        motionEventsPlugin->ReInit();

        return true;
    }


    bool MotionSelectionChangedMotionEventsPlugin()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionEventsPlugin::CLASS_ID);
        if (!plugin)
        {
            return false;
        }

        MotionEventsPlugin* motionEventsPlugin = (MotionEventsPlugin*)plugin;
        motionEventsPlugin->MotionSelectionChanged();

        return true;
    }


    bool MotionEventsPlugin::CommandAdjustMotionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)       { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitMotionEventsPlugin(); }
    bool MotionEventsPlugin::CommandAdjustMotionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)          { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitMotionEventsPlugin(); }
    bool MotionEventsPlugin::CommandSelectCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasMotionSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return MotionSelectionChangedMotionEventsPlugin();
    }
    bool MotionEventsPlugin::CommandSelectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasMotionSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return MotionSelectionChangedMotionEventsPlugin();
    }
    bool MotionEventsPlugin::CommandUnselectCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasMotionSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return MotionSelectionChangedMotionEventsPlugin();
    }
    bool MotionEventsPlugin::CommandUnselectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasMotionSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return MotionSelectionChangedMotionEventsPlugin();
    }
    bool MotionEventsPlugin::CommandClearSelectionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)     { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return MotionSelectionChangedMotionEventsPlugin(); }
    bool MotionEventsPlugin::CommandClearSelectionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)        { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return MotionSelectionChangedMotionEventsPlugin(); }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/MotionEvents/moc_MotionEventsPlugin.cpp>

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        , mAdjustMotionCallback(nullptr)
        , mSelectCallback(nullptr)
        , mUnselectCallback(nullptr)
        , mClearSelectionCallback(nullptr)
        , mDialogStack(nullptr)
        , mMotionEventPresetsWidget(nullptr)
        , mMotionEventWidget(nullptr)
        , mMotionTable(nullptr)
        , mTimeViewPlugin(nullptr)
        , mTrackHeaderWidget(nullptr)
        , mTrackDataWidget(nullptr)
        , mMotionWindowPlugin(nullptr)
        , mMotionListWindow(nullptr)
        , mMotion(nullptr)
    {
    }


    MotionEventsPlugin::~MotionEventsPlugin()
    {
        GetCommandManager()->RemoveCommandCallback(mAdjustMotionCallback, false);
        GetCommandManager()->RemoveCommandCallback(mSelectCallback, false);
        GetCommandManager()->RemoveCommandCallback(mUnselectCallback, false);
        GetCommandManager()->RemoveCommandCallback(mClearSelectionCallback, false);
        delete mAdjustMotionCallback;
        delete mSelectCallback;
        delete mUnselectCallback;
        delete mClearSelectionCallback;
    }


    void MotionEventsPlugin::Reflect(AZ::ReflectContext* context)
    {
        MotionEventPreset::Reflect(context);
        MotionEventPresetManager::Reflect(context);
    }

    // clone the log window
    EMStudioPlugin* MotionEventsPlugin::Clone()
    {
        return new MotionEventsPlugin();
    }


    // on before remove plugin
    void MotionEventsPlugin::OnBeforeRemovePlugin(uint32 classID)
    {
        if (classID == TimeViewPlugin::CLASS_ID)
        {
            mTimeViewPlugin = nullptr;
        }

        if (classID == MotionWindowPlugin::CLASS_ID)
        {
            mMotionWindowPlugin = nullptr;
        }
    }


    // init after the parent dock window has been created
    bool MotionEventsPlugin::Init()
    {
        GetEventPresetManager()->LoadFromSettings();
        GetEventPresetManager()->Load();

        // create callbacks
        mAdjustMotionCallback = new CommandAdjustMotionCallback(false);
        mSelectCallback = new CommandSelectCallback(false);
        mUnselectCallback = new CommandUnselectCallback(false);
        mClearSelectionCallback = new CommandClearSelectionCallback(false);
        GetCommandManager()->RegisterCommandCallback("AdjustMotion", mAdjustMotionCallback);
        GetCommandManager()->RegisterCommandCallback("Select", mSelectCallback);
        GetCommandManager()->RegisterCommandCallback("Unselect", mUnselectCallback);
        GetCommandManager()->RegisterCommandCallback("ClearSelection", mClearSelectionCallback);

        // create the dialog stack
        assert(mDialogStack == nullptr);
        mDialogStack = new MysticQt::DialogStack(mDock);
        mDock->setWidget(mDialogStack);

        // create the motion event presets widget
        mMotionEventPresetsWidget = new MotionEventPresetsWidget(mDialogStack, this);
        mDialogStack->Add(mMotionEventPresetsWidget, "Motion Event Presets", false, true);
        connect(mDock, &QDockWidget::visibilityChanged, this, &MotionEventsPlugin::WindowReInit);

        // create the motion event properties widget
        mMotionEventWidget = new MotionEventWidget(mDialogStack);
        mDialogStack->Add(mMotionEventWidget, "Motion Event Properties", false, true);

        ValidatePluginLinks();
        UpdateMotionEventWidget();

        return true;
    }


    void MotionEventsPlugin::ValidatePluginLinks()
    {
        if (!mTimeViewPlugin)
        {
            EMStudioPlugin* timeViewBasePlugin = EMStudio::GetPluginManager()->FindActivePlugin(TimeViewPlugin::CLASS_ID);
            if (timeViewBasePlugin)
            {
                mTimeViewPlugin     = (TimeViewPlugin*)timeViewBasePlugin;
                mTrackDataWidget    = mTimeViewPlugin->GetTrackDataWidget();
                mTrackHeaderWidget  = mTimeViewPlugin->GetTrackHeaderWidget();

                connect(mTrackDataWidget, &TrackDataWidget::MotionEventPresetsDropped, this, &MotionEventsPlugin::OnEventPresetDropped);
                connect(mTimeViewPlugin, &TimeViewPlugin::SelectionChanged, this, &MotionEventsPlugin::UpdateMotionEventWidget);
                connect(this, &MotionEventsPlugin::OnColorChanged, mTimeViewPlugin, &TimeViewPlugin::ReInit);
            }
        }

        if (!mMotionWindowPlugin)
        {
            EMStudioPlugin* motionBasePlugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionWindowPlugin::CLASS_ID);
            if (motionBasePlugin)
            {
                mMotionWindowPlugin = (MotionWindowPlugin*)motionBasePlugin;
                mMotionListWindow   = mMotionWindowPlugin->GetMotionListWindow();

                connect(mMotionListWindow, &MotionListWindow::MotionSelectionChanged, this, &MotionEventsPlugin::MotionSelectionChanged);
            }
        }
    }


    void MotionEventsPlugin::MotionSelectionChanged()
    {
        EMotionFX::Motion* motion = GetCommandManager()->GetCurrentSelection().GetSingleMotion();
        if (mMotion != motion)
        {
            mMotion = motion;
            ReInit();
        }
    }


    void MotionEventsPlugin::ReInit()
    {
        ValidatePluginLinks();

        // update the selection array as well as the motion event widget
        UpdateMotionEventWidget();
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
        QTableWidget* eventPresetsTable = mMotionEventPresetsWidget->GetMotionEventPresetsTable();
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
        double dropTimeInSeconds = mTimeViewPlugin->PixelToTime(position.x());
        //mTimeViewPlugin->CalcTime( position.x(), &dropTimeInSeconds, nullptr, nullptr, nullptr, nullptr );

        // get the time track on which we dropped the preset
        TimeTrack* timeTrack = mTimeViewPlugin->GetTrackAt(position.y());
        if (!timeTrack || !mMotion)
        {
            return;
        }

        // get the corresponding motion event track
        EMotionFX::MotionEventTable* eventTable = mMotion->GetEventTable();
        EMotionFX::MotionEventTrack* eventTrack = eventTable->FindTrackByName(timeTrack->GetName());
        if (eventTrack == nullptr)
        {
            return;
        }

        // get the motion event presets table
        QTableWidget* eventPresetsTable = mMotionEventPresetsWidget->GetMotionEventPresetsTable();
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
                createMotionEventCommand->SetMotionID(mMotion->GetID());
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


    void MotionEventsPlugin::UpdateMotionEventWidget()
    {
        if (!mMotionEventWidget || !mTimeViewPlugin)
        {
            return;
        }

        mTimeViewPlugin->UpdateSelection();
        if (mTimeViewPlugin->GetNumSelectedEvents() != 1)
        {
            mMotionEventWidget->ReInit();
        }
        else
        {
            EventSelectionItem selectionItem = mTimeViewPlugin->GetSelectedEvent(0);
            mMotionEventWidget->ReInit(selectionItem.mMotion, selectionItem.GetMotionEvent());
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

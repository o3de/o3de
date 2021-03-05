/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/Recorder.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/SelectionList.h>
#include <MysticQt/Source/MysticQtManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/RecorderGroup.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TimeViewPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TimeViewToolBar.h>
#include <QAction>
#include <QMenu>

namespace EMStudio
{
    RecorderGroup::RecorderGroup(TimeViewPlugin* plugin, TimeViewToolBar* toolbar)
        : QObject(toolbar)
    {
        m_clearRecordAction = toolbar->addAction(
            MysticQt::GetMysticQt()->FindIcon("/Images/Icons/Clear.svg"),
            "Clear recording", toolbar, &TimeViewToolBar::OnClearRecordButton);

        m_recordAction = toolbar->addAction(
            MysticQt::GetMysticQt()->FindIcon("/Images/Icons/RecordButton.svg"),
            "Clear recording", toolbar, &TimeViewToolBar::OnRecordButton);

        m_recordOptionsAction = toolbar->addAction(
            MysticQt::GetMysticQt()->FindIcon("/Images/Icons/Settings.svg"), "");
        {
            QMenu* recordOptionsMenu = new QMenu(toolbar);
            recordOptionsMenu->addAction(tr("Recording options"))->setEnabled(false);
            m_recordMotionsOnly = recordOptionsMenu->addAction(tr("Record motions only"), toolbar, [=] {
                m_recordStatesOnly->setChecked(false);
                m_recordAllNodes->setChecked(false);
                });
            m_recordMotionsOnly->setCheckable(true);
            m_recordMotionsOnly->setChecked(true);

            m_recordStatesOnly = recordOptionsMenu->addAction(tr("Record states only"), toolbar, [=] {
                m_recordMotionsOnly->setChecked(false);
                m_recordAllNodes->setChecked(false);
                });
            m_recordStatesOnly->setCheckable(true);
            m_recordStatesOnly->setChecked(false);

            m_recordAllNodes = recordOptionsMenu->addAction(tr("Record all nodes"), toolbar, [=] {
                m_recordMotionsOnly->setChecked(false);
                m_recordStatesOnly->setChecked(false);
                });
            m_recordAllNodes->setCheckable(true);
            m_recordAllNodes->setChecked(false);

            recordOptionsMenu->addSeparator();

            m_recordEvents = recordOptionsMenu->addAction(tr("Record events"));
            m_recordEvents->setCheckable(true);
            m_recordEvents->setChecked(true);

            m_recordOptionsAction->setMenu(recordOptionsMenu);
        }

        m_displayOptionsAction = toolbar->addAction(
            MysticQt::GetMysticQt()->FindIcon("/Images/Icons/Visualization.svg"),
            "Show display and visual options");
        {
            QMenu* contextMenu = new QMenu(toolbar);

            contextMenu->addAction(tr("Display"))->setEnabled(false);

            m_displayOptionNodeActivity = contextMenu->addAction("Node Activity", toolbar, [=] {
                plugin->SetRedrawFlag();
                });
            m_displayOptionNodeActivity->setCheckable(true);
            m_displayOptionNodeActivity->setChecked(true);

            m_displayOptionMotionEvents = contextMenu->addAction("Motion Events", [=] {
                plugin->SetRedrawFlag();
                });
            m_displayOptionMotionEvents->setCheckable(true);
            m_displayOptionMotionEvents->setChecked(true);

            m_displayOptionRelativeGraph = contextMenu->addAction("Relative Graph", [=] {
                plugin->SetRedrawFlag();
                });
            m_displayOptionRelativeGraph->setCheckable(true);
            m_displayOptionRelativeGraph->setChecked(true);

            contextMenu->addSeparator();

            contextMenu->addAction(tr("Visual Options"))->setEnabled(false);

            m_sortNodeActivity = contextMenu->addAction("Sort Node Activity", [=] {
                plugin->SetRedrawFlag();
                });
            m_sortNodeActivity->setCheckable(true);
            m_sortNodeActivity->setChecked(true);

            m_useNoteTypeColors = contextMenu->addAction("Use Node Type Colors", [=] {
                plugin->SetRedrawFlag();
                });
            m_useNoteTypeColors->setCheckable(true);
            m_useNoteTypeColors->setChecked(false);

            m_detailedNodes = contextMenu->addAction("Detailed Nodes", [=] {
                toolbar->OnDetailedNodes();
                });
            m_detailedNodes->setCheckable(true);
            m_detailedNodes->setChecked(false);

            m_limitGraphHeightAction = contextMenu->addAction("Limit Graph Height", [=] {
                plugin->SetRedrawFlag();
                });
            m_limitGraphHeightAction->setCheckable(true);
            m_limitGraphHeightAction->setChecked(true);

            m_displayOptionsAction->setMenu(contextMenu);
        }

        m_separatorRight = toolbar->addSeparator();
    }

    bool RecorderGroup::UpdateInterface(TimeViewMode mode, bool showRightSeparator)
    {
        const bool isVisible = (mode == TimeViewMode::AnimGraph);
        m_recordOptionsAction->setVisible(isVisible);
        m_displayOptionsAction->setVisible(isVisible);
        m_recordAction->setVisible(isVisible);
        m_clearRecordAction->setVisible(isVisible);
        m_separatorRight->setVisible(isVisible && showRightSeparator);

        if (isVisible)
        {
            const CommandSystem::SelectionList& selectionList = CommandSystem::GetCommandManager()->GetCurrentSelection();
            const EMotionFX::ActorInstance* actorInstance = selectionList.GetSingleActorInstance();
            const bool singleActorInstanceSelected = (actorInstance != nullptr);

            EMotionFX::Recorder& recorder = EMotionFX::GetRecorder();
            const bool isRecording = recorder.GetIsRecording();
            const bool optionsEnabled = !isRecording && singleActorInstanceSelected;

            m_recordAction->setEnabled(singleActorInstanceSelected);
            m_recordOptionsAction->setEnabled(optionsEnabled);
            m_displayOptionsAction->setEnabled(optionsEnabled);
            m_clearRecordAction->setEnabled(optionsEnabled && recorder.HasRecording());

            if (isRecording)
            {
                m_recordAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/StopRecorder.svg"));
                m_recordAction->setToolTip("Stop recording");
            }
            else
            {
                m_recordAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/RecordButton.svg"));
                m_recordAction->setToolTip("Start recording");
            }
        }

        return isVisible;
    }
} // namespace EMStudio

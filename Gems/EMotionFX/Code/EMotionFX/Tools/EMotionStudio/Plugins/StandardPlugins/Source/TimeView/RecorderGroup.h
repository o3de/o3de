/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TimeViewShared.h>
#include <QAction>
#include <QObject>

namespace EMStudio
{
    class TimeViewPlugin;
    class TimeViewToolBar;

    class RecorderGroup
        : public QObject
    {
        Q_OBJECT // AUTOMOC

    public:
        enum RecordingMode
        {
            Default,
            Recording,
            PlaybackRecording
        };

        RecorderGroup(TimeViewPlugin* plugin, TimeViewToolBar* toolbar);
        bool UpdateInterface(TimeViewMode mode, bool showRightSeparator = true);

        bool GetRecordMotionsOnly() const { return m_recordMotionsOnly->isChecked(); }
        bool GetRecordStatesOnly() const { return m_recordStatesOnly->isChecked(); }
        bool GetRecordEvents() const { return m_recordEvents->isChecked(); }

        bool GetDisplayNodeActivity() const { return m_displayOptionNodeActivity->isChecked(); }
        bool GetDisplayMotionEvents() const { return m_displayOptionMotionEvents->isChecked(); }
        bool GetDisplayRelativeGraph() const { return m_displayOptionRelativeGraph->isChecked(); }

        bool GetSortNodeActivity() const { return m_sortNodeActivity->isChecked(); }
        bool GetUseNodeTypeColors() const { return m_useNoteTypeColors->isChecked(); }
        bool GetDetailedNodes() const { return m_detailedNodes->isChecked(); }
        bool GetLimitGraphHeight() const { return m_limitGraphHeightAction->isChecked(); }

    private:
        QAction* m_recordOptionsAction = nullptr;
        QAction* m_recordMotionsOnly = nullptr;
        QAction* m_recordStatesOnly = nullptr;
        QAction* m_recordAllNodes = nullptr;
        QAction* m_recordEvents = nullptr;

        QAction* m_displayOptionsAction = nullptr;
        QAction* m_displayOptionNodeActivity = nullptr;
        QAction* m_displayOptionMotionEvents = nullptr;
        QAction* m_displayOptionRelativeGraph = nullptr;

        QAction* m_sortNodeActivity = nullptr;
        QAction* m_useNoteTypeColors = nullptr;
        QAction* m_detailedNodes = nullptr;
        QAction* m_limitGraphHeightAction = nullptr;

        QAction* m_recordAction = nullptr;
        QAction* m_clearRecordAction = nullptr;

        QAction* m_separatorRight = nullptr;
    };
} // namespace EMStudio

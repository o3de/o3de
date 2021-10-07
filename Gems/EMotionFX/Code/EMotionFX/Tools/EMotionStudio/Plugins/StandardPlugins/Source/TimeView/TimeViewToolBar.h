/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/LogManager.h>
#include <EMotionFX/Source/Motion.h>
#include "../StandardPluginsConfig.h"
#include <QToolBar>

#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/PlaybackControlsGroup.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/PlaybackOptionsGroup.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/RecorderGroup.h>
#endif

namespace EMStudio
{
    // forward declarations
    class TimeViewPlugin;
    class MotionWindowPlugin;

    class TimeViewToolBar
        : public QToolBar
    {
        Q_OBJECT // AUTOMOC
        MCORE_MEMORYOBJECTCATEGORY(TimeViewToolBar, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        TimeViewToolBar(TimeViewPlugin* plugin, QWidget* parent = nullptr);
        ~TimeViewToolBar();

        void OnPlayForwardButton();
        void OnSeekForwardButton();
        void OnSeekBackwardButton();
        void OnSkipForwardButton();
        void OnSkipBackwardButton();
        void OnRecordButton();
        void OnClearRecordButton();
        void OnTimeChangeStart();
        void OnDetailedNodes();
        void UpdateMotions();
        void UpdateInterface();

        static MotionWindowPlugin* GetMotionWindowPlugin();

        RecorderGroup* GetRecorderGroup() const { return m_recorderGroup; }
        PlaybackControlsGroup* GetPlaybackControlsGroup() const { return m_playbackControls; }
        PlaybackOptionsGroup* GetPlaybackOptionsGroup() const { return m_playbackOptions; }

    signals:
        void RecorderStateChanged();

    protected:
        QSize sizeHint() const override;

        RecorderGroup::RecordingMode GetCurrentRecordingMode() const;

    private:
        TimeViewPlugin* m_plugin = nullptr;

        RecorderGroup* m_recorderGroup;
        PlaybackControlsGroup* m_playbackControls;
        PlaybackOptionsGroup* m_playbackOptions;
    };
} // namespace EMStudio

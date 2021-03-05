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
        TimeViewPlugin* mPlugin = nullptr;

        RecorderGroup* m_recorderGroup;
        PlaybackControlsGroup* m_playbackControls;
        PlaybackOptionsGroup* m_playbackOptions;
    };
} // namespace EMStudio

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TimeViewShared.h>
#include <QObject>

QT_FORWARD_DECLARE_CLASS(QAction)

namespace EMotionFX
{
    class Motion;
};

namespace EMStudio
{
    class TimeViewToolBar;

    class PlaybackControlsGroup
        : public QObject
    {
        Q_OBJECT // AUTOMOC

    public:
        PlaybackControlsGroup(TimeViewToolBar* toolbar);
        bool UpdateInterface(TimeViewMode mode, bool showRightSeparator = true);

        enum EPlayButtonState
        {
            Play,
            Pause,
            Stop
        };
        EPlayButtonState GetPlayButtonState() const { return m_playButtonState; }

    private:
        bool IsMotionPlaying(EMotionFX::Motion* motion) const;

        QAction* m_skipBackwardAction = nullptr;
        QAction* m_seekBackwardAction = nullptr;
        QAction* m_playForwardAction = nullptr;
        QAction* m_seekForwardAction = nullptr;
        QAction* m_skipForwardAction = nullptr;
        QAction* m_separatorRight = nullptr;

        EPlayButtonState m_playButtonState = Play;
    };
} // namespace EMStudio

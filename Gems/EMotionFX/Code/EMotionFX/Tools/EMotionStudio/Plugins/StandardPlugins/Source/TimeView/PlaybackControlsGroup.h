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

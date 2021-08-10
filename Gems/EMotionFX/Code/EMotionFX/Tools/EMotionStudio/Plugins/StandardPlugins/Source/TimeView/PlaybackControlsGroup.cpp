/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/MotionSystem.h>
#include <EMotionFX/Source/Recorder.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <MysticQt/Source/MysticQtManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/PlaybackControlsGroup.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TimeViewToolBar.h>
#include <QAction>

namespace EMStudio
{
    PlaybackControlsGroup::PlaybackControlsGroup(TimeViewToolBar* toolbar)
        : QObject(toolbar)
    {
        m_skipBackwardAction = toolbar->addAction(
            MysticQt::GetMysticQt()->FindIcon("Images/Icons/SkipBackward.svg"),
            "Skip backward", toolbar, &TimeViewToolBar::OnSkipBackwardButton);

        m_seekBackwardAction = toolbar->addAction(
            MysticQt::GetMysticQt()->FindIcon("Images/Icons/SeekBackward.svg"),
            "Seek backward", toolbar, &TimeViewToolBar::OnSeekBackwardButton);

        m_playForwardAction = toolbar->addAction(
            MysticQt::GetMysticQt()->FindIcon("Images/Icons/PlayForward.svg"),
            "Play", toolbar, &TimeViewToolBar::OnPlayForwardButton);

        m_seekForwardAction = toolbar->addAction(
            MysticQt::GetMysticQt()->FindIcon("Images/Icons/SeekForward.svg"),
            "Seek forward", toolbar, &TimeViewToolBar::OnSeekForwardButton);

        m_skipForwardAction = toolbar->addAction(
            MysticQt::GetMysticQt()->FindIcon("Images/Icons/SkipForward.svg"),
            "Skip forward", toolbar, &TimeViewToolBar::OnSkipForwardButton);

        m_separatorRight = toolbar->addSeparator();
    }

    bool PlaybackControlsGroup::UpdateInterface(TimeViewMode mode, bool showRightSeparator)
    {
        bool isVisible = false;
        switch (mode)
        {
            case TimeViewMode::Motion:
            {
                isVisible = true;

                const CommandSystem::SelectionList& selection = CommandSystem::GetCommandManager()->GetCurrentSelection();
                EMotionFX::Motion* selectedMotion = selection.GetSingleMotion();
                const bool isMotionPlaying = IsMotionPlaying(selectedMotion);

                const bool isEnabled = (selectedMotion != nullptr);
                m_skipBackwardAction->setEnabled(isEnabled);
                m_seekBackwardAction->setEnabled(false);
                m_playForwardAction->setEnabled(isEnabled);
                m_seekForwardAction->setEnabled(false);
                m_skipForwardAction->setEnabled(isEnabled);

                if (isMotionPlaying)
                {
                    m_playButtonState = Pause;
                    m_playForwardAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Pause.svg"));
                    m_playForwardAction->setToolTip("Pause animation");
                }
                else
                {
                    m_playButtonState = Play;
                    m_playForwardAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/PlayForward.svg"));
                    m_playForwardAction->setToolTip("Play animation");
                }

                break;
            }

            case TimeViewMode::AnimGraph:
            {
                EMotionFX::Recorder& recorder = EMotionFX::GetRecorder();
                isVisible = (!recorder.GetIsRecording() && recorder.HasRecording());

                if (isVisible)
                {
                    const bool isEnabled = (recorder.HasRecording() && recorder.GetIsInPlayMode());
                    m_skipBackwardAction->setEnabled(isEnabled);
                    m_seekBackwardAction->setEnabled(isEnabled);
                    m_seekForwardAction->setEnabled(isEnabled);
                    m_skipForwardAction->setEnabled(isEnabled);
                    m_playForwardAction->setEnabled(isEnabled);

                    if (recorder.GetIsInAutoPlayMode())
                    {
                        m_playButtonState = Pause;
                        m_playForwardAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Pause.svg"));
                        m_playForwardAction->setToolTip("Pause playback");
                    }
                    else
                    {
                        m_playButtonState = Play;
                        m_playForwardAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/PlayForward.svg"));
                        m_playForwardAction->setToolTip("Play recording");
                    }
                }

                break;
            }
        }

        // Visibility
        m_skipBackwardAction->setVisible(isVisible);
        m_seekBackwardAction->setVisible(isVisible);
        m_playForwardAction->setVisible(isVisible);
        m_seekForwardAction->setVisible(isVisible);
        m_skipForwardAction->setVisible(isVisible);
        m_separatorRight->setVisible(isVisible && showRightSeparator);
        return isVisible;
    }

    bool PlaybackControlsGroup::IsMotionPlaying(EMotionFX::Motion* motion) const
    {
        if (!motion)
        {
            return false;
        }

        const size_t numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
        for (size_t i = 0; i < numActorInstances; ++i)
        {
            EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(i);

            if (actorInstance->GetMotionSystem()->CheckIfIsPlayingMotion(motion, /*ignorePausedMotions=*/true))
            {
                return true;
            }
        }

        return false;
    }
} // namespace EMStudio

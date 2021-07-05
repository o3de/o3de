/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TimeViewToolBar.h>

#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TimeViewPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TimeViewShared.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TrackDataWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TrackDataHeaderWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TimeInfoWidget.h>

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionWindow/MotionWindowPlugin.h>

#include <MysticQt/Source/MysticQtManager.h>
#include <MCore/Source/Compare.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/Recorder.h>
#include <EMotionFX/Source/MotionSystem.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>

namespace EMStudio
{
    TimeViewToolBar::TimeViewToolBar(TimeViewPlugin* plugin, QWidget* parent)
        : QToolBar(parent)
    {
        setObjectName("TimeViewToolBar");

        mPlugin = plugin;

        m_recorderGroup = new RecorderGroup(plugin, this);
        m_playbackControls = new PlaybackControlsGroup(this);
        m_playbackOptions = new PlaybackOptionsGroup(this);

        QWidget* spacer = new QWidget();
        spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        addWidget(spacer);

        for (QToolButton* button : findChildren<QToolButton*>())
        {
            button->setPopupMode(QToolButton::InstantPopup);
        }

        UpdateInterface();
    }

    TimeViewToolBar::~TimeViewToolBar()
    {
    }

    RecorderGroup::RecordingMode TimeViewToolBar::GetCurrentRecordingMode() const
    {
        EMotionFX::Recorder& recorder = EMotionFX::GetRecorder();

        // if we're recording
        if (recorder.GetIsRecording()) // we're recording
        {
            return RecorderGroup::Recording;
        }
        else if (recorder.GetIsInPlayMode()) // we are playing back a recording
        {
            return RecorderGroup::PlaybackRecording;
        }
        else
        {
            return RecorderGroup::Default;
        }
    }

    void TimeViewToolBar::OnPlayForwardButton()
    {
        switch (mPlugin->GetMode())
        {
            case TimeViewMode::Motion:
            {
                const AZStd::vector<EMotionFX::MotionInstance*>& motionInstances = MotionWindowPlugin::GetSelectedMotionInstances();

                if (m_playbackControls->GetPlayButtonState() == PlaybackControlsGroup::Pause)
                {
                    // we are playing, pause instead!
                    for (EMotionFX::MotionInstance* motionInstance : motionInstances)
                    {
                        motionInstance->Pause();
                    }
                }
                else
                {
                    EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionWindowPlugin::CLASS_ID);
                    if (plugin)
                    {
                        for (EMotionFX::MotionInstance* motionInstance : motionInstances)
                        {
                            motionInstance->UnPause();
                        }

                        const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
                        const uint32 numSelectedMotions = selectionList.GetNumSelectedMotions();

                        AZStd::vector<EMotionFX::Motion*> motionsToPlay;
                        motionsToPlay.reserve(numSelectedMotions);

                        for (uint32 i = 0; i < numSelectedMotions; ++i)
                        {
                            EMotionFX::Motion* motion = selectionList.GetMotion(i);

                            // check if the given motion is already playing
                            bool isPlaying = false;
                            for (const EMotionFX::MotionInstance* motionInstance : motionInstances)
                            {
                                if (motion == motionInstance->GetMotion())
                                {
                                    isPlaying = true;
                                    break;
                                }
                            }

                            // only start playing the motion in case it is not being played already
                            if (!isPlaying)
                            {
                                motionsToPlay.push_back(motion);
                            }
                        }

                        MotionWindowPlugin* motionWindowPlugin = (MotionWindowPlugin*)plugin;
                        motionWindowPlugin->PlayMotions(motionsToPlay);
                    }
                }

                break;
            }

            case TimeViewMode::AnimGraph:
            {
                switch(GetCurrentRecordingMode())
                {
                    case RecorderGroup::Default:
                    {

                        break;
                    }
                    case RecorderGroup::Recording:
                    {
                        EMotionFX::GetRecorder().StartPlayBack();
                        EMotionFX::GetRecorder().SetAutoPlay(true);
                        break;
                    }
                    case RecorderGroup::PlaybackRecording:
                    {
                        if (EMotionFX::GetRecorder().GetIsInAutoPlayMode())
                        {
                            EMotionFX::GetRecorder().SetAutoPlay(false);
                        }
                        else
                        {
                            EMotionFX::GetRecorder().SetAutoPlay(true);
                        }
                        break;
                    }
                }

                break;
            }
        }

        UpdateInterface();
        emit RecorderStateChanged();
    }

    void TimeViewToolBar::OnSeekForwardButton()
    {
        if (GetCurrentRecordingMode() == RecorderGroup::PlaybackRecording)
        {
            const float newTime = MCore::Min<float>(EMotionFX::GetRecorder().GetCurrentPlayTime() + (1.0f / 60.0f), EMotionFX::GetRecorder().GetRecordTime());
            EMotionFX::GetRecorder().SetCurrentPlayTime(newTime);

            if (mPlugin)
            {
                mPlugin->SetCurrentTime(newTime);
                mPlugin->GetTimeInfoWidget()->update();
                mPlugin->SetRedrawFlag();
            }
        }

        emit RecorderStateChanged();
    }

    void TimeViewToolBar::OnSeekBackwardButton()
    {
        const float newTime = MCore::Max<float>(EMotionFX::GetRecorder().GetCurrentPlayTime() - (1.0f / 60.0f), 0.0f);
        EMotionFX::GetRecorder().SetCurrentPlayTime(newTime);

        if (mPlugin)
        {
            mPlugin->SetCurrentTime(newTime);
            mPlugin->GetTimeInfoWidget()->update();
            mPlugin->SetRedrawFlag();
        }

        emit RecorderStateChanged();
    }

    void TimeViewToolBar::OnSkipForwardButton()
    {
        switch (GetCurrentRecordingMode())
        {
        case RecorderGroup::Default:
        {
            const AZStd::vector<EMotionFX::MotionInstance*>& motionInstances = MotionWindowPlugin::GetSelectedMotionInstances();
            const size_t numMotionInstances = motionInstances.size();
            for (size_t i = 0; i < numMotionInstances; ++i)
            {
                EMotionFX::MotionInstance* motionInstance = motionInstances[i];
                motionInstance->SetCurrentTime(motionInstance->GetDuration());
            }
            break;
        }
        case RecorderGroup::PlaybackRecording:
        {
            EMotionFX::GetRecorder().SetCurrentPlayTime(EMotionFX::GetRecorder().GetRecordTime());
            if (mPlugin)
            {
                mPlugin->SetCurrentTime(EMotionFX::GetRecorder().GetCurrentPlayTime());
                mPlugin->SetRedrawFlag();
            }
            break;
        }
        default:
            Q_UNREACHABLE();
        }

        emit RecorderStateChanged();
    }

    void TimeViewToolBar::OnSkipBackwardButton()
    {
        switch (GetCurrentRecordingMode())
        {
        case RecorderGroup::Default:
        {
            const AZStd::vector<EMotionFX::MotionInstance*>& motionInstances = MotionWindowPlugin::GetSelectedMotionInstances();
            const size_t numMotionInstances = motionInstances.size();
            for (size_t i = 0; i < numMotionInstances; ++i)
            {
                EMotionFX::MotionInstance* motionInstance = motionInstances[i];
                motionInstance->Rewind();
            }
            break;
        }
        case RecorderGroup::PlaybackRecording:
        {
            EMotionFX::GetRecorder().SetCurrentPlayTime(0.0f);
            if (mPlugin)
            {
                mPlugin->SetCurrentTime(EMotionFX::GetRecorder().GetCurrentPlayTime());
                mPlugin->SetRedrawFlag();
            }
            break;
        }
        default:
            Q_UNREACHABLE();
        }

        emit RecorderStateChanged();
    }

    void TimeViewToolBar::OnRecordButton()
    {
        if (!EMotionFX::GetRecorder().GetIsRecording())
        {
            // Check if at least one actor instance has an anim graph playing.
            bool activateAnimGraph = true;
            const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
            const uint32 numActorInstances = selectionList.GetNumSelectedActorInstances();
            for (uint32 i = 0; i < numActorInstances; ++i)
            {
                EMotionFX::ActorInstance* actorInstance = selectionList.GetActorInstance(i);
                if (!actorInstance->GetIsOwnedByRuntime() && actorInstance->GetAnimGraphInstance())
                {
                    activateAnimGraph = false;
                }
            }

            // Activate the anim graph in case there is no anim graph playing while we want to start recording the anim graph.
            if (activateAnimGraph)
            {
                EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
                if (plugin)
                {
                    AnimGraphPlugin* animGraphPlugin = static_cast<AnimGraphPlugin*>(plugin);
                    animGraphPlugin->GetActionManager().ActivateAnimGraph();
                }
            }

            EMotionFX::Recorder::RecordSettings settings;
            settings.mFPS                       = 1000000;
            settings.mRecordTransforms          = true;
            settings.mRecordAnimGraphStates     = true;
            settings.mRecordNodeHistory         = true;
            settings.mRecordScale               = true;
            settings.mInitialAnimGraphAnimBytes = 4 * 1024 * 1024; // 4 mb
            settings.mHistoryStatesOnly         = m_recorderGroup->GetRecordStatesOnly();
            settings.mRecordEvents              = m_recorderGroup->GetRecordEvents();

            if (m_recorderGroup->GetRecordMotionsOnly())
            {
                settings.mNodeHistoryTypes.insert(azrtti_typeid<EMotionFX::AnimGraphMotionNode>());
            }

            EMotionFX::GetRecorder().StartRecording(settings);

            // reinit the time view plugin
            if (mPlugin)
            {
                mPlugin->ReInit();
                mPlugin->SetScale(1.0);
                mPlugin->SetScrollX(0);
            }
        }
        else
        {
            EMotionFX::GetRecorder().StopRecording();
            EMotionFX::GetRecorder().StartPlayBack();
            EMotionFX::GetRecorder().SetAutoPlay(false);
            EMotionFX::GetRecorder().SetCurrentPlayTime(0.0f);

            // reinit the time view plugin
            if (mPlugin)
            {
                mPlugin->ReInit();
                mPlugin->OnZoomAll();
                mPlugin->SetCurrentTime(0.0f);
                mPlugin->GetTrackDataWidget()->setFocus();
                mPlugin->GetTrackDataHeaderWidget()->setFocus();
            }
        }

        UpdateInterface();

        emit RecorderStateChanged();
    }

    void TimeViewToolBar::OnClearRecordButton()
    {
        EMotionFX::GetRecorder().Clear();
        UpdateInterface();

        // reinit the time view plugin
        if (mPlugin)
        {
            mPlugin->ReInit();
            mPlugin->SetScale(1.0);
            mPlugin->SetScrollX(0);
            mPlugin->SetCurrentTime(0.0f);
        }

        emit RecorderStateChanged();
    }

    void TimeViewToolBar::UpdateMotions()
    {
        const CommandSystem::SelectionList& selection = CommandSystem::GetCommandManager()->GetCurrentSelection();

        // create our command group
        MCore::CommandGroup commandGroup("Adjust default motion instances");

        // get the number of selected motions and iterate through them
        const uint32 numMotions = selection.GetNumSelectedMotions();
        for (uint32 i = 0; i < numMotions; ++i)
        {
            MotionWindowPlugin* plugin = GetMotionWindowPlugin();
            MotionWindowPlugin::MotionTableEntry* entry = plugin ? plugin->FindMotionEntryByID(selection.GetMotion(i)->GetID()) : nullptr;
            if (!entry)
            {
                MCore::LogError("Cannot find motion table entry for the given motion.");
                continue;
            }

            EMotionFX::Motion* motion = entry->mMotion;
            EMotionFX::PlayBackInfo* playbackInfo = motion->GetDefaultPlayBackInfo();
            AZStd::string commandParameters;

            if (MCore::Compare<float>::CheckIfIsClose(playbackInfo->mPlaySpeed, m_playbackOptions->GetPlaySpeed(), 0.001f) == false)
            {
                commandParameters += AZStd::string::format("-playSpeed %f ", m_playbackOptions->GetPlaySpeed());
            }

            // Either loop forever or freeze at the last frame after playing the animation once.
            if (m_playbackOptions->GetLoopForever())
            {
                commandParameters += AZStd::string::format("-numLoops %i ", EMFX_LOOPFOREVER);
            }
            else
            {
                commandParameters += "-numLoops 1 -freezeAtLastFrame true ";
            }

            const bool mirrorMotion = m_playbackOptions->GetMirrorMotion();
            if (playbackInfo->mMirrorMotion != mirrorMotion)
            {
                commandParameters += AZStd::string::format("-mirrorMotion %s ", AZStd::to_string(mirrorMotion).c_str());
            }

            const EMotionFX::EPlayMode playMode = m_playbackOptions->GetPlayMode();
            if (playbackInfo->mPlayMode != playMode)
            {
                commandParameters += AZStd::string::format("-playMode %i ", static_cast<AZ::u8>(playMode));
            }

            const bool inPlace = m_playbackOptions->GetInPlace();
            if (playbackInfo->mInPlace != inPlace)
            {
                commandParameters += AZStd::string::format("-inPlace %s ", AZStd::to_string(inPlace).c_str());
            }

            const bool retarget = m_playbackOptions->GetRetarget();
            if (playbackInfo->mRetarget != retarget)
            {
                commandParameters += AZStd::string::format("-retarget %s ", AZStd::to_string(retarget).c_str());
            }

            // in case the command parameters are empty it means nothing changed, so we can skip this command
            if (!commandParameters.empty())
            {
                commandGroup.AddCommandString(AZStd::string::format("AdjustDefaultPlayBackInfo -filename \"%s\" %s", motion->GetFileName(), commandParameters.c_str()).c_str());
            }
        }

        // execute the group command
        AZStd::string outResult;
        if (CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult) == false)
        {
            MCore::LogError(outResult.c_str());
        }
    }

    void TimeViewToolBar::UpdateInterface()
    {
        const TimeViewMode mode = mPlugin->GetMode();

        const bool playbackOptionsVisible = m_playbackOptions->UpdateInterface(mode, /*showRightSeparator=*/false);
        const bool playbackControlsVisible = m_playbackControls->UpdateInterface(mode, /*showRightSeparator=*/playbackOptionsVisible);
        const bool recorderGroupVisible =  m_recorderGroup->UpdateInterface(mode, /*showRightSeparator=*/playbackControlsVisible);
    }

    void TimeViewToolBar::OnDetailedNodes()
    {
        mPlugin->SetRedrawFlag();

        if (!m_recorderGroup->GetDetailedNodes())
        {
            mPlugin->mTrackDataWidget->mNodeHistoryItemHeight = 20;
        }
        else
        {
            mPlugin->mTrackDataWidget->mNodeHistoryItemHeight = 35;
        }
    }

    // return the preferred size
    QSize TimeViewToolBar::sizeHint() const
    {
        return QSize(150, 35);
    }

    MotionWindowPlugin* TimeViewToolBar::GetMotionWindowPlugin()
    {
        return static_cast<MotionWindowPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(MotionWindowPlugin::CLASS_ID));
    }

    void TimeViewToolBar::OnTimeChangeStart()
    {
        UpdateInterface();
        emit RecorderStateChanged();
    }
} // namespace EMStudio

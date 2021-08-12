/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <MysticQt/Source/MysticQtManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/PlaybackOptionsGroup.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TimeViewToolBar.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionWindow/MotionWindowPlugin.h>
#include <QAction>

namespace EMStudio
{
    PlaybackOptionsGroup::PlaybackOptionsGroup(TimeViewToolBar* toolbar)
        : QObject(toolbar)
    {
        m_loopForeverAction = toolbar->addAction(
            MysticQt::GetMysticQt()->FindIcon("Images/Icons/Loop.svg"),
            "Loop forever", toolbar, &TimeViewToolBar::UpdateMotions);
        m_loopForeverAction->setCheckable(true);

        m_mirrorAction = toolbar->addAction(
            MysticQt::GetMysticQt()->FindIcon("Images/Icons/Mirror.svg"),
            "Mirror", toolbar, &TimeViewToolBar::UpdateMotions);
        m_mirrorAction->setCheckable(true);

        m_backwardAction = toolbar->addAction(
            MysticQt::GetMysticQt()->FindIcon("Images/Icons/MoveBackward.svg"),
            "Move backward", toolbar, &TimeViewToolBar::UpdateMotions);
        m_backwardAction->setCheckable(true);

        m_inPlaceAction = toolbar->addAction(
            MysticQt::GetMysticQt()->FindIcon("Images/Icons/InPlace.svg"),
            "In place", toolbar, &TimeViewToolBar::UpdateMotions);
        m_inPlaceAction->setCheckable(true);

        m_retargetAction = toolbar->addAction(
            MysticQt::GetMysticQt()->FindIcon("Images/Icons/Retarget.svg"),
            "Retarget", toolbar, &TimeViewToolBar::UpdateMotions);
        m_retargetAction->setCheckable(true);

        m_separatorSpeed = toolbar->addSeparator();

        m_speedSlider = new AzQtComponents::SliderDouble(Qt::Horizontal);
        m_speedSlider->setMaximumWidth(80);
        m_speedSlider->setMinimumWidth(30);
        m_speedSlider->setRange(0.005, 2.0);
        m_speedSlider->setToolTip(tr("The playback speed factor. A value of 1 stands for the original speed, while for example 2 means twice the original speed.\nPress the button on the right of this slider to reset to the normal speed."));
        connect(m_speedSlider, &AzQtComponents::SliderDouble::valueChanged, this, &PlaybackOptionsGroup::OnSpeedSliderValueChanged);
        connect(m_speedSlider, &AzQtComponents::SliderDouble::sliderReleased, toolbar, &TimeViewToolBar::UpdateMotions);
        m_speedAction = toolbar->addWidget(m_speedSlider);

        m_speedResetAction = toolbar->addAction(
            MysticQt::GetMysticQt()->FindIcon("Images/Icons/Reset.svg"),
            tr("Reset the play speed to its normal speed."), this, &PlaybackOptionsGroup::ResetPlaySpeed);
        connect(m_speedResetAction, &QAction::triggered, toolbar, &TimeViewToolBar::UpdateMotions);

        m_separatorRight = toolbar->addSeparator();

        ResetPlaySpeed();
    }

    bool PlaybackOptionsGroup::UpdateInterface(TimeViewMode mode, bool showRightSeparator)
    {
        const bool isVisible = (mode == TimeViewMode::Motion);
        m_loopForeverAction->setVisible(isVisible);
        m_mirrorAction->setVisible(isVisible);
        m_backwardAction->setVisible(isVisible);
        m_inPlaceAction->setVisible(isVisible);
        m_retargetAction->setVisible(isVisible);
        m_separatorSpeed->setVisible(isVisible);
        m_speedAction->setVisible(isVisible);
        m_speedSlider->setVisible(isVisible);
        m_speedResetAction->setVisible(isVisible);
        m_separatorRight->setVisible(isVisible && showRightSeparator);

        if (isVisible)
        {
            const CommandSystem::SelectionList& selection = CommandSystem::GetCommandManager()->GetCurrentSelection();

            const size_t numSelectedMotions = selection.GetNumSelectedMotions();
            const bool isEnabled = (numSelectedMotions == 1);

            m_loopForeverAction->setEnabled(isEnabled);
            m_mirrorAction->setEnabled(isEnabled);
            m_backwardAction->setEnabled(isEnabled);
            m_inPlaceAction->setEnabled(isEnabled);
            m_retargetAction->setEnabled(isEnabled);
            m_speedAction->setEnabled(isEnabled);
            m_speedSlider->setEnabled(isEnabled);
            m_speedResetAction->setEnabled(isEnabled);

            if (isEnabled)
            {
                MotionWindowPlugin* motionWindowPlugin = TimeViewToolBar::GetMotionWindowPlugin();
                if (motionWindowPlugin)
                {
                    const size_t numMotions = selection.GetNumSelectedMotions();
                    for (size_t i = 0; i < numMotions; ++i)
                    {
                        MotionWindowPlugin::MotionTableEntry* entry = motionWindowPlugin->FindMotionEntryByID(selection.GetMotion(i)->GetID());
                        if (!entry)
                        {
                            AZ_Warning("EMotionFX", false, "Cannot find motion table entry for the given motion.");
                            continue;
                        }

                        EMotionFX::Motion* motion = entry->m_motion;
                        const EMotionFX::PlayBackInfo* defaultPlayBackInfo = motion->GetDefaultPlayBackInfo();

                        m_loopForeverAction->setChecked(defaultPlayBackInfo->m_numLoops == EMFX_LOOPFOREVER);
                        m_mirrorAction->setChecked(defaultPlayBackInfo->m_mirrorMotion);
                        m_inPlaceAction->setChecked(defaultPlayBackInfo->m_inPlace);
                        m_retargetAction->setChecked(defaultPlayBackInfo->m_retarget);

                        const bool playBackward = (defaultPlayBackInfo->m_playMode == EMotionFX::PLAYMODE_BACKWARD);
                        m_backwardAction->setChecked(playBackward);

                        SetPlaySpeed(defaultPlayBackInfo->m_playSpeed);
                    }
                }
            }
            else
            {
                m_loopForeverAction->setChecked(false);
                m_mirrorAction->setChecked(false);
                m_backwardAction->setChecked(false);
                m_inPlaceAction->setChecked(false);
                m_retargetAction->setChecked(false);
            }
        }

        return isVisible;
    }

    EMotionFX::EPlayMode PlaybackOptionsGroup::GetPlayMode() const
    {
        if (m_backwardAction->isChecked())
        {
            return EMotionFX::PLAYMODE_BACKWARD;
        }

        return EMotionFX::PLAYMODE_FORWARD;
    }

    bool PlaybackOptionsGroup::GetMirrorMotion() const
    {
        return m_mirrorAction->isChecked();
    }

    bool PlaybackOptionsGroup::GetLoopForever() const
    {
        return m_loopForeverAction->isChecked();
    }

    float PlaybackOptionsGroup::GetPlaySpeed() const
    {
        return aznumeric_cast<float>(m_speedSlider->value());
    }

    void PlaybackOptionsGroup::SetPlaySpeed(float value)
    {
        m_speedSlider->setValue(value);
    }

    void PlaybackOptionsGroup::ResetPlaySpeed()
    {
        SetPlaySpeed(1.0f);
    }

    bool PlaybackOptionsGroup::GetInPlace() const
    {
        return m_inPlaceAction->isChecked();
    }

    bool PlaybackOptionsGroup::GetRetarget() const
    {
        return m_retargetAction->isChecked();
    }

    void PlaybackOptionsGroup::OnSpeedSliderValueChanged(double newPlayspeed)
    {
        const AZStd::vector<EMotionFX::MotionInstance*>& motionInstances = MotionWindowPlugin::GetSelectedMotionInstances();
        for (EMotionFX::MotionInstance* motionInstance : motionInstances)
        {
            motionInstance->SetPlaySpeed(aznumeric_cast<float>(newPlayspeed));
        }
    }
} // namespace EMStudio

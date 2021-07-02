/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <EMotionFX/Source/PlayBackInfo.h>
#include <AzQtComponents/Components/Widgets/Slider.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TimeViewShared.h>
#include <QObject>

QT_FORWARD_DECLARE_CLASS(QAction)

namespace EMStudio
{
    class TimeViewToolBar;

    class PlaybackOptionsGroup
        : public QObject
    {
        Q_OBJECT // AUTOMOC

    public:
        PlaybackOptionsGroup(TimeViewToolBar* toolbar);
        bool UpdateInterface(TimeViewMode mode, bool showRightSeparator = true);

        EMotionFX::EPlayMode GetPlayMode() const;
        bool GetMirrorMotion() const;
        bool GetLoopForever() const;
        bool GetInPlace() const;
        bool GetRetarget() const;

        float GetPlaySpeed() const;
        void SetPlaySpeed(float value);

    private slots:
        void ResetPlaySpeed();
        void OnSpeedSliderValueChanged(double newPlayspeed);

    private:
        QAction* m_loopForeverAction = nullptr;
        QAction* m_mirrorAction = nullptr;
        QAction* m_backwardAction = nullptr;
        QAction* m_inPlaceAction = nullptr;
        QAction* m_retargetAction = nullptr;

        QAction* m_separatorSpeed = nullptr;
        AzQtComponents::SliderDouble* m_speedSlider = nullptr;
        QAction* m_speedAction = nullptr;
        QAction* m_speedResetAction = nullptr;

        QAction* m_separatorRight = nullptr;
    };
} // namespace EMStudio

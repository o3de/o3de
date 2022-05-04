/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include "../StandardPluginsConfig.h"
#include <MCore/Source/StandardHeaders.h>
#include <QWidget>
#endif

QT_FORWARD_DECLARE_CLASS(QPushButton)

namespace EMStudio
{
    // forward declarations
    class MotionExtractionWindow;
    class MotionRetargetingWindow;
    class MotionWindowPlugin;

    class MotionPropertiesWindow
        : public QWidget
    {
        Q_OBJECT // AUTOMOC

    public:
        MotionPropertiesWindow(QWidget* parent, MotionWindowPlugin* motionWindowPlugin);
        ~MotionPropertiesWindow();

        void UpdateMotions();
        void UpdateInterface();

        void AddSubProperties(QWidget* widget);
        void FinalizeSubProperties();

    private:
        MotionWindowPlugin* m_motionWindowPlugin = nullptr;
        MotionExtractionWindow* m_motionExtractionWindow = nullptr;
        MotionRetargetingWindow* m_motionRetargetingWindow = nullptr;

        static constexpr const char* s_headerIcon = ":/EMotionFX/ActorComponent.svg";
    };
} // namespace EMStudio

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

namespace EMStudio
{
    class MotionExtractionWindow;

    class MotionPropertiesWindow : public QWidget
    {
        Q_OBJECT // AUTOMOC

    public:
        MotionPropertiesWindow(QWidget* parent);
        ~MotionPropertiesWindow();

        void UpdateInterface();

        void AddSubProperties(QWidget* widget);
        void FinalizeSubProperties();

    private:
        class CommandSelectCallback : public MCore::Command::Callback
        {
        public:
            CommandSelectCallback(MotionPropertiesWindow* window, bool executePreUndo, bool executePreCommand = false);
            bool Execute(MCore::Command* command, const MCore::CommandLine& commandLine);
            bool Undo(MCore::Command* command, const MCore::CommandLine& commandLine);

        private:
            MotionPropertiesWindow* m_window = nullptr;
        };
        AZStd::vector<MCore::Command::Callback*> m_callbacks;

        MotionExtractionWindow* m_motionExtractionWindow = nullptr;

        static constexpr const char* s_headerIcon = ":/EMotionFX/ActorComponent.svg";
    };
} // namespace EMStudio

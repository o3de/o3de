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
#include "../../../../EMStudioSDK/Source/NodeSelectionWindow.h"
#include <AzQtComponents/Components/Widgets/BrowseEdit.h>
#include <EMotionFX/Source/PlayBackInfo.h>
#include <QWidget>
#endif


QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QVBoxLayout)

namespace MysticQt
{
    class LinkWidget;
}

namespace EMStudio
{
    // forward declarations
    class MotionWindowPlugin;

    class MotionExtractionWindow
        : public QWidget
    {
        Q_OBJECT // AUTOMOC
        MCORE_MEMORYOBJECTCATEGORY(MotionExtractionWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        MotionExtractionWindow(QWidget* parent, MotionWindowPlugin* motionWindowPlugin);
        ~MotionExtractionWindow();

        void Init();

        EMotionFX::EMotionExtractionFlags GetMotionExtractionFlags() const;

    public slots:
        void UpdateInterface();
        void OnMotionExtractionFlagsUpdated();

        void OnSelectMotionExtractionNode();
        void OnMotionExtractionNodeSelected(AZStd::vector<SelectionItem> selection);

    private:
        // helper functions
        void CreateFlagsWidget();
        void CreateWarningWidget();

        // general
        MotionWindowPlugin*             m_motionWindowPlugin;

        // flags widget
        QWidget*                        m_flagsWidget;
        QCheckBox*                      m_captureHeight;

        //
        QVBoxLayout*                    m_mainVerticalLayout;
        QVBoxLayout*                    m_childVerticalLayout;
        QWidget*                        m_warningWidget;
        bool                            m_warningShowed;

        // motion extraction node selection
        NodeSelectionWindow*            m_motionExtractionNodeSelectionWindow;
        AzQtComponents::BrowseEdit*     m_warningSelectNodeLink;

        // Command callbacks
        class SelectActorCallback
            : public MCore::Command::Callback
        {
        public:
            SelectActorCallback(MotionExtractionWindow* motionExtractionWindow);
            bool Execute(MCore::Command* command, const MCore::CommandLine& commandLine);
            bool Undo(MCore::Command* command, const MCore::CommandLine& commandLine);

        private:
            MotionExtractionWindow* m_motionExtractionWindow = nullptr;
        };

        class UpdateMotionExtractionWindowCallback
            : public MCore::Command::Callback
        {
        public:
            UpdateMotionExtractionWindowCallback(MotionExtractionWindow* motionExtractionWindow);
            bool Execute(MCore::Command* command, const MCore::CommandLine& commandLine);
            bool Undo(MCore::Command* command, const MCore::CommandLine& commandLine);

        private:
            MotionExtractionWindow* m_motionExtractionWindow = nullptr;
        };

        AZStd::vector<MCore::Command::Callback*> m_commandCallbacks;
    };
} // namespace EMStudio

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <EMotionStudio/EMStudioSDK/Source/NodeSelectionWindow.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/StandardPluginsConfig.h>
#include <AzQtComponents/Components/Widgets/BrowseEdit.h>
#include <EMotionFX/Source/PlayBackInfo.h>
#include <MCore/Source/StandardHeaders.h>
#include <QWidget>
#endif

QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QVBoxLayout)
QT_FORWARD_DECLARE_CLASS(QPushButton)

namespace MysticQt
{
    class LinkWidget;
}

namespace AzToolsFramework
{
    class ReflectedPropertyEditor;
}

namespace EMStudio
{
    class MotionExtractionWindow : public QWidget
    {
        Q_OBJECT // AUTOMOC
        MCORE_MEMORYOBJECTCATEGORY(MotionExtractionWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        MotionExtractionWindow(QWidget* parent);
        ~MotionExtractionWindow();

        void Init();

        EMotionFX::EMotionExtractionFlags GetMotionExtractionFlags() const;

    public slots:
        void UpdateInterface();
        void OnMotionExtractionFlagsUpdated();
        void OnRootMotionCheckboxClicked();
        void OnSaveMotion();

        void OnSelectMotionExtractionNode();
        void OnMotionExtractionNodeSelected(AZStd::vector<SelectionItem> selection);

    private:
        // helper functions
        void CreateFlagsWidget();
        void CreateWarningWidget();
        void CreateRootMotionWidgets();

        // flags widget
        QWidget* m_flagsWidget = nullptr;
        QCheckBox* m_captureHeight = nullptr;

        // Root motion extraction widgets
        QCheckBox* m_extractRootMotionCheck = nullptr;
        QPushButton* m_saveMotionButton = nullptr;
        AzToolsFramework::ReflectedPropertyEditor* m_rootMotionExtractionWidget = nullptr;

        //
        QVBoxLayout* m_mainVerticalLayout = nullptr;
        QVBoxLayout* m_childVerticalLayout = nullptr;
        QWidget* m_warningWidget = nullptr;
        bool m_warningShowed = false;

        // motion extraction node selection
        NodeSelectionWindow* m_motionExtractionNodeSelectionWindow = nullptr;
        AzQtComponents::BrowseEdit* m_warningSelectNodeLink = nullptr;

        // Command callbacks
        class SelectActorCallback : public MCore::Command::Callback
        {
        public:
            SelectActorCallback(MotionExtractionWindow* motionExtractionWindow);
            bool Execute(MCore::Command* command, const MCore::CommandLine& commandLine);
            bool Undo(MCore::Command* command, const MCore::CommandLine& commandLine);

        private:
            MotionExtractionWindow* m_motionExtractionWindow = nullptr;
        };

        class UpdateMotionExtractionWindowCallback : public MCore::Command::Callback
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

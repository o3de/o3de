/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/EventHandler.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/StandardPluginsConfig.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/ToolBarPlugin.h>
#include <QProgressBar>
#endif

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QIcon)
QT_FORWARD_DECLARE_CLASS(QLabel)

namespace AzQtComponents
{
    class SliderDouble;
}

namespace EMStudio
{
    class CommandBarPlugin
        : public EMStudio::ToolBarPlugin
    {
        Q_OBJECT // AUTOMOC
        MCORE_MEMORYOBJECTCATEGORY(CommandBarPlugin, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        enum
        {
            CLASS_ID = 0x00000002
        };

        CommandBarPlugin();
        ~CommandBarPlugin();

        // overloaded
        const char* GetName() const override;
        uint32 GetClassID() const override;

        bool GetIsFloatable() const override                            { return false; }
        bool GetIsVertical() const override                             { return false; }
        bool GetIsMovable() const override                              { return true;  }
        Qt::ToolBarAreas GetAllowedAreas() const override               { return Qt::TopToolBarArea | Qt::BottomToolBarArea; }
        Qt::ToolButtonStyle GetToolButtonStyle() const override         { return Qt::ToolButtonIconOnly; }

        // overloaded main init function
        bool Init() override;
        EMStudioPlugin* Clone() const override { return new CommandBarPlugin(); }

        void UpdateLockSelectionIcon();

        void OnProgressStart();
        void OnProgressEnd();
        void OnProgressText(const char* text);
        void OnProgressValue(float percentage);

    public slots:
        void OnEnter();
        void OnLockSelectionButton();
        void ResetGlobalSimSpeed();
        void OnGlobalSimSpeedChanged(double value);

    private:
        class ProgressHandler
            : public EMotionFX::EventHandler
        {
        public:
            AZ_CLASS_ALLOCATOR_DECL

            ProgressHandler(CommandBarPlugin* commandbarPlugin)
                : EventHandler() { m_commandbarPlugin = commandbarPlugin; }

            const AZStd::vector<EMotionFX::EventTypes> GetHandledEventTypes() const override { return { EMotionFX::EVENT_TYPE_ON_PROGRESS_START, EMotionFX::EVENT_TYPE_ON_PROGRESS_END, EMotionFX::EVENT_TYPE_ON_PROGRESS_TEXT, EMotionFX::EVENT_TYPE_ON_PROGRESS_VALUE, EMotionFX::EVENT_TYPE_ON_SUB_PROGRESS_TEXT, EMotionFX::EVENT_TYPE_ON_SUB_PROGRESS_VALUE }; }
            void OnProgressStart() override                                 { m_commandbarPlugin->OnProgressStart(); }
            void OnProgressEnd() override                                   { m_commandbarPlugin->OnProgressEnd(); }
            void OnProgressText(const char* text) override                  { m_commandbarPlugin->OnProgressText(text); }
            void OnProgressValue(float percentage) override                 { m_commandbarPlugin->OnProgressValue(percentage); }
            void OnSubProgressText(const char* text) override               { MCORE_UNUSED(text); }
            void OnSubProgressValue(float percentage) override              { MCORE_UNUSED(percentage); }
        private:
            CommandBarPlugin* m_commandbarPlugin;
        };

        MCORE_DEFINECOMMANDCALLBACK(CommandToggleLockSelectionCallback);
        CommandToggleLockSelectionCallback* m_toggleLockSelectionCallback = nullptr;

        QLineEdit* m_commandEdit = nullptr;
        QLineEdit* m_resultEdit = nullptr;
        QAction* m_lockSelectionAction = nullptr;
        QAction* m_globalSimSpeedResetAction = nullptr;
        AzQtComponents::SliderDouble* m_globalSimSpeedSlider = nullptr;
        QAction* m_globalSimSpeedSliderAction = nullptr;
        QIcon* m_lockEnabledIcon = nullptr;
        QIcon* m_lockDisabledIcon = nullptr;
        QAction* m_commandEditAction = nullptr;
        QAction* m_commandResultAction = nullptr;

        // progress bar
        QAction* m_progressBarAction = nullptr;
        QAction* m_progressTextAction = nullptr;
        QProgressBar* m_progressBar = nullptr;
        QLabel* m_progressText = nullptr;
        ProgressHandler* m_progressHandler = nullptr;
    };
} // namespace EMStudio

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <source/ui/AssetListTabWidget.h>
#include <source/ui/SeedTabWidget.h>
#include <source/ui/BundleListTabWidget.h>
#include <source/ui/RulesTabWidget.h>

#include <AzCore/Debug/TraceMessageBus.h>
#include <AzToolsFramework/UI/Logging/LogLine.h>
#include <AzToolsFramework/UI/Logging/LogTableModel.h>

#include <QMainWindow>
#include <QMessageBox>
#include <QSharedPointer>
#endif

namespace Ui
{
    class MainWindow;
}

namespace AssetBundler
{
    class GUIApplicationManager;

    class MainWindow
        : public QMainWindow
        , public AZ::Debug::TraceMessageBus::Handler
    {
        Q_OBJECT

    public:

        explicit MainWindow(AssetBundler::GUIApplicationManager* guiApplicationManager, QWidget* parent = 0);
        virtual ~MainWindow();

        void Activate();

        void ApplyConfig();

        void WriteToLog(const AZStd::string& message, AzToolsFramework::Logging::LogLine::LogType logType);
        void WriteToLog(const QString& message, AzToolsFramework::Logging::LogLine::LogType logType);
        void WriteToLog(const char* message, AzToolsFramework::Logging::LogLine::LogType logType);
        void WriteToLog(const char* message, const char* window, AzToolsFramework::Logging::LogLine::LogType logType);

        /////////////////////////////////////////////////////////
        // TraceMessageBus overrides
        bool OnPreError(const char* window, const char* fileName, int line, const char* func, const char* message) override;
        bool OnPreWarning(const char* window, const char* fileName, int line, const char* func, const char* message) override;
        bool OnPrintf(const char* window, const char* message) override;
        /////////////////////////////////////////////////////////

    public Q_SLOTS:
        void ShowWindow();

    protected:
        void closeEvent(QCloseEvent* event) override;

    private:
        QSharedPointer<Ui::MainWindow> m_ui;

        // Tabs
        QSharedPointer<AssetListTabWidget> m_assetListTab;
        QSharedPointer<SeedTabWidget> m_seedListTab;
        QSharedPointer<RulesTabWidget> m_rulesTab;
        QSharedPointer<BundleListTabWidget> m_bundleListTab;

        enum TabIndex
        {
            Seeds,
            AssetLists,
            Rules,
            Bundles,
            MAX
        };

        void OnSupportClicked();

        // Log
        QSharedPointer<AzToolsFramework::Logging::LogTableModel> m_logModel;

        void ShowLogContextMenu(const QPoint& pos);

        // Detecting Unsaved Changes
        QSharedPointer<QMessageBox> m_unsavedChangesMsgBox;

        bool HasUnsavedChanges();
        void SaveCurrentSelection();
        void SaveAll();

        AssetBundler::GUIApplicationManager* m_guiApplicationManager = nullptr;
    };
} // namespace AssetBundler

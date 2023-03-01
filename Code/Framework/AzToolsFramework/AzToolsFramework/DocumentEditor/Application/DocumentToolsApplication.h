/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/IO/FileIO.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/UserSettings/UserSettingsProvider.h>

#include <AzFramework/Application/Application.h>

#include <AzQtComponents/Application/AzQtApplication.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <AzToolsFramework/API/AssetDatabaseBus.h>
#include <AzToolsFramework/API/EditorPythonConsoleBus.h>
#include <AzToolsFramework/Logger/TraceLogger.h>

#include <QTimer>

#include <AzToolsFramework/DocumentEditor/ToolsMainWindowNotificationBus.h>
#include <AzToolsFramework/DocumentEditor/AssetBrowser/ToolsAssetBrowserInteractions.h>

namespace AzToolsFramework
{
    //! Base class for  tools to inherit from
    class DocumentToolsApplication
        : public AzFramework::Application
        , public AzQtComponents::AzQtApplication
        , protected AzToolsFramework::AssetDatabase::AssetDatabaseRequestsBus::Handler
        , protected AzToolsFramework::EditorPythonConsoleNotificationBus::Handler
        , protected AZ::UserSettingsOwnerRequestBus::Handler
        , protected ToolsMainWindowNotificationBus::Handler
    {
    public:
        AZ_TYPE_INFO(DocumentToolsApplication, "{C723D098-9070-4276-8EDB-4AD9522CE3E0}");
        AZ_DISABLE_COPY_MOVE(DocumentToolsApplication);

        using Base = AzFramework::Application;

        DocumentToolsApplication(const char* targetName, int* argc, char*** argv);
        ~DocumentToolsApplication();

        virtual bool LaunchLocalServer();

        // AzFramework::Application overrides...
        void CreateReflectionManager() override;
        void Reflect(AZ::ReflectContext* context) override;
        void RegisterCoreComponents() override;
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
        void CreateStaticModules(AZStd::vector<AZ::Module*>& outModules) override;
        const char* GetCurrentConfigurationName() const override;
        void StartCommon(AZ::Entity* systemEntity) override;
        void Destroy() override;
        void RunMainLoop() override;

    protected:
        void OnIdle();

        // sToolMainWindowNotificationBus::Handler overrides...
        void OnMainWindowClosing() override;

        // AssetDatabaseRequestsBus::Handler overrides...
        bool GetAssetDatabaseLocation(AZStd::string& result) override;

        // AZ::ComponentApplication overrides...
        void QueryApplicationType(AZ::ApplicationTypeQuery& appType) const override;

        // AZ::UserSettingsOwnerRequestBus::Handler overrides...
        void SaveSettings() override;

        // EditorPythonConsoleNotificationBus::Handler overrides...
        void OnTraceMessage(AZStd::string_view message) override;
        void OnErrorMessage(AZStd::string_view message) override;
        void OnExceptionMessage(AZStd::string_view message) override;

        //! List of filters for assets that need to be pre-built to run the application
        virtual AZStd::vector<AZStd::string> GetCriticalAssetFilters() const;

        virtual void LoadSettings();
        virtual void UnloadSettings();
        virtual void ConnectToAssetProcessor();
        virtual void CompileCriticalAssets();
        virtual void ProcessCommandLine(const AZ::CommandLine& commandLine);

        void PrintAlways(const AZStd::string& output);
        void RedirectStdoutToNull();

        static void PyIdleWaitFrames(uint32_t frames);
        static void PyExit();
        static void PyCrash();
        static void PyTestOutput(const AZStd::string& output);

        // Adding static instance access for static Python methods
        static DocumentToolsApplication* GetInstance();
        static DocumentToolsApplication* m_instance;

        AzToolsFramework::TraceLogger m_traceLogger;

        AZStd::unique_ptr<AzQtComponents::StyleManager> m_styleManager;

        //! Local user settings are used to store asset browser tree expansion state
        AZ::UserSettingsProvider m_localUserSettings;

        //! Are local settings loaded
        bool m_activatedLocalUserSettings = false;

        //AzToolsFramework::LocalSocket m_socket;
        //AzToolsFramework::LocalServer m_server;

        AZStd::unique_ptr<AzToolsFramework::ToolsAssetBrowserInteractions> m_assetBrowserInteractions;

        const AZStd::string m_targetName;
        const AZ::Crc32 m_toolId = {};

        // Disable warning for dll export since this member won't be used outside this class
        AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING;
        AZ::IO::FileDescriptorRedirector m_stdoutRedirection = AZ::IO::FileDescriptorRedirector(1); // < 1 for STDOUT
        AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING;
    };
} // namespace ToolsFramework

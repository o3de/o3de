/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/Communication/LocalServer.h>
#include <AtomToolsFramework/Communication/LocalSocket.h>
#include <AtomToolsFramework/AssetBrowser/AtomToolsAssetBrowserInteractions.h>

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

namespace AtomToolsFramework
{
    //! Base class for Atom tools to inherit from
    class AtomToolsApplication
        : public AzQtComponents::AzQtApplication
        , public AzFramework::Application
        , protected AzToolsFramework::AssetDatabase::AssetDatabaseRequestsBus::Handler
        , protected AzToolsFramework::EditorPythonConsoleNotificationBus::Handler
        , protected AZ::UserSettingsOwnerRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(AtomToolsApplication, AZ::SystemAllocator)
        AZ_TYPE_INFO(AtomToolsApplication, "{A0DF25BA-6F74-4F11-9F85-0F99278D5986}");
        AZ_DISABLE_COPY_MOVE(AtomToolsApplication);

        using Base = AzFramework::Application;

        AtomToolsApplication(const char* targetName, int* argc, char*** argv);
        AtomToolsApplication(const char* targetName, int* argc, char*** argv, AZ::ComponentApplicationSettings componentAppSettings);
        ~AtomToolsApplication();

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
        static AtomToolsApplication* GetInstance();
        static AtomToolsApplication* m_instance;

        AzToolsFramework::TraceLogger m_traceLogger;

        AZStd::unique_ptr<AzQtComponents::StyleManager> m_styleManager;

        //! Local user settings are used to store asset browser tree expansion state
        AZ::UserSettingsProvider m_localUserSettings;

        //! Are local settings loaded
        bool m_activatedLocalUserSettings = false;

        AtomToolsFramework::LocalSocket m_socket;
        AtomToolsFramework::LocalServer m_server;

        AZStd::unique_ptr<AtomToolsFramework::AtomToolsAssetBrowserInteractions> m_assetBrowserInteractions;

        const AZStd::string m_targetName;
        const AZ::Crc32 m_toolId = {};

        // Disable warning for dll export since this member won't be used outside this class
        AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING;
        AZ::IO::FileDescriptorRedirector m_stdoutRedirection = AZ::IO::FileDescriptorRedirector(1); // < 1 for STDOUT
        AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING;
    };
} // namespace AtomToolsFramework

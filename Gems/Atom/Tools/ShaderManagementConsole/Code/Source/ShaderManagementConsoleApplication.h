/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once


#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/UserSettings/UserSettingsProvider.h>
#include <AzCore/Debug/TraceMessageBus.h>

#include <AzFramework/Application/Application.h>
#include <AzFramework/Asset/AssetSystemBus.h>

#include <AzToolsFramework/API/AssetDatabaseBus.h>
#include <AzToolsFramework/API/EditorPythonConsoleBus.h>

#include <Atom/Document/ShaderManagementConsoleDocumentSystemRequestBus.h>
#include <Atom/Window/ShaderManagementConsoleWindowNotificationBus.h>

#include <QApplication>
#include <QTimer>

namespace ShaderManagementConsole
{
    class ShaderManagementConsoleApplication
        : public AzFramework::Application
        , public QApplication
        , private AzToolsFramework::AssetDatabase::AssetDatabaseRequestsBus::Handler
        , private ShaderManagementConsoleWindowNotificationBus::Handler
        , private AzFramework::AssetSystemStatusBus::Handler
        , private AZ::UserSettingsOwnerRequestBus::Handler
        , private AZ::Debug::TraceMessageBus::Handler
        , private AzToolsFramework::EditorPythonConsoleNotificationBus::Handler
    {
    public:
        AZ_TYPE_INFO(ShaderManagementConsole::ShaderManagementConsoleApplication, "{30F90CA5-1253-49B5-8143-19CEE37E22BB}");

        using Base = AzFramework::Application;

        ShaderManagementConsoleApplication(int* argc, char*** argv);
        virtual ~ShaderManagementConsoleApplication() = default;

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::Application
        void CreateReflectionManager() override;
        void Reflect(AZ::ReflectContext* context) override;
        void RegisterCoreComponents() override;
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
        void CreateStaticModules(AZStd::vector<AZ::Module*>& outModules) override;
        const char* GetCurrentConfigurationName() const override;
        void StartCommon(AZ::Entity* systemEntity) override;
        void Tick(float deltaOverride = -1.f) override;
        void Stop() override;

    private:
        //////////////////////////////////////////////////////////////////////////
        // AssetDatabaseRequestsBus::Handler overrides...
        bool GetAssetDatabaseLocation(AZStd::string& result) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // ShaderManagementConsoleWindowNotificationBus::Handler overrides...
        void OnShaderManagementConsoleWindowClosing() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::Application overrides...
        void Destroy() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::ApplicationRequests::Bus overrides...
        void QueryApplicationType(AZ::ApplicationTypeQuery& appType) const override;
        //////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // EditorPythonConsoleNotificationBus::Handler overrides...
        void OnTraceMessage(AZStd::string_view message) override;
        void OnErrorMessage(AZStd::string_view message) override;
        void OnExceptionMessage(AZStd::string_view message) override;
        ////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::AssetSystemStatusBus::Handler overrides...
        void AssetSystemAvailable() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AZ::UserSettingsOwnerRequestBus::Handler overrides...
        void SaveSettings() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AZ::Debug::TraceMessageBus::Handler overrides...
        bool OnPrintf(const char* window, const char* message) override;
        //////////////////////////////////////////////////////////////////////////

        void CompileCriticalAssets();

        void ProcessCommandLine();

        void LoadSettings();
        void UnloadSettings();

        bool LaunchDiscoveryService();

        void StartInternal();

        static void PyIdleWaitFrames(uint32_t frames);

        //! Local user settings are used to store asset browser tree expansion state
        AZ::UserSettingsProvider m_localUserSettings;

        //! Are local settings loaded
        bool m_activatedLocalUserSettings = false;

        QTimer m_timer;

        bool m_started = false;
        bool m_closing = false;
    };
} // namespace ShaderManagementConsole

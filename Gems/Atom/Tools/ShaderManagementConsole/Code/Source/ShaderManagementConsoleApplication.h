/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Document/ShaderManagementConsoleDocumentSystemRequestBus.h>
#include <Atom/Window/ShaderManagementConsoleWindowNotificationBus.h>
#include <AtomToolsFramework/Application/AtomToolsApplication.h>

#include <QTimer>

namespace ShaderManagementConsole
{
    class ShaderManagementConsoleApplication
        : public AtomToolsFramework::AtomToolsApplication
        , private ShaderManagementConsoleWindowNotificationBus::Handler
    {
    public:
        AZ_TYPE_INFO(ShaderManagementConsole::ShaderManagementConsoleApplication, "{A31B1AEB-4DA3-49CD-884A-CC998FF7546F}");

        using Base = AzFramework::Application;

        ShaderManagementConsoleApplication(int* argc, char*** argv);
        virtual ~ShaderManagementConsoleApplication() = default;

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::Application
        void CreateStaticModules(AZStd::vector<AZ::Module*>& outModules) override;
        const char* GetCurrentConfigurationName() const override;

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
        // AzFramework::AssetSystemStatusBus::Handler overrides...
        void AssetSystemAvailable() override;
        //////////////////////////////////////////////////////////////////////////

        void ProcessCommandLine();
        void StartInternal() override;
        AZStd::string_view GetBuildTargetName() override;
    };
} // namespace ShaderManagementConsole

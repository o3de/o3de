/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentApplication.h>
#include <AzToolsFramework/API/EditorWindowRequestBus.h>
#include <Window/ShaderManagementConsoleWindow.h>

namespace ShaderManagementConsole
{
    class ShaderManagementConsoleApplication
        : public AtomToolsFramework::AtomToolsDocumentApplication
        , private AzToolsFramework::EditorWindowRequestBus::Handler
    {
    public:
        AZ_TYPE_INFO(ShaderManagementConsole::ShaderManagementConsoleApplication, "{A31B1AEB-4DA3-49CD-884A-CC998FF7546F}");

        using Base = AtomToolsFramework::AtomToolsDocumentApplication;

        ShaderManagementConsoleApplication(int* argc, char*** argv);
        ~ShaderManagementConsoleApplication();

        // AzFramework::Application overrides...
        void Reflect(AZ::ReflectContext* context) override;
        const char* GetCurrentConfigurationName() const override;
        void StartCommon(AZ::Entity* systemEntity) override;
        void Destroy() override;

        // AtomToolsFramework::AtomToolsApplication overrides...
        AZStd::vector<AZStd::string> GetCriticalAssetFilters() const override;

        // AzToolsFramework::EditorWindowRequests::Bus::Handler
        QWidget* GetAppMainWindow() override;

    private:
        AZStd::unique_ptr<ShaderManagementConsoleWindow> m_window;
    };
} // namespace ShaderManagementConsole

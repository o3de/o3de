/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/Application/AtomToolsApplication.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>

namespace ShaderManagementConsole
{
    class ShaderManagementConsoleApplication
        : public AtomToolsFramework::AtomToolsApplication
    {
    public:
        AZ_TYPE_INFO(ShaderManagementConsole::ShaderManagementConsoleApplication, "{A31B1AEB-4DA3-49CD-884A-CC998FF7546F}");

        using Base = AtomToolsFramework::AtomToolsApplication;

        ShaderManagementConsoleApplication(int* argc, char*** argv);

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::Application
        void CreateStaticModules(AZStd::vector<AZ::Module*>& outModules) override;
        const char* GetCurrentConfigurationName() const override;

    private:
        void ProcessCommandLine(const AZ::CommandLine& commandLine);
        AZStd::string GetBuildTargetName() const override;
        AZStd::vector<AZStd::string> GetCriticalAssetFilters() const override;
    };
} // namespace ShaderManagementConsole

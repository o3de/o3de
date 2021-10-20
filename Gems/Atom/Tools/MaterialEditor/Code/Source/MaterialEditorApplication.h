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

namespace MaterialEditor
{
    class MaterialThumbnailRenderer;

    class MaterialEditorApplication
        : public AtomToolsFramework::AtomToolsApplication
    {
    public:
        AZ_TYPE_INFO(MaterialEditor::MaterialEditorApplication, "{30F90CA5-1253-49B5-8143-19CEE37E22BB}");

        using Base = AtomToolsFramework::AtomToolsApplication;

        MaterialEditorApplication(int* argc, char*** argv);

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::Application
        void CreateStaticModules(AZStd::vector<AZ::Module*>& outModules) override;
        const char* GetCurrentConfigurationName() const override;

    private:
        void ProcessCommandLine(const AZ::CommandLine& commandLine) override;
        AZStd::string GetBuildTargetName() const override;
        AZStd::vector<AZStd::string> GetCriticalAssetFilters() const override;
    };
} // namespace MaterialEditor

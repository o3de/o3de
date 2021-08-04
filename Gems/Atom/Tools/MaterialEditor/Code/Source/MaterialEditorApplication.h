/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Document/MaterialDocumentSystemRequestBus.h>
#include <AtomToolsFramework/Application/AtomToolsApplication.h>

#include <QTimer>

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
        virtual ~MaterialEditorApplication();

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::Application
        void CreateStaticModules(AZStd::vector<AZ::Module*>& outModules) override;
        const char* GetCurrentConfigurationName() const override;

    private:
        void ProcessCommandLine(const AZ::CommandLine& commandLine) override;
        AZStd::string GetBuildTargetName() const override;

        //! List of common asset filters for things that need to be compiled to run the material editor
            //! Some of these things will not be necessary once we have proper support for queued asset loading and reloading
        AZStd::vector<AZStd::string> GetCriticalAssetFilters() const override;
     };
} // namespace MaterialEditor

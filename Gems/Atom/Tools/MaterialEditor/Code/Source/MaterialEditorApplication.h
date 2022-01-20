/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/Document/AtomToolsDocumentApplication.h>

namespace MaterialEditor
{
    class MaterialThumbnailRenderer;

    class MaterialEditorApplication
        : public AtomToolsFramework::AtomToolsDocumentApplication
    {
    public:
        AZ_TYPE_INFO(MaterialEditor::MaterialEditorApplication, "{30F90CA5-1253-49B5-8143-19CEE37E22BB}");

        using Base = AtomToolsFramework::AtomToolsDocumentApplication;

        MaterialEditorApplication(int* argc, char*** argv);

        // AzFramework::Application overrides...
        void CreateStaticModules(AZStd::vector<AZ::Module*>& outModules) override;
        const char* GetCurrentConfigurationName() const override;

        // AtomToolsFramework::AtomToolsApplication overrides...
        AZStd::string GetBuildTargetName() const override;
        AZStd::vector<AZStd::string> GetCriticalAssetFilters() const override;
    };
} // namespace MaterialEditor

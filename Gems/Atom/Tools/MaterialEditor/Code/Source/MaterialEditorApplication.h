/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Document/MaterialDocumentSystemRequestBus.h>
#include <Atom/Window/MaterialEditorWindowNotificationBus.h>
#include <AtomToolsFramework/Application/AtomToolsApplication.h>

#include <QTimer>

namespace MaterialEditor
{
    class MaterialThumbnailRenderer;

    class MaterialEditorApplication
        : public AtomToolsFramework::AtomToolsApplication
        , private MaterialEditorWindowNotificationBus::Handler
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
        void StartCommon(AZ::Entity* systemEntity) override;
        void Stop() override;

    private:
        //////////////////////////////////////////////////////////////////////////
        // MaterialEditorWindowNotificationBus::Handler overrides...
        void OnMaterialEditorWindowClosing() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::Application overrides...
        void Destroy() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::AssetSystemStatusBus::Handler overrides...
        void AssetSystemAvailable() override;
        //////////////////////////////////////////////////////////////////////////

        void ProcessCommandLine(const AZ::CommandLine& commandLine) override;
        void StartInternal() override;
        AZStd::string_view GetBuildTargetName() override;
     };
} // namespace MaterialEditor

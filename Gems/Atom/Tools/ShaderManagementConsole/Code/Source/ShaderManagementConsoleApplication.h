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
#include <ShaderManagementConsoleRequestBus.h>
#include <Window/ShaderManagementConsoleWindow.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>

namespace ShaderManagementConsole
{
    class ShaderManagementConsoleApplication
        : public AtomToolsFramework::AtomToolsDocumentApplication
        , private ShaderManagementConsoleRequestBus::Handler
        , private AzToolsFramework::EditorWindowRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ShaderManagementConsoleApplication, AZ::SystemAllocator)
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

        // AzToolsFramework::EditorWindowRequestBus::Handler
        QWidget* GetAppMainWindow() override;

        // ShaderManagementConsoleRequestBus::Handler overrides...
        AZ::Data::AssetInfo GetSourceAssetInfo(const AZStd::string& sourceAssetFileName) override;
        AZStd::vector<AZ::Data::AssetId> FindMaterialAssetsUsingShader(const AZStd::string& shaderFilePath) override;
        AZStd::vector<AZ::RPI::ShaderCollection::Item> GetMaterialInstanceShaderItems(const AZ::Data::AssetId& assetId) override;
        AZStd::vector<AZ::Data::AssetId> GetAllMaterialAssetIds() override;
        AZStd::string GenerateRelativeSourcePath(const AZStd::string& fullShaderPath) override;
        AZ::RPI::ShaderOptionValue MakeShaderOptionValueFromInt(int value) override;

    private:
        AZStd::unique_ptr<ShaderManagementConsoleWindow> m_window;
    };
} // namespace ShaderManagementConsole

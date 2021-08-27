/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzToolsFramework/API/EditorWindowRequestBus.h>

#include <Atom/RPI.Reflect/Material/MaterialAsset.h>

#include <AtomToolsFramework/Window/AtomToolsMainWindowFactoryRequestBus.h>
#include <Atom/Core/ShaderManagementConsoleRequestBus.h>
#include <Source/Window/ShaderManagementConsoleBrowserInteractions.h>
#include <Source/Window/ShaderManagementConsoleWindow.h>

namespace ShaderManagementConsole
{
    //! ShaderManagementConsoleWindowComponent is the entry point for the Shader Management Console gem user interface, and is mainly
    //! used for initialization and registration of other classes, including ShaderManagementConsoleWindow.
    class ShaderManagementConsoleWindowComponent
        : public AZ::Component
        , private AtomToolsFramework::AtomToolsMainWindowFactoryRequestBus::Handler
        , private ShaderManagementConsoleRequestBus::Handler
        , private AzToolsFramework::EditorWindowRequestBus::Handler
    {
    public:
        AZ_COMPONENT(ShaderManagementConsoleWindowComponent, "{03976F19-3C74-49FE-A15F-7D3CADBA616C}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

    private:
        // Temporary structure when generating shader variants.
        struct ShaderVariantListInfo
        {
            AZStd::string m_materialFileName;
            AZStd::vector<AZ::RPI::ShaderCollection::Item> m_shaderItems;
        };

        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::EditorWindowRequests::Bus::Handler
        QWidget* GetAppMainWindow() override;
        //////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AtomToolsMainWindowFactoryRequestBus::Handler overrides...
        void CreateMainWindow() override;
        void DestroyMainWindow() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // ShaderManagementConsoleRequestBus::Handler overrides...
        AZ::Data::AssetInfo GetSourceAssetInfo(const AZStd::string& sourceAssetFileName) override;
        AZStd::vector<AZ::Data::AssetId> FindMaterialAssetsUsingShader(const AZStd::string& shaderFilePath) override;
        AZStd::vector<AZ::RPI::ShaderCollection::Item> GetMaterialInstanceShaderItems(const AZ::Data::AssetId& assetId) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        AZStd::unique_ptr<ShaderManagementConsoleWindow> m_window;
        AZStd::unique_ptr<ShaderManagementConsoleBrowserInteractions> m_assetBrowserInteractions;
    };
}
